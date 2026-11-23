#ifndef REMASTER_GAMEPLAY_ENHANCED_H
#define REMASTER_GAMEPLAY_ENHANCED_H

namespace remaster {
namespace gameplay {

// Safety envelope bounds (±15% maximum deviation from vanilla)
#define MIN_TEAM_WIDTH_FACTOR 0.85f
#define MAX_TEAM_WIDTH_FACTOR 1.15f
#define MIN_PRESSING_FACTOR   0.85f
#define MAX_PRESSING_FACTOR   1.15f
#define MIN_PASS_LEAD_FACTOR  0.85f
#define MAX_PASS_LEAD_FACTOR  1.15f
#define MIN_BALL_DRAG_FACTOR  0.95f
#define MAX_BALL_DRAG_FACTOR  1.05f

// Enhanced Gameplay configuration schema
struct EnhancedGameplayConfig {
    bool enabled;                  // Master toggle for Enhanced Gameplay (0 = Classic Mode)
    float teamWidthFactor;         // Tactical team width (0.85 - 1.15, default 1.05)
    bool dynamicPressing;          // Dynamic late-game pressing when trailing
    float pressingAggressionCap;   // Max pressing multiplier (1.00 - 1.15, default 1.10)
    float passLeadFactor;          // Through-ball channel lead multiplier (0.85 - 1.15, default 1.05)
    float ballDragFactor;          // Ball air drag modifier (0.95 - 1.05, default 1.00)
    bool enforceSafetyEnvelope;    // Clamps all variables within ±15%
};

// Initializes the Enhanced Gameplay subsystem with INI configuration.
bool init();

// Shuts down the subsystem and restores 100% vanilla settings.
void shutdown();

// Updates dynamic gameplay parameters on each match tick / telemetry sample.
// Evaluates score differential and match minute to modulate late-game pressing.
void update(int phase, float matchClock, int homeScore, int awayScore);

// Returns true if Enhanced Mode is active (false = bit-exact Classic Mode).
bool isEnhancedActive();

// Accessors for modulated gameplay parameters (strictly clamped)
float getTeamWidthMultiplier();
float getCurrentPressingMultiplier();
float getPassLeadMultiplier();
float getBallDragMultiplier();

// Clamps a float value within [minVal, maxVal].
float clampValue(float value, float minVal, float maxVal);

// Configuration accessors for inspection and testing
const EnhancedGameplayConfig& getConfig();
void setConfig(const EnhancedGameplayConfig& cfg);

}  // namespace gameplay
}  // namespace remaster

#endif  // REMASTER_GAMEPLAY_ENHANCED_H
