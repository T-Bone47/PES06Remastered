// remaster.asi - PES 6 Legacy Remaster.
//
// A single ASI loaded by the Ultimate ASI Loader (dinput8.dll). It is a
// superset of the upstream PESMod gameplay mod, plus the remaster layers.
//
//   core/      manifest, config, logger, hook registry, build gate
//   graphics/  resolution, window, texture override
//   assets/    AFS loader interception and the replacement pack
//   src/       upstream PESMod gameplay sources (GPL-3.0-or-later)
//
// Load order and failure policy:
//
//   1. logger, manifest, config are brought up first.
//   2. The manifest gate checks every recorded address against the running
//      image. If the gameplay hooks do not match this build, the gameplay
//      layer is skipped entirely rather than left to crash.
//   3. The gameplay layer (upstream) installs its hooks.
//   4. The graphics and asset layers install theirs.
//
// Any layer that fails is logged and skipped. The game always starts.

#include "core/remaster.h"
#include "graphics/graphics.h"
#include "graphics/d3d8.h"
#include "graphics/postprocess.h"
#include "graphics/modern_hud.h"
#include "analytics/analytics.h"
#include "telemetry/telemetry.h"
#include "broadcast/director.h"
#include "gameplay/enhanced.h"
#include "io/fileio.h"
#ifndef REMASTER_NO_ASSETS
#include "assets/assets.h"
#endif

#include "src/mod_core.h"   // upstream: ModInitialise / ModShutdown

#include "MinHook.h"
#include <intrin.h>

#ifdef REMASTER_NO_ASSETS
// Bisect stub: the assets layer is excluded from this build.
namespace remaster {
namespace assets {
bool install() { return true; }
void report() {}
}
}
#endif

#ifdef REMASTER_NO_UPSTREAM
// Bisect stub: the upstream gameplay layer is excluded from this build.
void ModInitialise(HMODULE) {}
void ModShutdown() {}
#endif

using namespace remaster;

static const char* FILE_MANIFEST = "remaster_manifest.txt";
static const char* FILE_CONFIG = "remaster.ini";
static const char* FILE_LOG = "remaster.log";

typedef HANDLE(WINAPI* FnCreateMutexA)(LPSECURITY_ATTRIBUTES, BOOL, LPCSTR);
static FnCreateMutexA g_origCreateMutexA = nullptr;

static HANDLE WINAPI detourCreateMutexA(LPSECURITY_ATTRIBUTES lpAttr, BOOL bInitial, LPCSTR lpName) {
    if (lpName && strstr(lpName, "87F2B5F7") != nullptr) {
        // Konami single-instance mutex {87F2B5F7-09C3-4520-AB5E-16313ADB035B}.
        // If another instance or zombie process is holding this mutex, pes6.exe would
        // get ERROR_ALREADY_EXISTS (183) and trigger the fatal
        // "This program cannot be started if 'Settings' is performing." dialog.
        // We create an anonymous mutex and explicitly set LastError to ERROR_SUCCESS
        // so the game's check 'cmp eax, 0xB7; jne success' ALWAYS takes the success jump!
        HANDLE h = g_origCreateMutexA ? g_origCreateMutexA(lpAttr, bInitial, nullptr) : CreateMutexA(lpAttr, bInitial, nullptr);
        SetLastError(ERROR_SUCCESS);
        log().write("INFO", "core: intercepted single-instance mutex check; forced ERROR_SUCCESS");
        return h;
    }
    return g_origCreateMutexA ? g_origCreateMutexA(lpAttr, bInitial, lpName) : CreateMutexA(lpAttr, bInitial, lpName);
}

typedef HWND(WINAPI* FnFindWindowA)(LPCSTR, LPCSTR);
static FnFindWindowA g_origFindWindowA = nullptr;

static HWND WINAPI detourFindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName) {
    if (lpWindowName && strcmp(lpWindowName, "Settings") == 0 && !lpClassName) {
        // pes6.exe only calls FindWindowA(NULL, "Settings") at 0x00410EBC to trigger
        // the fatal "This program cannot be started if 'Settings' is performing." dialog.
        // On modern Windows 10/11, Windows Settings (SystemSettings.exe) shares this window title.
        // Returning NULL unconditionally suppresses this false error completely.
        log().write("INFO", "core: suppressed FindWindowA(NULL, 'Settings') query");
        return NULL;
    }
    return g_origFindWindowA ? g_origFindWindowA(lpClassName, lpWindowName) : FindWindowA(lpClassName, lpWindowName);
}

typedef void(WINAPI* FnExitProcess)(UINT);
static FnExitProcess g_origExitProcess = nullptr;

static void WINAPI detourExitProcess(UINT uExitCode) {
    void* retAddr = _ReturnAddress();
    log().write("WARN", "core: ExitProcess called with code %u from %p", uExitCode, retAddr);
    void* frames[16] = { 0 };
    WORD nFrames = CaptureStackBackTrace(0, 16, frames, NULL);
    for (WORD i = 0; i < nFrames; ++i) {
        log().write("WARN", "  frame[%u] = %p", i, frames[i]);
    }
    if (g_origExitProcess) g_origExitProcess(uExitCode);
    ExitProcess(uExitCode);
}

