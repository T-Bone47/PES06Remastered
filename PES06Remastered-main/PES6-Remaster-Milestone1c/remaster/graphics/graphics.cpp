// remaster graphics subsystem - resolution and window size.
//
// Two hooks, both inside the game process, both on functions the game itself
// calls. No d3d8.dll proxy, no import-table rewriting, no second module.
//
//  FUN_00404C60  D3D init. Writes the resolution globals
//                (0x00F83FD8 width / 0x00F83FDC height). We let it run, then
//                overwrite them, so the later device creation picks up our
//                values.
//
//  FUN_00410C20  CreateWindowExA wrapper. Takes the client width/height as
//                plain arguments, so we can pass ours.
//
// Evidence for both addresses and for the global layout comes from the Ghidra
// analysis of pes6.exe (sha256 0c7f804b8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f),
// recorded in remaster_manifest.txt.

#include "graphics.h"
#include "../core/remaster.h"

#include "MinHook.h"

#include <stdio.h>

namespace remaster {
namespace graphics {

// Game globals (image base 0x00400000).
static const unsigned int G_WIDTH = 0x00F83FD8;
static const unsigned int G_HEIGHT = 0x00F83FDC;
static const unsigned int G_WIDTH2 = 0x00F83FE0;
static const unsigned int G_HEIGHT2 = 0x00F83FE4;

typedef int(__cdecl* FnD3DInit)(void*, void*);
typedef void*(__cdecl* FnCreateWindow)(void*, int, int);

static FnD3DInit g_origD3DInit = 0;
static FnCreateWindow g_origCreateWindow = 0;

static int g_width = 0;
static int g_height = 0;

static void writeResolutionGlobals(int w, int h) {
    *(unsigned int*)G_WIDTH = (unsigned int)w;
    *(unsigned int*)G_HEIGHT = (unsigned int)h;
    *(unsigned int*)G_WIDTH2 = (unsigned int)w;
    *(unsigned int*)G_HEIGHT2 = (unsigned int)h;
}

static int __cdecl detourD3DInit(void* self, void* arg2) {
    int result = g_origD3DInit(self, arg2);
    if (g_width > 0 && g_height > 0) {
        writeResolutionGlobals(g_width, g_height);
        log().write("INFO", "graphics: resolution globals set to %dx%d", g_width, g_height);
    }
    return result;
}

static void* __cdecl detourCreateWindow(void* hInstance, int width, int height) {
    if (g_width > 0 && g_height > 0) {
        log().write("INFO", "graphics: window %dx%d -> %dx%d", width, height, g_width, g_height);
        width = g_width;
        height = g_height;
    }
    return g_origCreateWindow(hInstance, width, height);
}

bool install() {
    bool ok = true;

    g_width = config().getInt("graphics", "width", 0);
    g_height = config().getInt("graphics", "height", 0);
    bool windowed = config().getBool("graphics", "windowed", true);

    if (!config().getBool("graphics", "enabled", true)) {
        log().write("INFO", "graphics: disabled in config");
        return true;
    }
    if (g_width <= 0 || g_height <= 0) {
        log().write("INFO", "graphics: no width/height configured - subsystem inactive");
        return true;
    }
    if (!windowed) {
        log().write("INFO", "graphics: windowed=0 requested but fullscreen is not "
                            "implemented yet - forcing windowed");
    }

    unsigned int initAddr = manifest().getAddr("d3d.init");
    unsigned int winAddr = manifest().getAddr("d3d.createwindow");

    if (!initAddr || !winAddr) {
        log().write("WARN", "graphics: manifest is missing d3d.init / d3d.createwindow");
        return false;
    }

    // d3d.init
    if (MH_CreateHook((void*)initAddr, (void*)detourD3DInit, (LPVOID*)&g_origD3DInit) != MH_OK ||
        MH_EnableHook((void*)initAddr) != MH_OK) {
        log().write("WARN", "graphics: failed to hook d3d.init (0x%08X)", initAddr);
        ok = false;
    } else {
        hooks().install("graphics.d3d_init", (void*)initAddr, (void*)detourD3DInit,
                        manifest().get("sig.d3d_init", 0));
    }

    // d3d.createwindow
    if (MH_CreateHook((void*)winAddr, (void*)detourCreateWindow, (LPVOID*)&g_origCreateWindow) != MH_OK ||
        MH_EnableHook((void*)winAddr) != MH_OK) {
        log().write("WARN", "graphics: failed to hook d3d.createwindow (0x%08X)", winAddr);
        ok = false;
    } else {
        hooks().install("graphics.create_window", (void*)winAddr, (void*)detourCreateWindow,
                        manifest().get("sig.create_window", 0));
    }

    return ok;
}

}  // namespace graphics
}  // namespace remaster
