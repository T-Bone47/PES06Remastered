#include "modern_hud.h"
#include "../analytics/analytics.h"
#include "../core/remaster.h"

#include <stdio.h>
#include <string.h>

namespace remaster {
namespace graphics {

static const ThemeStyle s_themeStyles[] = {
    // THEME_UCL_2006
    {
        THEME_UCL_2006,
        "2006 UEFA Champions League",
        "ucl",
        0xE60A1832, // Primary: Deep navy
        0xFF00E5FF, // Secondary: Neon cyan
        0xFFFFD700, // Accent: Star gold
        0xFFFFFFFF, // Font: Crisp white
        0xFF050E20, // ClockBg: Midnight indigo
        0xFF0F264A, // ScoreBg: Royal navy
        "UCL",
        true,       // showXG
        true        // showMomentum
    },
    // THEME_SKY_SPORTS_2006
    {
        THEME_SKY_SPORTS_2006,
        "2006 Sky Sports Super Sunday",
        "sky_sports",
        0xEE002244, // Primary: Sky navy
        0xFFFFD200, // Secondary: Vivid yellow
        0xFFD60000, // Accent: Live red
        0xFFFFFFFF, // Font: White
        0xFF00152B, // ClockBg: Dark navy
        0xFF003366, // ScoreBg: Vivid navy
        "SKY",
        true,       // showXG
        true        // showMomentum
    },
    // THEME_MOTD_VINTAGE
    {
        THEME_MOTD_VINTAGE,
        "BBC Match of the Day",
        "motd",
        0xF0121212, // Primary: Carbon black
        0xFFD4AF37, // Secondary: Metallic gold
        0xFFE0E0E0, // Accent: Silver
        0xFFFFFFFF, // Font: White
        0xFF000000, // ClockBg: Pitch black
        0xFF1E1E1E, // ScoreBg: Dark charcoal
        "MOTD",
        false,      // showXG (minimalist vintage)
        true        // showMomentum
    },
    // THEME_WC_2006
    {
        THEME_WC_2006,
        "2006 FIFA World Cup Germany",
        "wc2006",
        0xEE083A1E, // Primary: Forest green
        0xFFFFFFFF, // Secondary: Crisp white
        0xFFFFCC00, // Accent: German gold
        0xFFFFFFFF, // Font: White
        0xFF041F10, // ClockBg: Dark emerald
        0xFF0D522C, // ScoreBg: Turf green
        "WC06",
        true,       // showXG
        true        // showMomentum
    },
    // THEME_MODERN_GLASS
    {
        THEME_MODERN_GLASS,
        "Modern Frosted Glass",
        "modern",
        0xCC181C24, // Primary: Translucent frosted glass
        0xFF4A90E2, // Secondary: Ice blue
        0xFFFF4757, // Accent: Coral alert
        0xFFFFFFFF, // Font: Pure white
        0xAA101216, // ClockBg: Deep glass
        0xAA222834, // ScoreBg: Smoky blue
        "LIVE",
        true,       // showXG
        true        // showMomentum
    }
};

static BroadcastTheme g_activeTheme = THEME_UCL_2006;

static BroadcastScoreBug g_scoreBug = {
    THEME_UCL_2006,               // currentTheme
    "2006 UEFA Champions League", // themeName
    "ucl",                        // themeKey
    0xE60A1832,                   // primaryColor
    0xFF00E5FF,                   // secondaryColor
    0xFFFFD700,                   // accentColor
    0xFFFFFFFF,                   // fontColor
    0xFF050E20,                   // clockBgColor
    0xFF0F264A,                   // scoreBgColor
    "UCL",                        // badgeStr
    "[UCL] HOM 0 - 0 AWY (00:00)",// formattedHeader
    "00:00",                      // clockStr
    "",                           // stoppageStr
    "HOM",                        // homeAbbr
    "AWY",                        // awayAbbr
    "0 - 0",                      // scoreStr
    "1ST HALF",                   // matchPeriodStr
    "xG 0.00 - 0.00",             // xgStr
    "50% - 50%",                  // momentumStr
    50.0f,                        // homeMomentumPct
    false,                        // isStoppageActive
    false,                        // isGoalBannerActive
    0.0f                          // goalBannerTimer
};

BroadcastTheme parseBroadcastTheme(const char* str) {
    if (!str || str[0] == '\0') return THEME_UCL_2006;
    if (_stricmp(str, "ucl") == 0 || _stricmp(str, "champions_league") == 0 || _stricmp(str, "ucl2006") == 0) {
        return THEME_UCL_2006;
    }
    if (_stricmp(str, "sky_sports") == 0 || _stricmp(str, "sky") == 0 || _stricmp(str, "premier_league") == 0) {
        return THEME_SKY_SPORTS_2006;
    }
    if (_stricmp(str, "motd") == 0 || _stricmp(str, "bbc") == 0 || _stricmp(str, "match_of_the_day") == 0) {
        return THEME_MOTD_VINTAGE;
    }
    if (_stricmp(str, "wc2006") == 0 || _stricmp(str, "worldcup") == 0 || _stricmp(str, "world_cup") == 0) {
        return THEME_WC_2006;
    }
    if (_stricmp(str, "modern") == 0 || _stricmp(str, "glass") == 0 || _stricmp(str, "frosted") == 0) {
        return THEME_MODERN_GLASS;
    }
    return THEME_UCL_2006;
}

const char* getBroadcastThemeName(BroadcastTheme theme) {
    int idx = (int)theme;
    if (idx >= 0 && idx < (int)(sizeof(s_themeStyles) / sizeof(s_themeStyles[0]))) {
        return s_themeStyles[idx].name;
    }
    return s_themeStyles[0].name;
}

const char* getBroadcastThemeKey(BroadcastTheme theme) {
    int idx = (int)theme;
    if (idx >= 0 && idx < (int)(sizeof(s_themeStyles) / sizeof(s_themeStyles[0]))) {
        return s_themeStyles[idx].key;
    }
    return s_themeStyles[0].key;
}

const ThemeStyle& getThemeStyle(BroadcastTheme theme) {
    int idx = (int)theme;
    if (idx >= 0 && idx < (int)(sizeof(s_themeStyles) / sizeof(s_themeStyles[0]))) {
        return s_themeStyles[idx];
    }
    return s_themeStyles[0];
}

void setBroadcastTheme(BroadcastTheme theme) {
    g_activeTheme = theme;
    const ThemeStyle& s = getThemeStyle(theme);
    g_scoreBug.currentTheme = theme;
    strncpy_s(g_scoreBug.themeName, sizeof(g_scoreBug.themeName), s.name, _TRUNCATE);
    strncpy_s(g_scoreBug.themeKey, sizeof(g_scoreBug.themeKey), s.key, _TRUNCATE);
    g_scoreBug.primaryColor = s.primaryColor;
    g_scoreBug.secondaryColor = s.secondaryColor;
    g_scoreBug.accentColor = s.accentColor;
    g_scoreBug.fontColor = s.fontColor;
    g_scoreBug.clockBgColor = s.clockBgColor;
    g_scoreBug.scoreBgColor = s.scoreBgColor;
    strncpy_s(g_scoreBug.badgeStr, sizeof(g_scoreBug.badgeStr), s.badgeTag, _TRUNCATE);
}

BroadcastTheme getBroadcastTheme() {
    return g_activeTheme;
}

static void sanitizeAbbreviation(const char* name, char* out, size_t outSize, const char* fallback) {
    if (!name || name[0] == '\0') {
        strncpy_s(out, outSize, fallback, outSize - 1);
        return;
    }
    size_t len = 0;
    for (size_t i = 0; name[i] != '\0' && len < 3 && len + 1 < outSize; ++i) {
        char c = name[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
            out[len++] = c;
        }
    }
    out[len] = '\0';
    if (len == 0) {
        strncpy_s(out, outSize, fallback, outSize - 1);
    }
}

bool initModernHud() {
    memset(&g_scoreBug, 0, sizeof(g_scoreBug));

    // Load theme from config
    const char* themeCfg = config().get("graphics", "scoreboard_theme", "ucl");
    BroadcastTheme t = parseBroadcastTheme(themeCfg);
    setBroadcastTheme(t);

    strcpy_s(g_scoreBug.clockStr, sizeof(g_scoreBug.clockStr), "00:00");
    strcpy_s(g_scoreBug.scoreStr, sizeof(g_scoreBug.scoreStr), "0 - 0");
    strcpy_s(g_scoreBug.homeAbbr, sizeof(g_scoreBug.homeAbbr), "HOM");
    strcpy_s(g_scoreBug.awayAbbr, sizeof(g_scoreBug.awayAbbr), "AWY");
    strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "1ST HALF");
    strcpy_s(g_scoreBug.xgStr, sizeof(g_scoreBug.xgStr), "xG 0.00 - 0.00");
    strcpy_s(g_scoreBug.momentumStr, sizeof(g_scoreBug.momentumStr), "50% - 50%");
    g_scoreBug.homeMomentumPct = 50.0f;

