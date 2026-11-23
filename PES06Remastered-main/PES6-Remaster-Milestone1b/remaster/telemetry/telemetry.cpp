// remaster telemetry subsystem - read-only match state.
//
// A background thread samples the game's match state and writes it to a CSV
// in the game folder, and can dump the process's writable memory at labelled
// moments for differential analysis.
//
// Everything it does is a READ. It never writes to the game's memory and it
// never calls into the game's code. Every read is guarded so a bad address
// can only ever produce a skipped sample.
//
// ---------------------------------------------------------------------------
// Where the values come from
//
// pes6.exe, sha256
// 0c7f804be8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f
//
//   0x01311774   the match mode. FUN_00425198 is an obfuscated trampoline:
//                  PUSHFD / CLC /
//                  MOV  EAX, [0x0446A458] /
//                  ADC  EAX, [0x004A1D11] /
//                  MOV  EAX, [EAX]        -> EAX = *(*(0x0446A458) + *(0x004A1D11))
//                  POPFD / JMP 0x00662425
//                and FUN_00662425 clamps it: 0 if <= 0 or > 2, else 1 or 2.
//                The two constants are embedded in the instruction stream, so
//                the sum is a fixed address - 0x01311774 - in the writable
//                tail of .data. It is read here directly, never called.
//                The mode selects which entity table the game uses.
//
//   0x03BECC60   a match state value. Observed 0 in the menu, then 2, 3 and
//                12 as a match progresses.
//
//   0x03BE12C9   a match type byte. FUN_00662D70 reads it: 4 for a league
//                and 5 for a master league.
//
//   0x03A6A260   read as a float.
//
//   0x01037B58   the base of the LIVE entity table. FUN_0068D9B0 returns it
//                and FUN_0067F490 returns
//                    DAT_01037B58 + (index & 0xFFFF) * 0x48
//                for an index below 5000, when FUN_00666120 is non-zero.
//                This is the live match state: the players, the ball and the
//                referee.
//
//   0x01131FF4   the base of the side 0 ROSTER table (0x7C bytes per record).
//   0x01132048   the base of the side 1 roster table.
//                FUN_00866B90 and FUN_00866CB0 return them. Measured to be
//                static for the whole of a match, so the roster is not where
//                the live state lives.
//
//   0x03A8AA78   the match squad table, 2 teams x 32 player ids.
//
//   0x03A6A000   a window of globals, sampled for the differential scan.
//
// The record stride and the entity stride are different: 0x7C for the roster,
// 0x48 for the live entities. Reading the roster and expecting live values is
// why the first attempt found nothing.
//
// ---------------------------------------------------------------------------

#include "telemetry.h"
#include "../core/remaster.h"
#include "../io/fileio.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

