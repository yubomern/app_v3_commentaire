/*
 * DatabaseManager.cpp - Gestion simple de la base de données interne.
 * Ce module stocke les enregistrements de crash en mémoire et fournit
 * une interface pour l'insertion et la lecture des résultats d'analyse.
 */

#include "DatabaseManager.hpp"
#include <algorithm>
#include <iostream>
#include <map>

namespace Analyzer {

DatabaseManager::DatabaseManager(const std::string& db_path) : db_path_(db_path), next_id_(1) {
    std::cout << "Using in-memory database (SQLite3 temporarily disabled)" << std::endl;
}

DatabaseManager::~DatabaseManager() {
    // Cleanup handled by vector destructor
}

bool DatabaseManager::InsertCrashRecord(const CrashRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    CrashRecord rec = record;
    rec.id = next_id_++;
    records_.push_back(rec);
    return true;
}

std::vector<CrashRecord> DatabaseManager::GetAllRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_;
}

std::vector<CrashRecord> DatabaseManager::GetRecordsBySeverity(const std::string& severity) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CrashRecord> filtered;
    std::copy_if(records_.begin(), records_.end(), std::back_inserter(filtered),
                 [&severity](const CrashRecord& rec) { return rec.severity == severity; });
    return filtered;
}

bool DatabaseManager::ClearAllRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
    next_id_ = 1;
    return true;
}

int DatabaseManager::GetTotalCrashCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return records_.size();
}

std::vector<std::pair<std::string, int>> DatabaseManager::GetCrashTypeDistribution() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::map<std::string, int> distribution;
    for (const auto& record : records_) {
        distribution[record.fault_type]++;
    }

    std::vector<std::pair<std::string, int>> result;
    for (const auto& pair : distribution) {
        result.push_back(pair);
    }
    return result;
}

double DatabaseManager::GetAverageConfidence() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (records_.empty()) return 0.0;

    double sum = 0.0;
    for (const auto& record : records_) {
        sum += record.confidence;
    }
    return sum / records_.size();
}

} // namespace Analyzer