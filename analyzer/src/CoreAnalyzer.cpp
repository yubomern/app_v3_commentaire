/*
 * CoreAnalyzer.cpp - Moteur principal de l'analyse de crash.
 * Ce fichier orchestre le pipeline : lecture de données, analyse ML,
 * détection de motifs, stockage et export.
 */

#include "CoreAnalyzer.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

#ifdef PLATFORM_LINUX
#include <execinfo.h>
#include <unistd.h>
#endif

namespace Analyzer {

CoreAnalyzer::CoreAnalyzer(const std::string& db_path) : db_(db_path), embedded_mode_(false) {
    // Initialize database
    // Train ML model with historical data if available
    TrainMLFromHistoricalData();
}

void CoreAnalyzer::ProcessInput(const std::string& input_csv, const std::string& output_csv) {
    auto raw_data = CSVExporter::ImportFromCSV(input_csv);
    if (raw_data.empty()) {
        std::cerr << "⚠️ No data found in " << input_csv << "\n";
        return;
    }

    std::cout << "🔍 Analyzing " << raw_data.size() << " crash records...\n";

    for (const auto& raw : raw_data) {
        // Enhanced analysis with backtrace collection
        std::string backtrace = CollectBacktrace();
        auto analysis = AnalyzeCrashWithBacktrace(raw.fault_type, backtrace, raw.cpu, raw.mem);

        // Generate solution proposals
        SolutionProposer proposer;
        auto solution = proposer.GenerateSolution(raw.fault_type, PatternMatcher::SeverityToString(analysis.signature.severity),
                                                raw.cpu, raw.mem, embedded_mode_);

        CrashRecord rec;
        rec.timestamp = raw.timestamp;
        rec.fault_type = raw.fault_type;
        rec.file = raw.file;
        rec.line = raw.line;
        rec.cpu_usage = raw.cpu;
        rec.mem_usage = raw.mem;
        rec.probable_cause = analysis.signature.probable_cause;
        rec.severity = PatternMatcher::SeverityToString(analysis.signature.severity);
        rec.solution_hint = analysis.signature.solution_hint;
        rec.confidence = analysis.signature.confidence;
        rec.backtrace = backtrace;
        rec.proposed_solution = solution.immediate_actions.empty() ?
                               analysis.embedded_system_considerations :
                               solution.immediate_actions[0];
        rec.ml_prediction = "ML Analysis: " + std::to_string(analysis.signature.confidence * 100) + "% confidence";

        db_.InsertCrashRecord(rec);
        cache_.push_back(rec);
    }

    // Export enriched data
    ExportToCSV(output_csv);
    ExportToJSON(output_csv + ".json");
    std::cout << "✅ Analysis complete. Results saved to DB, CSV and JSON\n";
}

void CoreAnalyzer::ProcessInputWithML(const std::string& input_csv, const std::string& output_csv) {
    auto raw_data = CSVExporter::ImportFromCSV(input_csv);
    if (raw_data.empty()) {
        std::cerr << "⚠️ No data found in " << input_csv << "\n";
        return;
    }

    std::cout << "🤖 ML-Enhanced Analysis of " << raw_data.size() << " crash records...\n";

    for (const auto& raw : raw_data) {
        // ML Analysis
        auto ml_result = ml_analyzer_.AnalyzeCrash(raw.fault_type, "", raw.cpu, raw.mem);

        // Pattern Analysis
        auto patterns = PatternMatcher::Analyze(raw.fault_type, "", raw.cpu, raw.mem);

        CrashRecord rec;
        rec.timestamp = raw.timestamp;
        rec.fault_type = raw.fault_type;
        rec.file = raw.file;
        rec.line = raw.line;
        rec.cpu_usage = raw.cpu;
        rec.mem_usage = raw.mem;
        rec.probable_cause = ml_result.predicted_cause;
        rec.severity = ml_result.risk_level;
        rec.solution_hint = ml_result.recommended_solution;
        rec.confidence = ml_result.confidence_score;
        rec.backtrace = "ML Analysis Backtrace";
        rec.proposed_solution = ml_result.recommended_solution;
        rec.ml_prediction = "ML: " + ml_result.predicted_cause + " (" + std::to_string(ml_result.confidence_score * 100) + "%)";

        db_.InsertCrashRecord(rec);
        cache_.push_back(rec);
    }

    ExportToCSV(output_csv);
    ExportToJSON(output_csv + ".json");
    std::cout << "✅ ML Analysis complete. Enhanced results saved\n";
}

std::string CoreAnalyzer::CollectBacktrace() {
#ifdef __linux__
    const int max_frames = 64;
    void* buffer[max_frames];
    int num_frames = backtrace(buffer, max_frames);
    char** symbols = backtrace_symbols(buffer, num_frames);

    std::ostringstream oss;
    oss << "Backtrace (" << num_frames << " frames):\n";
    for (int i = 0; i < num_frames; ++i) {
        oss << "  [" << i << "] " << symbols[i] << "\n";
    }

    free(symbols);
    return oss.str();
#else
    return "Backtrace collection not available on this platform";
#endif
}

AnalysisResult CoreAnalyzer::AnalyzeCrashWithBacktrace(const std::string& crash_desc,
                                                     const std::string& backtrace,
                                                     double cpu_usage,
                                                     double mem_usage) {
    return PatternMatcher::DetailedAnalysis(crash_desc, "", backtrace, cpu_usage, mem_usage);
}

std::vector<std::string> CoreAnalyzer::GetEmbeddedRecommendations(const std::string& crash_type) {
    return PatternMatcher::GetEmbeddedSolutions(crash_type);
}

void CoreAnalyzer::PrintSummary() const {
    if (cache_.empty()) return;

    std::cout << "\n📊 ENHANCED ANALYSIS SUMMARY:\n";
    std::cout << "==================================================\n";
    std::cout << "Total Crashes Analyzed: " << cache_.size() << "\n";
    std::cout << "Embedded Mode: " << (embedded_mode_ ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "--------------------------------------------------\n";

    for (const auto& c : cache_) {
        std::cout << "⏱  " << c.timestamp << "\n";
        std::cout << "🔍 Type: " << c.fault_type << " | Severity: " << c.severity << "\n";
        std::cout << "💡 Cause: " << c.probable_cause << "\n";
        std::cout << "🛠  Solution: " << c.solution_hint << "\n";
        std::cout << "🎯 Confidence: " << PatternMatcher::ConfidenceToString(c.confidence) << "\n";
        if (!c.backtrace.empty() && c.backtrace != "ML Analysis Backtrace") {
            std::cout << "🔧 Backtrace: Available (" << c.backtrace.length() << " chars)\n";
        }
        if (!c.ml_prediction.empty()) {
            std::cout << "🤖 ML Prediction: " << c.ml_prediction << "\n";
        }
        std::cout << "--------------------------------------------------\n";
    }

    // Database statistics
    auto distribution = db_.GetCrashTypeDistribution();
    std::cout << "\n📈 CRASH DISTRIBUTION:\n";
    for (const auto& dist : distribution) {
        std::cout << "  " << dist.first << ": " << dist.second << " occurrences\n";
    }
    std::cout << "Average Confidence: " << PatternMatcher::ConfidenceToString(db_.GetAverageConfidence()) << "\n";
}

bool CoreAnalyzer::ExportToCSV(const std::string& filename) {
    std::vector<CrashData> export_data;
    for (const auto& c : cache_) {
        export_data.push_back({
            c.timestamp, c.fault_type, c.file, c.line, c.cpu_usage, c.mem_usage,
            c.probable_cause, c.severity, c.solution_hint, c.confidence,
            c.backtrace, c.proposed_solution, c.ml_prediction
        });
    }
    return CSVExporter::ExportToCSV(filename, export_data);
}

bool CoreAnalyzer::ExportToJSON(const std::string& filename) {
    std::vector<CrashData> export_data;
    for (const auto& c : cache_) {
        export_data.push_back({
            c.timestamp, c.fault_type, c.file, c.line, c.cpu_usage, c.mem_usage,
            c.probable_cause, c.severity, c.solution_hint, c.confidence,
            c.backtrace, c.proposed_solution, c.ml_prediction
        });
    }
    return CSVExporter::ExportToJSON(filename, export_data);
}

void CoreAnalyzer::TrainMLFromHistoricalData() {
    // Train ML model with existing database records
    auto records = db_.GetAllRecords();
    if (records.size() > 10) {  // Need minimum data for training
        std::vector<std::string> descriptions, causes;
        for (const auto& rec : records) {
            descriptions.push_back(rec.fault_type + " " + rec.probable_cause);
            causes.push_back(rec.probable_cause);
        }
        ml_analyzer_.TrainModel(descriptions, causes);
        std::cout << "🤖 ML Model trained with " << records.size() << " historical records\n";
    }
}

} // namespace Analyzer