/*
 * CoreDump.cpp - Gestion des données de coredump.
 * Implémente la lecture, la validation et l'enregistrement des coredumps
 * ainsi que la création des fichiers de métadonnées associés.
 */

#include "CoreDump.hpp"
#include "Platform.hpp"
#include <cstring>
#include <cstdio>
#include <ctime>
#include <array>

#ifdef _WIN32
#include <windows.h>
#endif

// ═════════════════════════════════════════════════════════════════════════════
//  CoreDumpManager  –  Singleton implementation
// ═════════════════════════════════════════════════════════════════════════════

CoreDumpManager::CoreDumpManager() {
    std::memset(&data_, 0, sizeof(data_));
}

CoreDumpManager& CoreDumpManager::Instance() {
    // C++11 guarantees thread-safe initialisation of local statics.
    static CoreDumpManager instance;
    return instance;
}

// ─── Store ───────────────────────────────────────────────────────────────────
void CoreDumpManager::Store(INTEGER_TYPE* /*stack_ptr*/,
                             const char*  file_name,
                             uint32_t     line_number,
                             uint32_t     aux_code,
                             FaultType    type) {
    if (IsSaved()) return;

    data_.software_version = 1;
    data_.aux_code         = aux_code;
    data_.type             = type;

    // ── Time ────────────────────────────────────────────────────────────────
    data_.timestamp = std::time(nullptr);
    FormatTimestamp(data_.timestamp, data_.date_time, sizeof(data_.date_time));

    // ── Location ────────────────────────────────────────────────────────────
    data_.line_number = line_number;
    if (file_name) {
        std::strncpy(data_.file_name, file_name, FILE_NAME_LEN - 1);
        data_.file_name[FILE_NAME_LEN - 1] = '\0';
    }

    // ── Process / thread ─────────────────────────────────────────────────────
    data_.process_id = PlatformGetPid();
    data_.thread_id  = PlatformGetTid();

    // ── OS metrics + call stack + analysis ───────────────────────────────────
    CaptureSystemMetrics(&data_.metrics);
    CaptureCallStack();
    Analyze();

    // ── Mark valid LAST (ensures readers see complete data) ─────────────────
    data_.key     = KEY_CORE_DUMP_STORED;
    data_.not_key = ~KEY_CORE_DUMP_STORED;
    data_.is_valid = true;
}

// ─── IsSaved ─────────────────────────────────────────────────────────────────
bool CoreDumpManager::IsSaved() const {
    if (!data_.is_valid)                                    return false;
    if (data_.key    != KEY_CORE_DUMP_STORED)               return false;
    if (data_.not_key != ~KEY_CORE_DUMP_STORED)             return false;
    if (data_.timestamp == 0)                               return false;
    if (data_.stack_depth < 0 ||
        data_.stack_depth > MAX_STACK_SIZE)                 return false;

    // Reject stale dumps older than 7 days
    const std::time_t age = std::time(nullptr) - data_.timestamp;
    if (age > 7 * 24 * 3600)                               return false;

    return true;
}

// ─── Get ──────────────────────────────────────────────────────────────────────
const CoreDumpData* CoreDumpManager::Get() const {
    return IsSaved() ? &data_ : nullptr;
}

// ─── Reset ────────────────────────────────────────────────────────────────────
void CoreDumpManager::Reset() {
    data_.is_valid = false;                   // invalidate first
    std::memset(&data_, 0, sizeof(data_));
}

// ─── ExportTo ────────────────────────────────────────────────────────────────
bool CoreDumpManager::ExportTo(ExportFormat format, const char* file_path) {
    if (!IsSaved() || !file_path) return false;
    auto exporter = CrashExporterFactory::Create(format);
    return exporter ? exporter->Export(data_, file_path) : false;
}

// ─── Private helpers ─────────────────────────────────────────────────────────
void CoreDumpManager::FormatTimestamp(std::time_t ts, char* buf, std::size_t size) {
    struct tm t{};
#ifdef _WIN32
    localtime_s(&t, &ts);
#else
    localtime_r(&ts, &t);   // thread-safe POSIX version
#endif
    std::strftime(buf, size, "%Y-%m-%d %H:%M:%S", &t);
}

