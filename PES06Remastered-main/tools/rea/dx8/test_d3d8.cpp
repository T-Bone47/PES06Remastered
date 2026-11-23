// Round-trip probe: proves the exact D3D8 ABI layout the system d3d8.dll uses.
//
// Creates a real IDirect3D8 device, then reads the presentation parameters back
// via IDirect3DDevice8::GetCreationParameters. If our struct layout matches
// the DLL's, the values we wrote come back identical. If the offsets differ,
// they come back shifted and we see garbage.

#include <windows.h>
#include <stdio.h>
#include <string.h>

// ---- Microsoft D3D8 ABI (NOT mingw's d3d8types.h, which is a different layout)
typedef enum D3DDEVTYPE8 {
    D3DDEVTYPE_UNKNOWN = 0,
    D3DDEVTYPE_HAL = 1,
    D3DDEVTYPE_REF = 2,
    D3DDEVTYPE_SW = 3,
    D3DDEVTYPE_NULL = 4,
} D3DDEVTYPE8;

typedef enum D3DFORMAT8 {
    D3DFMT_UNKNOWN = 0,
    D3DFMT_R5G6B5 = 2,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_A8R8G8B8 = 21,
} D3DFORMAT8;

typedef enum D3DMULTISAMPLE_TYPE8 { D3DMULTISAMPLE_DEFAULT = 0 } D3DMULTISAMPLE_TYPE8;
typedef enum D3DSWAPEFFECT8 { D3DSWAPEFFECT_DISCARD = 1 } D3DSWAPEFFECT8;

typedef struct D3DPRESENT_PARAMETERS_MS {
    UINT   BackBufferWidth;
    UINT   BackBufferHeight;
    UINT   BackBufferFormat;         // D3DFORMAT
    UINT   BackBufferCount;
    UINT   MultiSampleType;          // D3DMULTISAMPLE_TYPE
    DWORD  MultiSampleQuality;
    UINT   SwapEffect;               // D3DSWAPEFFECT
    HWND   hDeviceWindow;
    BOOL   Windowed;
    BOOL   EnableAutoDepthStencil;
    UINT   AutoDepthStencilFormat;   // D3DFORMAT
    DWORD  Flags;
    UINT   FullScreen_RefreshRateInHz;
    UINT   FullScreen_ModeIndex;
} D3DPRESENT_PARAMETERS_MS;

typedef struct D3DDISPLAYMODE_MS {
    UINT Width;
    UINT Height;
    UINT RefreshRate;
    UINT Format;
} D3DDISPLAYMODE_MS;

#define D3D8_SDK_VERSION 220

// IDirect3D8 vtable slots (verified against the DirectX 8.0 SDK header).
#define ID3D8_SLOT_CREATEDEVICE 14
// IDirect3DDevice8 vtable slots.
#define ID3D8DEV_SLOT_GETCREATIONPARAMS 9
#define ID3D8DEV_SLOT_GETDEVICECAPS 7
#define ID3D8DEV_SLOT_PRESENT 15
#define ID3D8DEV_SLOT_RELEASE 2

typedef HRESULT(WINAPI *PFN_CreateDevice)(void*, UINT, D3DDEVTYPE8, HWND, DWORD,
                                           D3DPRESENT_PARAMETERS_MS*, void**);
typedef HRESULT(WINAPI *PFN_GetCreationParameters)(void*, D3DPRESENT_PARAMETERS_MS*);
typedef UINT(WINAPI *PFN_GetDeviceCaps)(void*, UINT, void*);
typedef ULONG(WINAPI *PFN_Release)(void*);

