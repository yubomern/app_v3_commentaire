/*
 * PatternMatcher.cpp - Recherche de signatures de crash dans les données.
 * Ce fichier contient l'algorithme de correspondance de motifs qui détermine
 * les causes probables et les recommandations associées.
 */

#include "PatternMatcher.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace Analyzer {

std::vector<CrashSignature> PatternMatcher::embedded_patterns_;
bool PatternMatcher::patterns_loaded_ = false;

std::string to_lower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

bool contains_any(const std::string& haystack, const std::vector<std::string>& needles) {
    std::string lower = to_lower(haystack);
    for (const auto& n : needles) {
        if (lower.find(to_lower(n)) != std::string::npos) return true;
    }
    return false;
}

void PatternMatcher::LoadEmbeddedPatterns() {
    if (patterns_loaded_) return;

    // Memory-related crashes (most common in embedded systems)
    embedded_patterns_.push_back({
        "SIGSEGV/NullPtr", "Invalid memory access or null pointer dereference",
        "Check all pointer initializations before use. Use smart pointers (std::unique_ptr/shared_ptr). Validate array bounds.",
        {"Add null checks before pointer dereference", "Use RAII principles", "Implement bounds checking", "Use static analysis tools"},
        4, 0.92, "Memory"
    });

    // Arithmetic errors
    embedded_patterns_.push_back({
        "SIGFPE/DivZero", "Integer or floating point division by zero",
        "Add explicit checks for zero divisors. Use assertions or exceptions in math-heavy functions.",
        {"Validate denominators before division", "Use floating-point for safety", "Implement overflow checks"},
        3, 0.88, "Logic"
    });

    // Stack issues (critical in embedded with limited stack)
    embedded_patterns_.push_back({
        "StackOverflow", "Uncontrolled recursion or large stack allocation",
        "Convert recursion to iteration. Increase stack size via linker flags or ulimit -s. Avoid large local arrays.",
        {"Convert to iterative algorithms", "Increase stack size in linker script", "Use heap allocation for large data", "Monitor stack usage"},
        4, 0.85, "Memory"
    });

    // Heap management (important in resource-constrained systems)
    embedded_patterns_.push_back({
        "HeapCorrupt", "Memory management error (double free, UAF, overflow)",
        "Use AddressSanitizer (-fsanitize=address). Replace raw new/delete with smart pointers or containers.",
        {"Use smart pointers", "Implement custom memory pools", "Enable AddressSanitizer", "Use static memory allocation where possible"},
        4, 0.90, "Memory"
    });

    // Logic errors
    embedded_patterns_.push_back({
        "Assert/Abort", "Logical inconsistency or explicit abort",
        "Review condition checks. Ensure input validation happens before critical operations.",
        {"Add comprehensive input validation", "Use design-by-contract", "Implement watchdog timers", "Add error recovery mechanisms"},
        2, 0.80, "Logic"
    });

    // System-level issues
    embedded_patterns_.push_back({
        "Deadlock", "Thread synchronization deadlock",
        "Review mutex acquisition order. Use std::lock() for multiple mutexes. Implement timeout on locks.",
        {"Use std::scoped_lock for multiple mutexes", "Implement lock ordering", "Add deadlock detection", "Use lock-free algorithms where possible"},
        3, 0.75, "System"
    });

    patterns_loaded_ = true;
}

