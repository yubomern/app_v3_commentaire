/*
 * simulator_main.cpp - Simulateur de crash autonome.
 * Ce fichier lance la simulation de plantage, génère des rapports CSV
 * et écrit des coredumps pour le pipeline d'analyse.
 *
 * Usage :
 *   ./crash_simulator              # un crash aléatoire
 *   ./crash_simulator --count N    # N crashs aléatoires
 *   ./crash_simulator --config scenario.cfg  # charger un scénario
 */

#include "Simulator.hpp"
#include "CrashTypes.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/resource.h>
#include <filesystem>
#include <map>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;
using namespace CrashSimulator;

// ─── Configuration loaded from scenario file ─────────────────────────────────
struct ScenarioConfig {
    int  crash_count    = 1;
    int  intensity      = 5;
    bool random_mode    = true;          // always true: no manual flags
    bool logging        = true;
    std::string output_dir = "simulator_output";
    std::string output_csv = "crash_report.csv";
    int  delay_ms       = 500;           // ms between crashes in multi-run
};

// ─── Parse key=value scenario file ───────────────────────────────────────────
ScenarioConfig loadConfig(const std::string& path) {
    ScenarioConfig cfg;
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[Simulator] Config file not found: " << path
                  << " — using defaults.\n";
        return cfg;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key   = line.substr(0, eq);
        std::string value = line.substr(eq + 1);
        if (key == "crash_count")  cfg.crash_count  = std::stoi(value);
        if (key == "intensity")    cfg.intensity     = std::stoi(value);
        if (key == "logging")      cfg.logging       = (value == "true" || value == "1");
        if (key == "output_dir")   cfg.output_dir    = value;
        if (key == "output_csv")   cfg.output_csv    = value;
        if (key == "delay_ms")     cfg.delay_ms      = std::stoi(value);
    }
    return cfg;
}

// ─── Enable OS coredumps (Objective 2) ───────────────────────────────────────
static void setupCoredumps(const std::string& dumpDir) {
    // ulimit -c unlimited
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &rl) == 0) {
        std::cout << "   ✅ ulimit -c unlimited set.\n";
    } else {
        perror("   ⚠️  setrlimit(RLIMIT_CORE)");
    }

    // Set kernel core_pattern:  core.<exe>.<pid>.<timestamp>
    // Requires root; we attempt but handle failure gracefully.
    fs::create_directories(dumpDir);
    std::string pattern = dumpDir + "/core.%e.%p.%t";
    std::ofstream kp("/proc/sys/kernel/core_pattern");
    if (kp.is_open()) {
        kp << pattern << "\n";
        std::cout << "   ✅ core_pattern set: " << pattern << "\n";
    } else {
        std::cout << "   ℹ️  Cannot write /proc/sys/kernel/core_pattern "
                     "(run as root for full coredump naming).\n";
        std::cout << "   ℹ️  Coredumps will use default pattern.\n";
    }
}

// ─── Signal handler ───────────────────────────────────────────────────────────
static void sigHandler(int s) {
    std::cout << "\n⚠️  Signal " << s << " received — exiting.\n";
    _exit(s);
}

