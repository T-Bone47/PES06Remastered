#include "analytics.h"
#include "../core/remaster.h"

#include <math.h>
#include <string.h>

namespace remaster {
namespace analytics {

// Standard pitch dimensions (meters)
static const float PITCH_HALF_LEN = 52.5f;
static const float POST_Y1 = -3.66f;
static const float POST_Y2 = 3.66f;
static const float GOAL_WIDTH = 7.32f;

static MatchAnalytics g_analytics = {
    0.0f,   // homeXG
    0.0f,   // awayXG
    50.0f,  // homeMomentum
    50.0f,  // awayMomentum
    50.0f,  // homeFieldTilt
    50.0f,  // awayFieldTilt
    50.0f,  // homeTerritoryPct
    50.0f,  // awayTerritoryPct
    0,      // homeShots
    0,      // awayShots
    0,      // homeShotsOnTarget
    0,      // awayShotsOnTarget
    0.0f,   // lastShotXG
    -1      // lastShooterTeam
};

static float g_homeTerritoryAccum = 0.0f;
static float g_awayTerritoryAccum = 0.0f;
static float g_homeHalfAccum = 0.0f;
static float g_awayHalfAccum = 0.0f;
static float g_momentumRaw = 0.0f;  // [-1.0 (Away) to +1.0 (Home)]
static float g_prevBallX = 0.0f;
static float g_lastShotClock = -999.0f;
static int g_lastShooterTeam = -1;

static TacticalMetrics g_tactical = {
    30.0f, // homeDefensiveLineHeight
    30.0f, // awayDefensiveLineHeight
    22.0f, // homeCompactness
    22.0f, // awayCompactness
    0,     // homeCompletedPasses
    0,     // awayCompletedPasses
    0,     // homeInterceptions
    0,     // awayInterceptions
    50.0f, // homePossessionPct
    50.0f  // awayPossessionPct
};

static CoachingInsight g_coaching = {
    TIP_NONE,
    "Squad lines balanced.",
    1.0f
};

static int s_prevPossessor = -1;
static float s_homePossessionTime = 0.0f;
static float s_awayPossessionTime = 0.0f;

bool init() {
    resetMatch();
    log().write("INFO", "analytics: Live Tactical Intelligence & xG engine initialized");
    return true;
}

void resetMatch() {
    memset(&g_analytics, 0, sizeof(g_analytics));
    g_analytics.homeMomentum = 50.0f;
    g_analytics.awayMomentum = 50.0f;
    g_analytics.homeFieldTilt = 50.0f;
    g_analytics.awayFieldTilt = 50.0f;
    g_analytics.homeTerritoryPct = 50.0f;
    g_analytics.awayTerritoryPct = 50.0f;
    g_analytics.lastShooterTeam = -1;
    g_homeTerritoryAccum = 0.0f;
    g_awayTerritoryAccum = 0.0f;
    g_homeHalfAccum = 0.0f;
    g_awayHalfAccum = 0.0f;
    g_momentumRaw = 0.0f;
    g_prevBallX = 0.0f;
    g_lastShotClock = -999.0f;
    g_lastShooterTeam = -1;

    memset(&g_tactical, 0, sizeof(g_tactical));
    g_tactical.homeDefensiveLineHeight = 30.0f;
    g_tactical.awayDefensiveLineHeight = 30.0f;
    g_tactical.homeCompactness = 22.0f;
    g_tactical.awayCompactness = 22.0f;
    g_tactical.homePossessionPct = 50.0f;
    g_tactical.awayPossessionPct = 50.0f;
    s_prevPossessor = -1;
    s_homePossessionTime = 0.0f;
    s_awayPossessionTime = 0.0f;

    g_coaching.tip = TIP_NONE;
    strcpy_s(g_coaching.advice, sizeof(g_coaching.advice), "Squad lines balanced.");
    g_coaching.confidence = 1.0f;
}

float calculateShotXG(float ballX, float ballY, float ballZ, ShotType shotType, bool isHomeTeam) {
    if (shotType == SHOT_PENALTY) {
        return 0.76f;  // Standard international penalty conversion benchmark
    }

    // Target goal line X: Home attacks positive X (+52.5), Away attacks negative X (-52.5)
    float goalX = isHomeTeam ? PITCH_HALF_LEN : -PITCH_HALF_LEN;

    // Euclidean distance to goal center
    float dx = goalX - ballX;
    float dy = -ballY;
    float dist = sqrtf(dx * dx + dy * dy);

    // Enforce minimum distance to prevent divide by zero
    if (dist < 0.5f) dist = 0.5f;

    // Vectors to left post and right post
    float v1x = goalX - ballX;
    float v1y = POST_Y1 - ballY;
    float len1 = sqrtf(v1x * v1x + v1y * v1y);

    float v2x = goalX - ballX;
    float v2y = POST_Y2 - ballY;
    float len2 = sqrtf(v2x * v2x + v2y * v2y);

    float angle = 0.0f;
    if (len1 > 0.01f && len2 > 0.01f) {
        float dot = (v1x * v2x + v1y * v2y) / (len1 * len2);
        if (dot > 1.0f) dot = 1.0f;
        if (dot < -1.0f) dot = -1.0f;
        angle = acosf(dot);  // Radians
    }

    // Logistic regression logit:
    // logit = beta0 - beta1 * dist + beta2 * angle - beta3 * isHeader + beta4 * isCounter
    float logit = 0.85f - 0.145f * dist + 1.65f * angle;

    if (shotType == SHOT_HEADER) {
        logit -= 0.70f;  // Headers convert at ~half the rate of feet
    } else if (shotType == SHOT_COUNTER_ATTACK) {
        logit += 0.40f;  // Counter-attacks offer clearer shooting channels
    } else if (shotType == SHOT_FREE_KICK) {
        logit -= 0.55f;  // Direct free kicks face set defensive walls
    }

    // Height penalty (aerial bouncing ball)
    if (ballZ > 1.2f) {
        logit -= 0.35f * (ballZ - 1.2f);
    }

    // Logistic sigmoid
    float xg = 1.0f / (1.0f + expf(-logit));

    // Bounds clamp [0.01, 0.99]
    if (xg < 0.01f) xg = 0.01f;
    if (xg > 0.99f) xg = 0.99f;

    return xg;
}

void recordShot(float ballX, float ballY, float ballZ, ShotType shotType, bool isHomeTeam, bool onTarget, float matchClock) {
    int teamIdx = isHomeTeam ? 0 : 1;

    // Shot event deduplication: reject double-counting from multi-tick triggers within 1.5s
    if (matchClock > 0.0f && g_lastShotClock > 0.0f) {
        if (fabsf(matchClock - g_lastShotClock) < 1.5f && g_lastShooterTeam == teamIdx) {
            log().write("INFO", "analytics: Deduplicated duplicate shot event at clock %.1fs", matchClock);
            return;
        }
    }

    float xg = calculateShotXG(ballX, ballY, ballZ, shotType, isHomeTeam);

    if (isHomeTeam) {
        g_analytics.homeShots++;
        if (onTarget) g_analytics.homeShotsOnTarget++;
        g_analytics.homeXG += xg;
        g_analytics.lastShooterTeam = 0;
    } else {
        g_analytics.awayShots++;
        if (onTarget) g_analytics.awayShotsOnTarget++;
        g_analytics.awayXG += xg;
        g_analytics.lastShooterTeam = 1;
    }
    g_analytics.lastShotXG = xg;
    g_lastShotClock = matchClock;
    g_lastShooterTeam = teamIdx;

    log().write("INFO", "analytics: Shot recorded [%s, type=%d, xG=%.3f, cum=(%.2f - %.2f)]",
                isHomeTeam ? "Home" : "Away", (int)shotType, xg, g_analytics.homeXG, g_analytics.awayXG);
}

void updateMomentum(float ballX, float ballY, float ballZ, int phase, float dtSeconds, bool homeAttackingPositiveX) {
    (void)ballY;
    (void)ballZ;

    // Halftime coordinate inversion: map ball coordinate so Home always attacks positive X internally
    float effBallX = homeAttackingPositiveX ? ballX : -ballX;

    // Pauses, cutscenes, replays, or dead-ball transitions (phase != 3):
    // Freeze momentum updates so idle time does not artificially bias or decay live momentum
    if (phase != 3) {
        g_prevBallX = effBallX;
        return;
    }

    if (dtSeconds <= 0.0f) dtSeconds = 0.25f;
    if (dtSeconds > 1.0f) dtSeconds = 1.0f;

    // 1. Separate Half-Field Territory Tracking:
    if (effBallX > 0.0f) {
        g_homeHalfAccum += dtSeconds;
    } else {
        g_awayHalfAccum += dtSeconds;
    }
    float totalHalfTime = g_homeHalfAccum + g_awayHalfAccum;
    if (totalHalfTime > 1.0f) {
        g_analytics.homeTerritoryPct = (g_homeHalfAccum / totalHalfTime) * 100.0f;
        g_analytics.awayTerritoryPct = 100.0f - g_analytics.homeTerritoryPct;
    }

    // 2. Final-Third Territorial Pressure Evaluation (Field Tilt):
    // Attacking third for Home: effBallX > 17.5m
    // Attacking third for Away: effBallX < -17.5m
    float targetPressure = 0.0f;
    if (effBallX > 17.5f) {
        // Deep home attacking third: strong positive pressure
        float depth = (effBallX - 17.5f) / 35.0f;
        targetPressure = 0.4f + 0.6f * depth;
        g_homeTerritoryAccum += dtSeconds * (1.0f + depth);
    } else if (effBallX < -17.5f) {
        // Deep away attacking third: strong negative pressure
        float depth = (-effBallX - 17.5f) / 35.0f;
        targetPressure = -(0.4f + 0.6f * depth);
        g_awayTerritoryAccum += dtSeconds * (1.0f + depth);
    } else {
        // Midfield territory: slight gradient
        targetPressure = (effBallX / 17.5f) * 0.25f;
    }

    // 3. Forward ball velocity towards attacking goal
    float ballVelX = (effBallX - g_prevBallX) / dtSeconds;
    g_prevBallX = effBallX;

    float attackBoost = ballVelX * 0.02f;
    if (attackBoost > 0.3f) attackBoost = 0.3f;
    if (attackBoost < -0.3f) attackBoost = -0.3f;
    targetPressure += attackBoost;

    // 4. Smooth Exponential Moving Average (EMA) for Match Momentum
    float alpha = 0.15f * dtSeconds;
    if (alpha > 0.5f) alpha = 0.5f;
    g_momentumRaw += alpha * (targetPressure - g_momentumRaw);

    // Clamp raw momentum [-1.0, 1.0]
    if (g_momentumRaw < -1.0f) g_momentumRaw = -1.0f;
    if (g_momentumRaw > 1.0f) g_momentumRaw = 1.0f;

    // Map to percentage [0.0% to 100.0%]
    g_analytics.homeMomentum = 50.0f + g_momentumRaw * 50.0f;
    g_analytics.awayMomentum = 100.0f - g_analytics.homeMomentum;

    // 5. Distinct Field Tilt Percentage (ratio of final-third territorial dominance)
    float totalFinalThird = g_homeTerritoryAccum + g_awayTerritoryAccum;
    if (totalFinalThird > 1.0f) {
        g_analytics.homeFieldTilt = (g_homeTerritoryAccum / totalFinalThird) * 100.0f;
        g_analytics.awayFieldTilt = 100.0f - g_analytics.homeFieldTilt;
    } else {
        g_analytics.homeFieldTilt = 50.0f;
        g_analytics.awayFieldTilt = 50.0f;
    }
}

const MatchAnalytics& getAnalytics() {
    return g_analytics;
}

void updateMatchEntities(const unsigned char* entities, int entityCount, int phase, float clockSeconds, float dtSeconds) {
    (void)clockSeconds;
    if (!entities || entityCount < 23 || phase != 3) {
        return;
    }
    if (dtSeconds <= 0.0f) dtSeconds = 0.25f;

    // Entity 22 is ball
    float ballX = *(const float*)(entities + 22 * 0x48 + 0x00);
    float ballY = *(const float*)(entities + 22 * 0x48 + 0x04);
    float ballZ = *(const float*)(entities + 22 * 0x48 + 0x08);

    // 1. Possession & Passing Network Tracking
    int closestPlayer = -1;
    float closestDistSq = 9999.0f;

    for (int i = 0; i < 22; ++i) {
        float px = *(const float*)(entities + i * 0x48 + 0x00);
        float py = *(const float*)(entities + i * 0x48 + 0x04);
        float pz = *(const float*)(entities + i * 0x48 + 0x08);
        float dx = px - ballX;
        float dy = py - ballY;
        float dz = pz - ballZ;
        float distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < closestDistSq) {
            closestDistSq = distSq;
            closestPlayer = i;
        }
    }