void CoreDumpManager::CaptureCallStack() {
    std::array<void*, MAX_STACK_SIZE> raw{};
    int frames = PlatformBacktrace(raw.data(), MAX_STACK_SIZE);
    
    data_.stack_depth = (frames < MAX_STACK_SIZE) ? frames : MAX_STACK_SIZE;
    for (int i = 0; i < data_.stack_depth; ++i)
        data_.call_stack[i] = reinterpret_cast<INTEGER_TYPE>(raw[static_cast<std::size_t>(i)]);
}

// ─── Analyze ─────────────────────────────────────────────────────────────────
void CoreDumpManager::Analyze() {
    CrashAnalysis* a = &data_.analysis;
    std::memset(a, 0, sizeof(*a));

    switch (data_.type) {
        case FaultType::SegmentationFault:
            std::strncpy(a->probable_cause,  "Segmentation Fault - invalid memory access",  CAUSE_LEN - 1);
            std::strncpy(a->severity,        "CRITICAL",                                     SEVERITY_LEN - 1);
            std::strncpy(a->recommendation,  "Validate pointers and array bounds",           RECOMMENDATION_LEN - 1);
            a->confidence_score = 85;
            break;

        case FaultType::HardwareException:
            std::strncpy(a->probable_cause,  "Hardware Exception - divide-by-zero or illegal instruction", CAUSE_LEN - 1);
            std::strncpy(a->severity,        "HIGH",                                                        SEVERITY_LEN - 1);
            std::strncpy(a->recommendation,  "Validate inputs and guard against overflow",                  RECOMMENDATION_LEN - 1);
            a->confidence_score = 80;
            break;

        case FaultType::SoftwareAssertion:
            std::strncpy(a->probable_cause,  "Software Assertion - logic error detected",   CAUSE_LEN - 1);
            std::strncpy(a->severity,        "MEDIUM",                                      SEVERITY_LEN - 1);
            std::strncpy(a->recommendation,  "Review condition checks and input validation", RECOMMENDATION_LEN - 1);
            a->confidence_score = 90;
            break;

        case FaultType::AbortSignal:
            std::strncpy(a->probable_cause,  "Abort Signal - abnormal termination",         CAUSE_LEN - 1);
            std::strncpy(a->severity,        "HIGH",                                        SEVERITY_LEN - 1);
            std::strncpy(a->recommendation,  "Check for unhandled exceptions or resource leaks", RECOMMENDATION_LEN - 1);
            a->confidence_score = 75;
            break;

        default:
            std::strncpy(a->probable_cause,  "Unknown cause",                               CAUSE_LEN - 1);
            std::strncpy(a->severity,        "LOW",                                         SEVERITY_LEN - 1);
            std::strncpy(a->recommendation,  "Manual investigation required",               RECOMMENDATION_LEN - 1);
            a->confidence_score = 50;
            break;
    }

    // ── Heuristic adjustments ────────────────────────────────────────────────
    const SystemMetrics& m = data_.metrics;

    if (m.memory_total_kb > 0) {
        double mem_pct = 100.0 * static_cast<double>(m.memory_used_kb)
                                / static_cast<double>(m.memory_total_kb);
        if (mem_pct > 90.0) {
            strncat(a->probable_cause, " + Memory exhaustion suspected",
                    CAUSE_LEN - strlen(a->probable_cause) - 1);
            a->confidence_score = (a->confidence_score + 10 > 100) ? 100 : a->confidence_score + 10;
        }
    }

    if (m.cpu_usage_percent > 90.0) {
        strncat(a->probable_cause, " + High CPU load at crash time",
                CAUSE_LEN - strlen(a->probable_cause) - 1);
        a->confidence_score = (a->confidence_score + 5 > 100) ? 100 : a->confidence_score + 5;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  CrashExporterFactory
// ═════════════════════════════════════════════════════════════════════════════
std::unique_ptr<ICrashExporter> CrashExporterFactory::Create(ExportFormat format) {
    switch (format) {
        case ExportFormat::JSON: return std::make_unique<JsonCrashExporter>();
        case ExportFormat::CSV:  return std::make_unique<CsvCrashExporter>();
        default:                 return nullptr;
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  JsonCrashExporter
// ═════════════════════════════════════════════════════════════════════════════
bool JsonCrashExporter::Export(const CoreDumpData& d, const char* filePath) {
    FILE* f = fopen(filePath, "a");  // Append mode
    if (!f) return false;

    // Check if file is empty to decide on comma
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    bool is_first = (file_size == 0);
    
    if (!is_first) {
        // Remove trailing ']' from previous entry
        fseek(f, -1, SEEK_END);
        fprintf(f, ",\n");
    } else {
        // Start of new JSON array
        fprintf(f, "[\n");
    }

    fprintf(f, "  {\n");
    fprintf(f, "  \"timestamp\"      : \"%s\",\n",  d.date_time);
    fprintf(f, "  \"unix_timestamp\" : %ld,\n",     static_cast<long>(d.timestamp));
    fprintf(f, "  \"fault_type\"     : %u,\n",      static_cast<unsigned>(d.type));
    fprintf(f, "  \"file\"           : \"%s\",\n",  d.file_name);
    fprintf(f, "  \"line\"           : %u,\n",      d.line_number);
    fprintf(f, "  \"function\"       : \"%s\",\n",  d.function_name);
    fprintf(f, "  \"process_id\"     : %d,\n",      d.process_id);
    fprintf(f, "  \"thread_id\"      : %d,\n",      d.thread_id);
    fprintf(f, "  \"stack_depth\"    : %d,\n",      d.stack_depth);

    fprintf(f, "  \"call_stack\": [\n");
    for (int i = 0; i < d.stack_depth; ++i) {
        const char* sep = (i < d.stack_depth - 1) ? "," : "";
        fprintf(f, "    \"0x%lx\"%s\n", static_cast<unsigned long>(d.call_stack[i]), sep);
    }
    fprintf(f, "  ],\n");

    fprintf(f, "  \"system_metrics\": {\n");
    fprintf(f, "    \"cpu_usage_percent\" : %.2f,\n", d.metrics.cpu_usage_percent);
    fprintf(f, "    \"memory_used_kb\"    : %llu,\n",
            static_cast<unsigned long long>(d.metrics.memory_used_kb));
    fprintf(f, "    \"memory_total_kb\"   : %llu,\n",
            static_cast<unsigned long long>(d.metrics.memory_total_kb));
    fprintf(f, "    \"thread_count\"      : %d,\n",   d.metrics.thread_count);
    fprintf(f, "    \"process_name\"      : \"%s\"\n", d.metrics.process_name);
    fprintf(f, "  },\n");

    fprintf(f, "  \"analysis\": {\n");
    fprintf(f, "    \"probable_cause\"   : \"%s\",\n", d.analysis.probable_cause);
    fprintf(f, "    \"severity\"         : \"%s\",\n", d.analysis.severity);
    fprintf(f, "    \"recommendation\"   : \"%s\",\n", d.analysis.recommendation);
    fprintf(f, "    \"confidence_score\" : %d\n",      d.analysis.confidence_score);
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fclose(f);
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
//  CsvCrashExporter
// ═════════════════════════════════════════════════════════════════════════════
bool CsvCrashExporter::Export(const CoreDumpData& d, const char* filePath) {
    // Check if file exists to determine if we need header
    FILE* check = fopen(filePath, "r");
    bool file_exists = (check != NULL);
    if (check) fclose(check);
    
    FILE* f = fopen(filePath, "a");  // Append mode
    if (!f) return false;

    // Header row (only if file is new)
    if (!file_exists) {
        fprintf(f, "timestamp,unix_timestamp,fault_type,file,line,function,"
                   "process_id,thread_id,stack_depth,"
                   "cpu_usage_percent,memory_used_kb,memory_total_kb,thread_count,"
                   "process_name,probable_cause,severity,recommendation,confidence_score\n");
    }

    // Data row
    fprintf(f, "\"%s\",%ld,%u,\"%s\",%u,\"%s\",%d,%d,%d,"
               "%.2f,%llu,%llu,%d,"
               "\"%s\",\"%s\",\"%s\",\"%s\",%d\n",
            d.date_time,
            static_cast<long>(d.timestamp),
            static_cast<unsigned>(d.type),
            d.file_name,
            d.line_number,
            d.function_name,
            d.process_id,
            d.thread_id,
            d.stack_depth,
            d.metrics.cpu_usage_percent,
            static_cast<unsigned long long>(d.metrics.memory_used_kb),
            static_cast<unsigned long long>(d.metrics.memory_total_kb),
            d.metrics.thread_count,
            d.metrics.process_name,
            d.analysis.probable_cause,
            d.analysis.severity,
            d.analysis.recommendation,
            d.analysis.confidence_score);

    fclose(f);
    return true;
}
