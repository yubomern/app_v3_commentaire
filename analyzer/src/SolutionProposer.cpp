/*
 * SolutionProposer.cpp - Génération de recommandations de correction.
 * Ce module prépare des actions immédiates et des mesures préventives
 * adaptées aux types de crash identifiés.
 */

#include "SolutionProposer.hpp"
#include <algorithm>

namespace Analyzer {

SolutionProposer::SolutionProposer() {
    InitializeSolutions();
}

SolutionProposer::~SolutionProposer() {
    // Cleanup if needed
}

void SolutionProposer::InitializeSolutions() {
    // Memory-related crashes
    solution_database_["SIGSEGV/NullPtr"] = {
        "SIGSEGV/NullPtr",
        "HIGH",
        {
            "Add null pointer checks before dereference",
            "Use assert() for debugging builds",
            "Implement RAII patterns with smart pointers"
        },
        {
            "Use std::unique_ptr and std::shared_ptr consistently",
            "Implement comprehensive input validation",
            "Add static analysis tools to CI/CD pipeline",
            "Use AddressSanitizer in debug builds"
        },
        {
            "Implement memory protection units (MPU)",
            "Use watchdog timers for null pointer recovery",
            "Add memory boundary checks in embedded allocators",
            "Implement dual-redundant pointer validation"
        },
        "2-4 hours",
        "Unit tests for pointer operations, integration tests with invalid inputs"
    };

    // Arithmetic errors
    solution_database_["SIGFPE/DivZero"] = {
        "SIGFPE/DivZero",
        "MEDIUM",
        {
            "Add division by zero checks",
            "Use floating-point division as safe fallback",
            "Validate denominators before division operations"
        },
        {
            "Implement safe math library with overflow checks",
            "Use design-by-contract for mathematical functions",
            "Add comprehensive input range validation"
        },
        {
            "Use fixed-point arithmetic where possible",
            "Implement hardware floating-point unit validation",
            "Add arithmetic overflow detection in embedded math libraries"
        },
        "1-2 hours",
        "Boundary value tests for all mathematical operations"
    };

    // Stack issues
    solution_database_["StackOverflow"] = {
        "StackOverflow",
        "CRITICAL",
        {
            "Convert recursive algorithms to iterative",
            "Reduce stack frame sizes",
            "Increase stack size in linker script"
        },
        {
            "Implement stack usage monitoring",
            "Use heap allocation for large data structures",
            "Set appropriate stack size limits in embedded systems"
        },
        {
            "Configure MPU for stack protection",
            "Implement stack canaries",
            "Use separate stacks for interrupts vs main code",
            "Monitor stack usage with hardware counters"
        },
        "4-8 hours",
        "Stack usage analysis, worst-case execution time testing"
    };

    // Heap corruption
    solution_database_["HeapCorrupt"] = {
        "HeapCorrupt",
        "CRITICAL",
        {
            "Replace malloc/free with new/delete",
            "Use smart pointers and containers",
            "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
        },
        {
            "Implement custom memory pools for embedded systems",
            "Use static memory allocation where possible",
            "Add heap corruption detection algorithms",
            "Implement memory defragmentation strategies"
        },
        {
            "Use memory-protected regions for heap",
            "Implement heap integrity checks",
            "Use dual-heap systems for critical allocations",
            "Add memory access pattern monitoring"
        },
        "6-12 hours",
        "Memory leak tests, heap corruption stress tests, valgrind analysis"
    };

    // System deadlocks
    solution_database_["Deadlock"] = {
        "Deadlock",
        "HIGH",
        {
            "Use std::scoped_lock for multiple mutexes",
            "Implement lock ordering discipline",
            "Add timeout to all lock operations"
        },
        {
            "Implement deadlock detection algorithms",
            "Use lock-free data structures where possible",
            "Add thread priority inheritance",
            "Implement comprehensive thread analysis tools"
        },
        {
            "Use real-time operating system primitives",
            "Implement priority ceiling protocol",
            "Add deadlock recovery mechanisms",
            "Use hardware-assisted locking where available"
        },
        "4-6 hours",
        "Multi-threaded stress tests, deadlock detection validation"
    };
}

SolutionProposal SolutionProposer::GenerateSolution(const std::string& crash_type,
                                                  const std::string& severity,
                                                  double cpu_usage,
                                                  double mem_usage,
                                                  bool embedded_system) {
    auto it = solution_database_.find(crash_type);
    if (it != solution_database_.end()) {
        SolutionProposal proposal = it->second;

        // Adjust risk level based on system metrics
        std::string dynamic_risk = DetermineRiskLevel(cpu_usage, mem_usage);
        if (dynamic_risk == "CRITICAL" || proposal.risk_level == "CRITICAL") {
            proposal.risk_level = "CRITICAL";
        }

        // Add embedded-specific solutions if requested
        if (embedded_system) {
            auto embedded_solutions = GenerateEmbeddedRecommendations(crash_type);
            proposal.embedded_specific_solutions.insert(
                proposal.embedded_specific_solutions.end(),
                embedded_solutions.begin(),
                embedded_solutions.end()
            );
        }

        return proposal;
    }

    // Default solution for unknown crash types
    return {
        crash_type,
        "UNKNOWN",
        {"Implement comprehensive error handling", "Add logging and monitoring"},
        {"Review code for similar patterns", "Implement defensive programming practices"},
        {"Add system monitoring", "Implement watchdog recovery"},
        "2-4 hours",
        "General system testing and code review"
    };
}

std::vector<std::string> SolutionProposer::GetQuickFixes(const std::string& crash_type) {
    auto it = solution_database_.find(crash_type);
    if (it != solution_database_.end()) {
        return it->second.immediate_actions;
    }
    return {"Add error checking", "Implement logging", "Add defensive programming"};
}

std::vector<std::string> SolutionProposer::GetEmbeddedSolutions(const std::string& crash_type) {
    return GenerateEmbeddedRecommendations(crash_type);
}

std::string SolutionProposer::AssessSystemImpact(const std::string& crash_type,
                                               double cpu_usage, double mem_usage) {
    std::string risk_level = DetermineRiskLevel(cpu_usage, mem_usage);

    std::string impact = "System Impact Assessment:\n";
    impact += "Risk Level: " + risk_level + "\n";

    if (cpu_usage > 90.0) {
        impact += "CRITICAL: High CPU usage indicates potential system overload\n";
    }
    if (mem_usage > 90.0) {
        impact += "CRITICAL: High memory usage indicates potential memory exhaustion\n";
    }

    auto it = solution_database_.find(crash_type);
    if (it != solution_database_.end()) {
        impact += "Crash Type: " + it->second.crash_type + "\n";
        impact += "Estimated Recovery Time: " + it->second.estimated_fix_time + "\n";
    }

    return impact;
}

std::string SolutionProposer::DetermineRiskLevel(double cpu_usage, double mem_usage) {
    if (cpu_usage > 90.0 || mem_usage > 90.0) {
        return "CRITICAL";
    } else if (cpu_usage > 70.0 || mem_usage > 70.0) {
        return "HIGH";
    } else if (cpu_usage > 50.0 || mem_usage > 50.0) {
        return "MEDIUM";
    }
    return "LOW";
}

std::vector<std::string> SolutionProposer::GenerateEmbeddedRecommendations(const std::string& crash_type) {
    std::vector<std::string> recommendations;

    if (crash_type.find("Memory") != std::string::npos || crash_type.find("Heap") != std::string::npos) {
        recommendations = {
            "Implement memory protection units (MPU)",
            "Use memory pools for dynamic allocation",
            "Add memory access pattern monitoring",
            "Implement dual-redundant memory validation"
        };
    } else if (crash_type.find("Stack") != std::string::npos) {
        recommendations = {
            "Configure stack overflow protection",
            "Use separate stacks for different contexts",
            "Implement stack usage monitoring",
            "Set appropriate stack sizes in linker scripts"
        };
    } else if (crash_type.find("Deadlock") != std::string::npos) {
        recommendations = {
            "Implement priority inheritance protocol",
            "Use real-time OS synchronization primitives",
            "Add deadlock detection and recovery",
            "Implement lock-free algorithms where possible"
        };
    } else {
        recommendations = {
            "Add hardware watchdog timers",
            "Implement system health monitoring",
            "Use redundant system design",
            "Add error recovery mechanisms"
        };
    }

    return recommendations;
}

} // namespace Analyzer