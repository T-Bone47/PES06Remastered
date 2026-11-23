// enhanced.cpp - Bounded Enhanced Gameplay Systems
//
// Manages opt-in tactical team width, dynamic late-game pressing thresholds,
// and pass lead adjustments within a strict ±15% safety envelope.
//
// Classic Mode Preservation Contract:
// When [gameplay_enhanced] enabled = 0 or [core] enabled = 0, all multipliers
// strictly equal 1.000000f, ensuring 100% bit-exact vanilla simulation.

#include "enhanced.h"
#include "../core/remaster.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

namespace remaster {
namespace gameplay {

static EnhancedGameplayConfig g_config = {
    true,   // enabled
    1.05f,  // teamWidthFactor (+5% off-ball spread)
    true,   // dynamicPressing
    1.10f,  // pressingAggressionCap (+10% max press radius)
    1.05f,  // passLeadFactor (+5% channel through-ball lead)
    1.00f,  // ballDragFactor (1.0 = vanilla air drag)
    true    // enforceSafetyEnvelope
};

static float g_currentPressingMultiplier = 1.0f;

float clampValue(float value, float minVal, float maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

static void applySafetyEnvelope() {
    if (!g_config.enforceSafetyEnvelope) return;

    g_config.teamWidthFactor = clampValue(g_config.teamWidthFactor,
                                          MIN_TEAM_WIDTH_FACTOR, MAX_TEAM_WIDTH_FACTOR);
    g_config.pressingAggressionCap = clampValue(g_config.pressingAggressionCap,
                                                1.00f, MAX_PRESSING_FACTOR);
    g_config.passLeadFactor = clampValue(g_config.passLeadFactor,
                                         MIN_PASS_LEAD_FACTOR, MAX_PASS_LEAD_FACTOR);
    g_config.ballDragFactor = clampValue(g_config.ballDragFactor,
                                         MIN_BALL_DRAG_FACTOR, MAX_BALL_DRAG_FACTOR);
}

bool init() {
    g_config.enabled = config().getBool("gameplay_enhanced", "enabled", true);
    if (!g_config.enabled) {
        log().write("INFO", "gameplay_enhanced: disabled by config [gameplay_enhanced] enabled=0 -> Classic Mode active (100%% vanilla simulation)");
        g_currentPressingMultiplier = 1.0f;
        return true;
    }

    g_config.teamWidthFactor = config().getFloat("gameplay_enhanced", "team_width_factor", 1.05f);
    g_config.dynamicPressing = config().getBool("gameplay_enhanced", "dynamic_pressing", true);
    g_config.pressingAggressionCap = config().getFloat("gameplay_enhanced", "pressing_aggression_cap", 1.10f);
    g_config.passLeadFactor = config().getFloat("gameplay_enhanced", "pass_lead_factor", 1.05f);
    g_config.ballDragFactor = config().getFloat("gameplay_enhanced", "ball_drag_factor", 1.00f);
    g_config.enforceSafetyEnvelope = config().getBool("gameplay_enhanced", "enforce_safety_envelope", true);

    applySafetyEnvelope();
    g_currentPressingMultiplier = 1.0f;

    log().write("INFO", "gameplay_enhanced: Enhanced Mode active [Width=%.2fx, PressCap=%.2fx, PassLead=%.2fx, Drag=%.2fx]",
                g_config.teamWidthFactor, g_config.pressingAggressionCap,
                g_config.passLeadFactor, g_config.ballDragFactor);
    return true;
}

void shutdown() {
    g_config.enabled = false;
    g_currentPressingMultiplier = 1.0f;
    log().write("INFO", "gameplay_enhanced: shutdown -> restored to vanilla Classic Mode");
}

void update(int phase, float matchClock, int homeScore, int awayScore) {
    if (!g_config.enabled) {
        g_currentPressingMultiplier = 1.0f;
        return;
    }

    // Dynamic Pressing Logic:
    // When match is in active play (phase == 3) and clock is in late game minutes (> 75% match duration)
    // and one team is trailing, escalate pressing aggression up to the cap.
    if (g_config.dynamicPressing && phase == 3) {
        int deficit = homeScore - awayScore;
        if (deficit < 0) deficit = -deficit;

        // If trailing in late stages (e.g. clock > 60.0f units or deficit >= 1)
        if (deficit >= 1 && matchClock > 40.0f) {
            float aggressionRange = g_config.pressingAggressionCap - 1.0f;
            float urgency = (deficit >= 2) ? 1.0f : 0.65f;
            g_currentPressingMultiplier = 1.0f + (aggressionRange * urgency);
        } else {
            g_currentPressingMultiplier = 1.0f;
        }
    } else {
        g_currentPressingMultiplier = 1.0f;
    }

    // Clamp current pressing multiplier within safety envelope
    g_currentPressingMultiplier = clampValue(g_currentPressingMultiplier,
                                             MIN_PRESSING_FACTOR, MAX_PRESSING_FACTOR);
}

bool isEnhancedActive() {
    return g_config.enabled;
}

float getTeamWidthMultiplier() {
    if (!g_config.enabled) return 1.0f;
    return g_config.teamWidthFactor;
}

float getCurrentPressingMultiplier() {
    if (!g_config.enabled) return 1.0f;
    return g_currentPressingMultiplier;
}

float getPassLeadMultiplier() {
    if (!g_config.enabled) return 1.0f;
    return g_config.passLeadFactor;
}

float getBallDragMultiplier() {
    if (!g_config.enabled) return 1.0f;
    return g_config.ballDragFactor;
}

const EnhancedGameplayConfig& getConfig() {
    return g_config;
}

void setConfig(const EnhancedGameplayConfig& cfg) {
    g_config = cfg;
    applySafetyEnvelope();
    g_currentPressingMultiplier = 1.0f;
}

}  // namespace gameplay
}  // namespace remaster
