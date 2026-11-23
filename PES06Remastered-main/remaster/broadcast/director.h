#ifndef REMASTER_BROADCAST_DIRECTOR_H
#define REMASTER_BROADCAST_DIRECTOR_H

namespace remaster {
namespace broadcast {

// Operational states of the Intelligent Broadcast Director.
enum DirectorState {
    DIR_STATE_IDLE = 0,             // Inactive or in menus
    DIR_STATE_LINEUP = 1,           // Pre-match lineup / stadium presentation
    DIR_STATE_ACTIVE_PLAY = 2,      // Standard active match play (neutral pitch)
    DIR_STATE_MIDFIELD_BUILDUP = 3, // Midfield tactical overview (wide angle)
    DIR_STATE_BOX_PENETRATION = 4,  // High-intensity penalty box tracking (tight)
    DIR_STATE_GOAL_CELEBRATION = 5, // Goal scored incident / celebration
    DIR_STATE_SET_PIECE = 6,        // Set piece (corner, free-kick, penalty)
    DIR_STATE_REPLAY = 7            // Instant replay playback
};

// Contextual camera presets selected by the director.
enum CameraPreset {
    CAM_PRESET_VANILLA = 0,           // Stock vanilla camera
    CAM_PRESET_DYNAMIC_BROADCAST = 1, // Dynamic pitch-tracking broadcast camera
    CAM_PRESET_TACTICAL_HIGH = 2,     // Elevated tactical wide camera
    CAM_PRESET_BOX_FOCUS = 3,         // Low, tight box tracking camera
    CAM_PRESET_CINEMATIC_REPLAY = 4   // Multi-angle cinematic replay camera
};

// Broadcast Director configuration schema.
struct BroadcastConfig {
    bool enabled;                  // Master toggle for broadcast director
    bool dynamicCamera;            // Dynamic pitch-position zoom tracking
    float baseZoom;                // Vanilla camera distance (default 1430.0f)
    float midfieldZoomMult;        // Midfield overview multiplier (default 1.15f -> 1644.5f)
    float boxZoomMult;             // Box penetration multiplier (default 0.85f -> 1215.5f)
    float goalZoomMult;            // Goal celebration multiplier (default 0.65f -> 929.5f)
    float zoomLerpSpeed;           // Smoothing factor per tick (default 0.08f)
    bool stadiumClippingFix;       // Fix stadium roof / upper tier geometry clipping
    bool instantReplaySkip;        // One-key / instant replay bypass
    bool cinematicReplays;         // Contextual multi-angle replay director
};

// Match incident record for broadcast logging and sidecar telemetry.
struct MatchEventIncident {
    unsigned int id;
    DirectorState state;
    float clock;
    float ballX;
    float ballY;
    float ballZ;
    char description[64];
};

// Initializes the broadcast director with INI configuration.
bool init();

// Closes and unhooks the broadcast director, restoring vanilla memory state.
void shutdown();

// Updates director state on each match tick / telemetry sample.
void update(float ballX, float ballY, float ballZ, int phase, float clock);

// Checks if the master broadcast system is active.
bool isEnabled();

// Returns current director state.
DirectorState getState();

// Returns current camera preset.
CameraPreset getCameraPreset();

// Returns current interpolated zoom distance.
float getCurrentZoom();

// Returns target zoom distance.
float getTargetZoom();

// Requests an instant skip of the current replay if active.
bool requestReplaySkip();

// Queries the latest match incident.
bool getLatestIncident(MatchEventIncident* out);

// Checks if stadium roof clipping patches are installed.
bool isClippingFixActive();

// Config accessor for runtime inspection.
const BroadcastConfig& getConfig();

// Sets config directly (used by unit tests).
void setConfig(const BroadcastConfig& cfg);

}  // namespace broadcast
}  // namespace remaster

#endif  // REMASTER_BROADCAST_DIRECTOR_H
