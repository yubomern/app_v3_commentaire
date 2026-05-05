#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════════
# run_pipeline.sh  v3.0 — Pipeline complet automatisé
#
# Objectif 1 : Script unique pour exécuter l'ensemble du projet.
#              Sans flags manuels ni intervention de test manuelle.
#
# Pipeline:
#   0. Setup OS kernel coredumps (ulimit + core_pattern)
#   1. Build all targets (simulator, analyzer, coredump_handler, test_suite)
#   2. Run crash_simulator with scenario.cfg  → CSV + .core files
#   3. Run crash_analyzer                     → DB + enriched CSV + JSON
#   4. Run automated test_suite               → pass/fail report
#   5. Launch Streamlit dashboard (optional)
#
# Usage:
#   ./scripts/run_pipeline.sh [--no-dashboard] [--clean] [--scenario FILE]
# ═══════════════════════════════════════════════════════════════════════════════
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
BIN_DIR="$BUILD_DIR/bin"
OUT_SIM="$ROOT_DIR/simulator_output"
OUT_ANA="$ROOT_DIR/analyzer_output"
SCENARIO="$ROOT_DIR/scenario.cfg"
LAUNCH_DASHBOARD=true

# ── Parse args ────────────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --no-dashboard) LAUNCH_DASHBOARD=false ;;
        --clean)        rm -rf "$BUILD_DIR" ;;
        --scenario)     shift; SCENARIO="$1" ;;
    esac
done

# ── Helpers ───────────────────────────────────────────────────────────────────
section() { echo; echo "══════════════════════════════════════════════════"; echo "  $1"; echo "══════════════════════════════════════════════════"; }
ok()      { echo "   ✅ $*"; }
info()    { echo "   ℹ️  $*"; }
warn()    { echo "   ⚠️  $*"; }

# ── Step 0: OS Kernel Coredump Configuration ──────────────────────────────────
section "0) OS Kernel — Enable Coredumps"

ulimit -c unlimited
ok "ulimit -c unlimited set"

mkdir -p "$OUT_SIM/coredumps"
CORE_PATTERN="$OUT_SIM/coredumps/core.%e.%p.%t"
if sudo sh -c "echo '$CORE_PATTERN' > /proc/sys/kernel/core_pattern" 2>/dev/null; then
    ok "core_pattern → $CORE_PATTERN"
else
    warn "Cannot write core_pattern (not root). Using default Linux pattern."
    info "Run: sudo sh -c 'echo \"$CORE_PATTERN\" > /proc/sys/kernel/core_pattern'"
fi

# Optional: enable core_uses_pid
sudo sh -c 'echo 1 > /proc/sys/kernel/core_uses_pid' 2>/dev/null || true

# Show current status
echo "   core_pattern  : $(cat /proc/sys/kernel/core_pattern 2>/dev/null || echo 'N/A')"
echo "   core_uses_pid : $(cat /proc/sys/kernel/core_uses_pid 2>/dev/null || echo 'N/A')"

# ── Step 1: Build ─────────────────────────────────────────────────────────────
section "1) Build — All Targets"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake "$ROOT_DIR" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$BIN_DIR" -G Ninja 2>/dev/null \
    || cmake "$ROOT_DIR" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$BIN_DIR"

cmake --build . --parallel "$(nproc)"
ok "Build successful"
ls -lh "$BIN_DIR/"

cd "$ROOT_DIR"

# ── Step 2: Run Simulator ─────────────────────────────────────────────────────
section "2) Crash Simulator — Random Crash Generation"

mkdir -p "$OUT_SIM"
info "Using scenario: $SCENARIO"

# The simulator is run in a subshell so if it actually crashes the
# pipeline continues. We capture the exit code.
"$BIN_DIR/crash_simulator" --config "$SCENARIO" || true

if [[ -f "$OUT_SIM/crash_report.csv" ]]; then
    ROWS=$(wc -l < "$OUT_SIM/crash_report.csv")
    ok "crash_report.csv generated — $ROWS lines"
else
    warn "crash_report.csv not found — check simulator output above"
fi

CORE_COUNT=$(find "$OUT_SIM/coredumps" -maxdepth 1 -type f 2>/dev/null | wc -l)
info "Core files in $OUT_SIM/coredumps: $CORE_COUNT"

# ── Step 3: Run Analyzer ──────────────────────────────────────────────────────
section "3) Crash Analyzer — AI Analysis Pipeline"

mkdir -p "$OUT_ANA"

"$BIN_DIR/crash_analyzer" \
    --input  "$OUT_SIM/crash_report.csv" \
    --cores  "$OUT_SIM/coredumps" \
    --output "$OUT_ANA" \
    --db     "$ROOT_DIR/crash_analysis.db"

if [[ -f "$OUT_ANA/analysis_report.csv" ]]; then
    AROWS=$(wc -l < "$OUT_ANA/analysis_report.csv")
    ok "analysis_report.csv — $AROWS lines"
fi
[[ -f "$OUT_ANA/analysis_report.csv.json" ]] && ok "analysis_report.json generated"
[[ -f "$ROOT_DIR/crash_analysis.db"       ]] && ok "crash_analysis.db created"

# ── Step 4: Automated Test Suite ──────────────────────────────────────────────
section "4) Automated Test Suite"

cd "$ROOT_DIR"
"$BIN_DIR/test_suite" && TEST_OK=true || TEST_OK=false

if $TEST_OK; then
    ok "All tests passed"
else
    warn "Some tests failed — see output above"
fi

# ── Step 5: Dashboard ─────────────────────────────────────────────────────────
section "5) Dashboard"

if $LAUNCH_DASHBOARD; then
    if command -v streamlit &>/dev/null; then
        ok "Launching Streamlit dashboard..."
        cd "$ROOT_DIR"
        streamlit run dashboard/dashboard.py \
            --server.headless true \
            --server.port 8501 &
        DASH_PID=$!
        sleep 2
        if kill -0 $DASH_PID 2>/dev/null; then
            ok "Dashboard running at http://localhost:8501  (PID $DASH_PID)"
        fi
    else
        warn "streamlit not installed. Run: pip3 install streamlit pandas plotly"
        info "Then: streamlit run dashboard/dashboard.py"
    fi
else
    info "Dashboard launch skipped (--no-dashboard)"
    info "Run manually: streamlit run dashboard/dashboard.py"
fi

# ── Final Summary ─────────────────────────────────────────────────────────────
section "Pipeline Complete"
echo "  📄 Simulator CSV  : $OUT_SIM/crash_report.csv"
echo "  🗄️  Database       : $ROOT_DIR/crash_analysis.db"
echo "  📊 Analysis CSV   : $OUT_ANA/analysis_report.csv"
echo "  📋 Analysis JSON  : $OUT_ANA/analysis_report.csv.json"
echo "  💻 Dashboard      : http://localhost:8501"
echo
