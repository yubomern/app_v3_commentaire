/*
 * Options.hpp - Définitions de configuration et constantes globales.
 * Ce fichier regroupe les options de compilation, les tailles des buffers
 * et les constantes partagées par le gestionnaire de coredump.
 */

#pragma once

// ─── Platform Detection ─────────────────────────────────────────


// ─── Feature Flags ────────────────────────────────────────────────────────────
#define USE_SYSTEM_METRICS
#define ENABLE_JSON_EXPORT
#define ENABLE_CSV_EXPORT

// ─── Constants ────────────────────────────────────────────────────────────────
constexpr int MAX_STACK_SIZE    = 32;
constexpr int FILE_NAME_LEN     = 256;
constexpr int FUNC_NAME_LEN     = 128;
constexpr int PROCESS_NAME_LEN  = 64;
constexpr int CAUSE_LEN         = 256;
constexpr int SEVERITY_LEN      = 32;
constexpr int RECOMMENDATION_LEN = 256;

// ─── Pointer-width integer ────────────────────────────────────────────────────
#include <cstdint>
#include <cstddef>

#if SIZE_MAX == UINT32_MAX
    using INTEGER_TYPE = int32_t;
#elif SIZE_MAX == UINT64_MAX
    using INTEGER_TYPE = int64_t;
#else
    #error "Unsupported pointer width"
#endif
