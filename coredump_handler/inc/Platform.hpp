/*
 * Platform.hpp - Abstraction de la plateforme.
 * Fournit des fonctions utilitaires compatibles Linux et Windows pour
 * l'attente, la détection de processus et la gestion des signaux.
 */

#pragma once

#include "Options.hpp"

// To disable Windows support for Linux-only builds, comment out the _WIN32 sections below
// or define NO_WINDOWS_SUPPORT before including this file

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <csignal>
#else
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <execinfo.h>
#endif

// ─── Sleep (milliseconds) ─────────────────────────────────────────────────────
inline void PlatformSleepMs(unsigned int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000u);
#endif
}

// ─── Process / Thread IDs ─────────────────────────────────────────────────────
inline int PlatformGetPid() {
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

inline int PlatformGetTid() {
#ifdef _WIN32
    return static_cast<int>(GetCurrentThreadId());
#else
    return static_cast<int>(gettid());
#endif
}

// ─── Stack backtrace ──────────────────────────────────────────────────────────
inline int PlatformBacktrace(void** buffer, int maxFrames) {
#ifdef _WIN32
    // Windows backtrace not implemented, return 0
    (void)buffer;
    (void)maxFrames;
    return 0;
#else
    return backtrace(buffer, maxFrames);
#endif
}

// ─── Signal handling ──────────────────────────────────────────────────────────
inline void PlatformInstallSignal(int sig, void (*handler)(int)) {
#ifdef _WIN32
    // Windows signal handling is limited, use signal for basic support
    signal(sig, handler);
#else
    // Use sigaction for reliable, POSIX-compliant signal handling.
    struct sigaction sa{};
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND; // restore default handler after first call
    sigaction(sig, &sa, nullptr);
#endif
}
