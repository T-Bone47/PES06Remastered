// Load a DLL/ASI directly and report the loader error, so a failing load can
// be diagnosed without guessing.
#include <windows.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 2) { printf("usage: loadtest <dll>\n"); return 2; }
    SetErrorMode(0);
    HMODULE h = LoadLibraryA(argv[1]);
    DWORD err = GetLastError();
    printf("LoadLibraryA(\"%s\") -> %p   GetLastError=%lu\n", argv[1], h, err);
    if (h) {
        printf("  module path: ");
        char p[MAX_PATH];
        if (GetModuleFileNameA(h, p, MAX_PATH)) printf("%s\n", p);
        FreeLibrary(h);
        printf("  freed ok\n");
    }
    else {
        LPVOID ex = 0;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                       0, err, 0, (LPSTR)&ex, 0, 0);
        printf("  %s\n", ex ? (char*)ex : "(no message)");
        if (ex) { LocalFree(ex); }
    }
    return h ? 0 : 1;
}
