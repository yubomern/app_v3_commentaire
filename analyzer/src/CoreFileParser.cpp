/*
 * CoreFileParser.cpp - Analyse et extraction de données à partir de fichiers .core Linux.
 * Ce module utilise GDB en mode batch pour lire le signal, la pile d'appels,
 * les registres, l'adresse de la fonction et les informations de thread.
 */

#include "CoreFileParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <cstdio>
#include <array>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace fs = std::filesystem;
namespace Analyzer {

// ─── Run GDB non-interactively ─────────────────────────────────────────────
std::string CoreFileParser::runGdb(const std::string& core_path,
                                   const std::string& binary,
                                   const std::string& commands) const {
#ifdef _WIN32
    return "[ERROR] Windows build does not support GDB-based core parsing.";
#else
    // Build GDB batch command
    std::string cmd = "gdb --batch --quiet";
    if (!binary.empty() && fs::exists(binary))
        cmd += " \"" + binary + "\"";
    else
        cmd += " /proc/self/exe";   // fallback — gives at least signal info

    cmd += " \"" + core_path + "\"";

    // Pipe commands via stdin using here-string
    cmd += " 2>&1 <<'__GDB_EOF__'\n";
    cmd += commands;
    cmd += "\n__GDB_EOF__\n";

    // Alternatively use -ex flags (more portable)
    cmd = "gdb --batch --quiet";
    if (!binary.empty() && fs::exists(binary))
        cmd += " \"" + binary + "\"";

    // Build -ex chain
    std::istringstream ss(commands);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty())
            cmd += " -ex \"" + line + "\"";
    }
    cmd += " \"" + core_path + "\" 2>&1";

    // Execute and capture output
    std::array<char, 4096> buf{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "[ERROR] popen failed";
    while (fgets(buf.data(), buf.size(), pipe))
        result += buf.data();
    pclose(pipe);
    return result;
#endif
}

// ─── Try to detect associated binary from core file metadata ──────────────
std::string CoreFileParser::detectBinary(const std::string& core_path) {
#ifdef _WIN32
    return "";
#else
    // Use 'file' command to get program name from core header
    std::string cmd = "file \"" + core_path + "\" 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    char buf[512];
    std::string out;
    while (fgets(buf, sizeof(buf), p)) out += buf;
    pclose(p);

    // Parse: "core file from 'crash_simulator'"
    std::regex re(R"(from '([^']+)')");
    std::smatch m;
    if (std::regex_search(out, m, re))
        return m[1].str();

    return "";
#endif
}

// ─── Parse GDB output into CoreFileInfo ────────────────────────────────────
CoreFileInfo CoreFileParser::parseGdbOutput(const std::string& gdb_out,
                                             const std::string& core_path) const {
    CoreFileInfo info;
    info.core_path  = core_path;
    info.raw_backtrace = gdb_out;

    // ── Signal detection ────────────────────────────────────────────────────
    // GDB prints: "Program terminated with signal SIGSEGV, ..."
    {
        std::regex re(R"(signal\s+(SIG\w+))");
        std::smatch m;
        if (std::regex_search(gdb_out, m, re)) {
            info.crash_signal = m[1].str();
            // Map signal name to number
            static const std::map<std::string,int> sig_map = {
                {"SIGSEGV",11},{"SIGABRT",6},{"SIGFPE",8},
                {"SIGILL",4},{"SIGBUS",7},{"SIGKILL",9},
                {"SIGTRAP",5},{"SIGTERM",15}
            };
            auto it = sig_map.find(info.crash_signal);
            if (it != sig_map.end()) info.signal_number = it->second;
        }
    }

    // ── Fault address ───────────────────────────────────────────────────────
    {
        std::regex re(R"(Address\s+(0x[0-9a-fA-F]+))");
        std::smatch m;
        if (std::regex_search(gdb_out, m, re))
            info.fault_address = m[1].str();
    }

    // ── Parse backtrace frames ──────────────────────────────────────────────
    // GDB format: "#0  0x00007f... in function_name (args) at file.cpp:42"
    {
        std::regex re(R"(#(\d+)\s+(0x[0-9a-fA-F]+)\s+in\s+(\S+)\s*(?:\([^)]*\))?\s*(?:at\s+([^:]+):(\d+))?)");
        std::sregex_iterator it(gdb_out.begin(), gdb_out.end(), re);
        std::sregex_iterator end;
        ThreadInfo main_thread;
        main_thread.thread_id = 0;
        for (; it != end; ++it) {
            CoreFrame f;
            f.frame_no  = std::stoi((*it)[1].str());
            f.address   = (*it)[2].str();
            f.function  = (*it)[3].str();
            f.file      = (*it)[4].matched ? (*it)[4].str() : "";
            f.line      = (*it)[5].matched ? std::stoi((*it)[5].str()) : 0;
            main_thread.frames.push_back(f);
        }
        if (!main_thread.frames.empty()) {
            info.threads.push_back(main_thread);
            info.top_function = main_thread.frames[0].function;
            info.top_file     = main_thread.frames[0].file;
            info.top_line     = main_thread.frames[0].line;
        }
    }

    // ── Registers (info registers output) ──────────────────────────────────
    // Format: "rax            0x0      0"
    {
        std::regex re(R"(^(\w+)\s+(0x[0-9a-fA-F]+))", std::regex::multiline);
        std::sregex_iterator it(gdb_out.begin(), gdb_out.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it)
            info.registers[(*it)[1].str()] = (*it)[2].str();
    }

    // ── Timestamp from file mtime ────────────────────────────────────────────
    {
        std::error_code ec;
        auto ftime = fs::last_write_time(core_path, ec);
        if (!ec) {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() +
                std::chrono::system_clock::now());
            std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
            std::ostringstream ts;
            ts << std::put_time(std::localtime(&tt), "%Y-%m-%dT%H:%M:%S");
            info.timestamp = ts.str();
        }
    }

    info.parsed_ok = !info.crash_signal.empty() || !info.threads.empty();
    if (!info.parsed_ok)
        info.parse_error = "GDB could not extract signal or backtrace";

    return info;
}

