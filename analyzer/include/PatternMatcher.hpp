#pragma once
/*
 * PatternMatcher.hpp - Interface de correspondance de motifs.
 * Déclare les signatures et les structures utilisées pour détecter
 * les causes probables à partir des données de crash.
 */

#include <string>
#include <vector>
#include <map>

namespace Analyzer {

struct CrashSignature {
    std::string pattern;          // Regex-like or keyword match
    std::string probable_cause;
    std::string solution_hint;
    std::vector<std::string> proposed_solutions;  // Multiple solution options
    int severity;                 // 1:LOW, 2:MEDIUM, 3:HIGH, 4:CRITICAL
    double confidence;            // 0.0 - 1.0
    std::string risk_category;    // Memory, Logic, Performance, System
};

struct AnalysisResult {
    CrashSignature signature;
    std::string detailed_analysis;
    std::vector<std::string> recommended_actions;
    std::string embedded_system_considerations;  // Specific to embedded Linux
};

class PatternMatcher {
public:
    static std::vector<CrashSignature> Analyze(const std::string& crash_desc,
                                               const std::string& call_stack,
                                               double cpu_usage, double mem_usage);

    static AnalysisResult DetailedAnalysis(const std::string& crash_desc,
                                          const std::string& call_stack,
                                          const std::string& backtrace,
                                          double cpu_usage, double mem_usage);

    static std::string SeverityToString(int severity);
    static std::string ConfidenceToString(double score);

    // Embedded system specific analysis
    static std::vector<std::string> GetEmbeddedSolutions(const std::string& crash_type);
    static std::string AssessEmbeddedImpact(const std::string& crash_type);

private:
    static void LoadEmbeddedPatterns();
    static std::vector<CrashSignature> embedded_patterns_;
    static bool patterns_loaded_;
};

} // namespace Analyzer