namespace remaster {
namespace telemetry {

#define MAX_ENTITIES 64
#define ENTITY_SIZE 0x48
#define ROSTER_SLOTS 32
#define ROSTER_SIZE 0x7C
#define GLOBALS_WINDOW 0x400
#define GLOBALS_BASE 0x03A6A000
#define DEFAULT_INTERVAL_MS 250

static const unsigned int G_MODE = 0x01311774;
static const unsigned int G_PHASE = 0x03BECC60;
static const unsigned int G_MATCHTYPE = 0x03BE12C9;
static const unsigned int G_CLOCK = 0x03A6A260;
static const unsigned int G_ENTITY_BASE = 0x01037B58;
static const unsigned int G_ROSTER0 = 0x01131FF4;
static const unsigned int G_ROSTER1 = 0x01132048;
static const unsigned int G_SQUAD = 0x03A8AA78;

static HANDLE g_thread = 0;
static volatile LONG g_enabled = 0;
static volatile LONG g_running = 0;
static int g_intervalMs = DEFAULT_INTERVAL_MS;
static int g_dumpRoster = 1;
static char g_path[MAX_PATH * 2] = { 0 };
static char g_scanPath[MAX_PATH * 2] = { 0 };

// ---------------------------------------------------------------- guarded reads

template <typename T>
static bool safeRead(unsigned int address, T* out) {
    __try {
        *out = *(T*)address;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool safeReadBytes(unsigned int address, unsigned char* out, size_t n) {
    __try {
        memcpy(out, (void*)address, n);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// ---------------------------------------------------------------- the row

struct Sample {
    unsigned long long wallMs;
    int mode;
    int phase;
    int matchType;
    float clock;
    int haveEntities;
    int haveRoster;
    unsigned char entities[MAX_ENTITIES * ENTITY_SIZE];
    unsigned char roster[2 * ROSTER_SLOTS * ROSTER_SIZE];
    unsigned char globals[GLOBALS_WINDOW];
};

static void writeRow(FILE* f, const Sample& s) {
    fprintf(f, "%llu,%d,%d,%d,%.6f", s.wallMs, s.mode, s.phase, s.matchType, s.clock);
    for (int i = 0; i < MAX_ENTITIES * ENTITY_SIZE; i++) {
        fprintf(f, ",%02x", s.entities[i]);
    }
    if (g_dumpRoster) {
        for (int i = 0; i < 2 * ROSTER_SLOTS * ROSTER_SIZE; i++) {
            fprintf(f, ",%02x", s.roster[i]);
        }
    }
    for (int i = 0; i < GLOBALS_WINDOW; i++) {
        fprintf(f, ",%02x", s.globals[i]);
    }
    fputc('\n', f);
}

// ---------------------------------------------------------------- scan dumps

// Writes every writable, committed region of the process to one file, so two
// dumps taken at different moments can be diffed.
static void dumpWritableRegions(const char* label) {
    char path[MAX_PATH * 2];
    snprintf(path, sizeof(path), "%s\\%s.bin", g_scanPath, label);
    FILE* f = fopen(path, "wb");
    if (!f) {
        log().write("WARN", "telemetry: cannot write the scan dump %s", path);
        return;
    }
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t minAddr = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;
    uintptr_t addr = minAddr;
    int regions = 0;
    unsigned long long total = 0;
    while (addr < maxAddr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery((void*)addr, &mbi, sizeof(mbi)) == 0) {
            break;
        }
        bool writable = (mbi.State == MEM_COMMIT) && (mbi.Protect & PAGE_READWRITE) &&
                        !(mbi.Protect & PAGE_GUARD);
        if (writable && mbi.RegionSize) {
            unsigned char* buf = (unsigned char*)malloc(mbi.RegionSize);
            if (buf) {
                if (safeReadBytes((unsigned int)mbi.BaseAddress, buf, mbi.RegionSize)) {
                    fprintf(f, "R %llX %llX\n", (unsigned long long)mbi.BaseAddress,
                            (unsigned long long)mbi.RegionSize);
                    fwrite(buf, 1, mbi.RegionSize, f);
                    total += mbi.RegionSize;
                    regions++;
                }
                free(buf);
            }
        }
        uintptr_t next = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
        if (next <= addr) {
            break;
        }
        addr = next;
    }
    fclose(f);
    log().write("INFO", "telemetry: scan dump '%s' -> %d regions, %.1f MB",
                label, regions, total / 1048576.0);
}

// Watches a label file. When its contents change, a dump is taken with that
// text as the label. This is how a human-played match can be labelled from
// outside the game.
static bool readLabel(char* out, size_t n) {
    FILE* f = fopen(g_scanPath, "r");
    if (!f) {
        return false;
    }
    size_t got = fread(out, 1, n - 1, f);
    fclose(f);
    out[got] = 0;
    // trim
    while (got && (out[got - 1] == '\r' || out[got - 1] == '\n' || out[got - 1] == ' ')) {
        out[--got] = 0;
    }
    return got > 0;
}

// ---------------------------------------------------------------- the thread

static DWORD WINAPI telemetryThread(LPVOID param) {
    Sleep(2000);

    char* folder = gamePath("");
    if (folder) {
        snprintf(g_path, sizeof(g_path), "%sremaster_telemetry.csv", folder);
        snprintf(g_scanPath, sizeof(g_scanPath), "%sremaster_scan", folder);
        CreateDirectoryA(g_scanPath, 0);
    }
    else {
        snprintf(g_path, sizeof(g_path), "remaster_telemetry.csv");
        snprintf(g_scanPath, sizeof(g_scanPath), "remaster_scan");
        CreateDirectoryA(g_scanPath, 0);
    }

    FILE* f = fopen(g_path, "w");
    if (!f) {
        log().write("WARN", "telemetry: cannot open %s", g_path);
        return 1;
    }
    fprintf(f, "# PES 6 remaster telemetry\n");
    fprintf(f, "# columns: wall_ms,mode,phase,matchtype,clock, then %d live entities of 0x%X bytes,",
            MAX_ENTITIES, ENTITY_SIZE);
    if (g_dumpRoster) {
        fprintf(f, " then %d roster records of 0x%X bytes (2 sides x %d slots),",
                2 * ROSTER_SLOTS, ROSTER_SIZE, ROSTER_SLOTS);
    }
    fprintf(f, " then %d bytes of globals at 0x%08X\n", GLOBALS_WINDOW, GLOBALS_BASE);
    fprintf(f, "# entity base 0x%08X, mode 0x%08X, phase 0x%08X, matchtype 0x%08X\n",
            G_ENTITY_BASE, G_MODE, G_PHASE, G_MATCHTYPE);
    fprintf(f, "# write a label into remaster_scan\\label.txt to take a memory dump\n");
    fflush(f);

    char lastLabel[256] = { 0 };
    char label[256];
    int samples = 0;
    int failed = 0;
    int dumps = 0;

    Sample s;
    memset(&s, 0, sizeof(s));

    while (g_running && g_enabled) {
        s.wallMs = GetTickCount64();
        unsigned int mode = 0, phase = 0, entityBase = 0;
        unsigned int roster0 = 0, roster1 = 0;
        unsigned char matchType = 0;
        s.clock = 0.0f;
        safeRead(G_MODE, &mode);
        safeRead(G_PHASE, &phase);
        safeRead(G_MATCHTYPE, &matchType);
        safeRead(G_CLOCK, &s.clock);
        safeRead(G_ENTITY_BASE, &entityBase);
        safeRead(G_ROSTER0, &roster0);
        safeRead(G_ROSTER1, &roster1);
        s.mode = (int)mode;
        s.phase = (int)phase;
        s.matchType = (int)matchType;

        memset(s.entities, 0, sizeof(s.entities));
        memset(s.roster, 0, sizeof(s.roster));
        memset(s.globals, 0, sizeof(s.globals));

        s.haveEntities = entityBase ? safeReadBytes(entityBase, s.entities, sizeof(s.entities)) : 0;
        if (g_dumpRoster) {
            if (roster0) {
                safeReadBytes(roster0, s.roster, ROSTER_SLOTS * ROSTER_SIZE);
            }
            if (roster1) {
                safeReadBytes(roster1, s.roster + ROSTER_SLOTS * ROSTER_SIZE, ROSTER_SLOTS * ROSTER_SIZE);
            }
        }
        safeReadBytes(GLOBALS_BASE, s.globals, sizeof(s.globals));

        writeRow(f, s);
        samples++;
        if ((samples % 16) == 0) {
            fflush(f);
        }

        // the file watcher, so a file the game writes is noticed
        fileio::poll();

        // a labelled memory dump, requested from outside the game
        if (readLabel(label, sizeof(label))) {
            if (strcmp(label, lastLabel) != 0) {
                strncpy(lastLabel, label, sizeof(lastLabel) - 1);
                lastLabel[sizeof(lastLabel) - 1] = 0;
                dumpWritableRegions(lastLabel);
                dumps++;
            }
        }

        Sleep(g_intervalMs);
    }

    fflush(f);
    fclose(f);
    log().write("INFO", "telemetry: %d samples, %d failed reads, %d dumps",
                samples, failed, dumps);
    return 0;
}

bool install() {
    if (!config().getBool("telemetry", "enabled", false)) {
        log().write("INFO", "telemetry: disabled by config");
        return true;
    }
    if (g_thread) {
        return true;
    }
    g_intervalMs = config().getInt("telemetry", "interval_ms", DEFAULT_INTERVAL_MS);
    if (g_intervalMs < 50) {
        g_intervalMs = 50;
    }
    g_dumpRoster = config().getBool("telemetry", "roster", true) ? 1 : 0;
    g_running = 1;
    g_enabled = 1;
    g_thread = CreateThread(0, 0, telemetryThread, 0, 0, 0);
    if (!g_thread) {
        g_running = 0;
        log().write("WARN", "telemetry: could not start the sampling thread");
        return false;
    }
    log().write("INFO", "telemetry: sampling every %d ms", g_intervalMs);
    return true;
}

void shutdown() {
    g_enabled = 0;
    g_running = 0;
    if (g_thread) {
        WaitForSingleObject(g_thread, 2000);
        CloseHandle(g_thread);
        g_thread = 0;
    }
}

}  // namespace telemetry
}  // namespace remaster
