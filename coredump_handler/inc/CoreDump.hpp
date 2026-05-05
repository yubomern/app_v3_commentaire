/*
 * CoreDump.hpp - Structures principales pour la gestion des coredumps.
 * Définit les types de défaut, la structure de snapshot et les données
 * associées au crash.
 */

#pragma once

#include "Options.hpp"
#include "SystemMetrics.hpp"
#include <cstdint>
#include <ctime>
#include <memory>

// ─── Magic Key ────────────────────────────────────────────────────────────────
static constexpr uint32_t KEY_CORE_DUMP_STORED = 0xDEADBEEFu;

// ─── Crash Types ──────────────────────────────────────────────────────────────
enum class FaultType : uint32_t {
    HardwareException  = 0,
    SoftwareAssertion  = 1,
    SegmentationFault  = 2,
    AbortSignal        = 3,
    Unknown            = 0xFF
};

// ─── Automatic analysis result ────────────────────────────────────────────────
struct CrashAnalysis {
    char probable_cause[CAUSE_LEN];
    char severity[SEVERITY_LEN];
    char recommendation[RECOMMENDATION_LEN];
    int  confidence_score;   // 0–100
};

// ─── Core crash snapshot ──────────────────────────────────────────────────────
struct CoreDumpData {
    // Validity
    uint32_t   key;                            // Magic = KEY_CORE_DUMP_STORED
    uint32_t   not_key;                        // Must equal ~key
    bool       is_valid;                       // Set last on write, cleared first on reset

    // Metadata
    uint32_t   software_version;
    uint32_t   aux_code;
    FaultType  type;
    std::time_t timestamp;
    char       date_time[32];

    // Location
    uint32_t   line_number;
    char       file_name[FILE_NAME_LEN];
    char       function_name[FUNC_NAME_LEN];

    // Runtime context
    int        process_id;
    int        thread_id;
    INTEGER_TYPE call_stack[MAX_STACK_SIZE];
    int        stack_depth;

    // OS snapshot + analysis
    SystemMetrics  metrics;
    CrashAnalysis  analysis;
};

// ═════════════════════════════════════════════════════════════════════════════
//  ICrashExporter  – Strategy interface (Factory target)
// ═════════════════════════════════════════════════════════════════════════════
class ICrashExporter {
public:
    virtual ~ICrashExporter() = default;
    virtual bool Export(const CoreDumpData& data, const char* filePath) = 0;
};

// ─── Concrete exporters ───────────────────────────────────────────────────────
class JsonCrashExporter : public ICrashExporter {
public:
    bool Export(const CoreDumpData& data, const char* filePath) override;
};

class CsvCrashExporter : public ICrashExporter {
public:
    bool Export(const CoreDumpData& data, const char* filePath) override;
};

// ─── Factory ─────────────────────────────────────────────────────────────────
enum class ExportFormat { JSON, CSV };

class CrashExporterFactory {
public:
    static std::unique_ptr<ICrashExporter> Create(ExportFormat format);
};

// ═════════════════════════════════════════════════════════════════════════════
//  CoreDumpManager  – Singleton
// ═════════════════════════════════════════════════════════════════════════════
class CoreDumpManager {
public:
    // ── Singleton access ────────────────────────────────────────────────────
    static CoreDumpManager& Instance();

    // Non-copyable, non-movable
    CoreDumpManager(const CoreDumpManager&)            = delete;
    CoreDumpManager& operator=(const CoreDumpManager&) = delete;
    CoreDumpManager(CoreDumpManager&&)                 = delete;
    CoreDumpManager& operator=(CoreDumpManager&&)      = delete;

    // ── Core API ────────────────────────────────────────────────────────────

    /// Capture a crash snapshot. Safe to call from a signal handler.
    void Store(INTEGER_TYPE* stack_ptr,
               const char*  file_name,
               uint32_t     line_number,
               uint32_t     aux_code,
               FaultType    type);

    /// Returns true when a valid crash snapshot is present.
    bool IsSaved() const;

    /// Returns a const pointer to the snapshot (nullptr if not saved).
    const CoreDumpData* Get() const;

    /// Erase the snapshot (call after handling the crash on startup).
    void Reset();

    /// Export via the Factory – picks the right exporter automatically.
    bool ExportTo(ExportFormat format, const char* file_path);

    /// Run the heuristic analyser (called automatically by Store).
    void Analyze();

private:
    CoreDumpManager();   // private ctor

    void FormatTimestamp(std::time_t ts, char* buf, std::size_t size);
    void CaptureCallStack();

    CoreDumpData data_;  // single in-process instance
};

// ─── Helper macro ────────────────────────────────────────────────────────────
#define CORE_DUMP_STORE(type)  \
    CoreDumpManager::Instance().Store(nullptr, __FILE__, __LINE__, 0, (type))
