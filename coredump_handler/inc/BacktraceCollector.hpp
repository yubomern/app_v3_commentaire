/*
 * BacktraceCollector.hpp - Interface de collecte de pile d'appels.
 * Définit la structure des frames de backtrace et l'API de collecte.
 */

#pragma once
#include <string>
#include <vector>

namespace CrashSimulator {

struct BacktraceFrame {
    std::string function_name;
    std::string file_name;
    int line_number;
    std::string address;
};

class BacktraceCollector {
public:
    BacktraceCollector();
    ~BacktraceCollector();

    // Collect backtrace at current point
    std::vector<BacktraceFrame> CollectBacktrace();

    // Collect backtrace from signal handler context
    std::vector<BacktraceFrame> CollectBacktraceFromSignal(void* signal_context);

    // Format backtrace for reporting
    std::string FormatBacktrace(const std::vector<BacktraceFrame>& frames);

    // Extract function names from backtrace
    std::vector<std::string> ExtractFunctionNames(const std::vector<BacktraceFrame>& frames);

    // Symbol resolution
    std::string ResolveSymbol(void* address);

private:
    // Platform-specific backtrace collection
    std::vector<BacktraceFrame> CollectLinuxBacktrace();
    std::vector<BacktraceFrame> CollectWindowsBacktrace();

    // Address to symbol resolution
    void* ResolveAddress(const std::string& symbol_name);
};

} // namespace CrashSimulator