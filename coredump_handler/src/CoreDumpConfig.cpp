/*
 * CoreDumpConfig.cpp - Configuration du comportement des coredumps.
 * Gère l'initialisation des paramètres du kernel, la création du dossier
 * de sortie et la persistance des réglages pour les dumps.
 */

#include "CoreDumpConfig.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>

namespace OSKernel {

CoreDumpManager::CoreDumpManager() : is_enabled_(false) {
    initializePatterns();
    // Try to load current configuration
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        is_enabled_ = (rlim.rlim_cur > 0);
        config_.max_size = rlim.rlim_cur;
    }
    config_.enabled = is_enabled_;
    config_.output_dir = ".";
    config_.compress = false;
    config_.timestamp_suffix = true;
}

CoreDumpManager::~CoreDumpManager() = default;

void CoreDumpManager::initializePatterns() {
    patterns_ = {
        {"SIGSEGV", "Segmentation fault - invalid memory access", 
         SIGSEGV, "Validate pointer access and check array bounds", 4},
        
        {"SIGFPE", "Floating point exception - division by zero", 
         SIGFPE, "Check divisor before division operations", 3},
        
        {"SIGILL", "Illegal instruction - invalid CPU operation", 
         SIGILL, "Check instruction encoding and CPU compatibility", 3},
        
        {"SIGABRT", "Abort signal - abnormal termination", 
         SIGABRT, "Check for unhandled exceptions and assertions", 2},
        
        {"SIGBUS", "Bus error - misaligned memory access", 
         SIGBUS, "Check memory alignment requirements", 4},
        
        {"SIGPIPE", "Broken pipe - write to closed pipe", 
         SIGPIPE, "Handle pipe errors properly", 1},
        
        {"SIGTERM", "Termination request", 
         SIGTERM, "Check for external termination signals", 1},
        
        {"SIGINT", "Interrupt from keyboard (Ctrl+C)", 
         SIGINT, "Handle interrupt signals gracefully", 1},
        
        {"Stack Overflow", "Stack memory exhausted", 
         SIGSEGV, "Add recursion depth limit or increase stack size", 4},
        
        {"Heap Corruption", "Heap memory corruption", 
         SIGSEGV, "Check for buffer overflows and double frees", 4},
        
        {"Null Pointer", "Dereferencing null pointer", 
         SIGSEGV, "Add null checks before pointer dereference", 4},
        
        {"Use After Free", "Accessing freed memory", 
         SIGSEGV, "Use smart pointers or track memory lifecycle", 4}
    };
}

bool CoreDumpManager::configure(const CoreDumpConfig& config) {
    config_ = config;
    return enableCoreDump(config.max_size);
}

bool CoreDumpManager::enableCoreDump(size_t maxSize) {
    struct rlimit rlim;
    
    rlim.rlim_cur = (maxSize == 0) ? RLIM_INFINITY : maxSize;
    rlim.rlim_max = RLIM_INFINITY;
    
    if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
        std::cerr << "[CoreDump] Failed to set core dump limit: " 
                  << strerror(errno) << std::endl;
        return false;
    }
    
    // Also ignore SIGPIPE to prevent crashes from broken pipes
    signal(SIGPIPE, SIG_IGN);
    
    is_enabled_ = true;
    config_.enabled = true;
    config_.max_size = maxSize;
    
    std::cout << "[CoreDump] Core dump enabled (max size: " 
              << (maxSize == 0 ? "unlimited" : std::to_string(maxSize)) << ")" 
              << std::endl;
    
    return true;
}

bool CoreDumpManager::disableCoreDump() {
    struct rlimit rlim;
    
    rlim.rlim_cur = 0;
    rlim.rlim_max = 0;
    
    if (setrlimit(RLIMIT_CORE, &rlim) != 0) {
        return false;
    }
    
    is_enabled_ = false;
    config_.enabled = false;
    
    return true;
}

bool CoreDumpManager::isEnabled() const {
    return is_enabled_;
}

const CoreDumpConfig& CoreDumpManager::getConfig() const {
    return config_;
}

const std::vector<CrashPattern>& CoreDumpManager::getPatterns() const {
    return patterns_;
}

CrashPattern CoreDumpManager::getPattern(int signalNumber) const {
    for (const auto& pattern : patterns_) {
        if (pattern.signal_number == signalNumber) {
            return pattern;
        }
    }
    // Return default pattern if not found
    return {"Unknown", "Unknown signal", 0, "Review crash details", 1};
}

std::string CoreDumpManager::generateCoreFileName(const std::string& prefix) const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << prefix << "_";
    
    if (config_.timestamp_suffix) {
        oss << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S");
        oss << "_" << std::setfill('0') << std::setw(3) << ms.count();
    }
    
    oss << "_" << getpid() << ".core";
    
    return config_.output_dir + "/" + oss.str();
}

std::string CoreDumpManager::getSignalName(int signalNum) {
    switch (signalNum) {
        case 1: return "SIGHUP";
        case 2: return "SIGINT";
        case 3: return "SIGQUIT";
        case 4: return "SIGILL";
        case 5: return "SIGTRAP";
        case 6: return "SIGABRT";
        case 7: return "SIGBUS";
        case 8: return "SIGFPE";
        case 9: return "SIGKILL";
        case 10: return "SIGUSR1";
        case 11: return "SIGSEGV";
        case 12: return "SIGUSR2";
        case 13: return "SIGPIPE";
        case 14: return "SIGALRM";
        case 15: return "SIGTERM";
        case 16: return "SIGSTKFLT";
        case 17: return "SIGCHLD";
        case 18: return "SIGCONT";
        case 19: return "SIGSTOP";
        case 20: return "SIGTSTP";
        case 21: return "SIGTTIN";
        case 22: return "SIGTTOU";
        case 23: return "SIGURG";
        case 24: return "SIGXCPU";
        case 25: return "SIGXFSZ";
        case 26: return "SIGVTALRM";
        case 27: return "SIGPROF";
        case 28: return "SIGWINCH";
        case 29: return "SIGIO";
        case 30: return "SIGPWR";
        case 31: return "SIGSYS";
        default: return "UNKNOWN";
    }
}

int CoreDumpManager::getSignalNumber(const std::string& signalName) {
    if (signalName == "SIGSEGV") return 11;
    if (signalName == "SIGFPE") return 8;
    if (signalName == "SIGILL") return 4;
    if (signalName == "SIGABRT") return 6;
    if (signalName == "SIGBUS") return 7;
    if (signalName == "SIGPIPE") return 13;
    if (signalName == "SIGTERM") return 15;
    if (signalName == "SIGINT") return 2;
    return 0;
}

} // namespace OSKernel