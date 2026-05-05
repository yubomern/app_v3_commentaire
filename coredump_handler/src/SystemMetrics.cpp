/*
 * SystemMetrics.cpp - Capture des métriques système au moment du crash.
 * Remplit la structure de métriques avec l'utilisation CPU, mémoire,
 * PID, nom du processus et état des threads.
 */

#include "SystemMetrics.hpp"
#include "Platform.hpp"
#include <cstring>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

void CaptureSystemMetrics(SystemMetrics* metrics) {
    if (!metrics) return;
    std::memset(metrics, 0, sizeof(*metrics));

    // ── PID ──────────────────────────────────────────────────────────────────
    metrics->process_id = PlatformGetPid();

    // ── Process name ─────────────────────────────────────────────────────────
#ifdef _WIN32
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
        // Extract filename from path
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) {
            std::strncpy(metrics->process_name, lastSlash + 1, sizeof(metrics->process_name) - 1);
        } else {
            std::strncpy(metrics->process_name, exePath, sizeof(metrics->process_name) - 1);
        }
    } else {
        std::strncpy(metrics->process_name, "unknown", sizeof(metrics->process_name) - 1);
    }
#else
    FILE* fp = fopen("/proc/self/comm", "r");
    if (fp) {
        if (fgets(metrics->process_name, sizeof(metrics->process_name), fp)) {
            // Strip trailing newline
            char* nl = strchr(metrics->process_name, '\n');
            if (nl) *nl = '\0';
        }
        fclose(fp);
    } else {
        std::strncpy(metrics->process_name, "unknown", sizeof(metrics->process_name) - 1);
    }
#endif

    // ── CPU usage ────────────────────────────────────────────────────────────
#ifdef _WIN32
    // Simple CPU usage estimation (not as accurate as Linux)
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        static ULARGE_INTEGER prevIdle = {0}, prevKernel = {0}, prevUser = {0};
        ULARGE_INTEGER currIdle, currKernel, currUser;
        currIdle.LowPart = idleTime.dwLowDateTime;
        currIdle.HighPart = idleTime.dwHighDateTime;
        currKernel.LowPart = kernelTime.dwLowDateTime;
        currKernel.HighPart = kernelTime.dwHighDateTime;
        currUser.LowPart = userTime.dwLowDateTime;
        currUser.HighPart = userTime.dwHighDateTime;

        if (prevIdle.QuadPart != 0) {
            ULONGLONG idleDiff = currIdle.QuadPart - prevIdle.QuadPart;
            ULONGLONG totalDiff = (currKernel.QuadPart - prevKernel.QuadPart) + (currUser.QuadPart - prevUser.QuadPart);
            if (totalDiff > 0) {
                metrics->cpu_usage_percent = 100.0 * (1.0 - static_cast<double>(idleDiff) / static_cast<double>(totalDiff));
            }
        }
        prevIdle = currIdle;
        prevKernel = currKernel;
        prevUser = currUser;
    }
#else
    FILE* stat = fopen("/proc/stat", "r");
    if (stat) {
        unsigned long long user{}, nice{}, sys{}, idle{}, iowait{}, irq{}, softirq{};
        if (fscanf(stat, "cpu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &sys, &idle, &iowait, &irq, &softirq) == 7) {
            unsigned long long total    = user + nice + sys + idle + iowait + irq + softirq;
            unsigned long long idle_all = idle + iowait;
            metrics->cpu_usage_percent  = (total > 0)
                ? 100.0 * (1.0 - static_cast<double>(idle_all) / static_cast<double>(total))
                : 0.0;
        }
        fclose(stat);
    }
#endif

    // ── Memory ───────────────────────────────────────────────────────────────
#ifdef _WIN32
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        metrics->memory_total_kb = memInfo.ullTotalPhys / 1024;
        metrics->memory_used_kb = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / 1024;
    }
#else
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        unsigned long memTotal = 0, memAvailable = 0;
        while (fgets(line, sizeof(line), meminfo)) {
            sscanf(line, "MemTotal: %lu kB",     &memTotal);
            sscanf(line, "MemAvailable: %lu kB", &memAvailable);
        }
        metrics->memory_total_kb = static_cast<uint64_t>(memTotal);
        metrics->memory_used_kb  = static_cast<uint64_t>(
            (memTotal > memAvailable) ? (memTotal - memAvailable) : 0);
        fclose(meminfo);
    }
#endif

    // ── Thread count ─────────────────────────────────────────────────────────
#ifdef _WIN32
    // Get current process handle
    HANDLE hProcess = GetCurrentProcess();
    DWORD threadCount = 0;
    if (hProcess) {
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            // Note: Windows doesn't provide direct thread count in this API
            // This is a limitation; we'll use a default
            metrics->thread_count = 1; // safe default
        }
    }
    metrics->thread_count = 1; // safe default for Windows
#else
    metrics->thread_count = 1; // safe default
    FILE* status = fopen("/proc/self/status", "r");
    if (status) {
        char line[256];
        while (fgets(line, sizeof(line), status)) {
            if (sscanf(line, "Threads: %d", &metrics->thread_count) == 1) break;
        }
        fclose(status);
    }
#endif
}
