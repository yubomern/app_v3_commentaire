/*
 * MLAnalyzer.cpp - Analyse du crash par apprentissage automatique.
 * Ce module applique des modèles et des règles pour prédire les causes,
 * suggérer des solutions et classer la gravité.
 */

#include "MLAnalyzer.hpp"
#include <algorithm>
#include <sstream>
#include <set>
#include <cmath>
#include <iostream>

namespace Analyzer {

MLAnalyzer::MLAnalyzer() {
    LoadDefaultPatterns();
}

MLAnalyzer::~MLAnalyzer() {
    // Cleanup if needed
}

void MLAnalyzer::LoadDefaultPatterns() {
    // Default patterns for crash analysis
    cause_patterns_["segmentation fault"] = "Memory access violation - null pointer dereference or buffer overflow";
    cause_patterns_["division by zero"] = "Arithmetic error - division by zero in mathematical operation";
    cause_patterns_["illegal instruction"] = "CPU instruction error - invalid or unsupported opcode";
    cause_patterns_["stack overflow"] = "Stack exhaustion - infinite recursion or excessive local variables";
    cause_patterns_["heap corruption"] = "Memory management error - buffer overflow or use-after-free";
    cause_patterns_["deadlock"] = "Synchronization error - circular wait condition in threads";
    cause_patterns_["assertion failure"] = "Logic error - failed runtime assertion check";
    cause_patterns_["bus error"] = "Memory alignment error - unaligned memory access";

    // Solution database
    solution_database_["Memory access violation"] = {
        "Add null pointer checks before dereferencing",
        "Use smart pointers (unique_ptr, shared_ptr)",
        "Implement bounds checking for array access",
        "Use memory debugging tools (Valgrind, AddressSanitizer)"
    };

    solution_database_["Arithmetic error"] = {
        "Add division by zero checks",
        "Use floating-point arithmetic where appropriate",
        "Validate input parameters before calculations"
    };

    solution_database_["Stack exhaustion"] = {
        "Check for infinite recursion conditions",
        "Reduce stack frame size by using heap allocation",
        "Increase stack size limit if necessary",
        "Implement iterative algorithms instead of recursive"
    };
}

void MLAnalyzer::TrainModel(const std::vector<std::string>& crash_descriptions,
                           const std::vector<std::string>& causes) {
    if (crash_descriptions.size() != causes.size()) {
        std::cerr << "Training data size mismatch" << std::endl;
        return;
    }

    for (size_t i = 0; i < crash_descriptions.size(); ++i) {
        std::string keywords = ExtractKeywords(crash_descriptions[i]);
        cause_patterns_[keywords] = causes[i];
        crash_frequencies_[causes[i]]++;
    }

    std::cout << "ML Model trained with " << crash_descriptions.size() << " samples" << std::endl;
}

MLAnalysisResult MLAnalyzer::AnalyzeCrash(const std::string& crash_description,
                                         const std::string& backtrace,
                                         double cpu_usage,
                                         double mem_usage) {
    MLAnalysisResult result;
    result.confidence_score = 0.0;

    // Extract keywords from description and backtrace
    std::string combined_text = crash_description + " " + backtrace;
    std::string keywords = ExtractKeywords(combined_text);

    // Find best matching cause
    double best_similarity = 0.0;
    std::string best_cause;

    for (const auto& pattern : cause_patterns_) {
        double similarity = CalculateSimilarity(keywords, pattern.first);
        if (similarity > best_similarity) {
            best_similarity = similarity;
            best_cause = pattern.second;
        }
    }

    result.predicted_cause = best_cause.empty() ? "Unknown cause" : best_cause;
    result.confidence_score = best_similarity;

    // Determine risk level based on system metrics
    result.risk_level = PredictRiskLevel(crash_description, cpu_usage, mem_usage);

    // Find similar crashes
    result.similar_crashes = FindSimilarCrashes(crash_description);

    // Generate solution recommendation
    if (!best_cause.empty()) {
        auto it = solution_database_.find(best_cause);
        if (it != solution_database_.end() && !it->second.empty()) {
            result.recommended_solution = it->second[0]; // Use first solution
        } else {
            result.recommended_solution = "Investigate with debugging tools and code review";
        }
    }

    return result;
}

std::vector<std::string> MLAnalyzer::FindSimilarCrashes(const std::string& description) {
    std::vector<std::string> similar;
    std::string keywords = ExtractKeywords(description);

    for (const auto& pattern : cause_patterns_) {
        if (CalculateSimilarity(keywords, pattern.first) > 0.3) { // 30% similarity threshold
            similar.push_back(pattern.second);
        }
    }

    return similar;
}

std::string MLAnalyzer::PredictRiskLevel(const std::string& crash_type,
                                        double cpu_usage, double mem_usage) {
    // Simple risk assessment based on crash type and system load
    if (cpu_usage > 90.0 || mem_usage > 90.0) {
        return "CRITICAL";
    } else if (cpu_usage > 70.0 || mem_usage > 70.0) {
        return "HIGH";
    } else if (cpu_usage > 50.0 || mem_usage > 50.0) {
        return "MEDIUM";
    }
    return "LOW";
}

std::map<std::string, double> MLAnalyzer::GetCrashProbabilities() {
    std::map<std::string, double> probabilities;
    int total_crashes = 0;

    for (const auto& freq : crash_frequencies_) {
        total_crashes += freq.second;
    }

    for (const auto& freq : crash_frequencies_) {
        probabilities[freq.first] = static_cast<double>(freq.second) / total_crashes;
    }

    return probabilities;
}

std::vector<std::pair<std::string, int>> MLAnalyzer::GetCommonPatterns() {
    std::vector<std::pair<std::string, int>> patterns;
    for (const auto& freq : crash_frequencies_) {
        patterns.emplace_back(freq.first, freq.second);
    }

    // Sort by frequency (descending)
    std::sort(patterns.begin(), patterns.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return patterns;
}

double MLAnalyzer::CalculateSimilarity(const std::string& str1, const std::string& str2) {
    // Simple Jaccard similarity for keyword sets
    std::istringstream iss1(str1), iss2(str2);
    std::set<std::string> set1, set2;

    std::string word;
    while (iss1 >> word) set1.insert(word);
    while (iss2 >> word) set2.insert(word);

    std::vector<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                         std::back_inserter(intersection));

    std::vector<std::string> union_set;
    std::set_union(set1.begin(), set1.end(), set2.begin(), set2.end(),
                  std::back_inserter(union_set));

    if (union_set.empty()) return 0.0;
    return static_cast<double>(intersection.size()) / union_set.size();
}

std::string MLAnalyzer::ExtractKeywords(const std::string& text) {
    std::istringstream iss(text);
    std::string keywords;
    std::string word;

    while (iss >> word) {
        // Convert to lowercase and filter common words
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        if (word.length() > 2 && word != "the" && word != "and" && word != "for" && word != "are") {
            keywords += word + " ";
        }
    }

    return keywords;
}

} // namespace Analyzer