    sprintf_s(g_scoreBug.formattedHeader, sizeof(g_scoreBug.formattedHeader),
              "[%s] %s %s %s (%s)", g_scoreBug.badgeStr, g_scoreBug.homeAbbr,
              g_scoreBug.scoreStr, g_scoreBug.awayAbbr, g_scoreBug.clockStr);

    log().write("INFO", "modern_hud: Broadcast presentation score bug initialized [theme='%s' (%s)]",
                g_scoreBug.themeKey, g_scoreBug.themeName);
    return true;
}

void updateModernHud(float clockSeconds, int phase, int homeGoals, int awayGoals,
                     const char* homeTeam, const char* awayTeam) {
    if (clockSeconds < 0.0f) clockSeconds = 0.0f;

    // Team abbreviations
    sanitizeAbbreviation(homeTeam, g_scoreBug.homeAbbr, sizeof(g_scoreBug.homeAbbr), "HOM");
    sanitizeAbbreviation(awayTeam, g_scoreBug.awayAbbr, sizeof(g_scoreBug.awayAbbr), "AWY");

    // Match period and clock
    int totalMinutes = (int)(clockSeconds / 60.0f);
    int seconds = (int)clockSeconds % 60;

    if (phase == 0) {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "PRE-MATCH");
        g_scoreBug.isStoppageActive = false;
        g_scoreBug.stoppageStr[0] = '\0';
    } else if (clockSeconds <= 45.0f * 60.0f) {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "1ST HALF");
        g_scoreBug.isStoppageActive = false;
        g_scoreBug.stoppageStr[0] = '\0';
    } else if (clockSeconds > 45.0f * 60.0f && clockSeconds < 50.0f * 60.0f) {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "1ST HALF");
        int stoppageMins = (int)((clockSeconds - 45.0f * 60.0f) / 60.0f) + 1;
        sprintf_s(g_scoreBug.stoppageStr, sizeof(g_scoreBug.stoppageStr), "+%d'", stoppageMins);
        g_scoreBug.isStoppageActive = true;
    } else if (phase == 2 && totalMinutes >= 45 && totalMinutes < 50) {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "HALF TIME");
        g_scoreBug.isStoppageActive = false;
        g_scoreBug.stoppageStr[0] = '\0';
    } else if (clockSeconds <= 90.0f * 60.0f) {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "2ND HALF");
        g_scoreBug.isStoppageActive = false;
        g_scoreBug.stoppageStr[0] = '\0';
    } else {
        strcpy_s(g_scoreBug.matchPeriodStr, sizeof(g_scoreBug.matchPeriodStr), "2ND HALF");
        int stoppageMins = (int)((clockSeconds - 90.0f * 60.0f) / 60.0f) + 1;
        sprintf_s(g_scoreBug.stoppageStr, sizeof(g_scoreBug.stoppageStr), "+%d'", stoppageMins);
        g_scoreBug.isStoppageActive = true;
    }

    sprintf_s(g_scoreBug.clockStr, sizeof(g_scoreBug.clockStr), "%02d:%02d", totalMinutes, seconds);
    sprintf_s(g_scoreBug.scoreStr, sizeof(g_scoreBug.scoreStr), "%d - %d", homeGoals, awayGoals);

    // Live Analytics integration
    const analytics::MatchAnalytics& a = analytics::getAnalytics();
    const ThemeStyle& s = getThemeStyle(g_activeTheme);
    if (s.showXG) {
        sprintf_s(g_scoreBug.xgStr, sizeof(g_scoreBug.xgStr), "xG %.2f - %.2f", a.homeXG, a.awayXG);
    } else {
        g_scoreBug.xgStr[0] = '\0';
    }

    if (s.showMomentum) {
        sprintf_s(g_scoreBug.momentumStr, sizeof(g_scoreBug.momentumStr), "%.0f%% - %.0f%%", a.homeMomentum, a.awayMomentum);
        g_scoreBug.homeMomentumPct = a.homeMomentum;
    } else {
        g_scoreBug.momentumStr[0] = '\0';
        g_scoreBug.homeMomentumPct = 50.0f;
    }

    // Formatted header string
    if (g_scoreBug.isStoppageActive) {
        sprintf_s(g_scoreBug.formattedHeader, sizeof(g_scoreBug.formattedHeader),
                  "[%s] %s %s %s (%s %s)", g_scoreBug.badgeStr, g_scoreBug.homeAbbr,
                  g_scoreBug.scoreStr, g_scoreBug.awayAbbr, g_scoreBug.clockStr, g_scoreBug.stoppageStr);
    } else {
        sprintf_s(g_scoreBug.formattedHeader, sizeof(g_scoreBug.formattedHeader),
                  "[%s] %s %s %s (%s)", g_scoreBug.badgeStr, g_scoreBug.homeAbbr,
                  g_scoreBug.scoreStr, g_scoreBug.awayAbbr, g_scoreBug.clockStr);
    }

    if (g_scoreBug.isGoalBannerActive) {
        g_scoreBug.goalBannerTimer -= 0.25f;
        if (g_scoreBug.goalBannerTimer <= 0.0f) {
            g_scoreBug.isGoalBannerActive = false;
            g_scoreBug.goalBannerTimer = 0.0f;
        }
    }
}

void triggerGoalBanner(bool isHomeTeam) {
    (void)isHomeTeam;
    // Duplicate prevention: avoid resetting banner during active celebration
    if (g_scoreBug.isGoalBannerActive && g_scoreBug.goalBannerTimer > 3.0f) {
        return;
    }
    g_scoreBug.isGoalBannerActive = true;
    g_scoreBug.goalBannerTimer = 8.0f;  // Show for 8 seconds
}

const BroadcastScoreBug& getScoreBug() {
    return g_scoreBug;
}

}  // namespace graphics
}  // namespace remaster
