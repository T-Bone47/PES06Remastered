// remaster telemetry subsystem - read-only match state.
//
// A background thread samples the game's match state and writes it to a CSV
// in the game folder. Everything it does is a READ. It never writes to game
// memory, never calls into the game's code, and every read is guarded so a
// bad address can only ever produce a skipped sample.
//
// What is sampled, and where it was found (all confirmed in the Ghidra
// database of pes6.exe sha256
// 0c7f804be8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f):
//
//   0x01131FF4   base of the side 0 player record table
//                (FUN_00866B90 returns DAT_01131FF4)
//   0x01132048   base of the side 1 player record table
//                (FUN_00866CB0 returns DAT_01132048)
//   record       0x7C bytes; the accessor FUN_00861B90 returns
//                base + slot * 0x7C
//   0x03A6A260   the match clock, a float clamped to 0..0x23
//   0x03BECC60   a state value with 174 references across the binary
//   0x03A8AA78   the match squad table, 2 teams x 32 slots of player ids
//   mode         FUN_00425198 is an obfuscated trampoline; its body is
//                     MOV  EAX, [0x0446A458]
//                     ADC  EAX, [0x004A1D11]
//                     MOV  EAX, [EAX]
//                so the value is read as
//                     *(uint32*)(*(uint32*)0x0446A458 + *(uint32*)0x004A1D11)
//                without calling anything.

#include "telemetry.h"
#include "../core/remaster.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

