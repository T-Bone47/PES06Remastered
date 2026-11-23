// director.cpp - Intelligent Broadcast Director & Event Presentation
//
// Manages real-time contextual camera presets, incident detection,
// stadium geometry clipping mitigation, and instant replay skip.
//
// Fail-Closed & Classic Mode Contract:
// When disabled by config [broadcast] enabled = 0 or in Classic Mode,
// zero memory modifications are applied, ensuring 100% bit-exact
// vanilla camera and replay mechanics.

#include "director.h"
#include "../core/remaster.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

namespace remaster {
namespace broadcast {

// Memory RVAs verified in pes6.exe (Retail SHA-256: 649dc2fc...)
static const uintptr_t ADDR_CAM_ZOOM = 0x0086BCCD;      // Float immediate in push 1430.0f
static const uintptr_t ADDR_CAM_MODE = 0x03BE0912;      // Camera mode byte (0..9)
static const uintptr_t ADDR_MATCH_PHASE = 0x03BECC60;   // Match phase (0=menu, 2=lineup, 3=play, 12=replay)
static const uintptr_t ADDR_REPLAY_ACTIVE = 0x03BECC74; // Replay flag

// Stadium clipping patch locations (9 verified RVAs)
struct ClipPatchEntry {
    uintptr_t address;
    size_t size;
    unsigned char origBytes[8];
    unsigned char patchBytes[8];
};

static ClipPatchEntry g_clipPatches[] = {
    { 0x0096483C, 1, { 0x7A }, { 0xEB } },
    { 0x009648D0, 1, { 0x7A }, { 0xEB } },
    { 0x00964921, 1, { 0x75 }, { 0xEB } },
    { 0x00964938, 1, { 0x7A }, { 0xEB } },
    { 0x0096494D, 1, { 0x75 }, { 0xEB } },
    { 0x00964B6B, 1, { 0x7A }, { 0xEB } },
    { 0x00964BCA, 1, { 0x75 }, { 0xEB } },
    { 0x00964BE8, 1, { 0x75 }, { 0xEB } },
    { 0x00964C6D, 6, { 0x0F, 0x8A, 0xE8, 0xFC, 0xFF, 0xFF }, { 0xE9, 0xE9, 0xFC, 0xFF, 0xFF, 0x90 } }
};

static const size_t NUM_CLIP_PATCHES = sizeof(g_clipPatches) / sizeof(g_clipPatches[0]);

// Internal state
static BroadcastConfig g_config = {
    true,      // enabled
    true,      // dynamicCamera
    1430.0f,   // baseZoom
    1.15f,     // midfieldZoomMult (1644.5f)
    0.85f,     // boxZoomMult (1215.5f)
    0.65f,     // goalZoomMult (929.5f)
    0.08f,     // zoomLerpSpeed
    false,     // stadiumClippingFix (disabled to protect pitch/stadium render passes)
    true,      // instantReplaySkip
    true       // cinematicReplays
};

static DirectorState g_state = DIR_STATE_IDLE;
static CameraPreset g_cameraPreset = CAM_PRESET_VANILLA;
static float g_currentZoom = 1430.0f;
static float g_targetZoom = 1430.0f;
static float g_origZoomVal = 1430.0f;
static bool g_zoomPatched = false;
static bool g_clippingPatched = false;
static MatchEventIncident g_latestIncident = { 0 };
static unsigned int g_incidentCounter = 0;
static int g_prevPhase = 0;

// Safe guarded memory write
static bool writeMemory(uintptr_t address, const void* data, size_t size) {
    DWORD oldProtect = 0;
    if (!VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    memcpy((void*)address, data, size);
    VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
    return true;
}

static bool safeReadMemory(uintptr_t address, void* out, size_t size) {
    __try {
        memcpy(out, (const void*)address, size);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void applyClippingPatches() {
    // RVAs 0x0096483C..0x00964C6D guard pitch and stadium render passes.
    // Patching them to 0xEB unconditionally skips pitch turf draw calls.
    // Kept inert to preserve pristine vanilla pitch rendering.
    g_clippingPatched = false;
    log().write("INFO", "broadcast: stadium clipping patch bypassed to protect turf rendering");
}

static void revertClippingPatches() {
    g_clippingPatched = false;
}

static void setCameraZoomMemory(float zoomVal) {
    // Safety envelope clamp: 800.0f <= zoom <= 2200.0f
    if (zoomVal < 800.0f) zoomVal = 800.0f;
    if (zoomVal > 2200.0f) zoomVal = 2200.0f;

    writeMemory(ADDR_CAM_ZOOM, &zoomVal, sizeof(float));
}

bool init() {
    g_config.enabled = config().getBool("broadcast", "enabled", true);
    if (!g_config.enabled) {
        log().write("INFO", "broadcast: director disabled by config [broadcast] enabled=0");
        return true;
    }

    g_config.dynamicCamera = config().getBool("broadcast", "dynamic_camera", true);
    g_config.baseZoom = config().getFloat("broadcast", "base_zoom", 1430.0f);
    g_config.midfieldZoomMult = config().getFloat("broadcast", "midfield_zoom_mult", 1.15f);
    g_config.boxZoomMult = config().getFloat("broadcast", "box_zoom_mult", 0.85f);
    g_config.goalZoomMult = config().getFloat("broadcast", "goal_zoom_mult", 0.65f);
    g_config.zoomLerpSpeed = config().getFloat("broadcast", "zoom_lerp_speed", 0.08f);
    g_config.stadiumClippingFix = config().getBool("broadcast", "stadium_clipping_fix", false);
    g_config.instantReplaySkip = config().getBool("broadcast", "instant_replay_skip", true);
    g_config.cinematicReplays = config().getBool("broadcast", "cinematic_replays", true);

    g_currentZoom = g_config.baseZoom;
    g_targetZoom = g_config.baseZoom;

    // Verify and record original zoom
    float readZoom = 0.0f;
    if (safeReadMemory(ADDR_CAM_ZOOM, &readZoom, sizeof(float))) {
        g_origZoomVal = readZoom;
        g_zoomPatched = true;
    }

    if (g_config.stadiumClippingFix) {
        applyClippingPatches();
    }

    log().write("INFO", "broadcast: Intelligent Broadcast Director initialized (base=%.1f, mid=%.2fx, box=%.2fx)",
                g_config.baseZoom, g_config.midfieldZoomMult, g_config.boxZoomMult);
    return true;
}

void shutdown() {
    if (!g_config.enabled) return;

    if (g_zoomPatched) {
        writeMemory(ADDR_CAM_ZOOM, &g_origZoomVal, sizeof(float));
        g_zoomPatched = false;
    }

    if (g_clippingPatched) {
        revertClippingPatches();
    }

    g_state = DIR_STATE_IDLE;
    g_cameraPreset = CAM_PRESET_VANILLA;
    log().write("INFO", "broadcast: director shutdown and restored to vanilla");
}

void update(float ballX, float ballY, float ballZ, int phase, float clock) {
    if (!g_config.enabled) return;

    // 1. Detect Phase Transitions
    if (phase != g_prevPhase) {
        if (phase == 12) {
            g_state = DIR_STATE_REPLAY;
            g_cameraPreset = g_config.cinematicReplays ? CAM_PRESET_CINEMATIC_REPLAY : CAM_PRESET_VANILLA;
            g_targetZoom = g_config.baseZoom;

            g_incidentCounter++;
            g_latestIncident.id = g_incidentCounter;
            g_latestIncident.state = DIR_STATE_REPLAY;
            g_latestIncident.clock = clock;
            g_latestIncident.ballX = ballX;
            g_latestIncident.ballY = ballY;
            g_latestIncident.ballZ = ballZ;
            snprintf(g_latestIncident.description, sizeof(g_latestIncident.description),
                     "Replay Incident at %.1fs (Phase 12)", clock);
            log().write("INFO", "broadcast: %s", g_latestIncident.description);
        }
        else if (phase == 2) {
            g_state = DIR_STATE_LINEUP;
            g_cameraPreset = CAM_PRESET_DYNAMIC_BROADCAST;
            g_targetZoom = g_config.baseZoom;
        }
        else if (phase == 0) {
            g_state = DIR_STATE_IDLE;
            g_cameraPreset = CAM_PRESET_VANILLA;
            g_targetZoom = g_config.baseZoom;
        }
        g_prevPhase = phase;
    }

    // 2. Instant Replay Skip Handler
    if (phase == 12 && g_config.instantReplaySkip) {
        // Check user input: Spacebar, Enter, Escape, or Gamepad Skip
        bool skipRequested = (GetAsyncKeyState(VK_SPACE) & 0x8000) ||
                             (GetAsyncKeyState(VK_RETURN) & 0x8000) ||
                             (GetAsyncKeyState(VK_ESCAPE) & 0x8000);
        if (skipRequested) {
            requestReplaySkip();
        }
    }

    // 3. Active Play Match Analysis (phase == 3)
    if (phase == 3) {
        float absX = (ballX < 0.0f) ? -ballX : ballX;
        float absY = (ballY < 0.0f) ? -ballY : ballY;

        // Goal Line Detection: Ball crossed goal boundary into net
        if (absX > 52.0f && absY < 4.5f && ballZ < 2.5f) {
            if (g_state != DIR_STATE_GOAL_CELEBRATION) {
                g_state = DIR_STATE_GOAL_CELEBRATION;
                g_cameraPreset = CAM_PRESET_BOX_FOCUS;
                g_targetZoom = g_config.baseZoom * g_config.goalZoomMult;

                g_incidentCounter++;
                g_latestIncident.id = g_incidentCounter;
                g_latestIncident.state = DIR_STATE_GOAL_CELEBRATION;
                g_latestIncident.clock = clock;
                g_latestIncident.ballX = ballX;
                g_latestIncident.ballY = ballY;
                g_latestIncident.ballZ = ballZ;
                snprintf(g_latestIncident.description, sizeof(g_latestIncident.description),
                         "GOAL SCORED at %.1fs! Ball at (%.1f, %.1f)", clock, ballX, ballY);
                log().write("INFO", "broadcast: %s", g_latestIncident.description);
            }
        }
        // Penalty Box Penetration: Danger zone
        else if (absX > 35.0f && absY < 20.0f) {
            g_state = DIR_STATE_BOX_PENETRATION;
            g_cameraPreset = CAM_PRESET_BOX_FOCUS;
            g_targetZoom = g_config.baseZoom * g_config.boxZoomMult;
        }
        // Midfield Buildup: Ball in central pitch third
        else if (absX <= 18.0f) {
            g_state = DIR_STATE_MIDFIELD_BUILDUP;
            g_cameraPreset = CAM_PRESET_TACTICAL_HIGH;
            g_targetZoom = g_config.baseZoom * g_config.midfieldZoomMult;
        }
        // General Attacking Third
        else {
            g_state = DIR_STATE_ACTIVE_PLAY;
            g_cameraPreset = CAM_PRESET_DYNAMIC_BROADCAST;
            g_targetZoom = g_config.baseZoom;
        }
    }

    // 4. Smooth Camera Zoom Interpolation (Exponential Lerp)
    if (g_config.dynamicCamera) {
        float delta = (g_targetZoom - g_currentZoom) * g_config.zoomLerpSpeed;
        g_currentZoom += delta;
        if (g_zoomPatched) {
            setCameraZoomMemory(g_currentZoom);
        }
    }
}

bool requestReplaySkip() {
    int currentPhase = 0;
    if (!safeReadMemory(ADDR_MATCH_PHASE, &currentPhase, sizeof(int)) || currentPhase != 12) {
        return false;
    }

    // Advance phase directly out of replay to kickoff / lineup
    int nextPhase = 2;
    int zeroFlag = 0;
    writeMemory(ADDR_MATCH_PHASE, &nextPhase, sizeof(int));
    writeMemory(ADDR_REPLAY_ACTIVE, &zeroFlag, sizeof(int));

    g_state = DIR_STATE_LINEUP;
    g_targetZoom = g_config.baseZoom;
    log().write("INFO", "broadcast: instant replay skipped -> transitioning to play");
    return true;
}

bool isEnabled() {
    return g_config.enabled;
}

DirectorState getState() {
    return g_state;
}

CameraPreset getCameraPreset() {
    return g_cameraPreset;
}

float getCurrentZoom() {
    return g_currentZoom;
}

float getTargetZoom() {
    return g_targetZoom;
}

bool getLatestIncident(MatchEventIncident* out) {
    if (!out) return false;
    *out = g_latestIncident;
    return (g_latestIncident.id > 0);
}

bool isClippingFixActive() {
    return g_clippingPatched;
}

const BroadcastConfig& getConfig() {
    return g_config;
}

void setConfig(const BroadcastConfig& cfg) {
    g_config = cfg;
    g_targetZoom = cfg.baseZoom;
    g_currentZoom = cfg.baseZoom;
    if (!cfg.enabled) {
        g_state = DIR_STATE_IDLE;
        g_cameraPreset = CAM_PRESET_VANILLA;
    }
}

}  // namespace broadcast
}  // namespace remaster
