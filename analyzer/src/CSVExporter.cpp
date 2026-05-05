/*
 * CSVExporter.cpp - Exportation des résultats d'analyse vers CSV.
 * Génère un fichier CSV structuré contenant les causes probables,
 * la sévérité, les solutions et les prédictions ML.
 */

#include "CSVExporter.hpp"
#include <fstream>
#include <sstream>
#include <ctime>
#include <iostream>
#include <map>
#include <vector>

namespace Analyzer {

bool CSVExporter::ExportToCSV(const std::string& filename, const std::vector<CrashData>& data) {
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    out << "timestamp,fault_type,file,line,cpu_usage,mem_usage,probable_cause,severity,solution_hint,confidence,backtrace,solution,ml_prediction\n";
    for (const auto& d : data) {
        auto esc = [](const std::string& s) {
            std::string r; for (char c : s) { if (c=='"') r+="\"\""; else r+=c; } return r;
        };
        out << "\"" << esc(d.timestamp)      << "\","
            << "\"" << esc(d.fault_type)     << "\","
            << "\"" << esc(d.file)           << "\","
            << d.line    << ","
            << d.cpu     << ","
            << d.mem     << ","
            << "\"" << esc(d.cause)          << "\","
            << "\"" << esc(d.severity)       << "\","
            << "\"" << esc(d.hint)           << "\","
            << d.confidence << ","
            << "\"" << esc(d.backtrace)      << "\","
            << "\"" << esc(d.solution)       << "\","
            << "\"" << esc(d.ml_prediction)  << "\"\n";
    }
    return true;
}

bool CSVExporter::ExportToJSON(const std::string& filename, const std::vector<CrashData>& data) {
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    auto esc = [](const std::string& s) {
        std::string r;
        for (char c : s) {
            if (c=='"') r+="\\\"";
            else if (c=='\\') r+="\\\\";
            else if (c=='\n') r+="\\n";
            else r+=c;
        }
        return r;
    };

    out << "{\n";
    out << "  \"crash_analysis_report\": {\n";
    out << "    \"generated_at\": \"" << std::time(nullptr) << "\",\n";
    out << "    \"total_crashes\": " << data.size() << ",\n";
    out << "    \"crashes\": [\n";

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& d = data[i];
        out << "      {\n";
        out << "        \"timestamp\": \""        << esc(d.timestamp)     << "\",\n";
        out << "        \"fault_type\": \""       << esc(d.fault_type)    << "\",\n";
        out << "        \"file\": \""             << esc(d.file)          << "\",\n";
        out << "        \"line\": "               << d.line               << ",\n";
        out << "        \"cpu_usage\": "          << d.cpu                << ",\n";
        out << "        \"mem_usage\": "          << d.mem                << ",\n";
        out << "        \"probable_cause\": \""   << esc(d.cause)         << "\",\n";
        out << "        \"severity\": \""         << esc(d.severity)      << "\",\n";
        out << "        \"solution_hint\": \""    << esc(d.hint)          << "\",\n";
        out << "        \"confidence\": "         << d.confidence         << ",\n";
        out << "        \"backtrace\": \""        << esc(d.backtrace)     << "\",\n";
        out << "        \"proposed_solution\": \"" << esc(d.solution)     << "\",\n";
        out << "        \"ml_prediction\": \""   << esc(d.ml_prediction)  << "\"\n";
        out << "      }";
        if (i < data.size() - 1) out << ",";
        out << "\n";
    }
    out << "    ]\n  }\n}\n";
    return true;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
static std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.substr(1, s.size() - 2);
    return s;
}

// Parse one CSV line respecting double-quoted fields
static std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i+1] == '"') {
                field += '"'; ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

// ─── ImportFromCSV: header-aware, works with both simulator and analyzer CSV ──
std::vector<CrashData> CSVExporter::ImportFromCSV(const std::string& filename) {
    std::vector<CrashData> data;
    std::ifstream in(filename);
    if (!in.is_open()) return data;

    // Read header
    std::string header_line;
    if (!std::getline(in, header_line)) return data;

    auto headers = splitCsvLine(header_line);
    std::map<std::string, int> col;
    for (int i = 0; i < (int)headers.size(); ++i)
        col[stripQuotes(headers[i])] = i;

    // Flexible column lookup with aliases
    auto colIdx = [&](std::initializer_list<const char*> names) -> int {
        for (const char* n : names) {
            auto it = col.find(n);
            if (it != col.end()) return it->second;
        }
        return -1;
    };

    int idx_ts    = colIdx({"timestamp"});
    int idx_ft    = colIdx({"fault_type", "category_name", "category"});
    int idx_file  = colIdx({"file"});
    int idx_line  = colIdx({"line"});
    int idx_cpu   = colIdx({"cpu_usage", "cpu"});
    int idx_mem   = colIdx({"mem_usage", "mem"});
    int idx_cause = colIdx({"probable_cause", "cause", "description"});
    int idx_sev   = colIdx({"severity"});
    int idx_hint  = colIdx({"solution_hint", "hint"});
    int idx_conf  = colIdx({"confidence"});
    int idx_bt    = colIdx({"backtrace"});
    int idx_sol   = colIdx({"solution", "proposed_solution"});
    int idx_ml    = colIdx({"ml_prediction"});

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = splitCsvLine(line);
        if (fields.empty()) continue;

        auto get = [&](int idx) -> std::string {
            if (idx < 0 || idx >= (int)fields.size()) return "";
            return stripQuotes(fields[idx]);
        };
        auto getD = [&](int idx) -> double {
            std::string v = get(idx);
            try { return v.empty() ? 0.0 : std::stod(v); } catch (...) { return 0.0; }
        };
        auto getI = [&](int idx) -> int {
            std::string v = get(idx);
            try { return v.empty() ? 0 : std::stoi(v); } catch (...) { return 0; }
        };

        CrashData d;
        d.timestamp     = get(idx_ts);
        d.fault_type    = get(idx_ft);
        d.file          = get(idx_file);
        d.line          = getI(idx_line);
        d.cpu           = getD(idx_cpu);
        d.mem           = getD(idx_mem);
        d.cause         = get(idx_cause);
        d.severity      = get(idx_sev);
        d.hint          = get(idx_hint);
        d.confidence    = getD(idx_conf);
        d.backtrace     = get(idx_bt);
        d.solution      = get(idx_sol);
        d.ml_prediction = get(idx_ml);

        if (d.fault_type.empty() && d.timestamp.empty()) continue;
        data.push_back(d);
    }
    return data;
}

std::vector<CrashData> CSVExporter::ImportFromJSON(const std::string& filename) {
    std::vector<CrashData> data;
    std::ifstream in(filename);
    if (!in.is_open()) return data;
    // Simplified: return empty — JSON import not required for pipeline
    return data;
}

} // namespace Analyzer
