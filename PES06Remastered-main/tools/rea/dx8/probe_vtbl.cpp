// Safe IDirect3D8 vtable prober.
//
// The DX 8.0 SDK header (mingw/Wine) puts CreateDevice at slot 14, but the
// system d3d8.dll is the DirectX 8.1 runtime. Rather than trust either, we
// probe every slot with plausible, non-NULL arguments and identify the one
// that behaves like CreateDevice: it must write a non-NULL device pointer.
//
// Nothing here can crash on a NULL deref because every pointer slot is given
// a real buffer.

#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef HRESULT(WINAPI *PFN_ANY)(void*, void*, void*, void*, void*, void*, void*);

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

#define D3D8_SDK_VERSION 220

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);

    HMODULE d3d8 = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d8.dll");
    if (!d3d8) { printf("LoadLibrary failed %lu\n", GetLastError()); return 2; }
    typedef void* (WINAPI *PFN_Create8)(UINT);
    PFN_Create8 create8 = (PFN_Create8)GetProcAddress(d3d8, "Direct3DCreate8");
    void* d3d = create8(D3D8_SDK_VERSION);
    printf("IDirect3D8 = %p\n", d3d);
    if (!d3d) { printf("Direct3DCreate8 returned NULL\n"); return 2; }

    WNDCLASSA wc; memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "D3D8Probe";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("D3D8Probe", "probe", WS_OVERLAPPEDWINDOW, 0, 0, 640, 480,
                               NULL, NULL, GetModuleHandleA(NULL), NULL);
    printf("hwnd = %p\n", hwnd);

    void** vt = *(void***)d3d;
    printf("\nslot  vtable-ptr        hr          device?  note\n");
    printf("------------------------------------------------------\n");

    static char bufA[8192];
    static char bufB[8192];
    static char bufC[8192];

    for (int i = 3; i < 24; i++) {
        memset(bufA, 0, sizeof(bufA));
        memset(bufB, 0, sizeof(bufB));
        memset(bufC, 0, sizeof(bufC));

        D3DPRESENT_PARAMETERS_MS* pp = (D3DPRESENT_PARAMETERS_MS*)bufC;
        pp->BackBufferWidth = 640;
        pp->BackBufferHeight = 480;
        pp->BackBufferFormat = 22;   // D3DFMT_X8R8G8B8
        pp->BackBufferCount = 1;
        pp->SwapEffect = 1;          // D3DSWAPEFFECT_DISCARD
        pp->hDeviceWindow = hwnd;
        pp->Windowed = TRUE;
        pp->EnableAutoDepthStencil = TRUE;

        void* devOut = NULL;
        PFN_ANY fn = (PFN_ANY)vt[i];
        HRESULT hr = fn(d3d, bufA, bufB, hwnd, 0, bufC, &devOut);

        const char* note = "";
        // Only a genuine CreateDevice both succeeds (D3D_OK) and yields a device.
        if (devOut && hr == 0) note = "<<< CREATEDEVICE (D3D_OK + device ptr)";
        else if (hr == 0) note = "hr=D3D_OK";

        printf("%4d  %-16p  0x%08lX  %-8s %s\n", i, vt[i], (unsigned long)hr,
               devOut ? "YES" : "-", note);

        if (devOut && hr == 0) {
            void** dvt = *(void***)devOut;
            printf("\n>>> CreateDevice is vtable slot %d\n", i);
            printf(">>> device vtable[0]=QueryInterface %p\n", dvt[0]);
            // Read back the creation parameters with the MS layout.
            typedef HRESULT(WINAPI *PFN_GetCP)(void*, D3DPRESENT_PARAMETERS_MS*);
            PFN_GetCP getcp = (PFN_GetCP)dvt[9];
            D3DPRESENT_PARAMETERS_MS back; memset(&back, 0, sizeof(back));
            HRESULT hr2 = getcp(devOut, &back);
            printf(">>> GetCreationParameters (slot 9) hr=0x%08lX\n", (unsigned long)hr2);
            printf(">>>   W=%u H=%u fmt=%u swap=%u hwnd=%p windowed=%d\n",
                   back.BackBufferWidth, back.BackBufferHeight, back.BackBufferFormat,
                   back.SwapEffect, back.hDeviceWindow, back.Windowed);
            int ok = (back.BackBufferWidth == 640 && back.BackBufferHeight == 480 &&
                      back.BackBufferFormat == 22 && back.Windowed == TRUE &&
                      back.hDeviceWindow == hwnd && back.SwapEffect == 1);
            printf(">>> LAYOUT %s\n", ok ? "MATCHES (Microsoft 56-byte D3D8 layout)"
                                        : "MISMATCH -- struct offsets differ");
            return ok ? 0 : 1;
        }
    }
    printf("\nno slot produced a device pointer in [0,24]\n");
    return 3;
}
