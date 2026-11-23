#include <windows.h>
#include <stdio.h>

LONG WINAPI VectoredHandler(PEXCEPTION_POINTERS ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    // Ignore non-crash codes like thread naming, RPC, etc.
    if (code == 0xC0000005 || code == 0x80000003 || code == 0xC00000FD) {
        printf("\n======================================================\n");
        printf("VECTORED EXCEPTION DETECTED: 0x%08lX\n", code);
        printf("Exception Address: %p\n", ep->ExceptionRecord->ExceptionAddress);
        
        HMODULE hMod = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)ep->ExceptionRecord->ExceptionAddress, &hMod);
        char modName[MAX_PATH] = "(unknown)";
        if (hMod) {
            GetModuleFileNameA(hMod, modName, MAX_PATH);
        }
        printf("Faulting Module: %s (Base: %p, Offset: 0x%lX)\n",
               modName, hMod, (unsigned long)((char*)ep->ExceptionRecord->ExceptionAddress - (char*)hMod));
        
        if (code == 0xC0000005) {
            printf("Access Violation: %s at target address %p\n",
                   ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "READ" : "WRITE",
                   (void*)ep->ExceptionRecord->ExceptionInformation[1]);
        }
        printf("======================================================\n\n");
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
    AddVectoredExceptionHandler(1, VectoredHandler);
    const char* path = argc > 1 ? argv[1] : "build\\remaster.asi";
    printf("Loading %s with Vectored Exception Handler installed...\n", path);
    fflush(stdout);

    HMODULE h = LoadLibraryA(path);
    DWORD err = GetLastError();
    printf("LoadLibraryA -> %p (GetLastError=%lu)\n", h, err);
    if (h) {
        printf("Loaded successfully! Freeing...\n");
        FreeLibrary(h);
    }
    return h ? 0 : 1;
}
