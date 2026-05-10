#!/usr/bin/env python3
"""
dashboard/dashboard.py  v3.0
Dashboard d’investigation des crashs — Objectif 3
- Lit le CSV d’analyse et la base SQLite produits par crash_analyzer
- Affiche les crashs, la sévérité, la chronologie et les motifs
- Onglet IA : appelle une API pour corréler les crashs et proposer des corrections
- Onglet Fichiers Core : affiche les métadonnées des fichiers .core analysés
"""

import streamlit as st
import sqlite3
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from pathlib import Path
import os
import json
import requests
from datetime import datetime

# ─── Page config ──────────────────────────────────────────────────────────────
st.set_page_config(
    page_title="Crash Investigator",
    page_icon="🔍",
    layout="wide",
    initial_sidebar_state="expanded",
)

# ─── Paths (relative to dashboard dir or project root) ────────────────────────
BASE = Path(__file__).parent.parent
DB_PATH  = BASE / "crash_analysis.db"
CSV_PATH = BASE / "analyzer_output" / "analysis_report.csv"
JSON_PATH = BASE / "analyzer_output" / "analysis_report.csv.json"
CORE_DIR  = BASE / "simulator_output" / "coredumps"

# ─── CSS ──────────────────────────────────────────────────────────────────────
st.markdown("""
<style>
.metric-card {
    background: linear-gradient(135deg, #1e3a5f 0%, #2d6a9f 100%);
    padding: 1.2rem; border-radius: 10px; color: white; text-align: center;
}
.metric-value { font-size: 2.2rem; font-weight: 800; }
.metric-label { font-size: 0.85rem; opacity: 0.85; }
.sev-CRITICAL {
    background:#dc3545; color:white; padding:3px 8px;
    border-radius:4px; font-weight:700;
}
.sev-HIGH    { background:#fd7e14; color:white; padding:3px 8px; border-radius:4px; font-weight:700; }
.sev-MEDIUM  { background:#ffc107; color:black; padding:3px 8px; border-radius:4px; font-weight:700; }
.sev-LOW     { background:#28a745; color:white; padding:3px 8px; border-radius:4px; font-weight:700; }
.ai-box {
    background: #0d1117; border: 1px solid #30363d;
    border-radius: 8px; padding: 1rem; color: #e6edf3;
    font-family: monospace; white-space: pre-wrap; font-size: 0.9rem;
}
</style>
""", unsafe_allow_html=True)

# ─── Data loading ─────────────────────────────────────────────────────────────
@st.cache_data(ttl=30)
def load_csv() -> pd.DataFrame:
    if CSV_PATH.exists():
        try:
            df = pd.read_csv(CSV_PATH)
            if "timestamp" in df.columns:
                df["timestamp"] = pd.to_datetime(df["timestamp"], errors="coerce")
            return df
        except Exception:
            pass
    return pd.DataFrame()

@st.cache_data(ttl=30)
def load_db() -> pd.DataFrame:
    if not DB_PATH.exists():
        return pd.DataFrame()
    try:
        conn = sqlite3.connect(str(DB_PATH))
        df = pd.read_sql_query("SELECT * FROM crashes ORDER BY timestamp DESC", conn)
        conn.close()
        if "timestamp" in df.columns:
            df["timestamp"] = pd.to_datetime(df["timestamp"], errors="coerce")
        return df
    except Exception:
        return pd.DataFrame()