    // Ball controlled threshold: within 1.5m and below 2.0m height
    if (closestPlayer >= 0 && closestDistSq < (1.5f * 1.5f) && ballZ < 2.0f) {
        int currentTeam = (closestPlayer < 11) ? 0 : 1;
        if (currentTeam == 0) s_homePossessionTime += dtSeconds;
        else s_awayPossessionTime += dtSeconds;

        if (s_prevPossessor >= 0 && s_prevPossessor != closestPlayer) {
            int prevTeam = (s_prevPossessor < 11) ? 0 : 1;
            if (prevTeam == currentTeam) {
                // Completed Pass
                if (currentTeam == 0) g_tactical.homeCompletedPasses++;
                else g_tactical.awayCompletedPasses++;
            } else {
                // Turnover / Interception
                if (currentTeam == 0) g_tactical.homeInterceptions++;
                else g_tactical.awayInterceptions++;
            }
        }
        s_prevPossessor = closestPlayer;
    }

    float totalPossTime = s_homePossessionTime + s_awayPossessionTime;
    if (totalPossTime > 1.0f) {
        g_tactical.homePossessionPct = (s_homePossessionTime / totalPossTime) * 100.0f;
        g_tactical.awayPossessionPct = 100.0f - g_tactical.homePossessionPct;
    }

