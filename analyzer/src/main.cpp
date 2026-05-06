/*
 * analyzer/src/main.cpp  v3.0
 *
 * Objectif 3 : Analyseur de crash intelligent — lit le CSV du simulateur
 *               ET les vrais fichiers .core produits par le noyau.
 *
 * Utilisation :
 *   ./crash_analyzer                               # valeurs par défaut
 *   ./crash_analyzer --input path/crash.csv        # CSV du simulateur
 *   ./crash_analyzer --cores path/to/coredumps/   # analyse des .core
 *   ./crash_analyzer --binary ./my_app             # binaire pour GDB
 *   ./crash_analyzer --output path/out/            # dossier de sortie
 *   ./crash_analyzer --db crash.db                 # chemin SQLite
 */

#include "CoreAnalyzer.hpp"
#include "CoreFileParser.hpp"
#include "CSVExporter.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// ─── Simple arg parser ────────────────────────────────────────────────────────
static std::string getArg(int argc, char* argv[],
                           const std::string& flag,
                           const std::string& def = "") {
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == flag)
            return argv[i + 1];
    return def;
}

static bool hasFlag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == flag) return true;
    return false;
}

// ─── Merge core-file CSV rows into existing crash CSV ────────────────────────
static void appendCoreRowsToCsv(const std::string& csvPath,
                                 const std::vector<Analyzer::CoreFileInfo>& cores) {
    // Check whether file exists AND has data (header + at least one row)
    bool has_header = false;
    {
        std::ifstream check(csvPath);
        std::string hdr;
        if (check.is_open() && std::getline(check, hdr) && !hdr.empty())
            has_header = true;
    }

    std::ofstream f(csvPath, std::ios::app);
    if (!f.is_open()) return;

    if (!has_header) {
        f << "timestamp,fault_type,file,line,cpu_usage,mem_usage,"
             "probable_cause,severity,solution_hint,confidence,"
             "backtrace,solution,ml_prediction\n";
    }

    int written = 0;
    for (const auto& c : cores) {
        if (!c.parsed_ok) continue;
        f << Analyzer::CoreFileParser::toCsvRow(c) << "\n";
        ++written;
    }
    std::cout << "   [appendCoreRows] " << written << " core row(s) appended.\n";
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Banner ────────────────────────────────────────────────────────────────
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║   Crash Analyzer & AI Solution Engine  v3.0      ║\n";
    std::cout << "║   Pipeline: CSV + .core → DB → CSV → Dashboard   ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    if (hasFlag(argc, argv, "--help") || hasFlag(argc, argv, "-h")) {
        std::cout
            << "Usage: crash_analyzer [OPTIONS]\n\n"
            << "  --input  FILE   CSV from crash_simulator (default: simulator_output/crash_report.csv)\n"
            << "  --cores  DIR    Directory of .core files to parse (default: simulator_output/coredumps)\n"
            << "  --binary FILE   Executable for GDB symbol resolution\n"
            << "  --output DIR    Output directory (default: analyzer_output)\n"
            << "  --db     FILE   SQLite database path (default: crash_analysis.db)\n"
            << "  --no-cores      Skip .core file parsing\n\n";
        return 0;
    }

    // ── Arguments with sensible defaults ─────────────────────────────────────
    std::string input_csv  = getArg(argc, argv, "--input",
                                    "simulator_output/crash_report.csv");
    std::string cores_dir  = getArg(argc, argv, "--cores",
                                    "simulator_output/coredumps");
    std::string binary     = getArg(argc, argv, "--binary", "");
    std::string output_dir = getArg(argc, argv, "--output", "analyzer_output");
    std::string db_path    = getArg(argc, argv, "--db",    "crash_analysis.db");
    bool skip_cores        = hasFlag(argc, argv, "--no-cores");

    // ── Prepare output dirs ───────────────────────────────────────────────────
    fs::create_directories("simulator_output");
    fs::create_directories(output_dir);
    std::string output_csv  = output_dir + "/analysis_report.csv";
    std::string output_json = output_dir + "/analysis_report.json";

    std::cout << "📂 Input CSV   : " << input_csv  << "\n";
    std::cout << "📂 Core dir    : " << (skip_cores ? "(skipped)" : cores_dir) << "\n";
    std::cout << "📂 Output dir  : " << output_dir  << "\n";
    std::cout << "🗄️  Database    : " << db_path    << "\n\n";

    // ── Step 1: Parse .core files and merge into CSV ─────────────────────────
#ifndef _WIN32
    if (!skip_cores) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "🧩 Step 1/3: Parsing OS kernel .core files...\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

        Analyzer::CoreFileParser parser;
        auto cores = parser.parseDirectory(cores_dir, binary);

        if (cores.empty()) {
            std::cout << "   ℹ️  No .core files found in " << cores_dir
                      << " — skipping core analysis.\n";
        } else {
            std::cout << "   ✅ " << cores.size() << " core file(s) parsed.\n";
            // Append core-derived rows to the simulator CSV so the
            // analyzer sees everything in one pass.
            appendCoreRowsToCsv(input_csv, cores);
            std::cout << "   📄 Core records appended to: " << input_csv << "\n";
        }
        std::cout << "\n";
    }
#else
    if (!skip_cores) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "🧩 Step 1/3: Core parsing is not supported on Windows build.\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "   ℹ️  Use --no-cores to skip this step on Windows.\n\n";
    }
#endif

    // ── Step 2: AI analysis pipeline ─────────────────────────────────────────
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "🤖 Step 2/3: Running ML + Pattern analysis...\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    Analyzer::CoreAnalyzer analyzer(db_path);
    analyzer.ProcessInput(input_csv, output_csv);
    analyzer.PrintSummary();

    // ── Step 3: Export JSON ───────────────────────────────────────────────────
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "📊 Step 3/3: Exporting results...\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    std::cout << "   ✅ CSV  → " << output_csv  << "\n";
    std::cout << "   ✅ JSON → " << output_json << "\n";
    std::cout << "   ✅ DB   → " << db_path     << "\n\n";

    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ Analysis complete!                            ║\n";
    std::cout << "║                                                  ║\n";
    std::cout << "║  Launch dashboard:                               ║\n";
    std::cout << "║  streamlit run dashboard/dashboard.py            ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    return 0;
}
