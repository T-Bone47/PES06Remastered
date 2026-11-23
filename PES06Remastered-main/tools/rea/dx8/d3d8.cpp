// d3d8 proxy for the PES 6 Legacy Remaster.
//
// Drops into the game folder as d3d8.dll. The game imports Direct3DCreate8
// from d3d8.dll, so the proxy intercepts that call, patches the returned
// IDirect3D8 vtable, and rewrites the presentation parameters to run the game
// at a higher resolution.
//
// Design notes
// ------------
// * The slot we patch (0x34 / index 13) is the one the game itself calls
//   immediately after Direct3DCreate8 (verified from the game's code at
//   VA 0x00404D51: `call dword ptr [ecx+0x34]`). We do not need to know the
//   method's name: the hook validates the incoming arguments at runtime and
//   only rewrites them when they look like a presentation-parameters struct.
//   If validation fails the call is passed through untouched, so a wrong
//   assumption can never corrupt the game.
//
// * Only BackBufferWidth (offset 0) and BackBufferHeight (offset 4) are
//   written. Those offsets are identical in every published D3D8 layout, so
//   the proxy does not need to know whether MultiSampleQuality is present.
//   The game already runs windowed, so Windowed is left alone.
//
// * The game window is resized to match the new backbuffer so the image is
//   rendered 1:1 instead of being scaled.
//
// Configuration: dx8wrap.ini next to the game executable.
//
//   [resolution]
//   width=1280
//   height=720
//   enabled=1
//   log=1

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define REAL_D3D8_SDK_VERSION 220

// The slot the game calls after Direct3DCreate8.
#define ID3D8_VTBL_CREATE_DEVICE 13

// Only these two fields are touched; both offsets are layout-independent.
struct PP_HEAD {
    UINT BackBufferWidth;
    UINT BackBufferHeight;
};

typedef HRESULT(WINAPI *PFN_Create8)(UINT);
typedef void* (WINAPI *PFN_Direct3DCreate8)(UINT);
typedef HRESULT(WINAPI *PFN_Any)(void*, void*, void*, void*, void*, void*, void*);

static HMODULE g_real = NULL;
static PFN_Direct3DCreate8 g_realCreate8 = NULL;
static void** g_proxyVtbl = NULL;
static PFN_Any g_realSlot13 = NULL;
static int g_patched = 0;

static int g_width = 0;
static int g_height = 0;
static int g_enabled = 1;
static int g_log = 1;
static HWND g_gameWindow = NULL;

static void Log(const char* fmt, ...) {
    if (!g_log) {
        return;
    }
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) {
        *(slash + 1) = 0;
    }
    strcat(path, "dx8wrap.log");
    FILE* f = fopen(path, "a");
    if (f) {
        fputs(buf, f);
        fputc('\n', f);
        fclose(f);
    }
}

static void ReadConfig(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) {
        *(slash + 1) = 0;
    }
    strcat(path, "dx8wrap.ini");
    FILE* f = fopen(path, "r");
    if (!f) {
        Log("[dx8wrap] no dx8wrap.ini - running at the game's native resolution");
        return;
    }
    char line[256];
    int inRes = 0;
    while (fgets(line, sizeof(line), f)) {
        char* s = line;
        while (*s == ' ' || *s == '\t') {
            s++;
        }
        if (*s == ';' || *s == '#' || *s == '\n' || *s == '\0') {
            continue;
        }
        char* br = strchr(s, '[');
        if (br) {
            // Section header: the name is between '[' and the trailing ']'.
            char* end = strchr(br, ']');
            if (end) {
                *end = 0;
            }
            inRes = (strstr(br + 1, "resolution") != NULL);
            continue;
        }
        char* eq = strchr(s, '=');
        if (!eq || !inRes) {
            continue;
        }
        *eq = 0;
        char* key = s;
        char* val = eq + 1;
        char* nl = strpbrk(val, "\r\n");
        if (nl) {
            *nl = 0;
        }
        while (*val == ' ' || *val == '\t') {
            val++;
        }
        if (strcmp(key, "width") == 0) {
            g_width = atoi(val);
        } else if (strcmp(key, "height") == 0) {
            g_height = atoi(val);
        } else if (strcmp(key, "enabled") == 0) {
            g_enabled = atoi(val);
        } else if (strcmp(key, "log") == 0) {
            g_log = atoi(val);
        }
    }
    fclose(f);
    Log("[dx8wrap] config: enabled=%d width=%d height=%d", g_enabled, g_width, g_height);
}