@st.cache_data(ttl=60)
def list_core_files():
    if not CORE_DIR.exists():
        return []
    results = []
    for f in CORE_DIR.iterdir():
        if f.is_file() and ("core" in f.name.lower()):
            sidecar = Path(str(f) + ".json")
            meta = {}
            if sidecar.exists():
                try:
                    meta = json.loads(sidecar.read_text())
                except Exception:
                    pass
            results.append({
                "filename": f.name,
                "size_kb": round(f.stat().st_size / 1024, 1),
                "mtime": datetime.fromtimestamp(f.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S"),
                "executable": meta.get("executable", "?"),
                "pid": meta.get("pid", "?"),
                "signal": meta.get("signal", "?"),
            })
    return results

# ─── AI Analysis via Claude API ─────────────────────────────────────────────── [COMMENTED OUT]
# def ask_claude(prompt: str) -> str:
#     """Call Claude claude-sonnet-4-5 to analyze crash data."""
#     api_key = os.environ.get("ANTHROPIC_API_KEY", "")
#     if not api_key:
#         return "⚠️  ANTHROPIC_API_KEY not set. Export it before launching the dashboard."
#     if not api_key.startswith("sk-ant-"):
#         return "⚠️  ANTHROPIC_API_KEY looks invalid (should start with 'sk-ant-'). Please check your key."

#     headers = {
#         "x-api-key": api_key,
#         "anthropic-version": "2023-06-01",
#         "content-type": "application/json",
#     }
#     body = {
#         "model": "claude-haiku-4-5-20251001",
#         "max_tokens": 1500,
#         "messages": [{"role": "user", "content": prompt}],
#         "system": (
#             "You are an expert embedded-systems crash analyst. "
#             "Analyze crash data concisely. Use bullet points. "
#             "Provide: 1) Root cause, 2) Correlation patterns, 3) Actionable fixes."
#         ),
#     }
#     try:
#         r = requests.post("https://api.anthropic.com/v1/messages",
#                           headers=headers, json=body, timeout=60)
#         if r.status_code == 401:
#             return ("⚠️  401 Unauthorized — your ANTHROPIC_API_KEY is invalid or expired.\n"
#                     "Get a valid key at https://console.anthropic.com/ and re-export it:\n"
#                     "  export ANTHROPIC_API_KEY=sk-ant-...")
#         if r.status_code == 400:
#             try:
#                 detail = r.json().get("error", {}).get("message", r.text)
#             except Exception:
#                 detail = r.text
#             return f"⚠️  400 Bad Request — {detail}"
#         if r.status_code == 429:
#             return "⚠️  Rate limited (429). Wait a moment and try again."
#         r.raise_for_status()
#         data = r.json()
#         return data["content"][0]["text"]
#     except requests.exceptions.Timeout:
#         return "⚠️  API timeout — check your network or increase the timeout."
#     except requests.exceptions.ConnectionError:
#         return "⚠️  Cannot reach api.anthropic.com — check your network connection."
#     except Exception as e:
#         return f"⚠️  API error: {e}"

# def build_ai_prompt(df: pd.DataFrame) -> str:
#     if df.empty:
#         return "No crash data available."
#     top = df.head(20)
#     lines = []
#     for _, row in top.iterrows():
#         fault = row.get("fault_type", row.get("category", "?"))
#         sev   = row.get("severity", "?")
#         cause = row.get("probable_cause", row.get("cause", "?"))
#         hint  = row.get("solution_hint", row.get("hint", "?"))
#         lines.append(f"- {fault} | {sev} | {cause} | hint: {hint}")
#
#     counts = df.get("fault_type", df.get("category", pd.Series())).value_counts().head(5)
#     top_faults = ", ".join(f"{k}({v})" for k,v in counts.items())
#
#     return (
#         f"I have {len(df)} crash records from an embedded Linux system.\n\n"
#         f"Top fault types: {top_faults}\n\n"
#         f"Sample records (fault | severity | cause | hint):\n"
#         + "\n".join(lines) +
#         "\n\nPlease:\n"
#         "1. Identify the most probable root causes.\n"
#         "2. Detect any correlation patterns between crash types.\n"
#         "3. Provide 3–5 concrete, prioritized fixes for the development team."
#     )

# ─── Sidebar ──────────────────────────────────────────────────────────────────
with st.sidebar:
    st.title("🔍 Crash Investigator")
    st.caption("v3.0 — Automated Pipeline")

    st.divider()
    data_source = st.radio("Data source", ["CSV (Analyzer output)", "SQLite DB"])
    st.divider()

    if st.button("🔄 Refresh data"):
        st.cache_data.clear()
        st.rerun()

    st.divider()
    st.caption(f"DB: `{DB_PATH}`")
    st.caption(f"CSV: `{CSV_PATH}`")

# ─── Load data ────────────────────────────────────────────────────────────────
df = load_csv() if "CSV" in data_source else load_db()
if df.empty and data_source == "SQLite DB":
    df = load_csv()   # fallback

# ─── Tabs ─────────────────────────────────────────────────────────────────────
tab_overview, tab_timeline, tab_details, tab_cores = st.tabs([
    "📊 Overview", "📈 Timeline", "🗂 Details", "💾 Core Files"
])

# ══════════════════════════════════════════════════════════════
# TAB 1 — Overview
# ══════════════════════════════════════════════════════════════
with tab_overview:
    st.header("Crash Overview")

    if df.empty:
        st.warning("No data loaded. Run the pipeline first:\n```\n./scripts/run_pipeline.sh\n```")
    else:
        # KPI cards
        total = len(df)
        sev_col = "severity" if "severity" in df.columns else None
        critical = (df[sev_col] == "CRITICAL").sum() if sev_col else 0
        high     = (df[sev_col] == "HIGH").sum()     if sev_col else 0
        fault_col = "fault_type" if "fault_type" in df.columns else "category"
        unique_faults = df[fault_col].nunique() if fault_col in df.columns else 0

        c1, c2, c3, c4 = st.columns(4)
        for col, val, label in [
            (c1, total,         "Total Crashes"),
            (c2, critical,      "Critical"),
            (c3, high,          "High"),
            (c4, unique_faults, "Fault Types"),
        ]:
            col.markdown(
                f'<div class="metric-card">'
                f'<div class="metric-value">{val}</div>'
                f'<div class="metric-label">{label}</div></div>',
                unsafe_allow_html=True,
            )

        st.divider()

        col_a, col_b = st.columns(2)

        # Fault type distribution
        with col_a:
            if fault_col in df.columns:
                cnt = df[fault_col].value_counts().reset_index()
                cnt.columns = ["Fault Type", "Count"]
                fig = px.bar(cnt, x="Count", y="Fault Type", orientation="h",
                             color="Count", color_continuous_scale="reds",
                             title="Crashes by Fault Type")
                fig.update_layout(height=380, showlegend=False)
                st.plotly_chart(fig, use_container_width=True)

        # Severity pie
        with col_b:
            if sev_col:
                sev_cnt = df[sev_col].value_counts().reset_index()
                sev_cnt.columns = ["Severity", "Count"]
                colors = {"CRITICAL":"#dc3545","HIGH":"#fd7e14",
                          "MEDIUM":"#ffc107","LOW":"#28a745"}
                fig2 = px.pie(sev_cnt, names="Severity", values="Count",
                              color="Severity",
                              color_discrete_map=colors,
                              title="Severity Distribution")
                fig2.update_layout(height=380)
                st.plotly_chart(fig2, use_container_width=True)

        # Confidence histogram
        conf_col = "confidence" if "confidence" in df.columns else None
        if conf_col:
            st.subheader("ML Confidence Distribution")
            fig3 = px.histogram(df, x=conf_col, nbins=20,
                                color_discrete_sequence=["#2d6a9f"],
                                title="Analysis Confidence Score")
            fig3.update_layout(height=280)
            st.plotly_chart(fig3, use_container_width=True)

# ══════════════════════════════════════════════════════════════
# TAB 2 — Timeline
# ══════════════════════════════════════════════════════════════
with tab_timeline:
    st.header("Crash Timeline")

    if df.empty or "timestamp" not in df.columns:
        st.info("No timestamp data available.")
    else:
        df_t = df.dropna(subset=["timestamp"]).sort_values("timestamp")
        fault_col2 = "fault_type" if "fault_type" in df_t.columns else "category"

        if fault_col2 in df_t.columns:
            fig = px.scatter(df_t, x="timestamp", y=fault_col2,
                             color="severity" if "severity" in df_t.columns else fault_col2,
                             color_discrete_map={"CRITICAL":"#dc3545","HIGH":"#fd7e14",
                                                 "MEDIUM":"#ffc107","LOW":"#28a745"},
                             title="Crash Events over Time",
                             hover_data=[fault_col2, "severity", "probable_cause"]
                                         if "probable_cause" in df_t.columns else [fault_col2])
            fig.update_layout(height=450)
            st.plotly_chart(fig, use_container_width=True)

        # Crashes per hour
        df_t["hour"] = df_t["timestamp"].dt.floor("h")
        hourly = df_t.groupby("hour").size().reset_index(name="count")
        fig2 = px.area(hourly, x="hour", y="count",
                       title="Crash Rate per Hour",
                       color_discrete_sequence=["#2d6a9f"])
        fig2.update_layout(height=280)
        st.plotly_chart(fig2, use_container_width=True)

# ══════════════════════════════════════════════════════════════
# TAB 3 — Details
# ══════════════════════════════════════════════════════════════
with tab_details:
    st.header("Crash Records")

    if df.empty:
        st.info("No data loaded.")
    else:
        # Filters
        fcol = st.columns(3)
        fault_col3 = "fault_type" if "fault_type" in df.columns else "category"
        sev_col3   = "severity"   if "severity"   in df.columns else None

        with fcol[0]:
            if fault_col3 in df.columns:
                faults = ["All"] + sorted(df[fault_col3].dropna().unique().tolist())
                sel_fault = st.selectbox("Fault type", faults)
            else:
                sel_fault = "All"

        with fcol[1]:
            if sev_col3:
                sevs = ["All"] + sorted(df[sev_col3].dropna().unique().tolist())
                sel_sev = st.selectbox("Severity", sevs)
            else:
                sel_sev = "All"

        with fcol[2]:
            search = st.text_input("Search description / cause")

        filtered = df.copy()
        if sel_fault != "All" and fault_col3 in df.columns:
            filtered = filtered[filtered[fault_col3] == sel_fault]
        if sel_sev != "All" and sev_col3:
            filtered = filtered[filtered[sev_col3] == sel_sev]
        if search:
            mask = filtered.apply(lambda r: search.lower() in str(r).lower(), axis=1)
            filtered = filtered[mask]

        st.caption(f"Showing {len(filtered)} / {len(df)} records")
        st.dataframe(filtered, use_container_width=True, height=500)

        # Download
        csv_dl = filtered.to_csv(index=False)
        st.download_button("⬇️ Download filtered CSV", csv_dl,
                           "filtered_crashes.csv", "text/csv")

# ══════════════════════════════════════════════════════════════
# TAB 4 — AI Analysis [COMMENTED OUT]
# ══════════════════════════════════════════════════════════════
# with tab_ai:
#     st.header("🤖 AI-Powered Crash Analysis")
#     st.caption("Uses Claude claude-sonnet-4-5 to correlate crashes and suggest fixes.")
#
#     if df.empty:
#         st.warning("No crash data loaded — run the pipeline first.")
#     else:
#         col1, col2 = st.columns([3, 1])
#         with col1:
#             custom_prompt = st.text_area(
#                 "Custom question (or leave blank for automatic analysis)",
#                 height=80,
#                 placeholder="e.g. Why are there recurring SIGSEGV crashes? What memory issues should I fix first?"
#             )
#         with col2:
#             n_records = st.slider("Records to analyse", 5, 100,
#                                   min(20, len(df)), step=5)
#
#         if st.button("🔍 Analyse with AI", type="primary"):
#             df_sample = df.head(n_records)
#             prompt = custom_prompt.strip() if custom_prompt.strip() else build_ai_prompt(df_sample)
#
#             with st.spinner("Calling Claude AI..."):
#                 result = ask_claude(prompt)
#
#             st.subheader("AI Analysis Result")
#             st.markdown(f'<div class="ai-box">{result}</div>', unsafe_allow_html=True)
#
#             # Show prompt used
#             with st.expander("Prompt sent to AI"):
#                 st.code(prompt, language="text")
#
#     st.divider()
#     st.subheader("Configuration")
#     api_status = "✅ Set" if os.environ.get("ANTHROPIC_API_KEY") else "❌ Not set"
#     st.info(f"ANTHROPIC_API_KEY: {api_status}\n\n"
#             "Export before launching:\n```bash\nexport ANTHROPIC_API_KEY=sk-ant-...\nstreamlit run dashboard/dashboard.py\n```")

# ══════════════════════════════════════════════════════════════
# TAB 5 — Core Files
# ══════════════════════════════════════════════════════════════
with tab_cores:
    st.header("💾 OS Kernel Core Files")
    st.caption(f"Scanned from: `{CORE_DIR}`")

    cores = list_core_files()

    if not cores:
        st.info(
            "No .core files found.\n\n"
            "To generate core files:\n"
            "```bash\n"
            "ulimit -c unlimited\n"
            "sudo sh -c 'echo \"simulator_output/coredumps/core.%e.%p.%t\" > /proc/sys/kernel/core_pattern'\n"
            "./scripts/run_pipeline.sh\n"
            "```"
        )
    else:
        core_df = pd.DataFrame(cores)
        st.dataframe(core_df, use_container_width=True)

        st.subheader("Core File Sizes")
        fig = px.bar(core_df, x="filename", y="size_kb",
                     color="signal", title="Core File Sizes (KB)")
        fig.update_layout(height=300)
        st.plotly_chart(fig, use_container_width=True)

        st.subheader("Signal Distribution")
        sig_cnt = core_df["signal"].value_counts().reset_index()
        sig_cnt.columns = ["Signal", "Count"]
        fig2 = px.pie(sig_cnt, names="Signal", values="Count",
                      title="Signals that generated core files")
        fig2.update_layout(height=300)
        st.plotly_chart(fig2, use_container_width=True)

        # GDB analysis button
        st.divider()
        st.subheader("🔬 Analyse a Core File with GDB")
        sel_core = st.selectbox("Select core file", [c["filename"] for c in cores])
        binary_input = st.text_input("Path to crashing binary (optional)",
                                     placeholder="/path/to/crash_simulator")
        if st.button("Run GDB Analysis"):
            core_full = str(CORE_DIR / sel_core)
            cmd = f"gdb --batch -ex 'bt' -ex 'info signal' -ex 'info registers'"
            if binary_input and Path(binary_input).exists():
                cmd += f" '{binary_input}'"
            cmd += f" '{core_full}' 2>&1"
            import subprocess
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=30)
            output = result.stdout + result.stderr
            st.code(output or "(no output)", language="text")

# ─── Footer ───────────────────────────────────────────────────────────────────
st.divider()
st.caption("Crash Investigator v3.0 — Pipeline: crash_simulator → crash_analyzer → dashboard")