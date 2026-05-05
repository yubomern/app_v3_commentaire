#pragma once
/*
 * CoreAnalyzer.hpp - Interface du moteur d'analyse principal.
 * Cette interface définit la classe qui orchestre le traitement des entrées,
 * l'analyse ML, la correspondance de motifs et l'export des résultats.
 */

#include "DatabaseManager.hpp"
#include "CSVExporter.hpp"
#include "PatternMatcher.hpp"
#include "MLAnalyzer.hpp"
#include "SolutionProposer.hpp"
#include <string>
#include <vector>

namespace Analyzer {

class CoreAnalyzer {
public:
    CoreAnalyzer(const std::string& db_path = "crash_analysis.db");

    // Enhanced pipeline: CSV/Core -> ML Analysis -> Pattern Matching -> DB -> Export
    void ProcessInput(const std::string& input_csv, const std::string& output_csv = "analysis_report.csv");
    void ProcessInputWithML(const std::string& input_csv, const std::string& output_csv = "ml_analysis_report.csv");

    // Backtrace collection and analysis
    std::string CollectBacktrace();
    AnalysisResult AnalyzeCrashWithBacktrace(const std::string& crash_desc,
                                           const std::string& backtrace,
                                           double cpu_usage = 0.0,
                                           double mem_usage = 0.0);

    // Embedded system specific analysis
    void EnableEmbeddedMode(bool enabled) { embedded_mode_ = enabled; }
    std::vector<std::string> GetEmbeddedRecommendations(const std::string& crash_type);

    // Affiche résumé dans la console
    void PrintSummary() const;

    // Export functions
    bool ExportToCSV(const std::string& filename);
    bool ExportToJSON(const std::string& filename);

    const DatabaseManager& GetDB() const { return db_; }
    MLAnalyzer& GetMLAnalyzer() { return ml_analyzer_; }

private:
    DatabaseManager db_;
    MLAnalyzer ml_analyzer_;
    std::vector<CrashRecord> cache_;
    bool embedded_mode_;

    // Internal processing
    CrashData ProcessSingleCrash(const std::string& csv_line);
    void TrainMLFromHistoricalData();
};

} // namespace Analyzer