/*
 * CrashTypes.hpp - Définition des types et structures de crash.
 * Contient les catégories de défaillance, la configuration de simulation
 * et les structures de rapport utilisées par le simulateur.
 */

#pragma once

#include <string>
#include <random>
#include <chrono>
#include <cstdint>

namespace CrashSimulator {

enum class CrashCategory : uint32_t {
    SEGMENTATION_FAULT = 0,
    DIVISION_BY_ZERO,
    ILLEGAL_INSTRUCTION,
    STACK_OVERFLOW,
    HEAP_CORRUPTION,
    DEADLOCK,
    ASSERTION_FAILURE,
    RANDOM_MATH_FAULT,
    SYSTEM_CALL_FAILURE,
    ACCESS_VIOLATION,
    INVALID_HANDLE,
    STACK_BUFFER_OVERFLOW,
    BUS_ERROR,
    DOUBLE_FREE,
    USE_AFTER_FREE,
    ABORT,
    KILL_SIGNAL,
    TIMEOUT,
    COUNT  // Must be last
};

struct CrashConfig {
    CrashCategory category;
    unsigned int seed;
    int recursion_depth;
    size_t memory_size;
    bool random_mode;
    int intensity;  // 1-10
    bool generate_minidump;
};

struct CrashReport {
    CrashCategory category;
    unsigned int seed;
    std::chrono::system_clock::time_point timestamp;
    std::string description;
    int call_chain_depth;
    uint32_t exceptionCode;      // Added for Windows exception handling
    std::string exception_address;
    
    
    CrashReport() : category(CrashCategory::SEGMENTATION_FAULT), seed(0), 
                    call_chain_depth(0), exceptionCode(0) {}
};

inline std::string categoryToString(CrashCategory cat) {
    switch (cat) {
        case CrashCategory::SEGMENTATION_FAULT: return "Segmentation Fault";
        case CrashCategory::DIVISION_BY_ZERO: return "Integer Division by Zero";
        case CrashCategory::ILLEGAL_INSTRUCTION: return "Illegal Instruction";
        case CrashCategory::STACK_OVERFLOW: return "Stack Overflow";
        case CrashCategory::HEAP_CORRUPTION: return "Heap Corruption";
        case CrashCategory::DEADLOCK: return "Deadlock / Infinite Loop";
        case CrashCategory::ASSERTION_FAILURE: return "Assertion Failure";
        case CrashCategory::RANDOM_MATH_FAULT: return "Random Math Fault";
        case CrashCategory::SYSTEM_CALL_FAILURE: return "System Call Failure";
        case CrashCategory::ACCESS_VIOLATION: return "Access Violation";
        case CrashCategory::INVALID_HANDLE: return "Invalid Handle Usage";
        case CrashCategory::STACK_BUFFER_OVERFLOW: return "Stack Buffer Overflow";
        case CrashCategory::BUS_ERROR: return "Bus Error";
        case CrashCategory::DOUBLE_FREE: return "Double Free";
        case CrashCategory::USE_AFTER_FREE: return "Use After Free";
        case CrashCategory::ABORT: return "Abort Signal";
        case CrashCategory::KILL_SIGNAL: return "Kill Signal";
        case CrashCategory::TIMEOUT: return "Timeout";
        default: return "Unknown";
    }
}

// Crash severity levels
enum class CrashSeverity {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

inline int getSeverity(CrashCategory cat) {
    switch (cat) {
        case CrashCategory::SEGMENTATION_FAULT:
        case CrashCategory::ACCESS_VIOLATION:
        case CrashCategory::STACK_OVERFLOW:
        case CrashCategory::HEAP_CORRUPTION:
        case CrashCategory::BUS_ERROR:
            return 4; // CRITICAL
        case CrashCategory::DIVISION_BY_ZERO:
        case CrashCategory::ILLEGAL_INSTRUCTION:
        case CrashCategory::DOUBLE_FREE:
        case CrashCategory::USE_AFTER_FREE:
            return 3; // HIGH
        case CrashCategory::ABORT:
        case CrashCategory::KILL_SIGNAL:
            return 2; // MEDIUM
        case CrashCategory::TIMEOUT:
        case CrashCategory::STACK_BUFFER_OVERFLOW:
        case CrashCategory::DEADLOCK:
        case CrashCategory::ASSERTION_FAILURE:
        case CrashCategory::RANDOM_MATH_FAULT:
        case CrashCategory::SYSTEM_CALL_FAILURE:
        case CrashCategory::INVALID_HANDLE:
            return 1; // LOW
        default:
            return 2;
    }
}

// Crash information structure
struct CrashInfo {
    CrashCategory category;
    CrashSeverity severity;
    std::string description;
    std::string hint;
    int signal_number;
    std::string timestamp;
};

} // namespace CrashSimulator