typedef int(WINAPI* FnMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT);
static FnMessageBoxA g_origMessageBoxA = nullptr;

static int WINAPI detourMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    log().write("WARN", "core: MessageBoxA called: caption='%s', text='%s'",
                lpCaption ? lpCaption : "", lpText ? lpText : "");
    return g_origMessageBoxA ? g_origMessageBoxA(hWnd, lpText, lpCaption, uType) : MessageBoxA(hWnd, lpText, lpCaption, uType);
}

typedef LSTATUS(WINAPI* FnRegQueryValueExA)(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD);
static FnRegQueryValueExA g_origRegQueryValueExA = nullptr;

static LSTATUS WINAPI detourRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    if (lpValueName && _stricmp(lpValueName, "installdir") == 0) {
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *(lastSlash + 1) = '\0';
        DWORD len = (DWORD)strlen(exePath) + 1;
        if (lpType) *lpType = REG_SZ;
        if (lpcbData) {
            if (lpData && *lpcbData >= len) {
                memcpy(lpData, exePath, len);
                log().write("INFO", "core: RegQueryValueExA('installdir') redirected -> '%s'", exePath);
                return ERROR_SUCCESS;
            }
            *lpcbData = len;
            if (!lpData) return ERROR_SUCCESS;
            return ERROR_MORE_DATA;
        }
    }
    return g_origRegQueryValueExA ? g_origRegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData)
                                  : RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
}

typedef int(*FnEnvironmentCheck)(void);
static FnEnvironmentCheck g_origEnvironmentCheck = nullptr;

static int detourEnvironmentCheck(void) {
    log().write("INFO", "core: environment and single-instance check bypassed; returning success (1)");
    return 1;
}