int main(void) {
    printf("sizeof(D3DPRESENT_PARAMETERS_MS) = %u  (MS layout expects 56)\n",
           (unsigned)sizeof(D3DPRESENT_PARAMETERS_MS));

    HMODULE d3d8 = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d8.dll");
    if (!d3d8) { printf("LoadLibrary d3d8 failed: %lu\n", GetLastError()); return 2; }
    typedef void* (WINAPI *PFN_Create8)(UINT);
    PFN_Create8 create8 = (PFN_Create8)GetProcAddress(d3d8, "Direct3DCreate8");
    if (!create8) { printf("Direct3DCreate8 missing\n"); return 2; }

    void* d3d = create8(D3D8_SDK_VERSION);
    if (!d3d) { printf("Direct3DCreate8 returned NULL\n"); return 2; }
    printf("IDirect3D8 = %p\n", d3d);

    // Register a window class + create a small window.
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "D3D8Probe";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("D3D8Probe", "probe", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480,
                               NULL, NULL, GetModuleHandleA(NULL), NULL);
    if (!hwnd) { printf("CreateWindow failed %lu\n", GetLastError()); return 2; }

    void** vt = *(void***)d3d;
    PFN_CreateDevice realCreateDevice = (PFN_CreateDevice)vt[ID3D8_SLOT_CREATEDEVICE];

    D3DPRESENT_PARAMETERS_MS pp;
    memset(&pp, 0, sizeof(pp));
    pp.BackBufferWidth = 640;
    pp.BackBufferHeight = 480;
    pp.BackBufferFormat = D3DFMT_X8R8G8B8;
    pp.BackBufferCount = 1;
    pp.MultiSampleType = D3DMULTISAMPLE_DEFAULT;
    pp.MultiSampleQuality = 0;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.hDeviceWindow = hwnd;
    pp.Windowed = TRUE;
    pp.EnableAutoDepthStencil = TRUE;
    pp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    pp.Flags = 0;
    pp.FullScreen_RefreshRateInHz = 0;
    pp.FullScreen_ModeIndex = 0;

    void* dev = NULL;
    HRESULT hr = realCreateDevice(d3d, 0, D3DDEVTYPE_HAL, hwnd, 0, &pp, &dev);
    printf("CreateDevice hr=0x%08lX dev=%p\n", (unsigned long)hr, dev);
    if (!dev || FAILED(hr)) { printf("device creation failed\n"); return 3; }

    void** dvt = *(void***)dev;
    PFN_GetCreationParameters getParams = (PFN_GetCreationParameters)dvt[ID3D8DEV_SLOT_GETCREATIONPARAMS];
    D3DPRESENT_PARAMETERS_MS back;
    memset(&back, 0, sizeof(back));
    HRESULT hr2 = getParams(dev, &back);
    printf("GetCreationParameters hr=0x%08lX\n", (unsigned long)hr2);
    printf("  readback: W=%u H=%u fmt=%u count=%u msType=%u msQual=%u swap=%u hwnd=%p windowed=%d ads=%u adsFmt=%u flags=0x%X refresh=%u modeIdx=%u\n",
           back.BackBufferWidth, back.BackBufferHeight, back.BackBufferFormat, back.BackBufferCount,
           back.MultiSampleType, back.MultiSampleQuality, back.SwapEffect, back.hDeviceWindow,
           back.Windowed, back.EnableAutoDepthStencil, back.AutoDepthStencilFormat, back.Flags,
           back.FullScreen_RefreshRateInHz, back.FullScreen_ModeIndex);

    int ok = (back.BackBufferWidth == 640 && back.BackBufferHeight == 480 &&
               back.BackBufferFormat == D3DFMT_X8R8G8B8 && back.Windowed == TRUE &&
               back.hDeviceWindow == hwnd && back.SwapEffect == D3DSWAPEFFECT_DISCARD);
    printf("\nLAYOUT %s\n", ok ? "MATCHES the system d3d8.dll (Microsoft 56-byte layout)"
                                : "DOES NOT MATCH -- offsets are shifted");

    ((PFN_Release)dvt[ID3D8DEV_SLOT_RELEASE])(dev);
    ((PFN_Release)vt[2])(d3d);
    DestroyWindow(hwnd);
    return ok ? 0 : 1;
}
