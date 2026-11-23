#ifndef REMASTER_ANALYTICS_H
#define REMASTER_ANALYTICS_H

namespace remaster {
namespace analytics {

// Shot classification types for calibrated xG computation
enum ShotType {
    SHOT_OPEN_PLAY = 0,
    SHOT_HEADER = 1,
    SHOT_COUNTER_ATTACK = 2,
    SHOT_PENALTY = 3,
    SHOT_FREE_KICK = 4
};

// Tactical metrics computed directly from 22-player and ball tracking telemetry
struct TacticalMetrics {
    float homeDefensiveLineHeight; // Average distance of back line from own goal (meters)
    float awayDefensiveLineHeight;
    float homeCompactness;         // Squad dispersion radius around centroid (meters)
    float awayCompactness;
    unsigned int homeCompletedPasses;
    unsigned int awayCompletedPasses;
    unsigned int homeInterceptions;
    unsigned int awayInterceptions;
    float homePossessionPct;       // 0.0 to 100.0%
    float awayPossessionPct;
};

// Real-time tactical coaching recommendations based on observed telemetry
enum CoachingTip {
    TIP_NONE = 0,
    TIP_PRESS_HIGHER = 1,
    TIP_TIGHTEN_LINES = 2,
    TIP_EXPLOIT_FLANKS = 3,
    TIP_COUNTER_PRESS = 4
};

struct CoachingInsight {
    CoachingTip tip;
    char advice[96];
    float confidence;
};

// Snapshot of live match analytics for broadcast presentation & telemetry
struct MatchAnalytics {
    float homeXG;             // Accumulated Home Expected Goals
    float awayXG;             // Accumulated Away Expected Goals
    float homeMomentum;       // Home momentum percentage: 0.0 to 100.0 (baseline 50.0)
    float awayMomentum;       // Away momentum percentage: 0.0 to 100.0 (baseline 50.0)
    float homeFieldTilt;      // Attacking third territorial share: 0.0 to 100.0
    float awayFieldTilt;      // Away attacking third territorial share
    float homeTerritoryPct;   // Percentage of match time spent in opponent half
    float awayTerritoryPct;   // Percentage of match time spent in opponent half
    unsigned int homeShots;   // Total Home shots attempted
    unsigned int awayShots;   // Total Away shots attempted
    unsigned int homeShotsOnTarget;
    unsigned int awayShotsOnTarget;
    float lastShotXG;         // Most recent shot xG value
    int lastShooterTeam;      // 0 = Home, 1 = Away
};

// Initializes the analytics engine
bool init();

// Evaluates shot probability using distance, visible angle, and shot type
float calculateShotXG(float ballX, float ballY, float ballZ, ShotType shotType, bool isHomeTeam);

// Records a shot event, adds to team cumulative xG, and updates shot counters (with deduplication)
void recordShot(float ballX, float ballY, float ballZ, ShotType shotType, bool isHomeTeam, bool onTarget, float matchClock = 0.0f);

// Updates match momentum and territorial field tilt with side awareness (halftime coordinate inversion)
void updateMomentum(float ballX, float ballY, float ballZ, int phase, float dtSeconds, bool homeAttackingPositiveX = true);

// Evaluates complete 22-player tracking and ball telemetry to extract passes, interceptions, possession, and formations
void updateMatchEntities(const unsigned char* entities, int entityCount, int phase, float clockSeconds, float dtSeconds);

// Resets analytics at match start / kickoff
void resetMatch();

// Accesses current live analytics snapshot
const MatchAnalytics& getAnalytics();

// Accesses tactical metrics
const TacticalMetrics& getTacticalMetrics();

// Accesses intelligent coaching insight
const CoachingInsight& getCoachingInsight();

}  // namespace analytics
}  // namespace remaster

#endif  // REMASTER_ANALYTICS_H
