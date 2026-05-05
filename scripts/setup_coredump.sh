#!/bin/bash
# scripts/setup_coredump.sh
# Objectif 2 : Configurer le noyau Linux pour la génération des coredumps.
# À exécuter une fois en root (ou avec sudo) avant de lancer le pipeline.

set -euo pipefail

DUMP_DIR="${1:-/var/crash}"
PATTERN="$DUMP_DIR/core.%e.%p.%t"

echo "═══════════════════════════════════════════════════"
echo "  Linux Kernel Coredump Setup"
echo "═══════════════════════════════════════════════════"

# 1. Create coredump directory
mkdir -p "$DUMP_DIR"
chmod 1777 "$DUMP_DIR"
echo "✅ Dump directory: $DUMP_DIR"

# 2. ulimit (current shell + /etc/security/limits.conf for persistence)
ulimit -c unlimited
echo "✅ ulimit -c unlimited (current session)"

# Persist across reboots via /etc/security/limits.conf
if ! grep -q "core unlimited" /etc/security/limits.conf 2>/dev/null; then
    echo "* soft core unlimited" >> /etc/security/limits.conf
    echo "* hard core unlimited" >> /etc/security/limits.conf
    echo "✅ Persisted in /etc/security/limits.conf"
fi

# 3. core_pattern: named files with exe, pid, timestamp
echo "$PATTERN" > /proc/sys/kernel/core_pattern
echo "✅ core_pattern → $PATTERN"

# 4. core_uses_pid (belt-and-suspenders for older patterns)
echo 1 > /proc/sys/kernel/core_uses_pid
echo "✅ core_uses_pid = 1"

# 5. Make persistent via sysctl.conf
SYSCTL_ENTRY="kernel.core_pattern=$PATTERN"
if ! grep -qF "kernel.core_pattern" /etc/sysctl.conf 2>/dev/null; then
    echo "$SYSCTL_ENTRY" >> /etc/sysctl.conf
    echo "✅ Persisted in /etc/sysctl.conf"
fi

# 6. Show current status
echo
echo "─── Current Status ─────────────────────────────────"
echo "core_pattern  : $(cat /proc/sys/kernel/core_pattern)"
echo "core_uses_pid : $(cat /proc/sys/kernel/core_uses_pid)"
echo "ulimit -c     : $(ulimit -c)"
echo "dump dir      : $DUMP_DIR"
echo "────────────────────────────────────────────────────"
echo
echo "✅ Coredump setup complete."
echo "   Core files will be named: core.<exe>.<pid>.<unix_timestamp>"