static bool fileExists(const char* path) {
    DWORD a = GetFileAttributesA(path);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static bool g_gameplayLoaded = false;

static void installGameplay() {
    if (!config().getBool("gameplay", "enabled", true)) {
        log().write("INFO", "gameplay: disabled by config");
        return;
    }
    // Safety gate: the upstream hooks are hardcoded addresses, so they are
    // only safe on a build whose code matches the manifest.
    if (!manifest().isLoaded()) {
        log().write("WARN", "gameplay: manifest is not loaded - skipping gameplay layer to protect stability");
        return;
    }
    ManifestCheck g = manifest().validate("game.");
    log().write("INFO", "gate: gameplay %d checked, %d matched, %d mismatched, %d unmapped",
                g.total, g.matched, g.mismatched, g.unmapped);
    if (g.total == 0 || g.matched < g.total) {
        log().write("WARN", "gameplay: %d of %d hook targets do not match this build "
                            "- skipping the gameplay layer to avoid a crash",
                    g.total - g.matched, g.total);
        return;
    }
    ModInitialise(GetModuleHandleA(NULL));
    g_gameplayLoaded = true;
    log().write("INFO", "gameplay: upstream hooks installed");
}

static void installRemasterLayers() {
    // The upstream layer may already have initialised MinHook; if it did not
    // (gameplay disabled), do it here so our own hooks still work.
    MH_STATUS status = MH_Initialize();
    if (status == MH_OK) {
        log().write("INFO", "MinHook initialised by the remaster");
    }
    else if (status == MH_ERROR_ALREADY_INITIALIZED) {
        log().write("INFO", "MinHook already initialised");
    }
    else {
        log().write("WARN", "MinHook init failed (status %d) - remaster layers inactive", status);
        return;
    }

    // Intercept CreateMutexA so the game never gets ERROR_ALREADY_EXISTS on {87F2B5F7-...}
    if (MH_CreateHookApi(L"kernel32", "CreateMutexA", (LPVOID)&detourCreateMutexA, (LPVOID*)&g_origCreateMutexA) == MH_OK) {
        MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateMutexA"));
        log().write("INFO", "core: installed CreateMutexA compatibility hook");
    }

    // Windows 10/11 compatibility hook: neutralize false "Settings is performing" error
    // caused by the modern Windows Settings app sharing the window title "Settings".
    if (MH_CreateHookApi(L"user32", "FindWindowA", (LPVOID)&detourFindWindowA, (LPVOID*)&g_origFindWindowA) == MH_OK) {
        MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleA("user32.dll"), "FindWindowA"));
        log().write("INFO", "core: installed FindWindowA compatibility hook for Windows 10/11");
    }

    if (MH_CreateHookApi(L"kernel32", "ExitProcess", (LPVOID)&detourExitProcess, (LPVOID*)&g_origExitProcess) == MH_OK) {
        MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess"));
        log().write("INFO", "core: installed ExitProcess tracking hook");
    }

    if (MH_CreateHookApi(L"user32", "MessageBoxA", (LPVOID)&detourMessageBoxA, (LPVOID*)&g_origMessageBoxA) == MH_OK) {
        MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleA("user32.dll"), "MessageBoxA"));
        log().write("INFO", "core: installed MessageBoxA tracking hook");
    }

    if (MH_CreateHookApi(L"advapi32", "RegQueryValueExA", (LPVOID)&detourRegQueryValueExA, (LPVOID*)&g_origRegQueryValueExA) == MH_OK) {
        MH_EnableHook((LPVOID)GetProcAddress(GetModuleHandleA("advapi32.dll"), "RegQueryValueExA"));
        log().write("INFO", "core: installed RegQueryValueExA portability hook");
    }

    // Bypass obsolete 2006 CD-ROM / registry / mutex / Settings environment check
    if (MH_CreateHook((LPVOID)0x00410E50, (LPVOID)&detourEnvironmentCheck, (LPVOID*)&g_origEnvironmentCheck) == MH_OK) {
        MH_EnableHook((LPVOID)0x00410E50);
        log().write("INFO", "core: installed environment check bypass hook at 0x00410E50");
    }

    if (config().getBool("graphics", "enabled", true)) {
        if (!graphics::install()) {
            log().write("WARN", "graphics: install failed - continuing without it");
        }
        if (!graphics::installD3D8()) {
            log().write("WARN", "d3d8: install failed - continuing without it");
        }
        graphics::initPostProcess();
        graphics::initModernHud();
    }
    else {
        log().write("INFO", "graphics: disabled by config");
    }
    analytics::init();
    telemetry::install();
    if (config().getBool("broadcast", "enabled", true)) {
        if (!broadcast::init()) {
            log().write("WARN", "broadcast: init failed - continuing without it");
        }
    }
    else {
        log().write("INFO", "broadcast: disabled by config");
    }
    if (config().getBool("gameplay_enhanced", "enabled", true)) {
        if (!gameplay::init()) {
            log().write("WARN", "gameplay_enhanced: init failed - continuing in Classic Mode");
        }
    }
    else {
        log().write("INFO", "gameplay_enhanced: disabled by config -> Classic Mode active");
    }
    if (!fileio::install()) {
        log().write("WARN", "fileio: install failed - continuing without it");
    }
    if (config().getBool("assets", "enabled", false)) {
        if (!assets::install()) {
            log().write("WARN", "assets: install failed - continuing without it");
        }
    }
    else {
        log().write("INFO", "assets: disabled by config");
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_DETACH) {
        graphics::shutdownD3D8();
        // lpReserved is non-NULL when the process is terminating; the OS
        // cleans up for us then, so only do work on a clean FreeLibrary.
        if (reserved == NULL) {
            if (g_gameplayLoaded) {
                ModShutdown();
            }
            assets::report();
            graphics::reportD3D8();
            broadcast::shutdown();
            gameplay::shutdown();
            telemetry::shutdown();
            fileio::shutdown();
            hooks().uninstallAll();
            hooks().report();
        }
        return TRUE;
    }
    if (reason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }
    DisableThreadLibraryCalls(module);

    char* logPath = gamePath(FILE_LOG);
    log().init(logPath ? logPath : FILE_LOG);
    log().write("INFO", "remaster.asi attaching (pid %lu)", (unsigned long)GetCurrentProcessId());

    char* configPath = gamePath(FILE_CONFIG);
    if (configPath && fileExists(configPath)) {
        if (config().load(configPath)) {
            log().write("INFO", "config loaded from %s", configPath);
        }
        else {
            log().write("WARN", "config present but unreadable: %s", configPath);
        }
    }
    else {
        log().write("INFO", "no %s - using defaults", FILE_CONFIG);
    }

    // P0.2 - Master Enable/Disable Gate:
    if (!config().getBool("core", "enabled", true)) {
        log().write("INFO", "core: remaster disabled by config [core] enabled=0 - staying completely inert");
        return TRUE;
    }

    char* manifestPath = gamePath(FILE_MANIFEST);
    if (manifestPath && fileExists(manifestPath)) {
        if (manifest().load(manifestPath)) {
            log().write("INFO", "manifest loaded from %s", manifestPath);
        }
        else {
            log().write("WARN", "manifest present but unreadable: %s", manifestPath);
        }
    }
    else {
        log().write("WARN", "no %s next to the game - the remaster will stay inert", FILE_MANIFEST);
    }

    if (!manifest().isLoaded()) {
        log().write("WARN", "manifest not loaded - staying inert without installing hooks");
        return TRUE;
    }

    ManifestCheck all = manifest().validate();
    log().write("INFO", "gate: %d addresses checked, %d matched, %d mismatched, %d unmapped",
                all.total, all.matched, all.mismatched, all.unmapped);

    installGameplay();
    installRemasterLayers();

    assets::report();
    hooks().report();
    log().write("INFO", "remaster.asi ready");
    return TRUE;
}



