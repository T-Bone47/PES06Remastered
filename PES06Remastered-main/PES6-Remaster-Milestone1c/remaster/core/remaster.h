// remaster core - shared declarations.
//
// Deliberately dependency-free: no STL containers, no CRT DLL imports beyond
// what /MT provides statically. The ASI is loaded into a 32-bit game process
// and must not perturb it.

#ifndef REMASTER_H
#define REMASTER_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>

namespace remaster {

// ---------------------------------------------------------------- logger

class Logger {
public:
    void init(const char* path);
    void write(const char* level, const char* fmt, ...);

private:
    char path_[MAX_PATH];
};

Logger& log();

// ---------------------------------------------------------------- config

class Config {
public:
    static const int MAX_ENTRIES = 256;

    bool load(const char* path);
    void set(const char* section, const char* key, const char* value);
    const char* get(const char* section, const char* key, const char* fallback);
    int getInt(const char* section, const char* key, int fallback);
    bool getBool(const char* section, const char* key, bool fallback);

private:
    char keys_[MAX_ENTRIES][192];
    char values_[MAX_ENTRIES][256];
};

Config& config();

// ---------------------------------------------------------------- manifest

// Result of checking every "sig.*" entry in the manifest against the live
// image. A mismatched build must never install a hook.
struct ManifestCheck {
    int total;
    int matched;
    int mismatched;
    int unmapped;
};

class Manifest {
public:
    static const int MAX_ENTRIES = 512;

    bool load(const char* path);
    void set(const char* key, const char* value);
    const char* get(const char* key, const char* fallback) const;
    unsigned int getAddr(const char* key) const;
    // "AA BB ?? CC" -> byte array; 0xFF marks a wildcard.
    const unsigned char* getBytes(const char* key, int* out_len) const;
    // Checks every "sig.<key>" entry against the running image. When prefix is
    // non-null only keys starting with it are checked.
    ManifestCheck validate(const char* prefix = 0) const;

private:
    char keys_[MAX_ENTRIES][192];
    char values_[MAX_ENTRIES][256];
};

Manifest& manifest();

// ---------------------------------------------------------------- hook registry

struct HookEntry {
    const char* id;
    void* target;
    void* detour;
};

class HookRegistry {
public:
    static const int MAX_HOOKS = 64;

    // Installs a hook. Validates the target is code and, when a manifest
    // signature is given, that the prologue matches. A rejected hook is logged
    // and skipped; it never aborts the caller.
    bool install(const char* id, void* target, void* detour, const char* signature);
    void report();

    int count() const { return count_; }

private:
    HookEntry installed_[MAX_HOOKS];
    int count_ = 0;
};

HookRegistry& hooks();

// ---------------------------------------------------------------- paths

// Directory holding the running executable, with a trailing backslash.
bool moduleDir(char* out, int out_len);
char* gamePath(const char* leaf);

}  // namespace remaster

#endif  // REMASTER_H