namespace remaster {
namespace telemetry {

#define MAX_SLOTS 32        // record slots per side
#define RECORD_SIZE 0x7C       // bytes per record
#define GLOBALS_WINDOW 0x400   // raw bytes of globals to sample
#define GLOBALS_WINDOW 0x1000
#define GLOBALS_BASE 0x03A6A000
#define RECORD_SIZE 0x7C
#define SAMPLE_INTERVAL_MS 250   // default; overridden by the config

static const unsigned int G_SIDE0_BASE = 0x01131FF4;
static const unsigned int G_SIDE1_BASE = 0x01132048;
static const unsigned int G_CLOCK = 0x03A6A260;
static const unsigned int G_PHASE = 0x03BECC60;
static const unsigned int G_SQUAD = 0x03A8AA78;
static const unsigned int G_MODE_PTR = 0x0446A458;
static const unsigned int G_MODE_DELTA = 0x004A1D11;

static HANDLE g_thread = 0;
static volatile LONG g_enabled = 0;
static volatile LONG g_running = 0;
static int g_intervalMs = SAMPLE_INTERVAL_MS;
static char g_path[MAX_PATH * 2] = { 0 };

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

// The match mode discriminator, read rather than called.
static bool readMode(int* out) {
    unsigned int base = 0;
    unsigned int delta = 0;
    unsigned int value = 0;
    if (!safeRead(G_MODE_PTR, &base)) {
        return false;
    }
    if (!safeRead(G_MODE_DELTA, &delta)) {
        return false;
    }
    if (!safeRead(base + delta, &value)) {
        return false;
    }
    *out = (int)value;
    return true;
}

// ---------------------------------------------------------------- the writer

static void writeRow(FILE* f, unsigned long long wallMs, int mode, float clock,
                     unsigned int phase, const unsigned char* side0, const unsigned char* side1,
                     const unsigned char* globals) {
    fprintf(f, "%llu,%d,%.6f,%u", wallMs, mode, clock, phase);
    for (int s = 0; s < MAX_SLOTS; s++) {
        for (int i = 0; i < RECORD_SIZE; i++) {
            fprintf(f, ",%02x", side0[s * RECORD_SIZE + i]);
        }
    }
    for (int s = 0; s < MAX_SLOTS; s++) {
        for (int i = 0; i < RECORD_SIZE; i++) {
            fprintf(f, ",%02x", side1[s * RECORD_SIZE + i]);
        }
    }
    for (int i = 0; i < GLOBALS_WINDOW; i++) {
        fprintf(f, ",%02x", globals[i]);
    }
    fputc('\n', f);
}

static void writeSquad(FILE* f) {
    unsigned short ids[2 * MAX_SLOTS];
    if (!safeReadBytes(G_SQUAD, (unsigned char*)ids, sizeof(ids))) {
        return;
    }
    fprintf(f, "squad");
    for (int i = 0; i < 2 * MAX_SLOTS; i++) {
        fprintf(f, ",%u", ids[i]);
    }
    fputc('\n', f);
    fflush(f);
}

static DWORD WINAPI telemetryThread(LPVOID param) {
    // Let the loader finish before touching anything.
    Sleep(2000);

    char* folder = gamePath("");
    if (folder) {
        snprintf(g_path, sizeof(g_path), "%sremaster_telemetry.csv", folder);
    }
    else {
        snprintf(g_path, sizeof(g_path), "remaster_telemetry.csv");
    }

    FILE* f = fopen(g_path, "w");
    if (!f) {
        log().write("WARN", "telemetry: cannot open %s", g_path);
        return 1;
    }
    fprintf(f, "# PES 6 remaster telemetry, one row per sample\n");
    fprintf(f, "# columns: wall_ms,mode,clock,phase, then %d records of 0x%X bytes for side 0 slots 0..%d, the same for side 1, then %d bytes of globals at 0x%08X\n",
            MAX_SLOTS, RECORD_SIZE, MAX_SLOTS - 1, GLOBALS_WINDOW, GLOBALS_BASE);
    fprintf(f, "# record size 0x%X, side bases 0x%08X / 0x%08X\n",
            RECORD_SIZE, G_SIDE0_BASE, G_SIDE1_BASE);
    fflush(f);

    unsigned char side0[MAX_SLOTS * RECORD_SIZE];
    unsigned char side1[MAX_SLOTS * RECORD_SIZE];
    int failedReads = 0;
    int samples = 0;
    int squadWritten = 0;
    int wasInMatch = 0;
    unsigned long long lastProbe = 0;

    while (g_running && g_enabled) {
        unsigned int base0 = 0;
        unsigned int base1 = 0;
        bool have0 = safeRead(G_SIDE0_BASE, &base0);
        bool have1 = safeRead(G_SIDE1_BASE, &base1);

        // A match is considered active when both record tables exist and the
        // mode discriminator reads 1 or 2.
        int mode = 0;
        bool haveMode = readMode(&mode);
        bool inMatch = (have0 && base0 != 0) || (have1 && base1 != 0);

        unsigned long long nowMs = GetTickCount64();
        if (nowMs - lastProbe > 4000) {
            lastProbe = nowMs;
            float pclock = 0.0f;
            unsigned int pphase = 0;
            safeRead(G_CLOCK, &pclock);
            safeRead(G_PHASE, &pphase);
            unsigned int modeBase = 0, modeDelta = 0, modeVal = 0;
            safeRead(G_MODE_PTR, &modeBase);
            safeRead(G_MODE_DELTA, &modeDelta);
            if (modeBase) {
                safeRead(modeBase + modeDelta, &modeVal);
            }
            fprintf(f, "# probe t=%llu base0=%p base1=%p modeBase=%08X modeDelta=%08X mode=%u clock=%.4f phase=%u\n",
                    nowMs, (void*)base0, (void*)base1, modeBase, modeDelta, modeVal, pclock, pphase);
            fflush(f);
        }

        if (inMatch) {
            if (!wasInMatch) {
                wasInMatch = 1;
                writeSquad(f);
                squadWritten = 1;
            }
            float clock = 0.0f;
            unsigned int phase = 0;
            safeRead(G_CLOCK, &clock);
            safeRead(G_PHASE, &phase);

            memset(side0, 0, sizeof(side0));
            memset(side1, 0, sizeof(side1));
            bool ok0 = base0 ? safeReadBytes(base0, side0, sizeof(side0)) : true;
            bool ok1 = base1 ? safeReadBytes(base1, side1, sizeof(side1)) : true;
            if (ok0 && ok1) {
                unsigned long long now = GetTickCount64();
                unsigned char globals[GLOBALS_WINDOW];
                memset(globals, 0, sizeof(globals));
                safeReadBytes(GLOBALS_BASE, globals, sizeof(globals));
                writeRow(f, now, mode, clock, phase, side0, side1, globals);
                samples++;
                if ((samples % 16) == 0) {
                    fflush(f);
                }
            }
            else {
                failedReads++;
            }
        }
        else if (wasInMatch) {
            wasInMatch = 0;
            fflush(f);
        }

        Sleep(g_intervalMs);
    }

    fflush(f);
    fclose(f);
    log().write("INFO", "telemetry: %d samples, %d failed reads, squad written %d",
                samples, failedReads, squadWritten);
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
    g_running = 1;
    g_enabled = 1;
    g_thread = CreateThread(0, 0, telemetryThread, 0, 0, 0);
    if (!g_thread) {
        g_running = 0;
        log().write("WARN", "telemetry: could not start the sampling thread");
        return false;
    }
    g_intervalMs = config().getInt("telemetry", "interval_ms", SAMPLE_INTERVAL_MS);
    if (g_intervalMs < 50) {
        g_intervalMs = 50;
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
