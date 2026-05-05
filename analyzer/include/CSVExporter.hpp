#pragma once
/*
 * CSVExporter.hpp - Interface d'export CSV.
 * Déclare les structures et fonctions pour écrire les résultats d'analyse
 * dans un fichier CSV structuré.
 */

#include <string>
#include <vector>

namespace Analyzer {

struct CrashData {
    std::string timestamp;
    std::string fault_type;
    std::string file;
    int line;
    double cpu;
    double mem;
    std::string cause;
    std::string severity;
    std::string hint;
    double confidence;
    std::string backtrace;        // Added for backtrace support
    std::string solution;         // Added for solution proposals
    std::string ml_prediction;    // Added for ML analysis
};

class CSVExporter {
public:
    static bool ExportToCSV(const std::string& filename, const std::vector<CrashData>& data);
    static bool ExportToJSON(const std::string& filename, const std::vector<CrashData>& data);
    static std::vector<CrashData> ImportFromCSV(const std::string& filename);
    static std::vector<CrashData> ImportFromJSON(const std::string& filename);
};

} // namespace Analyzer