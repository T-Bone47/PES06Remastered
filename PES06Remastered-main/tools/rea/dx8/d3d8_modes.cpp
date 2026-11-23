// Enumerates the Direct3D 8 adapter display modes.
//
// The game's own mode selection (FUN_00404db0) walks a mode list and accepts
// the first entry that passes CheckDeviceType + CheckDeviceFormat, so the
// adapter's real mode list is what decides the achievable resolution.
//
// Usage: d3d8_modes.exe

#include <windows.h>
#include <stdio.h>

typedef struct DM {
    UINT Width, Height, RefreshRate, Format;
} DM;

typedef HRESULT(WINAPI *PFN_ANY)(void*, void*, void*, void*, void*, void*, void*);
typedef UINT(WINAPI *PFN_Count)(void*, UINT);
typedef HRESULT(WINAPI *PFN_Enum)(void*, UINT, UINT, DM*, UINT);
typedef HRESULT(WINAPI *PFN_GetMode)(void*, UINT, DM*);

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    HMODULE d3d8 = LoadLibraryA("C:\\Windows\\SysWOW64\\d3d8.dll");
    if (!d3d8) { printf("no d3d8\n"); return 2; }
    typedef void* (WINAPI *PFN_Create8)(UINT);
    void* d3d = ((PFN_Create8)GetProcAddress(d3d8, "Direct3DCreate8"))(220);
    if (!d3d) { printf("create8 null\n"); return 2; }
    void** vt = *(void***)d3d;

    // Slot 4 = GetAdapterModeCount, slot 5 = EnumAdapterModes (canonical D3D8).
    int countIdx = 4, enumIdx = 5;
    UINT count = ((PFN_Count)vt[countIdx])(d3d, 0);
    if (count == 0 || count > 512) {
        printf("slot %d did not return a plausible mode count (%u); trying slot 6\n",
               countIdx, count);
        countIdx = 6;
        count = ((PFN_Count)vt[countIdx])(d3d, 0);
    }
    if (count == 0 || count > 512) {
        printf("could not read the adapter mode count\n");
        return 3;
    }
    printf("adapter 0: %u display modes (GetAdapterModeCount at vtable slot %d)\n\n",
           count, countIdx);

    for (UINT i = 0; i < count; i++) {
        DM dm;
        memset(&dm, 0, sizeof(dm));
        HRESULT hr = ((PFN_Enum)vt[enumIdx])(d3d, 0, i, &dm, 0);
        if (FAILED(hr)) { printf("  mode %3u: failed 0x%08lX\n", i, (unsigned long)hr); continue; }
        printf("  mode %3u: %4u x %4u  refresh=%u  format=%u\n",
               i, dm.Width, dm.Height, dm.RefreshRate, dm.Format);
    }

    // Current display mode
    DM cur;
    memset(&cur, 0, sizeof(cur));
    HRESULT hr = ((PFN_GetMode)vt[6])(d3d, 0, &cur);
    if (SUCCEEDED(hr)) {
        printf("\ncurrent display mode: %u x %u refresh=%u format=%u\n",
               cur.Width, cur.Height, cur.RefreshRate, cur.Format);
    }
    return 0;
}