    // 2. Team Compactness (Outfield centroid & dispersion radius)
    // Home outfield: 1..10
    float homeCentroidX = 0.0f, homeCentroidY = 0.0f;
    float minHomeX = 999.0f;
    for (int i = 1; i < 11; ++i) {
        float px = *(const float*)(entities + i * 0x48 + 0x00);
        float py = *(const float*)(entities + i * 0x48 + 0x04);
        homeCentroidX += px;
        homeCentroidY += py;
        if (px < minHomeX) minHomeX = px;
    }
    homeCentroidX /= 10.0f;
    homeCentroidY /= 10.0f;
    float homeDispersion = 0.0f;
    for (int i = 1; i < 11; ++i) {
        float px = *(const float*)(entities + i * 0x48 + 0x00);
        float py = *(const float*)(entities + i * 0x48 + 0x04);
        float dx = px - homeCentroidX;
        float dy = py - homeCentroidY;
        homeDispersion += sqrtf(dx * dx + dy * dy);
    }
    g_tactical.homeCompactness = homeDispersion / 10.0f;

    // Home Defensive Line Height (distance from own goal at -52.5m)
    g_tactical.homeDefensiveLineHeight = minHomeX - (-52.5f);
    if (g_tactical.homeDefensiveLineHeight < 5.0f) g_tactical.homeDefensiveLineHeight = 5.0f;