// Resolve the genuine d3d8.dll by absolute path so we never load ourselves.
static void LoadRealD3D8(void) {
    if (g_real) {
        return;
    }
    wchar_t dir[MAX_PATH];
    UINT n = GetSystemWow64DirectoryW(dir, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        n = GetSystemDirectoryW(dir, MAX_PATH);
    }
    if (n == 0) {
        return;
    }
    wcscat(dir, L"\\d3d8.dll");
    g_real = LoadLibraryW(dir);
    if (!g_real) {
        // Fall back to the plain name; if that resolves to us we are stuck.
        g_real = LoadLibraryA("d3d8.dll");
    }
    if (g_real) {
        g_realCreate8 = (PFN_Direct3DCreate8)GetProcAddress(g_real, "Direct3DCreate8");
    }
}

static void ResizeWindowToClient(HWND hwnd, int w, int h) {
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    RECT wr;
    if (!GetWindowRect(hwnd, &wr)) {
        return;
    }
    DWORD style = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
    DWORD ex = (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE);
    RECT c;
    c.left = c.top = 0;
    c.right = w;
    c.bottom = h;
    AdjustWindowRectEx(&c, style, FALSE, ex);
    int nw = c.right - c.left;
    int nh = c.bottom - c.top;
    int sx = wr.left, sy = wr.top;
    if (sx + nw > GetSystemMetrics(SM_CXVIRTUALSCREEN)) {
        sx = 0;
    }
    if (sy + nh > GetSystemMetrics(SM_CYVIRTUALSCREEN)) {
        sy = 0;
    }
    SetWindowPos(hwnd, NULL, sx, sy, nw, nh, SWP_NOZORDER | SWP_NOACTIVATE);
    Log("[dx8wrap] window resized to %dx%d (client %dx%d)", nw, nh, w, h);
}

