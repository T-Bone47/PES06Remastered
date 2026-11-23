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
#include "telemetry/telemetry.h"
#include "io/fileio.h"
#ifndef REMASTER_NO_ASSETS
#include "assets/assets.h"
#endif

#include "src/mod_core.h"   // upstream: ModInitialise / ModShutdown

#include "MinHook.h"

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
    ManifestCheck g = manifest().validate("game.");
    log().write("INFO", "gate: gameplay %d checked, %d matched, %d mismatched, %d unmapped",
                g.total, g.matched, g.mismatched, g.unmapped);
    if (g.total > 0 && g.matched < g.total) {
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

    if (config().getBool("graphics", "enabled", true)) {
        if (!graphics::install()) {
            log().write("WARN", "graphics: install failed - continuing without it");
        }
        if (!graphics::installD3D8()) {
            log().write("WARN", "d3d8: install failed - continuing without it");
        }
    }
    else {
        log().write("INFO", "graphics: disabled by config");
    }
    telemetry::install();
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
        // lpReserved is non-NULL when the process is terminating; the OS
        // cleans up for us then, so only do work on a clean FreeLibrary.
        if (reserved == NULL) {
            if (g_gameplayLoaded) {
                ModShutdown();
            }
            assets::report();
            graphics::reportD3D8();
            telemetry::shutdown();
            fileio::shutdown();
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



