// Isolated slot tester: argv[1] = vtable slot index, argv[2] = layout (ms|min).
// Runs in its own process so a wrong guess can only kill this attempt.
//
//   probe_slot.exe 13 ms
//   probe_slot.exe 13 min

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Microsoft D3D8 layout: 56 bytes, MultiSampleQuality at +20, ModeIndex at +52.
typedef struct D3DPRESENT_PARAMETERS_MS {
    UINT   BackBufferWidth;
    UINT   BackBufferHeight;
    UINT   BackBufferFormat;
    UINT   BackBufferCount;
    UINT   MultiSampleType;
    DWORD  MultiSampleQuality;
    UINT   SwapEffect;
    HWND   hDeviceWindow;
    BOOL   Windowed;
    BOOL   EnableAutoDepthStencil;
    UINT   AutoDepthStencilFormat;
    DWORD  Flags;
    UINT   FullScreen_RefreshRateInHz;
    UINT   FullScreen_ModeIndex;
} D3DPRESENT_PARAMETERS_MS;

// mingw/Wine d3d8types.h layout: 52 bytes, no MultiSampleQuality.
typedef struct D3DPRESENT_PARAMETERS_MIN {
    UINT   BackBufferWidth;
    UINT   BackBufferHeight;
    UINT   BackBufferFormat;
    UINT   BackBufferCount;
    UINT   MultiSampleType;
    UINT   SwapEffect;
    HWND   hDeviceWindow;
    BOOL   Windowed;
    BOOL   EnableAutoDepthStencil;
    UINT   AutoDepthStencilFormat;
    DWORD  Flags;
    UINT   FullScreen_RefreshRateInHz;
    UINT   FullScreen_PresentationInterval;
} D3DPRESENT_PARAMETERS_MIN;

#define D3D8_SDK_VERSION 220

typedef HRESULT(WINAPI *PFN_ANY)(void*, void*, void*, void*, void*, void*, void*);
typedef HRESULT(WINAPI *PFN_GetCP)(void*, void*);

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    int slot = (argc > 1) ? atoi(argv[1]) : 14;
    int useMin = (argc > 2) && (strcmp(argv[2], "min") == 0);

    HMODULE d3d8 = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d8.dll");
    if (!d3d8) { printf("no d3d8\n"); return 2; }
    typedef void* (WINAPI *PFN_Create8)(UINT);
    void* d3d = ((PFN_Create8)GetProcAddress(d3d8, "Direct3DCreate8"))(D3D8_SDK_VERSION);
    if (!d3d) { printf("create8 null\n"); return 2; }

    WNDCLASSA wc; memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "D3D8Slot";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("D3D8Slot", "p", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480,
                               NULL, NULL, GetModuleHandleA(NULL), NULL);

    void** vt = *(void***)d3d;
    printf("testing slot %d  (vt[%d] = %p)\n", slot, slot, vt[slot]);

    char raw[128]; memset(raw, 0, sizeof(raw));
    if (useMin) {
        D3DPRESENT_PARAMETERS_MIN* p = (D3DPRESENT_PARAMETERS_MIN*)raw;
        p->BackBufferWidth = 640; p->BackBufferHeight = 480;
        p->BackBufferFormat = 22; p->BackBufferCount = 1;
        p->MultiSampleType = 0; p->SwapEffect = 1;
        p->hDeviceWindow = hwnd; p->Windowed = TRUE; p->EnableAutoDepthStencil = TRUE;
    } else {
        D3DPRESENT_PARAMETERS_MS* p = (D3DPRESENT_PARAMETERS_MS*)raw;
        p->BackBufferWidth = 640; p->BackBufferHeight = 480;
        p->BackBufferFormat = 22; p->BackBufferCount = 1;
        p->MultiSampleType = 0; p->MultiSampleQuality = 0; p->SwapEffect = 1;
        p->hDeviceWindow = hwnd; p->Windowed = TRUE; p->EnableAutoDepthStencil = TRUE;
    }
    printf("  layout=%s (%u bytes)\n", useMin ? "mingw 52-byte" : "MS 56-byte",
           useMin ? (unsigned)sizeof(D3DPRESENT_PARAMETERS_MIN)
                  : (unsigned)sizeof(D3DPRESENT_PARAMETERS_MS));

    void* dev = NULL;
    // CreateDevice(this, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pp, &dev)
    HRESULT hr = ((PFN_ANY)vt[slot])(d3d, (void*)0, (void*)1, hwnd, (void*)0, raw, &dev);
    printf("  hr=0x%08lX  dev=%p\n", (unsigned long)hr, dev);

    if (dev && hr == 0) {
        void** dvt = *(void***)dev;
        char back[128]; memset(back, 0, sizeof(back));
        HRESULT hr2 = ((PFN_GetCP)dvt[9])(dev, back);
        printf("  GetCreationParameters(slot9) hr=0x%08lX\n", (unsigned long)hr2);
        if (useMin) {
            D3DPRESENT_PARAMETERS_MIN* b = (D3DPRESENT_PARAMETERS_MIN*)back;
            printf("  readback: W=%u H=%u fmt=%u swap=%u hwnd=%p windowed=%d\n",
                   b->BackBufferWidth, b->BackBufferHeight, b->BackBufferFormat,
                   b->SwapEffect, b->hDeviceWindow, b->Windowed);
        } else {
            D3DPRESENT_PARAMETERS_MS* b = (D3DPRESENT_PARAMETERS_MS*)back;
            printf("  readback: W=%u H=%u fmt=%u swap=%u hwnd=%p windowed=%d\n",
                   b->BackBufferWidth, b->BackBufferHeight, b->BackBufferFormat,
                   b->SwapEffect, b->hDeviceWindow, b->Windowed);
        }
        printf("  ==> SLOT %d IS CreateDevice\n", slot);
        return 0;
    }
    printf("  ==> slot %d is not CreateDevice (hr=0x%08lX)\n", slot, (unsigned long)hr);
    return 3;
}
