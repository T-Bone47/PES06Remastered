// SEH-guarded D3D8 vtable matrix probe.
//
// Goal: identify which IDirect3D8 vtable slot is CreateDevice in the *system*
// d3d8.dll, and which D3DPRESENT_PARAMETERS layout it expects.
//
// Every candidate call is wrapped in __try/__except so a fault is recorded
// instead of killing the process.
//
//   probe_matrix.exe <slot> <mode>
//     mode: createdevice | hmodule | modecount | displaymode

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct PP_MS {
    UINT   BackBufferWidth, BackBufferHeight, BackBufferFormat, BackBufferCount;
    UINT   MultiSampleType; DWORD MultiSampleQuality; UINT SwapEffect; HWND hDeviceWindow;
    BOOL   Windowed, EnableAutoDepthStencil; UINT AutoDepthStencilFormat; DWORD Flags;
    UINT   FullScreen_RefreshRateInHz, FullScreen_ModeIndex;
} PP_MS;

typedef struct PP_MIN {
    UINT   BackBufferWidth, BackBufferHeight, BackBufferFormat, BackBufferCount;
    UINT   MultiSampleType; UINT SwapEffect; HWND hDeviceWindow;
    BOOL   Windowed, EnableAutoDepthStencil; UINT AutoDepthStencilFormat; DWORD Flags;
    UINT   FullScreen_RefreshRateInHz, FullScreen_PresentationInterval;
} PP_MIN;

#define D3D8_SDK_VERSION 220
typedef HRESULT(WINAPI *PFN_ANY)(void*, void*, void*, void*, void*, void*, void*);

static const char* g_mode;
static void* g_d3d;
static void** g_vt;
static HWND g_hwnd;
static int g_slot;
static HRESULT g_hr;
static void* g_dev;

static void doCall(void) {
    g_hr = 0xDEAD0000;
    g_dev = NULL;
    char raw[128];
    memset(raw, 0, sizeof(raw));

    if (strcmp(g_mode, "hmodule") == 0) {
        g_hr = ((PFN_ANY)g_vt[g_slot])(g_d3d, (void*)GetModuleHandleA(NULL), 0, 0, 0, 0, 0);
    } else if (strcmp(g_mode, "modecount") == 0) {
        g_hr = ((PFN_ANY)g_vt[g_slot])(g_d3d, (void*)0, 0, 0, 0, 0, 0);
    } else if (strcmp(g_mode, "displaymode") == 0) {
        char dm[64]; memset(dm, 0, sizeof(dm));
        g_hr = ((PFN_ANY)g_vt[g_slot])(g_d3d, (void*)0, dm, 0, 0, 0, 0);
    } else {
        // createdevice: (this, adapter, devtype, hwnd, flags, pp, &dev)
        int min = getenv("USE_MIN") != NULL;
        int ref = getenv("USE_REF") != NULL;
        if (min) {
            PP_MIN* p = (PP_MIN*)raw;
            p->BackBufferWidth = 640; p->BackBufferHeight = 480;
            p->BackBufferFormat = 22; p->BackBufferCount = 1;
            p->MultiSampleType = 0; p->SwapEffect = 1;
            p->hDeviceWindow = g_hwnd; p->Windowed = TRUE; p->EnableAutoDepthStencil = TRUE;
        } else {
            PP_MS* p = (PP_MS*)raw;
            p->BackBufferWidth = 640; p->BackBufferHeight = 480;
            p->BackBufferFormat = 22; p->BackBufferCount = 1;
            p->MultiSampleType = 0; p->MultiSampleQuality = 0; p->SwapEffect = 1;
            p->hDeviceWindow = g_hwnd; p->Windowed = TRUE; p->EnableAutoDepthStencil = TRUE;
        }
        g_hr = ((PFN_ANY)g_vt[g_slot])(g_d3d, (void*)0, (void*)(ref ? 2 : 1), g_hwnd,
                                       (void*)0, raw, &g_dev);
    }
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    g_slot = (argc > 1) ? atoi(argv[1]) : 13;
    g_mode = (argc > 2) ? argv[2] : "createdevice";

    HMODULE d3d8 = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d8.dll");
    if (!d3d8) { printf("no d3d8\n"); return 2; }
    typedef void* (WINAPI *PFN_Create8)(UINT);
    g_d3d = ((PFN_Create8)GetProcAddress(d3d8, "Direct3DCreate8"))(D3D8_SDK_VERSION);
    if (!g_d3d) { printf("create8 null\n"); return 2; }
    g_vt = *(void***)g_d3d;

    WNDCLASSA wc; memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC; wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL); wc.lpszClassName = "D3D8M";
    RegisterClassA(&wc);
    g_hwnd = CreateWindowA("D3D8M", "p", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480,
                            NULL, NULL, GetModuleHandleA(NULL), NULL);

    printf("slot=%d mode=%-12s min=%d ref=%d\n", g_slot, g_mode,
           getenv("USE_MIN") ? 1 : 0, getenv("USE_REF") ? 1 : 0);
    __try {
        doCall();
        printf("  returned hr=0x%08lX dev=%p\n", (unsigned long)g_hr, g_dev);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("  FAULTED (code 0x%08lX)\n", (unsigned long)GetExceptionCode());
    }
    if (g_dev && g_hr == 0) {
        void** dvt = *(void***)g_dev;
        char back[128]; memset(back, 0, sizeof(back));
        typedef HRESULT(WINAPI *PFN_GetCP)(void*, void*);
        HRESULT hr2 = ((PFN_GetCP)dvt[9])(g_dev, back);
        PP_MS* b = (PP_MS*)back;
        printf("  GetCreationParameters hr=0x%08lX  W=%u H=%u fmt=%u swap=%u win=%d\n",
               (unsigned long)hr2, b->BackBufferWidth, b->BackBufferHeight,
               b->BackBufferFormat, b->SwapEffect, b->Windowed);
    }
    return 0;
}
