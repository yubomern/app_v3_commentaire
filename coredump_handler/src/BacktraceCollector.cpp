/*
 * BacktraceCollector.cpp - Collecte de la pile d'appels.
 * Ce fichier contient les fonctions permettant de capturer la trace
 * d'exécution au moment du crash pour analyse ultérieure.
 */

#include "BacktraceCollector.hpp"
#include "Platform.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

#ifdef __linux__
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#endif

namespace CrashSimulator {

BacktraceCollector::BacktraceCollector() {
#ifdef _WIN32
    // Initialize Windows debugging symbols
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
#endif
}

BacktraceCollector::~BacktraceCollector() {
#ifdef _WIN32
    SymCleanup(GetCurrentProcess());
#endif
}

std::vector<BacktraceFrame> BacktraceCollector::CollectBacktrace() {
#ifdef __linux__
    return CollectLinuxBacktrace();
#else
    return CollectWindowsBacktrace();
#endif
}

std::vector<BacktraceFrame> BacktraceCollector::CollectBacktraceFromSignal(void* signal_context) {
    // For signal context, we'd need platform-specific handling
    // For now, fall back to regular backtrace
    return CollectBacktrace();
}

std::vector<BacktraceFrame> BacktraceCollector::CollectLinuxBacktrace() {
#ifdef __linux__
    std::vector<BacktraceFrame> frames;
    const int max_frames = 64;
    void* buffer[max_frames];
    int num_frames = backtrace(buffer, max_frames);

    char** symbols = backtrace_symbols(buffer, num_frames);
    if (!symbols) return frames;

    for (int i = 0; i < num_frames; ++i) {
        BacktraceFrame frame;
        frame.address = "0x" + std::to_string(reinterpret_cast<uintptr_t>(buffer[i]));

        // Parse symbol string: "binary(function+offset) [address]"
        std::string symbol_str = symbols[i];
        size_t paren_pos = symbol_str.find('(');
        size_t plus_pos = symbol_str.find('+', paren_pos);
        size_t bracket_pos = symbol_str.find('[', plus_pos);

        if (paren_pos != std::string::npos) {
            frame.function_name = symbol_str.substr(paren_pos + 1, plus_pos - paren_pos - 1);

            // Demangle C++ symbols
            if (!frame.function_name.empty()) {
                int status;
                char* demangled = abi::__cxa_demangle(frame.function_name.c_str(), nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    frame.function_name = demangled;
                    free(demangled);
                }
            }
        }

        frames.push_back(frame);
    }

    free(symbols);
    return frames;
#else
    return std::vector<BacktraceFrame>();
#endif
}

std::vector<BacktraceFrame> BacktraceCollector::CollectWindowsBacktrace() {
#ifdef _WIN32
    std::vector<BacktraceFrame> frames;
    const int max_frames = 64;
    void* stack[max_frames];
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    WORD num_frames = CaptureStackBackTrace(0, max_frames, stack, nullptr);

    for (WORD i = 0; i < num_frames; ++i) {
        BacktraceFrame frame;
        frame.address = "0x" + std::to_string(reinterpret_cast<uintptr_t>(stack[i]));

        // Get symbol information
        char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbol_buffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), nullptr, symbol)) {
            frame.function_name = symbol->Name;
        }

        // Get line information
        IMAGEHLP_LINE64 line_info = { sizeof(IMAGEHLP_LINE64) };
        DWORD displacement;
        if (SymGetLineFromAddr64(process, reinterpret_cast<DWORD64>(stack[i]), &displacement, &line_info)) {
            frame.file_name = line_info.FileName;
            frame.line_number = line_info.LineNumber;
        }

        frames.push_back(frame);
    }

    return frames;
#else
    return std::vector<BacktraceFrame>();
#endif
}

std::string BacktraceCollector::FormatBacktrace(const std::vector<BacktraceFrame>& frames) {
    std::ostringstream oss;
    oss << "Backtrace (" << frames.size() << " frames):\n";

    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        oss << "  [" << i << "] " << frame.address;

        if (!frame.function_name.empty()) {
            oss << " in " << frame.function_name;
        }

        if (!frame.file_name.empty() && frame.line_number > 0) {
            oss << " at " << frame.file_name << ":" << frame.line_number;
        }

        oss << "\n";
    }

    return oss.str();
}

std::vector<std::string> BacktraceCollector::ExtractFunctionNames(const std::vector<BacktraceFrame>& frames) {
    std::vector<std::string> function_names;
    for (const auto& frame : frames) {
        if (!frame.function_name.empty()) {
            function_names.push_back(frame.function_name);
        }
    }
    return function_names;
}

std::string BacktraceCollector::ResolveSymbol(void* address) {
#ifdef __linux__
    Dl_info info;
    if (dladdr(address, &info) && info.dli_sname) {
        int status;
        char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        if (status == 0 && demangled) {
            std::string result = demangled;
            free(demangled);
            return result;
        }
        return info.dli_sname;
    }
#endif
    return "unknown";
}

} // namespace CrashSimulator