    // Away outfield: 12..21
    float awayCentroidX = 0.0f, awayCentroidY = 0.0f;
    float maxAwayX = -999.0f;
    for (int i = 12; i < 22; ++i) {
        float px = *(const float*)(entities + i * 0x48 + 0x00);
        float py = *(const float*)(entities + i * 0x48 + 0x04);
        awayCentroidX += px;
        awayCentroidY += py;
        if (px > maxAwayX) maxAwayX = px;
    }
    awayCentroidX /= 10.0f;
    awayCentroidY /= 10.0f;
    float awayDispersion = 0.0f;
    for (int i = 12; i < 22; ++i) {
        float px = *(const float*)(entities + i * 0x48 + 0x00);
        float py = *(const float*)(entities + i * 0x48 + 0x04);
        float dx = px - awayCentroidX;
        float dy = py - awayCentroidY;
        awayDispersion += sqrtf(dx * dx + dy * dy);
    }
    g_tactical.awayCompactness = awayDispersion / 10.0f;
    g_tactical.awayDefensiveLineHeight = 52.5f - maxAwayX;
    if (g_tactical.awayDefensiveLineHeight < 5.0f) g_tactical.awayDefensiveLineHeight = 5.0f;

    // 3. Intelligent Tactical Coaching Insights
    if (g_tactical.homeDefensiveLineHeight < 16.0f && g_analytics.homeMomentum < 45.0f) {
        g_coaching.tip = TIP_PRESS_HIGHER;
        strcpy_s(g_coaching.advice, sizeof(g_coaching.advice), "Backline pinned deep. Step up defensive line to contest midfield.");
        g_coaching.confidence = 0.88f;
    } else if (g_tactical.homeCompactness > 28.0f) {
        g_coaching.tip = TIP_TIGHTEN_LINES;
        strcpy_s(g_coaching.advice, sizeof(g_coaching.advice), "Squad lines overextended. Narrow spacing between midfield and defense.");
        g_coaching.confidence = 0.82f;
    } else if (g_tactical.awayDefensiveLineHeight > 38.0f) {
        g_coaching.tip = TIP_EXPLOIT_FLANKS;
        strcpy_s(g_coaching.advice, sizeof(g_coaching.advice), "Opponent playing high offside trap. Exploit wide channels behind defense.");
        g_coaching.confidence = 0.85f;
    } else {
        g_coaching.tip = TIP_NONE;
        strcpy_s(g_coaching.advice, sizeof(g_coaching.advice), "Tactical structure balanced.");
        g_coaching.confidence = 0.90f;
    }
}

const TacticalMetrics& getTacticalMetrics() {
    return g_tactical;
}

const CoachingInsight& getCoachingInsight() {
    return g_coaching;
}

}  // namespace analytics
}  // namespace remaster
