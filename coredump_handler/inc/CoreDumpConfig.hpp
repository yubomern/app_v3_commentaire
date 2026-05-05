/*
 * CoreDumpConfig.hpp - Configuration du noyau pour la génération de coredumps.
 * Déclare les paramètres de sortie, les limites de taille et les règles
 * de collecte utilisées par le gestionnaire de coredump.
 */

#ifndef CORE_DUMP_CONFIG_HPP
#define CORE_DUMP_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace OSKernel {

// Core dump configuration
struct CoreDumpConfig {
    bool enabled;
    size_t max_size;        // 0 = unlimited
    std::string output_dir; // Where to save core dumps
    bool compress;
    bool timestamp_suffix;
};

// Crash patterns for detection
struct CrashPattern {
    std::string name;
    std::string description;
    int signal_number;      // Linux signal
    std::string hint;       // Solution hint
    int severity;           // 1-4
};

// Core dump manager class
class CoreDumpManager {
public:
    CoreDumpManager();
    ~CoreDumpManager();

    // Configure core dump settings
    bool configure(const CoreDumpConfig& config);
    bool enableCoreDump(size_t maxSize = 0);
    bool disableCoreDump();
    
    // Check if core dumps are enabled
    bool isEnabled() const;
    
    // Get current configuration
    const CoreDumpConfig& getConfig() const;
    
    // Get crash patterns
    const std::vector<CrashPattern>& getPatterns() const;
    CrashPattern getPattern(int signalNumber) const;
    
    // Generate core dump filename
    std::string generateCoreFileName(const std::string& prefix = "core") const;
    
    // Get signal name from number
    static std::string getSignalName(int signalNum);
    static int getSignalNumber(const std::string& signalName);

private:
    CoreDumpConfig config_;
    bool is_enabled_;
    std::vector<CrashPattern> patterns_;
    
    void initializePatterns();
};

// Signal numbers
namespace Signals {
    constexpr int SIGINT = 2;      // Interrupt
    constexpr int SIGILL = 4;      // Illegal instruction
    constexpr int SIGABRT = 6;     // Abort
    constexpr int SIGFPE = 8;      // Floating point exception
    constexpr int SIGSEGV = 11;    // Segmentation fault
    constexpr int SIGPIPE = 13;    // Broken pipe
    constexpr int SIGTERM = 15;    // Termination
}

} // namespace OSKernel

#endif // CORE_DUMP_CONFIG_HPP