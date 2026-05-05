#pragma once
/*
 * DatabaseManager.hpp - Interface de gestion des enregistrements de crash.
 * Déclare la structure de données et les méthodes pour stocker puis récupérer
 * les résultats de l'analyse en mémoire.
 */

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace Analyzer {

struct CrashRecord {
    int id;
    std::string timestamp;
    std::string fault_type;
    std::string file;
    int line;
    double cpu_usage;
    double mem_usage;
    std::string probable_cause;
    std::string severity;
    std::string solution_hint;
    double confidence;
    std::string backtrace;
    std::string proposed_solution;
    std::string ml_prediction;
};

class DatabaseManager {
public:
    DatabaseManager(const std::string& db_path = ":memory:");
    ~DatabaseManager();

    // Database operations
    bool InitializeDatabase() { return true; } // Always succeeds for in-memory
    bool InsertCrashRecord(const CrashRecord& record);
    std::vector<CrashRecord> GetAllRecords();
    std::vector<CrashRecord> GetRecordsBySeverity(const std::string& severity);
    bool ClearAllRecords();

    // Statistics
    int GetTotalCrashCount() const;
    std::vector<std::pair<std::string, int>> GetCrashTypeDistribution() const;
    double GetAverageConfidence() const;

private:
    std::vector<CrashRecord> records_;
    mutable std::mutex mutex_;
    std::string db_path_;
    int next_id_;
};

} // namespace Analyzer