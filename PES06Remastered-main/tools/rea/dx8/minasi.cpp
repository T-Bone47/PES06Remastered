// Minimal ASI: does nothing but write a file and return TRUE.
// Used to bisect a failing remaster.asi load.
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID r) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(h);
        HANDLE f = CreateFileA("C:\\Users\\olive\\Downloads\\PES6-Remaster\\tools\\rea\\minasi.txt",
                               GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            const char* m = "minimal asi attached\n";
            DWORD n = 0;
            WriteFile(f, m, (DWORD)strlen(m), &n, 0);
            CloseHandle(f);
        }
    }
    return TRUE;
}