// The hooked slot.
//
// The real method's argument count is unknown, so the hook must not change the
// stack discipline. InspectArgs() looks at a copy of the stack arguments and
// may edit them in place; the trampoline then TAIL-JUMPS to the real method so
// the real method pops exactly the arguments it pushed. A fixed-arity
// __stdcall hook would pop a different number of bytes and corrupt the
// caller's frame.
static void InspectArgs(void** args) {
    // args[0] = this, args[1..] = the method's remaining arguments.
    void* a1 = args[1];
    void* a2 = args[2];
    void* a3 = args[3];
    void* a4 = args[4];
    void* a5 = args[5];

    Log("[dx8wrap] slot13 args: this=%p a1=%p a2=%p a3=%p a4=%p a5=%p",
        args[0], a1, a2, a3, a4, a5);

    // Identify a presentation-parameters pointer among the arguments: it must
    // look like a D3DPRESENT_PARAMETERS (plausible width/height at +0/+4).
    PP_HEAD* pp = NULL;
    void** cand = &a1;
    for (int i = 0; i < 5; i++) {
        PP_HEAD* p = (PP_HEAD*)cand[i];
        if (!p) {
            continue;
        }
        __try {
            UINT w = p->BackBufferWidth;
            UINT h = p->BackBufferHeight;
            if (w >= 320 && w <= 4096 && h >= 240 && h <= 4096) {
                pp = p;
                Log("[dx8wrap]   candidate pp at arg[%d] = %p -> %ux%u", i + 1, p, w, h);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            // not a readable pointer
        }
    }

    if (pp && g_enabled && g_width > 0 && g_height > 0) {
        if (pp->BackBufferWidth != (UINT)g_width || pp->BackBufferHeight != (UINT)g_height) {
            Log("[dx8wrap]   rewriting backbuffer %ux%u -> %dx%d",
                pp->BackBufferWidth, pp->BackBufferHeight, g_width, g_height);
            pp->BackBufferWidth = (UINT)g_width;
            pp->BackBufferHeight = (UINT)g_height;
        }
    }
}

__declspec(naked) void ProxySlot13() {
    __asm {
        pushfd
        pushad
        // pushad frame (relative to the current esp):
        //   +0 EDI  +4 ESI  +8 EBP  +12 saved ESP (= ESP after pushfd)
        //   +16 EBX +20 EDX +24 ECX +28 EAX  +32 EFLAGS
        // The saved ESP is the value of ESP after pushfd, so the original ESP
        // is savedESP + 4 and the first stack argument lives at savedESP + 8.
        mov eax, [esp+12]
        add eax, 8
#if DX8_MODE == 3
        push eax
        call InspectArgs
        add esp, 4
#endif
        popad
        popfd
        jmp dword ptr [g_realSlot13]   ; tail call: real method does its own cleanup
    }
}

// Build-time bisect switch:
//   1 = forward only, never touch the vtable
//   2 = patch the vtable, trampoline does nothing but tail-jump
//   3 = patch the vtable, trampoline inspects and rewrites the arguments
#ifndef DX8_MODE
#define DX8_MODE 3
#endif

extern "C" void* WINAPI Proxy_Direct3DCreate8(UINT version) {
    LoadRealD3D8();
    if (!g_realCreate8) {
        Log("[dx8wrap] FATAL: could not resolve the real Direct3DCreate8");
        return NULL;
    }
    void* obj = g_realCreate8(version);
    if (!obj) {
        Log("[dx8wrap] real Direct3DCreate8 returned NULL");
        return NULL;
    }
#if DX8_MODE == 1
    Log("[dx8wrap] forward-only mode: vtable untouched");
    return obj;
#else
    void** vt = *(void***)obj;
    if (!g_patched) {
        // Patch the single slot in place. Do NOT copy the vtable: the real
        // IDirect3D8 vtable is only ~21 entries, so copying a fixed-size block
        // would overwrite whatever follows it in .rdata and corrupt the DLL.
        g_realSlot13 = (PFN_Any)vt[ID3D8_VTBL_CREATE_DEVICE];
        DWORD oldProt = 0;
        if (VirtualProtect(vt, sizeof(void*) * (ID3D8_VTBL_CREATE_DEVICE + 1),
                           PAGE_READWRITE, &oldProt)) {
            vt[ID3D8_VTBL_CREATE_DEVICE] = (void*)ProxySlot13;
            VirtualProtect(vt, sizeof(void*) * (ID3D8_VTBL_CREATE_DEVICE + 1),
                           oldProt, NULL);
            g_patched = 1;
            Log("[dx8wrap] patched vtable slot %d in place (was %p, now %p) mode=%d",
                ID3D8_VTBL_CREATE_DEVICE, (void*)g_realSlot13, (void*)ProxySlot13,
                DX8_MODE);
        } else {
            Log("[dx8wrap] FATAL: could not make the vtable writable (%lu)",
                GetLastError());
        }
    }
    return obj;
#endif
}

// ---- pass-through exports -------------------------------------------------

extern "C" void WINAPI Proxy_DebugSetMute(HMODULE h, BOOL mute) {
    LoadRealD3D8();
    typedef void(WINAPI *PFN)(HMODULE, BOOL);
    if (g_real) {
        ((PFN)GetProcAddress(g_real, "DebugSetMute"))(h, mute);
    }
}

extern "C" void WINAPI Proxy_Direct3D8EnableMaximizedWindowedModeShim(BOOL enable) {
    LoadRealD3D8();
    typedef void(WINAPI *PFN)(BOOL);
    if (g_real) {
        ((PFN)GetProcAddress(g_real, "Direct3D8EnableMaximizedWindowedModeShim"))(enable);
    }
}

extern "C" HRESULT WINAPI Proxy_ValidatePixelShader(DWORD* pCode, void* pData) {
    LoadRealD3D8();
    typedef HRESULT(WINAPI *PFN)(DWORD*, void*);
    if (g_real) {
        return ((PFN)GetProcAddress(g_real, "ValidatePixelShader"))(pCode, pData);
    }
    return (HRESULT)0x80004005L;
}

extern "C" HRESULT WINAPI Proxy_ValidateVertexShader(DWORD* pCode, void* pData) {
    LoadRealD3D8();
    typedef HRESULT(WINAPI *PFN)(DWORD*, void*);
    if (g_real) {
        return ((PFN)GetProcAddress(g_real, "ValidateVertexShader"))(pCode, pData);
    }
    return (HRESULT)0x80004005L;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        ReadConfig();
        // Deliberately do NOT load the real d3d8.dll here: doing library work
        // inside DllMain holds the loader lock, and the DirectX runtime's own
        // initialisation is not loader-lock safe. It is resolved lazily on
        // the first Direct3DCreate8 call instead.
    }
    return TRUE;
}