// ─── Public: parse one core file ───────────────────────────────────────────
CoreFileInfo CoreFileParser::parse(const std::string& core_path,
                                   const std::string& binary) const {
    if (!fs::exists(core_path)) {
        CoreFileInfo bad;
        bad.core_path   = core_path;
        bad.parse_error = "File not found: " + core_path;
        return bad;
    }

    std::string bin = binary;
    if (bin.empty()) bin = detectBinary(core_path);

    // GDB commands to run
    std::string cmds =
        "set pagination off\n"
        "info signal\n"
        "bt full\n"
        "info registers\n"
        "thread apply all bt\n";

    std::string gdb_out = runGdb(core_path, bin, cmds);
    std::cout << "   [CoreFileParser] GDB output (" << gdb_out.size()
              << " bytes) for " << fs::path(core_path).filename().string() << "\n";

    return parseGdbOutput(gdb_out, core_path);
}

// ─── Public: scan directory for core files ─────────────────────────────────
std::vector<CoreFileInfo> CoreFileParser::parseDirectory(const std::string& dir,
                                                          const std::string& binary) const {
    std::vector<CoreFileInfo> results;
    std::error_code ec;
    if (!fs::exists(dir, ec)) {
        std::cerr << "[CoreFileParser] Directory not found: " << dir << "\n";
        return results;
    }

    for (auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string name = entry.path().filename().string();
        // Match: core, core.*, *.core, core.<exe>.<pid>.<ts>
        bool is_core = (name == "core") ||
                       (name.substr(0, 5) == "core.") ||
                       (name.size() > 5 && name.substr(name.size()-5) == ".core");
        if (!is_core) continue;

        std::cout << "🔍 Parsing core file: " << entry.path().string() << "\n";
        auto info = parse(entry.path().string(), binary);
        results.push_back(info);
    }

    std::cout << "[CoreFileParser] Parsed " << results.size()
              << " core file(s) in " << dir << "\n";
    return results;
}

// ─── Convert to CSV row (matches CrashData layout) ─────────────────────────
std::string CoreFileParser::toCsvRow(const CoreFileInfo& info) {
    // Escape quotes and strip control chars
    auto esc = [](const std::string& s) -> std::string {
        std::string r;
        for (char c : s) {
            if (c == '"')  { r += "\"\""; }
            else if (c == '\n' || c == '\r') { r += ' '; }
            else           { r += c; }
        }
        return r;
    };
    // Backtrace: strip newlines, limit to 300 chars
    std::string bt_safe = esc(
        info.raw_backtrace.size() > 300
            ? info.raw_backtrace.substr(0, 300)
            : info.raw_backtrace
    );

    std::string signal = info.crash_signal.empty() ? "UNKNOWN_SIGNAL" : info.crash_signal;
    std::string func   = info.top_function.empty() ? "unknown_function" : info.top_function;
    std::string ts     = info.timestamp.empty() ? "1970-01-01 00:00:00" : info.timestamp;
    std::string bin    = info.binary_path.empty() ? "./binary" : info.binary_path;

    // Columns MUST match ImportFromCSV order:
    // timestamp, fault_type, file, line, cpu_usage, mem_usage,
    // probable_cause, severity, solution_hint, confidence,
    // backtrace, solution, ml_prediction
    std::ostringstream row;
    row << "\"" << esc(ts)                                      << "\","
        << "\"" << esc(signal)                                  << "\","
        << "\"" << esc(info.top_file)                           << "\","
        << info.top_line                                        << ","
        << "0,"
        << "0,"
        << "\"Core dump: crash at " << esc(func)                << "\","
        << "\"CRITICAL\","
        << "\"gdb " << esc(bin) << " " << esc(info.core_path)  << "\","
        << "0.90,"
        << "\"" << bt_safe                                      << "\","
        << "\"Recompile with -g then: crash_analyzer --cores .\","
        << "\"CoreFileParser v3.0\"";
    return row.str();
}

} // namespace Analyzer
