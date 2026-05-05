#pragma once
/*
 * CoreFileParser.hpp - Interface de l'analyse des fichiers core Linux.
 * Déclare les structures et fonctions pour extraire les informations
 * de signal, de pile d'appels, de registres et de threads.
 */

#include <string>
#include <vector>
#include <map>

namespace Analyzer {

struct CoreFrame {
    int    frame_no   = 0;
    std::string address;
    std::string function;
    std::string file;
    int    line       = 0;
};

struct ThreadInfo {
    int thread_id = 0;
    std::vector<CoreFrame> frames;
};

struct CoreFileInfo {
    std::string core_path;       // path to .core file
    std::string binary_path;     // associated executable (may be empty)
    std::string crash_signal;    // e.g. "SIGSEGV"
    int         signal_number = 0;
    std::string fault_address;
    std::string timestamp;
    std::vector<ThreadInfo>  threads;
    std::map<std::string,std::string> registers; // name → value
    bool        parsed_ok = false;
    std::string parse_error;

    // Derived / top-frame info
    std::string top_function;
    std::string top_file;
    int         top_line = 0;
    std::string raw_backtrace;   // full GDB bt output
};

class CoreFileParser {
public:
    CoreFileParser() = default;

    /**
     * Parse a single .core file.
     * @param core_path  Path to the core file.
     * @param binary     Path to the crashing binary (optional; auto-detected).
     * @return           Populated CoreFileInfo.
     */
    CoreFileInfo parse(const std::string& core_path,
                       const std::string& binary = "") const;

    /**
     * Scan a directory for *.core / core.* files and parse all of them.
     */
    std::vector<CoreFileInfo> parseDirectory(const std::string& dir,
                                             const std::string& binary = "") const;

    /** Convert CoreFileInfo to a one-line CSV row (matches CrashData layout). */
    static std::string toCsvRow(const CoreFileInfo& info);

private:
    std::string runGdb(const std::string& core_path,
                       const std::string& binary,
                       const std::string& commands) const;

    CoreFileInfo parseGdbOutput(const std::string& gdb_out,
                                const std::string& core_path) const;

    static std::string detectBinary(const std::string& core_path);
};

} // namespace Analyzer
