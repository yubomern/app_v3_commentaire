# CoreDump PFE — Automated Crash Analysis Pipeline v3.0

## Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    PIPELINE OVERVIEW                         │
│                                                              │
│  scenario.cfg ──► crash_simulator      │
│                        │                                     │
│                        └──► .core files (kernel coredumps)  │
│                                  │                           │
│                  crash_analyzer ◄─┘                          │
│                  (CoreFileParser + ML + DB)                   │
│                        │                                     │
│              ┌─────────┼─────────┐                           │
│              ▼         ▼         ▼                           │
│         analysis.csv  .json  crash.db                        │
│                        │                                     │
│                  dashboard.py (Streamlit + Claude AI)        │
└──────────────────────────────────────────────────────────────┘
```

## Quick Start

### 1. Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
cd ..
```

### 2. Configure coredumps (requires root)
```bash
sudo ./scripts/setup_coredump.sh simulator_output/coredumps
```

### 3. Run entire pipeline (single command)
```bash
./scripts/run_pipeline.sh
```

### 4. Launch dashboard
```bash
export ANTHROPIC_API_KEY=sk-ant-...   # for AI analysis tab
streamlit run dashboard/dashboard.py
```

---

## Module Descriptions

### 1. `crash_simulator` — Independent Crash Simulator
- Reads `scenario.cfg` (no manual flags for crash type)
- All crash types are selected **randomly** at runtime
- Writes `simulator_output/crash_report.csv` before each crash
- Triggers actual crashes → OS kernel generates `.core` files

**Config file** (`scenario.cfg`):
```ini
crash_count=10
intensity=7
output_dir=simulator_output
output_csv=crash_report.csv
delay_ms=300
```

### 2. `coredump_handler` — OS Kernel Integration
- Configures `ulimit -c unlimited`
- Sets `/proc/sys/kernel/core_pattern` → named files: `core.<exe>.<pid>.<ts>`
- Can be registered as kernel pipe handler for real-time core capture

**Setup**:
```bash
sudo sh -c 'echo "simulator_output/coredumps/core.%e.%p.%t" > /proc/sys/kernel/core_pattern'
ulimit -c unlimited
```

### 3. `crash_analyzer` — Intelligent Analyzer
- Reads CSV from simulator **and** real `.core` files via GDB
- Runs ML pattern matching + solution proposals
- Exports: enriched CSV, JSON, SQLite DB

```bash
./crash_analyzer \
  --input  simulator_output/crash_report.csv \
  --cores  simulator_output/coredumps \
  --output analyzer_output \
  --db     crash_analysis.db
```

### 4. `test_suite` — Automated Test Pipeline
- Replaces all manual flag-based tests
- Runs steps 0–5 automatically
- Returns exit code 1 on failure

```bash
./build/bin/test_suite
```

### 5. `dashboard/dashboard.py` — Visualization + AI
- 5 tabs: Overview, Timeline, Details, AI Analysis, Core Files
- AI tab calls Claude API for crash correlation + fix recommendations
- Core Files tab shows `.core` metadata and runs GDB on demand

---

## Coredump Naming Pattern

| Field | `%e` | `%p` | `%t` |
|-------|------|------|------|
| Meaning | Executable name | PID | Unix timestamp |
| Example | `crash_simulator` | `1234` | `1713600000` |

Full filename: `core.crash_simulator.1234.1713600000`

---

## File Tree

```
project/
├── CMakeLists.txt              ← builds all 4 targets
├── scenario.cfg                ← simulator config (no flags)
├── simulator/
│   ├── include/                ← CrashTypes, Randomizer, Simulator
│   └── src/
│       ├── simulator_main.cpp  ← ★ NEW: reads scenario.cfg
│       ├── Simulator.cpp
│       └── Randomizer.cpp
├── analyzer/
│   ├── include/
│   │   └── CoreFileParser.hpp  ← ★ NEW: parses real .core
│   └── src/
│       ├── main.cpp            ← ★ NEW: --cores + --input + --output
│       ├── CoreFileParser.cpp  ← ★ NEW: GDB integration
│       ├── CoreAnalyzer.cpp
│       ├── CSVExporter.cpp
│       ├── DatabaseManager.cpp
│       ├── MLAnalyzer.cpp
│       ├── PatternMatcher.cpp
│       └── SolutionProposer.cpp
├── coredump_handler/
│   ├── inc/                    ← CoreDump, BacktraceCollector, etc.
│   └── src/
│       ├── main.cpp            ← ★ NEW: --setup, --status, kernel pipe
│       └── ...
├── test/
│   └── test_suite.cpp          ← ★ NEW: full automated pipeline test
├── dashboard/
│   └── dashboard.py            ← ★ NEW: 5-tab Streamlit + Claude AI
└── scripts/
    ├── run_pipeline.sh         ← ★ NEW: single command for everything
    └── setup_coredump.sh       ← ★ NEW: kernel coredump setup
```
