#pragma once
/*
 * MLAnalyzer.hpp - Interface de l'analyse par apprentissage automatique.
 * Déclare les structures et la classe utilisées pour prédire les causes
 * et recommander des solutions en fonction des données de crash.
 */

#include <string>
#include <vector>
#include <map>
#include <unordered_map>

namespace Analyzer {

struct MLAnalysisResult {
    std::string predicted_cause;
    double confidence_score;
    std::string recommended_solution;
    std::vector<std::string> similar_crashes;
    std::string risk_level; // LOW, MEDIUM, HIGH, CRITICAL
};

class MLAnalyzer {
public:
    MLAnalyzer();
    ~MLAnalyzer();

    // Training and prediction
    void TrainModel(const std::vector<std::string>& crash_descriptions,
                   const std::vector<std::string>& causes);
    MLAnalysisResult AnalyzeCrash(const std::string& crash_description,
                                 const std::string& backtrace = "",
                                 double cpu_usage = 0.0,
                                 double mem_usage = 0.0);

    // Pattern recognition
    std::vector<std::string> FindSimilarCrashes(const std::string& description);
    std::string PredictRiskLevel(const std::string& crash_type,
                                double cpu_usage, double mem_usage);

    // Statistical analysis
    std::map<std::string, double> GetCrashProbabilities();
    std::vector<std::pair<std::string, int>> GetCommonPatterns();

private:
    // Simple ML model using frequency analysis and pattern matching
    std::unordered_map<std::string, std::string> cause_patterns_;
    std::unordered_map<std::string, int> crash_frequencies_;
    std::unordered_map<std::string, std::vector<std::string>> solution_database_;

    // Helper methods
    double CalculateSimilarity(const std::string& str1, const std::string& str2);
    std::string ExtractKeywords(const std::string& text);
    void LoadDefaultPatterns();
};

} // namespace Analyzer