// ─── CSV append helper ────────────────────────────────────────────────────────
// Columns MUST match ImportFromCSV order exactly:
// timestamp, fault_type, file, line, cpu_usage, mem_usage,
// probable_cause, severity, solution_hint, confidence,
// backtrace, solution, ml_prediction
static void appendCSV(const std::string& filepath, const CrashReport& r,
                      CrashCategory cat, int severity) {
    bool exists = fs::exists(filepath) && fs::file_size(filepath) > 0;
    std::ofstream f(filepath, std::ios::app);
    if (!f.is_open()) {
        std::cerr << "[Simulator] ERROR: cannot open CSV: " << filepath << "\n";
        return;
    }

    if (!exists) {
        f << "timestamp,fault_type,file,line,cpu_usage,mem_usage,"
             "probable_cause,severity,solution_hint,confidence,"
             "backtrace,solution,ml_prediction\n";
    }

    auto tt = std::chrono::system_clock::to_time_t(r.timestamp);
    std::ostringstream ts;
    ts << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");

    // Map severity int → string
    std::string sev_str = (severity==4?"CRITICAL":severity==3?"HIGH":
                           severity==2?"MEDIUM":"LOW");

    // Build a simple solution hint from CrashInfo
    CrashInfo info;
    info.hint = "Check crash category: " + categoryToString(cat);

    f << "\"" << ts.str()           << "\","   // timestamp
      << "\"" << categoryToString(cat) << "\","// fault_type
      << "\"\","                               // file  (unknown at this point)
      << "0,"                                  // line
      << "0,"                                  // cpu_usage
      << "0,"                                  // mem_usage
      << "\"" << r.description      << "\","   // probable_cause
      << "\"" << sev_str            << "\","   // severity
      << "\"Use GDB: gdb ./crash_simulator core -> bt\","  // solution_hint
      << "0.5,"                                // confidence
      << "\"" << r.description      << "\","   // backtrace (description as proxy)
      << "\"Analyse with crash_analyzer\","    // solution
      << "\"Simulator record\"\n";             // ml_prediction
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    // ── Parse args (no crash-type flags — only count / config) ──────────────
    ScenarioConfig cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if ((arg == "--count" || arg == "-n") && i + 1 < argc)
            cfg.crash_count = std::max(1, std::min(200, std::atoi(argv[++i])));
        else if ((arg == "--config" || arg == "-c") && i + 1 < argc)
            cfg = loadConfig(argv[++i]);
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0]
                      << " [--count N] [--config scenario.cfg]\n"
                         "  --count N      Number of random crashes (default 1)\n"
                         "  --config FILE  Load scenario config file\n";
            return 0;
        }
    }

    // ── Banner ────────────────────────────────────────────────────────────────
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║       Random Crash Simulator  v3.0  (Linux)              ║\n";
    std::cout << "║       Embedded Fault Generator — Automated Pipeline      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n\n";

    std::cout << "📌 Configuration:\n";
    std::cout << "   crash_count : " << cfg.crash_count  << "\n";
    std::cout << "   intensity   : " << cfg.intensity    << "\n";
    std::cout << "   output_dir  : " << cfg.output_dir   << "\n";
    std::cout << "   output_csv  : " << cfg.output_csv   << "\n\n";

    // ── Setup coredumps ───────────────────────────────────────────────────────
    std::cout << "🔧 Configuring OS kernel coredumps...\n";
    setupCoredumps(cfg.output_dir + "/coredumps");
    std::cout << "\n";

    // ── Prepare output dir ────────────────────────────────────────────────────
    fs::create_directories(cfg.output_dir);
    std::string csvPath = cfg.output_dir + "/" + cfg.output_csv;

    // Remove old CSV so we start fresh each run
    fs::remove(csvPath);

    // ── Run crashes ───────────────────────────────────────────────────────────
    int success = 0;
    for (int i = 0; i < cfg.crash_count; ++i) {
        std::cout << "═══════════════════════════════════════════════════════\n";
        std::cout << "🔴 Crash " << (i + 1) << " / " << cfg.crash_count << "\n";

        unsigned int seed = static_cast<unsigned int>(
            std::chrono::steady_clock::now().time_since_epoch().count()) + i;

        Simulator sim(seed);
        sim.setRandomMode(true);   // always random — no manual flag needed
        sim.setIntensity(cfg.intensity);
        sim.enableLogging(cfg.logging);
        sim.setOutputDir(cfg.output_dir);

        CrashCategory cat = sim.generateRandomCrash();
        sim.setCrashCategory(cat);

        CrashInfo info = sim.getCrashInfo(cat);
        int sev = getSeverity(cat);

        std::cout << "   📂 Category : " << categoryToString(cat) << "\n";
        std::cout << "   🎲 Seed     : " << seed                   << "\n";
        std::cout << "   ⚡ Severity : "
                  << (sev==4?"CRITICAL":sev==3?"HIGH":sev==2?"MEDIUM":"LOW")
                  << "\n";
        std::cout << "   📝 Info     : " << info.description        << "\n";
        std::cout << "   💡 Hint     : " << info.hint               << "\n\n";

        // Record before triggering (trigger may actually crash the process)
        CrashReport rep;
        rep.seed        = seed;
        rep.category    = cat;
        rep.timestamp   = std::chrono::system_clock::now();
        rep.description = info.description;
        rep.call_chain_depth = cfg.intensity;
        rep.exceptionCode    = 0;
        appendCSV(csvPath, rep, cat, sev);

        std::cout << "   📄 CSV record written: " << csvPath << "\n";
        std::cout << "💥 Triggering crash...\n";

        // triggerCrash will actually crash the process if it's a real fault.
        // In automated test runs the simulator is launched in a subprocess
        // so that only child processes die, not the pipeline.
        sim.triggerCrash(cat);

        std::cout << "   ℹ️  Process survived (signal was caught or simulated).\n";
        ++success;

        if (i < cfg.crash_count - 1 && cfg.delay_ms > 0) {
            usleep(cfg.delay_ms * 1000);
        }
    }

    std::cout << "\n✅ Simulator done. " << success << "/" << cfg.crash_count
              << " crash records written to: " << csvPath << "\n";
    std::cout << "📁 Coredumps (if generated): " << cfg.output_dir << "/coredumps/\n\n";

    return 0;
}