std::vector<CrashSignature> PatternMatcher::Analyze(const std::string& crash_desc,
                                                    const std::string& call_stack,
                                                    double cpu_usage, double mem_usage) {
    LoadEmbeddedPatterns();
    std::vector<CrashSignature> results;
    std::string combined = to_lower(crash_desc + " " + call_stack);

    for (const auto& pattern : embedded_patterns_) {
        if (contains_any(combined, {pattern.pattern})) {
            CrashSignature result = pattern;

            // Adjust confidence based on system metrics
            if (mem_usage > 90.0 && pattern.risk_category == "Memory") {
                result.probable_cause += " [Memory Exhaustion Critical]";
                result.confidence = std::min(1.0, result.confidence + 0.08);
            }
            if (cpu_usage > 85.0 && pattern.risk_category == "System") {
                result.probable_cause += " [High System Load]";
                result.confidence = std::min(1.0, result.confidence + 0.05);
            }

            results.push_back(result);
        }
    }

    if (results.empty()) {
        results.push_back({
            "Unknown", "No specific pattern matched - requires manual analysis",
            "Use GDB: `gdb ./app core` -> `bt` -> `info registers`. Check system logs and dmesg.",
            {"Manual debugging required", "Check system logs", "Use core dump analysis", "Review recent code changes"},
            1, 0.30, "Unknown"
        });
    }

    return results;
}

AnalysisResult PatternMatcher::DetailedAnalysis(const std::string& crash_desc,
                                               const std::string& call_stack,
                                               const std::string& backtrace,
                                               double cpu_usage, double mem_usage) {
    AnalysisResult result;
    auto signatures = Analyze(crash_desc, call_stack, cpu_usage, mem_usage);

    if (!signatures.empty()) {
        result.signature = signatures[0]; // Use the highest confidence match

        // Generate detailed analysis
        std::ostringstream analysis;
        analysis << "Crash Analysis Report\n";
        analysis << "===================\n\n";
        analysis << "Primary Cause: " << result.signature.probable_cause << "\n";
        analysis << "Severity: " << SeverityToString(result.signature.severity) << "\n";
        analysis << "Confidence: " << ConfidenceToString(result.signature.confidence) << "\n";
        analysis << "Risk Category: " << result.signature.risk_category << "\n\n";

        analysis << "System Context:\n";
        analysis << "- CPU Usage: " << cpu_usage << "%\n";
        analysis << "- Memory Usage: " << mem_usage << "%\n";
        analysis << "- Call Stack Depth: " << std::count(call_stack.begin(), call_stack.end(), '\n') << " frames\n\n";

        result.detailed_analysis = analysis.str();

        // Generate recommended actions
        result.recommended_actions = result.signature.proposed_solutions;

        // Embedded system considerations
        result.embedded_system_considerations = AssessEmbeddedImpact(result.signature.pattern);
    }

    return result;
}

std::string PatternMatcher::SeverityToString(int s) {
    switch(s) {
        case 4: return "CRITICAL";
        case 3: return "HIGH";
        case 2: return "MEDIUM";
        default: return "LOW";
    }
}

std::string PatternMatcher::ConfidenceToString(double c) {
    std::ostringstream oss;
    oss << (c * 100.0) << "%";
    return oss.str();
}

std::vector<std::string> PatternMatcher::GetEmbeddedSolutions(const std::string& crash_type) {
    LoadEmbeddedPatterns();

    for (const auto& pattern : embedded_patterns_) {
        if (pattern.pattern.find(crash_type) != std::string::npos) {
            return pattern.proposed_solutions;
        }
    }

    return {"Implement comprehensive error handling", "Add system monitoring", "Use watchdog timers"};
}

std::string PatternMatcher::AssessEmbeddedImpact(const std::string& crash_type) {
    if (crash_type.find("Memory") != std::string::npos || crash_type.find("Heap") != std::string::npos) {
        return "High impact on embedded systems: Memory corruption can cause system-wide instability. "
               "Consider using memory-protected regions or external watchdog for recovery.";
    } else if (crash_type.find("Stack") != std::string::npos) {
        return "Critical for embedded systems: Limited stack space. "
               "Monitor stack usage and implement stack overflow protection.";
    } else if (crash_type.find("Deadlock") != std::string::npos) {
        return "System-level impact: Thread deadlocks can freeze the entire system. "
               "Implement priority inheritance and deadlock detection mechanisms.";
    }

    return "Standard embedded considerations: Implement graceful error recovery and system monitoring.";
}

} // namespace Analyzer