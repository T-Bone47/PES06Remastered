#ifndef REMASTER_GRAPHICS_MODERN_HUD_H
#define REMASTER_GRAPHICS_MODERN_HUD_H

namespace remaster {
namespace graphics {

// Supported authentic television broadcast scoreboard themes
enum BroadcastTheme {
    THEME_UCL_2006 = 0,       // 2006 UEFA Champions League: Navy/Cyan starball styling
    THEME_SKY_SPORTS_2006 = 1,// 2006 Sky Sports Super Sunday: Navy/Yellow/Red high-contrast
    THEME_MOTD_VINTAGE = 2,   // BBC Match of the Day: Classic black/gold/white minimalist presentation
    THEME_WC_2006 = 3,        // 2006 FIFA World Cup Germany: Green/White/Black global tournament
    THEME_MODERN_GLASS = 4    // Modern translucent frosted glass aesthetic
};

// Visual styling parameters for a broadcast theme
struct ThemeStyle {
    BroadcastTheme themeId;
    char name[32];             // Display name, e.g. "2006 UEFA Champions League"
    char key[16];              // INI key, e.g. "ucl"
    unsigned int primaryColor; // Primary background (ARGB)
    unsigned int secondaryColor;// Secondary / highlight border (ARGB)
    unsigned int accentColor;  // Accent / live status (ARGB)
    unsigned int fontColor;    // Text color (ARGB)
    unsigned int clockBgColor; // Clock background (ARGB)
    unsigned int scoreBgColor; // Score container background (ARGB)
    char badgeTag[16];         // Screen bug tag e.g. "UCL", "SKY", "MOTD", "WC06"
    bool showXG;               // Display Expected Goals
    bool showMomentum;         // Display match momentum
};

// Structure holding the pre-formatted TV broadcast HUD presentation
struct BroadcastScoreBug {
    BroadcastTheme currentTheme; // Active theme
    char themeName[32];          // e.g. "2006 UEFA Champions League"
    char themeKey[16];           // e.g. "ucl"
    unsigned int primaryColor;   // Active background color (ARGB)
    unsigned int secondaryColor; // Active accent color (ARGB)
    unsigned int accentColor;    // Active alert color (ARGB)
    unsigned int fontColor;      // Active text color (ARGB)
    unsigned int clockBgColor;   // Active clock container color (ARGB)
    unsigned int scoreBgColor;   // Active score container color (ARGB)
    char badgeStr[16];           // e.g. "UCL"
    char formattedHeader[64];    // e.g. "[UCL] ARS 1 - 0 CHE (42:15)"
    char clockStr[16];           // Formatted clock, e.g. "42:15"
    char stoppageStr[16];        // Stoppage indicator, e.g. "+3'" or empty
    char homeAbbr[8];            // e.g. "ARS"
    char awayAbbr[8];            // e.g. "CHE"
    char scoreStr[16];           // e.g. "1 - 0"
    char matchPeriodStr[16];     // "1ST HALF", "2ND HALF", "HT", "FT"
    char xgStr[32];              // "xG: 1.45 - 0.72"
    char momentumStr[32];        // "Momentum: 62% - 38%"
    float homeMomentumPct;       // 0.0 to 100.0
    bool isStoppageActive;
    bool isGoalBannerActive;
    float goalBannerTimer;
};

// Parses a theme name or key string into BroadcastTheme enum
BroadcastTheme parseBroadcastTheme(const char* str);

// Retrieves human-readable name for a theme
const char* getBroadcastThemeName(BroadcastTheme theme);

// Retrieves config key for a theme
const char* getBroadcastThemeKey(BroadcastTheme theme);

// Retrieves the theme styling parameters
const ThemeStyle& getThemeStyle(BroadcastTheme theme);

// Sets the active broadcast theme
void setBroadcastTheme(BroadcastTheme theme);

// Retrieves the active broadcast theme
BroadcastTheme getBroadcastTheme();

// Initializes the Modern HUD system
bool initModernHud();

// Updates modern broadcast score bug layout based on live match state & analytics
void updateModernHud(float clockSeconds, int phase, int homeGoals, int awayGoals,
                     const char* homeTeam, const char* awayTeam);

// Triggers goal banner animation on match event
void triggerGoalBanner(bool isHomeTeam);

// Retrieves current broadcast score bug layout
const BroadcastScoreBug& getScoreBug();

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_MODERN_HUD_H
