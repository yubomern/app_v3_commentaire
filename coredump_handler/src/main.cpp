/*
 * coredump_handler/src/main.cpp  v3.0
 *
 * Objectif 2 : Intégration avec le noyau OS.
 * Ce binaire est enregistré comme gestionnaire de coredump via :
 *
 *   echo "|/path/to/coredump_handler %P %u %g %s %t %h %e" \
 *       > /proc/sys/kernel/core_pattern
 *
 * Lorsqu'un processus plante, le noyau envoie les données du core en entrée
 * standard de ce programme. Nous :
 *   1. sauvons le core brut dans COREDUMP_DIR/core.<exe>.<pid>.<ts>
 *   2. collectons les métriques système au moment du crash
 *   3. écrivons un fichier JSON de métadonnées de crash
 *   4. déclenchons éventuellement l'analyseur immédiatement
 *
 * Peut aussi être lancé seul pour tester ou configurer les coredumps :
 *   ./coredump_handler --setup          # configurer le noyau + ulimit
 *   ./coredump_handler --status         # vérifier la configuration
 */

#include "CoreDump.hpp"
#include "CoreDumpConfig.hpp"
#include "SystemMetrics.hpp"
#include "BacktraceCollector.hpp"
#include "Platform.hpp"
#include <csignal>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <sys/resource.h>
#include <unistd.h>
#include <filesystem>

namespace fs = std::filesystem;

// ─── Setup kernel coredump pattern ────────────────────────────────────────
static bool setupKernelCoredumps(const std::string& dump_dir,
                                  const std::string& handler_path = "") {
    // 1. ulimit -c unlimited for current session
    struct rlimit rl = { RLIM_INFINITY, RLIM_INFINITY };
    if (setrlimit(RLIMIT_CORE, &rl) != 0) {
        perror("setrlimit RLIMIT_CORE");
        return false;
    }

    // 2. Create dump directory
    std::error_code ec;
    fs::create_directories(dump_dir, ec);
    if (ec) {
        std::cerr << "Cannot create dump dir: " << ec.message() << "\n";
        return false;
    }

    // 3. Write /proc/sys/kernel/core_pattern
    std::string pattern;
    if (!handler_path.empty() && fs::exists(handler_path)) {
        // Pipe to this handler binary
        pattern = "|" + handler_path + " %P %u %g %s %t %h %e";
    } else {
        // Simple file pattern: core.<exe>.<pid>.<timestamp>
        pattern = dump_dir + "/core.%e.%p.%t";
    }

    std::ofstream kp("/proc/sys/kernel/core_pattern");
    if (kp.is_open()) {
        kp << pattern << "\n";
        kp.close();
        std::cout << "✅ core_pattern: " << pattern << "\n";
        return true;
    } else {
        std::cerr << "⚠️  Cannot write /proc/sys/kernel/core_pattern "
                     "(requires root).\n";
        std::cerr << "   Run: sudo sh -c 'echo \"" << pattern
                  << "\" > /proc/sys/kernel/core_pattern'\n";
        return false;
    }
}

// ─── Show current coredump status ─────────────────────────────────────────
static void showStatus() {
    std::cout << "\n=== Coredump Status ===\n";

    struct rlimit rl;
    getrlimit(RLIMIT_CORE, &rl);
    std::cout << "ulimit -c : ";
    if (rl.rlim_cur == RLIM_INFINITY) std::cout << "unlimited\n";
    else std::cout << rl.rlim_cur << " bytes\n";

    std::ifstream kp("/proc/sys/kernel/core_pattern");
    if (kp.is_open()) {
        std::string p; std::getline(kp, p);
        std::cout << "core_pattern: " << p << "\n";
    }

    std::ifstream ku("/proc/sys/kernel/core_uses_pid");
    if (ku.is_open()) {
        std::string v; std::getline(ku, v);
        std::cout << "core_uses_pid: " << v << "\n";
    }
    std::cout << "======================\n\n";
}

// ─── Save stdin core data to file (kernel pipe mode) ──────────────────────
static std::string saveCoreDump(const std::string& dump_dir,
                                 const std::string& exe_name,
                                 const std::string& pid,
                                 const std::string& ts) {
    std::string filename = dump_dir + "/core." + exe_name + "." + pid + "." + ts;
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Cannot open " << filename << " for writing\n";
        return "";
    }
    // Read raw core from stdin
    char buf[65536];
    while (std::cin.read(buf, sizeof(buf)) || std::cin.gcount() > 0)
        out.write(buf, std::cin.gcount());
    out.close();
    return filename;
}

// ─── Write JSON sidecar next to core file ─────────────────────────────────
static void writeSidecar(const std::string& core_path,
                         const std::string& exe, const std::string& pid,
                         const std::string& sig, const std::string& ts,
                         const std::string& host) {
    std::string json_path = core_path + ".json";
    std::ofstream j(json_path);
    if (!j.is_open()) return;

    j << "{\n"
      << "  \"core_file\": \"" << core_path << "\",\n"
      << "  \"executable\": \"" << exe << "\",\n"
      << "  \"pid\": " << pid << ",\n"
      << "  \"signal\": " << sig << ",\n"
      << "  \"timestamp\": \"" << ts << "\",\n"
      << "  \"hostname\": \"" << host << "\"\n"
      << "}\n";

    std::cout << "📄 Sidecar: " << json_path << "\n";
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Standalone modes ──────────────────────────────────────────────────────
    if (argc >= 2 && std::string(argv[1]) == "--setup") {
        std::string dir = (argc >= 3) ? argv[2] : "/var/crash";
        std::string handler = (argc >= 4) ? argv[3] : "";
        std::cout << "🔧 Setting up coredumps → " << dir << "\n";
        bool ok = setupKernelCoredumps(dir, handler);
        showStatus();
        return ok ? 0 : 1;
    }

    if (argc >= 2 && std::string(argv[1]) == "--status") {
        showStatus();
        return 0;
    }

    // ── Kernel pipe mode: args = PID UID GID SIG TIMESTAMP HOST EXE ─────────
    // Invoked by kernel as: handler <PID> <UID> <GID> <SIG> <TS> <HOST> <EXE>
    if (argc >= 8) {
        std::string pid  = argv[1];
        std::string sig  = argv[4];
        std::string ts   = argv[5];
        std::string host = argv[6];
        std::string exe  = argv[7];

        std::string dump_dir = "/var/crash";
        fs::create_directories(dump_dir);

        std::cout << "[coredump_handler] Crash: exe=" << exe
                  << " pid=" << pid << " sig=" << sig << "\n";

        std::string core_path = saveCoreDump(dump_dir, exe, pid, ts);
        if (!core_path.empty()) {
            writeSidecar(core_path, exe, pid, sig, ts, host);
            std::cout << "✅ Core saved: " << core_path
                      << " (" << fs::file_size(core_path) << " bytes)\n";
        }
        return 0;
    }

    // ── Default: configure coredumps for current session ─────────────────────
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║    Coredump Handler  v3.0 (Linux)         ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";
    std::cout << "Usage:\n";
    std::cout << "  ./coredump_handler --setup [DIR] [HANDLER_PATH]\n";
    std::cout << "  ./coredump_handler --status\n\n";
    showStatus();
    return 0;
}
