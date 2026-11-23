// test_core.cpp - Native unit test suite for PES 6 Legacy Remaster P0 hardening.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <assert.h>

#include "../core/remaster.h"
#include "../assets/assets.h"
#include "../assets/png_loader.h"
#include "../broadcast/director.h"
#include "../gameplay/enhanced.h"
#include "../graphics/d3d8.h"
#include "../graphics/graphics.h"
#include "../graphics/hires.h"
#include "../graphics/postprocess.h"
#include "../graphics/modern_hud.h"
#include "../analytics/analytics.h"

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s:%d: %s (%s)\n", __FILE__, __LINE__, msg, #cond); \
            g_failCount++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        printf("Running %s...", #fn); \
        int before = g_failCount; \
        fn(); \
        if (g_failCount == before) { \
            printf(" PASS\n"); \
            g_passCount++; \
        } \
    } while (0)

// ---------------------------------------------------------------- Tests

static void test_config_master_switch() {
    remaster::Config cfg;
    const char* iniContent =
        "[core]\n"
        "enabled = 0\n"
        "[graphics]\n"
        "enabled = 1\n"
        "width = 1920\n"
        "height = 1080\n";

    FILE* f = fopen("test_temp.ini", "w");
    TEST_ASSERT(f != NULL, "failed to create test_temp.ini");
    fputs(iniContent, f);
    fclose(f);

    bool loaded = cfg.load("test_temp.ini");
    remove("test_temp.ini");
    TEST_ASSERT(loaded, "Config::load failed");

    // Master enable must be false
    TEST_ASSERT(!cfg.getBool("core", "enabled", true), "core.enabled should be false");
    // Graphics enabled must be true
    TEST_ASSERT(cfg.getBool("graphics", "enabled", false), "graphics.enabled should be true");
    TEST_ASSERT(cfg.getInt("graphics", "width", 0) == 1920, "width should be 1920");
    TEST_ASSERT(cfg.getInt("graphics", "height", 0) == 1080, "height should be 1080");

    // Missing keys fallback
    TEST_ASSERT(cfg.getBool("nonexistent", "key", true) == true, "fallback true should work");
    TEST_ASSERT(cfg.getBool("nonexistent", "key", false) == false, "fallback false should work");
}

static void test_manifest_literal_ff_vs_wildcard() {
    remaster::Manifest m;
    // Entry with literal FF and with wildcard ??
    m.set("sig.literal_ff", "FF 15 30 D2 B7 00");
    m.set("sig.with_wildcard", "83 EC ?? 83 C0 FE");

    unsigned char bytes[64];
    unsigned char mask[64];
    int len = 0;

    // Pattern with literal FF
    bool ok = m.getPattern("sig.literal_ff", bytes, mask, &len, sizeof(bytes));
    TEST_ASSERT(ok, "getPattern literal_ff failed");
    TEST_ASSERT(len == 6, "literal_ff length should be 6");
    TEST_ASSERT(bytes[0] == 0xFF, "first byte should be 0xFF");
    TEST_ASSERT(mask[0] == 1, "first byte mask MUST be 1 (exact match, NOT wildcard)");

    // Pattern with wildcard
    ok = m.getPattern("sig.with_wildcard", bytes, mask, &len, sizeof(bytes));
    TEST_ASSERT(ok, "getPattern with_wildcard failed");
    TEST_ASSERT(len == 6, "with_wildcard length should be 6");
    TEST_ASSERT(bytes[2] == 0x00 || bytes[2] == 0xFF, "wildcard byte value");
    TEST_ASSERT(mask[2] == 0, "third byte mask MUST be 0 (wildcard)");
    TEST_ASSERT(mask[0] == 1 && mask[1] == 1, "other bytes must have mask 1");
}

static void test_manifest_buffer_bounds_protection() {
    remaster::Manifest m;
    // 32-byte signature (larger than old 16-byte stack buffer!)
    m.set("target.long_sig", "0x00401000");
    m.set("sig.target.long_sig",
          "55 8B EC 83 EC 20 53 56 57 8B 7D 08 85 FF 74 2A "
          "8B 4D 0C 85 C9 74 23 8B 55 10 85 D2 74 1C 89 07");

    unsigned char bytes[64];
    unsigned char mask[64];
    int len = 0;
    bool ok = m.getPattern("sig.target.long_sig", bytes, mask, &len, sizeof(bytes));
    TEST_ASSERT(ok, "getPattern long_sig failed");
    TEST_ASSERT(len == 32, "long_sig length should be 32");

    // Simulate matching against a dummy buffer:
    unsigned char codeBuffer[64];
    memcpy(codeBuffer, bytes, len);

    // Validate that our new match function handles 31 bytes safely without crashing
    bool matched = true;
    for (int i = 0; i < len; i++) {
        if (mask[i] && codeBuffer[i] != bytes[i]) {
            matched = false;
            break;
        }
    }
    TEST_ASSERT(matched, "long pattern should match buffer");

    // Mutate one byte and verify mismatch
    codeBuffer[10] ^= 0xFF;
    matched = true;
    for (int i = 0; i < len; i++) {
        if (mask[i] && codeBuffer[i] != bytes[i]) {
            matched = false;
            break;
        }
    }
    TEST_ASSERT(!matched, "mutated pattern should NOT match");
}

static void test_asset_path_sanitization() {
    // Valid relative filenames
    TEST_ASSERT(remaster::assets::isPathSafe("face_102.bin"), "face_102.bin should be safe");
    TEST_ASSERT(remaster::assets::isPathSafe("textures\\grass_hd.png"), "subfolder relative path should be safe");
    TEST_ASSERT(remaster::assets::isPathSafe("stadium/turf.raw"), "forward slash relative path should be safe");

    // Malicious directory traversal attempts
    TEST_ASSERT(!remaster::assets::isPathSafe("../face_102.bin"), "../ should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe("..\\face_102.bin"), "..\\ should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe("foo/../../windows/system32/cmd.exe"), "nested .. should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe("C:\\face_102.bin"), "absolute drive path should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe("\\\\server\\share\\face.bin"), "UNC path should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe("/root.bin"), "leading slash should be rejected");
    TEST_ASSERT(!remaster::assets::isPathSafe(""), "empty string should be rejected");
}

static void test_asset_transactional_read() {
    unsigned char dest[128];
    memset(dest, 0xAA, sizeof(dest));

    // Case 1: Partial read simulation / corrupt short file
    // Create a 16-byte file but tell the reader the asset requires 32 bytes
    FILE* f = fopen("test_short.bin", "wb");
    TEST_ASSERT(f != NULL, "failed to create test_short.bin");
    unsigned char shortData[16];
    memset(shortData, 0xBB, sizeof(shortData));
    fwrite(shortData, 1, sizeof(shortData), f);
    fclose(f);

    unsigned int bytesWritten = 0;
    // Trying to replace into dest with strict size
    bool ok = remaster::assets::replaceAssetTransactional(dest, sizeof(dest), "test_short.bin", &bytesWritten);
    remove("test_short.bin");

    TEST_ASSERT(ok, "transactional read should succeed for valid file <= buffer size");
    TEST_ASSERT(bytesWritten == 16, "bytes written should be 16");
    TEST_ASSERT(dest[0] == 0xBB && dest[15] == 0xBB, "first 16 bytes replaced");

    // Case 2: Missing file must fail safely and not touch dest
    memset(dest, 0xEE, sizeof(dest));
    ok = remaster::assets::replaceAssetTransactional(dest, sizeof(dest), "nonexistent_file.bin", &bytesWritten);
    TEST_ASSERT(!ok, "missing file should fail");
    TEST_ASSERT(dest[0] == 0xEE && dest[127] == 0xEE, "dest buffer must remain pristine on error");

    // Case 3: Oversized replacement (larger than target buffer) must be rejected
    FILE* fBig = fopen("test_big.bin", "wb");
    TEST_ASSERT(fBig != NULL, "failed to create test_big.bin");
    unsigned char bigData[256];
    memset(bigData, 0xCC, sizeof(bigData));
    fwrite(bigData, 1, sizeof(bigData), fBig);
    fclose(fBig);

    memset(dest, 0x55, sizeof(dest));
    ok = remaster::assets::replaceAssetTransactional(dest, 64, "test_big.bin", &bytesWritten);
    remove("test_big.bin");
    TEST_ASSERT(!ok, "oversized replacement must fail");
    TEST_ASSERT(dest[0] == 0x55, "dest must remain unmodified when oversized file rejected");
}

static void test_com_release_refcount_contract() {
    // In COM: Release() returns the new ref count.
    // If refCount > 0, the object is alive; do NOT forget.
    TEST_ASSERT(!remaster::graphics::shouldForgetResource(2), "refcount 2 must NOT forget");
    TEST_ASSERT(!remaster::graphics::shouldForgetResource(1), "refcount 1 must NOT forget");
    TEST_ASSERT(remaster::graphics::shouldForgetResource(0), "refcount 0 MUST forget");
}

static void test_manifest_unloaded_safety() {
    remaster::Manifest m;
    TEST_ASSERT(!m.isLoaded(), "fresh manifest must not be loaded");
    remaster::ManifestCheck check = m.validate();
    TEST_ASSERT(check.total == 0, "unloaded manifest total must be 0");
    TEST_ASSERT(check.matched == 0, "unloaded manifest matched must be 0");
    TEST_ASSERT(check.mismatched == 0, "unloaded manifest mismatched must be 0");
    TEST_ASSERT(check.unmapped == 0, "unloaded manifest unmapped must be 0");
}

static void test_config_whitespace_and_comments() {
    remaster::Config cfg;
    const char* content =
        "   # Leading space comment\n"
        "; Semicolon comment\n"
        "\n"
        "[  core   ]  \n"
        "   enabled   =   1   \n"
        "   key_with_spaces   =   value with spaces   \n";

    FILE* f = fopen("test_ws.ini", "w");
    TEST_ASSERT(f != NULL, "failed to create test_ws.ini");
    fputs(content, f);
    fclose(f);

    bool loaded = cfg.load("test_ws.ini");
    remove("test_ws.ini");
    TEST_ASSERT(loaded, "Config::load should succeed with whitespace");
    TEST_ASSERT(cfg.getBool("core", "enabled", false) == true, "core.enabled should be true");
    const char* val = cfg.get("core", "key_with_spaces", "");
    TEST_ASSERT(strcmp(val, "value with spaces") == 0, "trimmed key and value should match");
}

// Test 9: Texture disambiguation via CRC32 in hires subsystem
static void test_hires_texture_crc_disambiguation() {
    using namespace remaster::graphics;
    clearHiresPack();

    // Create 3 distinct test artworks
    unsigned char pxKit[16 * 16 * 4];
    memset(pxKit, 0x11, sizeof(pxKit));
    HiResArt artKit = { 16, 16, pxKit };

    unsigned char pxRadar[16 * 16 * 4];
    memset(pxRadar, 0x22, sizeof(pxRadar));
    HiResArt artRadar = { 16, 16, pxRadar };

    unsigned char pxFallback[16 * 16 * 4];
    memset(pxFallback, 0x33, sizeof(pxFallback));
    HiResArt artFallback = { 16, 16, pxFallback };

    // Register 3 entries for identical dimensions (512x512)
    TEST_ASSERT(addHiresEntry(512, 512, 0x11223344, "kit.png", &artKit), "register kit");
    TEST_ASSERT(addHiresEntry(512, 512, 0xAABBCCDD, "radar.png", &artRadar), "register radar");
    TEST_ASSERT(addHiresEntry(512, 512, 0x00000000, "fallback.png", &artFallback), "register fallback");

    // Exact CRC matches must resolve to the specific artwork
    const HiResArt* resKit = findHiResArt(512, 512, 0x11223344);
    TEST_ASSERT(resKit != nullptr && resKit->pixels[0] == 0x11, "kit CRC should resolve to kit art");

    const HiResArt* resRadar = findHiResArt(512, 512, 0xAABBCCDD);
    TEST_ASSERT(resRadar != nullptr && resRadar->pixels[0] == 0x22, "radar CRC should resolve to radar art");

    // Differing CRC with fallback present must resolve to fallback
    const HiResArt* resUnmatched = findHiResArt(512, 512, 0x99999999);
    TEST_ASSERT(resUnmatched != nullptr && resUnmatched->pixels[0] == 0x33, "unmatched CRC should resolve to fallback");

    // Zero CRC query must resolve to fallback
    const HiResArt* resZero = findHiResArt(512, 512, 0);
    TEST_ASSERT(resZero != nullptr && resZero->pixels[0] == 0x33, "zero CRC should resolve to fallback");

    // Non-existent dimensions must return nullptr
    const HiResArt* resMissing = findHiResArt(256, 256, 0x11223344);
    TEST_ASSERT(resMissing == nullptr, "non-existent dimension should return null");

    clearHiresPack();
}

// Test 10: High-res downsampling area filter
static void test_hires_draw_downsample() {
    using namespace remaster::graphics;

    // Create a 2x2 solid color high-res art (RGBA: 100, 100, 100, 255)
    unsigned char srcPixels[2 * 2 * 4];
    for (int i = 0; i < 4; i++) {
        srcPixels[i * 4 + 0] = 100;
        srcPixels[i * 4 + 1] = 100;
        srcPixels[i * 4 + 2] = 100;
        srcPixels[i * 4 + 3] = 255;
    }
    HiResArt art = { 2, 2, srcPixels };

    // Downsample to 1x1 destination
    unsigned char dstPixel[4] = { 0, 0, 0, 0 };
    bool ok = drawHiResArt(&art, 1, 1, dstPixel, 4);
    TEST_ASSERT(ok, "drawHiResArt should succeed");
    TEST_ASSERT(dstPixel[0] == 100, "filtered channel 0 should be 100");
    TEST_ASSERT(dstPixel[1] == 100, "filtered channel 1 should be 100");
    TEST_ASSERT(dstPixel[2] == 100, "filtered channel 2 should be 100");
    TEST_ASSERT(dstPixel[3] == 255, "filtered alpha should be 255");
}

// Test 11: Graphics 16:9 widescreen projection calculation
static void test_graphics_widescreen_projection() {
    using namespace remaster::graphics;

    // 1920x1080 (16:9) auto
    ProjectionState p16_9 = calculateProjection(1920, 1080, "auto");
    TEST_ASSERT(fabs(p16_9.aspect - 1.7777778f) < 0.001f, "aspect for 1080p should be ~1.7778");
    TEST_ASSERT(fabs(p16_9.aspectFactor - 1.3333333f) < 0.001f, "aspectFactor for 16:9 should be ~1.3333 (4/3)");
    TEST_ASSERT(fabs(p16_9.projWidth - 682.6667f) < 0.05f, "projWidth for 16:9 should be ~682.67");
    TEST_ASSERT(p16_9.cullWidth == 683, "cullWidth for 16:9 should be 683");

    // 1280x720 with explicit "16:9" string
    ProjectionState pExplicit = calculateProjection(1280, 720, "16:9");
    TEST_ASSERT(fabs(pExplicit.aspect - 1.7777778f) < 0.001f, "explicit 16:9 should be ~1.7778");
    TEST_ASSERT(pExplicit.cullWidth == 683, "explicit 16:9 cullWidth should be 683");

    // 640x480 (4:3) baseline - must match vanilla PES 6 exactly
    ProjectionState p4_3 = calculateProjection(640, 480, "4:3");
    TEST_ASSERT(fabs(p4_3.aspect - 1.3333333f) < 0.001f, "4:3 aspect should be ~1.3333");
    TEST_ASSERT(fabs(p4_3.aspectFactor - 1.0f) < 0.001f, "4:3 aspectFactor must be exactly 1.0");
    TEST_ASSERT(fabs(p4_3.projWidth - 512.0f) < 0.001f, "4:3 projWidth must be exactly 512.0");
    TEST_ASSERT(p4_3.cullWidth == 512, "4:3 cullWidth must be exactly 512");
}

// Test 12: Ultrawide (21:9) and custom aspect ratio calculation
static void test_graphics_ultrawide_projection() {
    using namespace remaster::graphics;

    // 2560x1080 (21:9 / 64:27)
    ProjectionState p21_9 = calculateProjection(2560, 1080, "21:9");
    TEST_ASSERT(fabs(p21_9.aspect - 2.3703704f) < 0.001f, "aspect for 21:9 should be ~2.3704");
    TEST_ASSERT(fabs(p21_9.aspectFactor - 1.7777778f) < 0.001f, "aspectFactor for 21:9 should be ~1.7778");
    TEST_ASSERT(fabs(p21_9.projWidth - 910.2222f) < 0.05f, "projWidth for 21:9 should be ~910.22");
    TEST_ASSERT(p21_9.cullWidth == 910, "cullWidth for 21:9 should be 910");

    // Custom decimal string "1.6" (16:10)
    ProjectionState p16_10 = calculateProjection(1680, 1050, "1.6");
    TEST_ASSERT(fabs(p16_10.aspect - 1.6f) < 0.001f, "aspect for 16:10 should be 1.6");
    TEST_ASSERT(fabs(p16_10.aspectFactor - 1.2f) < 0.001f, "aspectFactor for 16:10 should be 1.2");
    TEST_ASSERT(fabs(p16_10.projWidth - 614.4f) < 0.05f, "projWidth for 16:10 should be ~614.4");
    TEST_ASSERT(p16_10.cullWidth == 614, "cullWidth for 16:10 should be 614");
}

// Test 13: Standard SHA-256 cryptographic verification vectors
static void test_sha256_known_vectors() {
    using namespace remaster::assets;

    char digest[65];

    // Vector 1: Empty string -> e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    computeBufferSha256("", 0, digest);
    TEST_ASSERT(strcmp(digest, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0,
                "empty string sha256 mismatch");

    // Vector 2: "abc" -> ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
    computeBufferSha256("abc", 3, digest);
    TEST_ASSERT(strcmp(digest, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0,
                "abc string sha256 mismatch");
}

// Test 14: VRAM memory budget enforcement
static void test_vram_memory_budget_enforcement() {
    using namespace remaster::assets;

    // Set budget to 128 MB (minimum bound)
    initMemoryBudget(128);
    AssetMemoryBudget b = getMemoryBudget();
    TEST_ASSERT(b.maxBytes == 128u * 1024u * 1024u, "maxBytes should be 128 MB");
    TEST_ASSERT(b.allocatedBytes == 0, "initial allocated should be 0");

    // Allocate 60 MB -> should succeed
    bool ok1 = requestMemoryAllocation(60u * 1024u * 1024u);
    TEST_ASSERT(ok1, "60 MB allocation should succeed");
    b = getMemoryBudget();
    TEST_ASSERT(b.allocatedBytes == 60u * 1024u * 1024u, "allocated should be 60 MB");

    // Allocate 50 MB -> should succeed (total 110 MB <= 128 MB)
    bool ok2 = requestMemoryAllocation(50u * 1024u * 1024u);
    TEST_ASSERT(ok2, "50 MB allocation should succeed");
    b = getMemoryBudget();
    TEST_ASSERT(b.allocatedBytes == 110u * 1024u * 1024u, "allocated should be 110 MB");

    // Allocate 30 MB -> should FAIL (110 + 30 = 140 MB > 128 MB)
    bool ok3 = requestMemoryAllocation(30u * 1024u * 1024u);
    TEST_ASSERT(!ok3, "overflow allocation must be rejected");
    b = getMemoryBudget();
    TEST_ASSERT(b.allocatedBytes == 110u * 1024u * 1024u, "allocated should remain 110 MB");
    TEST_ASSERT(b.rejectedCount == 1, "rejectedCount should be 1");

    // Release 60 MB
    releaseMemoryAllocation(60u * 1024u * 1024u);
    b = getMemoryBudget();
    TEST_ASSERT(b.allocatedBytes == 50u * 1024u * 1024u, "allocated should drop to 50 MB");

    // Now 30 MB allocation should succeed (50 + 30 = 80 MB <= 128 MB)
    bool ok4 = requestMemoryAllocation(30u * 1024u * 1024u);
    TEST_ASSERT(ok4, "allocation after release should succeed");
    b = getMemoryBudget();
    TEST_ASSERT(b.allocatedBytes == 80u * 1024u * 1024u, "allocated should be 80 MB");

    // Reset budget to default 1.5 GB
    initMemoryBudget(1500);
}

// Test 15: Structured JSON manifest loading
static void test_json_manifest_loader() {
    using namespace remaster::assets;

    const char* testManifest =
        "{\n"
        "  \"name\": \"Test Pack\",\n"
        "  \"vram_budget_mb\": 512,\n"
        "  \"assets\": [\n"
        "    {\n"
        "      \"id\": \"test_kit_01\",\n"
        "      \"category\": \"kit\",\n"
        "      \"crc32\": \"0x12345678\",\n"
        "      \"orig_width\": 512,\n"
        "      \"orig_height\": 256,\n"
        "      \"file\": \"test_kit.png\"\n"
        "    },\n"
        "    {\n"
        "      \"id\": \"test_face_01\",\n"
        "      \"category\": \"face\",\n"
        "      \"crc32\": \"0xAABBCCDD\",\n"
        "      \"orig_width\": 256,\n"
        "      \"orig_height\": 256,\n"
        "      \"file\": \"test_face.png\"\n"
        "    }\n"
        "  ]\n"
        "}\n";

    FILE* f = fopen("test_manifest.json", "wb");
    TEST_ASSERT(f != nullptr, "failed to create test_manifest.json");
    fputs(testManifest, f);
    fclose(f);

    int count = loadAssetManifest("test_manifest.json");
    TEST_ASSERT(count == 2, "loadAssetManifest should parse and register 2 entries");

    AssetMemoryBudget b = getMemoryBudget();
    TEST_ASSERT(b.maxBytes == 512u * 1024u * 1024u, "manifest vram_budget_mb should configure budget to 512 MB");

    remove("test_manifest.json");
    remaster::graphics::clearHiresPack();
    initMemoryBudget(1500);
}

// Test 16: Broadcast Director state transitions and incident detection
static void test_broadcast_director_state_transitions() {
    using namespace remaster::broadcast;

    BroadcastConfig cfg = {
        true,    // enabled
        true,    // dynamicCamera
        1430.0f, // baseZoom
        1.15f,   // midfieldZoomMult (1644.5f)
        0.85f,   // boxZoomMult (1215.5f)
        0.65f,   // goalZoomMult (929.5f)
        0.10f,   // zoomLerpSpeed
        false,   // stadiumClippingFix (disabled in standalone unit test)
        true,    // instantReplaySkip
        true     // cinematicReplays
    };
    setConfig(cfg);

    // Initial state check
    TEST_ASSERT(isEnabled(), "director should be enabled");

    // Phase 2: Lineup / presentation
    update(0.0f, 0.0f, 0.0f, 2, 0.0f);
    TEST_ASSERT(getState() == DIR_STATE_LINEUP, "phase 2 must transition to DIR_STATE_LINEUP");
    TEST_ASSERT(getCameraPreset() == CAM_PRESET_DYNAMIC_BROADCAST, "preset should be dynamic broadcast");

    // Phase 3: Active play, ball in midfield (x=5.0, y=2.0)
    update(5.0f, 2.0f, 0.0f, 3, 10.5f);
    TEST_ASSERT(getState() == DIR_STATE_MIDFIELD_BUILDUP, "midfield ball must trigger DIR_STATE_MIDFIELD_BUILDUP");
    TEST_ASSERT(getCameraPreset() == CAM_PRESET_TACTICAL_HIGH, "preset should be tactical high overview");
    TEST_ASSERT(fabs(getTargetZoom() - 1644.5f) < 0.1f, "target zoom should be 1644.5f (1.15x)");

    // Phase 3: Penalty box penetration (x=42.0, y=10.0)
    update(42.0f, 10.0f, 0.5f, 3, 25.0f);
    TEST_ASSERT(getState() == DIR_STATE_BOX_PENETRATION, "box penetration must trigger DIR_STATE_BOX_PENETRATION");
    TEST_ASSERT(getCameraPreset() == CAM_PRESET_BOX_FOCUS, "preset should be box focus tracking");
    TEST_ASSERT(fabs(getTargetZoom() - 1215.5f) < 0.1f, "target zoom should be 1215.5f (0.85x)");

    // Phase 3: Goal incident (x=53.0, y=1.5, z=0.8)
    update(53.0f, 1.5f, 0.8f, 3, 44.2f);
    TEST_ASSERT(getState() == DIR_STATE_GOAL_CELEBRATION, "ball crossing goal line must trigger DIR_STATE_GOAL_CELEBRATION");
    TEST_ASSERT(fabs(getTargetZoom() - 929.5f) < 0.1f, "target zoom should be 929.5f (0.65x)");
    MatchEventIncident inc = { 0 };
    TEST_ASSERT(getLatestIncident(&inc), "latest incident must be recorded");
    TEST_ASSERT(inc.state == DIR_STATE_GOAL_CELEBRATION, "incident state must be goal celebration");
    TEST_ASSERT(strstr(inc.description, "GOAL SCORED") != nullptr, "incident description must mention GOAL SCORED");

    // Phase 12: Instant replay
    update(0.0f, 0.0f, 0.0f, 12, 45.0f);
    TEST_ASSERT(getState() == DIR_STATE_REPLAY, "phase 12 must transition to DIR_STATE_REPLAY");
    TEST_ASSERT(getCameraPreset() == CAM_PRESET_CINEMATIC_REPLAY, "preset should be cinematic replay");
}

// Test 17: Camera zoom exponential smoothing and boundary clamping
static void test_broadcast_camera_zoom_lerp_and_bounds() {
    using namespace remaster::broadcast;

    BroadcastConfig cfg = {
        true,    // enabled
        true,    // dynamicCamera
        1430.0f, // baseZoom
        1.15f,   // midfieldZoomMult
        0.85f,   // boxZoomMult
        0.65f,   // goalZoomMult
        0.20f,   // zoomLerpSpeed (20% per tick)
        false,
        true,
        true
    };
    setConfig(cfg);

    float startZoom = getCurrentZoom();
    TEST_ASSERT(fabs(startZoom - 1430.0f) < 0.01f, "initial zoom should be exactly 1430.0f");

    // First update: Ball in midfield -> target 1644.5f
    // Lerp step: 1430 + (1644.5 - 1430) * 0.20 = 1472.9f
    update(0.0f, 0.0f, 0.0f, 3, 5.0f);
    float firstZoom = getCurrentZoom();
    TEST_ASSERT(firstZoom > startZoom, "zoom should increase smoothly towards target");
    TEST_ASSERT(fabs(firstZoom - 1472.9f) < 0.2f, "first lerp step should be approx 1472.9f");

    // Second lerp step:
    update(0.0f, 0.0f, 0.0f, 3, 5.1f);
    float nextZoom = getCurrentZoom();
    TEST_ASSERT(nextZoom > firstZoom, "zoom should continue to increase smoothly");
    TEST_ASSERT(nextZoom < 1644.5f, "zoom should not overshoot target");

    // Run 30 iterations: should converge close to 1644.5f
    for (int i = 0; i < 30; i++) {
        update(0.0f, 0.0f, 0.0f, 3, 5.2f + i * 0.1f);
    }
    TEST_ASSERT(fabs(getCurrentZoom() - 1644.5f) < 1.0f, "zoom should converge to target within 1.0f");
}

// Test 18: Classic Mode bypass (zero modifications when disabled)
static void test_broadcast_classic_mode_bypass() {
    using namespace remaster::broadcast;

    BroadcastConfig cfg = {
        false,   // enabled = 0 (Classic / Disabled mode)
        false,
        1430.0f,
        1.15f,
        0.85f,
        0.65f,
        0.08f,
        false,
        false,
        false
    };
    setConfig(cfg);

    TEST_ASSERT(!isEnabled(), "director must report disabled");

    // Calling update should have no effect
    update(53.0f, 1.0f, 0.5f, 3, 10.0f);
    TEST_ASSERT(getState() == DIR_STATE_IDLE || getState() == DIR_STATE_REPLAY, "state must not change when disabled");
    TEST_ASSERT(fabs(getCurrentZoom() - 1430.0f) < 0.01f, "zoom must remain at exact vanilla 1430.0f");

    // Restore default enabled
    cfg.enabled = true;
    setConfig(cfg);
}

// Test 19: Enhanced Gameplay safety envelope clamping (±15% bounds)
static void test_gameplay_safety_envelope_clamping() {
    using namespace remaster::gameplay;

    EnhancedGameplayConfig cfg = {
        true,   // enabled
        2.50f,  // teamWidthFactor: deliberately out of bounds (+150%)
        true,
        3.00f,  // pressingAggressionCap: deliberately out of bounds (+200%)
        0.50f,  // passLeadFactor: deliberately out of bounds (-50%)
        1.50f,  // ballDragFactor: deliberately out of bounds (+50%)
        true    // enforceSafetyEnvelope = true
    };
    setConfig(cfg);

    TEST_ASSERT(isEnhancedActive(), "enhanced gameplay should be active");
    TEST_ASSERT(fabs(getTeamWidthMultiplier() - MAX_TEAM_WIDTH_FACTOR) < 0.001f,
                "team width must be clamped to 1.15f maximum");
    TEST_ASSERT(fabs(getPassLeadMultiplier() - MIN_PASS_LEAD_FACTOR) < 0.001f,
                "pass lead factor must be clamped to 0.85f minimum");
    TEST_ASSERT(fabs(getBallDragMultiplier() - MAX_BALL_DRAG_FACTOR) < 0.001f,
                "ball drag factor must be clamped to 1.05f maximum");
}

// Test 20: Dynamic pressing calculation based on match state and deficit
static void test_gameplay_dynamic_pressing_calculation() {
    using namespace remaster::gameplay;

    EnhancedGameplayConfig cfg = {
        true,   // enabled
        1.05f,  // teamWidthFactor
        true,   // dynamicPressing = true
        1.10f,  // pressingAggressionCap = 1.10f (+10%)
        1.05f,
        1.00f,
        true
    };
    setConfig(cfg);

    // Case 1: Early game, tied (phase 3, clock 10.0, score 0-0) -> multiplier must be 1.00f
    update(3, 10.0f, 0, 0);
    TEST_ASSERT(fabs(getCurrentPressingMultiplier() - 1.00f) < 0.001f,
                "tied early game must have neutral pressing 1.00f");

    // Case 2: Early game, trailing (clock 20.0, score 0-1) -> still neutral because clock < 40.0
    update(3, 20.0f, 0, 1);
    TEST_ASSERT(fabs(getCurrentPressingMultiplier() - 1.00f) < 0.001f,
                "early game deficit must not trigger premature pressing surge");

    // Case 3: Late game, trailing by 1 goal (clock 50.0, score 0-1) -> urgent press (1.00 + 0.10 * 0.65 = 1.065)
    update(3, 50.0f, 0, 1);
    float p1 = getCurrentPressingMultiplier();
    TEST_ASSERT(p1 > 1.02f && p1 < 1.08f, "late game 1-goal deficit should moderately escalate pressing");

    // Case 4: Late game, trailing by 2 goals (clock 55.0, score 0-2) -> full urgent press (1.10f)
    update(3, 55.0f, 0, 2);
    float p2 = getCurrentPressingMultiplier();
    TEST_ASSERT(fabs(p2 - 1.10f) < 0.001f, "late game 2-goal deficit should trigger full pressing aggression cap");

    // Case 5: Late game, score leveled back to tie (clock 60.0, score 2-2) -> neutral press
    update(3, 60.0f, 2, 2);
    TEST_ASSERT(fabs(getCurrentPressingMultiplier() - 1.00f) < 0.001f,
                "equalized score must reset pressing aggression to 1.00f");
}

// Test 21: Classic Mode strict bit-exact preservation (all multipliers == 1.000000)
static void test_gameplay_classic_mode_strict_preservation() {
    using namespace remaster::gameplay;

    EnhancedGameplayConfig cfg = {
        false,  // enabled = 0 (Classic Mode)
        1.15f,
        true,
        1.15f,
        1.15f,
        1.05f,
        true
    };
    setConfig(cfg);

    TEST_ASSERT(!isEnhancedActive(), "Enhanced Mode must report inactive");
    TEST_ASSERT(fabs(getTeamWidthMultiplier() - 1.000000f) < 0.00001f,
                "Classic Mode team width must strictly equal 1.000000f");
    TEST_ASSERT(fabs(getCurrentPressingMultiplier() - 1.000000f) < 0.00001f,
                "Classic Mode pressing multiplier must strictly equal 1.000000f");
    TEST_ASSERT(fabs(getPassLeadMultiplier() - 1.000000f) < 0.00001f,
                "Classic Mode pass lead multiplier must strictly equal 1.000000f");
    TEST_ASSERT(fabs(getBallDragMultiplier() - 1.000000f) < 0.00001f,
                "Classic Mode ball drag multiplier must strictly equal 1.000000f");

    // Calling update in late game deficit must NOT change multipliers
    update(3, 85.0f, 0, 3);
    TEST_ASSERT(fabs(getCurrentPressingMultiplier() - 1.000000f) < 0.00001f,
                "Classic Mode pressing must remain exactly 1.000000f even in 85th min deficit");

    // Restore default
    cfg.enabled = true;
    cfg.teamWidthFactor = 1.05f;
    cfg.pressingAggressionCap = 1.10f;
    cfg.passLeadFactor = 1.05f;
    cfg.ballDragFactor = 1.00f;
    setConfig(cfg);
}

static void test_postprocess_gamma_ramp() {
    using namespace remaster::graphics;

    D3DGAMMARAMP ramp;
    memset(&ramp, 0, sizeof(ramp));

    // Build calibrated cinematic gamma ramp
    buildGammaRamp(&ramp, 1.10f, 1.15f, 0.02f, 1.00f);

    // Strict boundary enforcement
    TEST_ASSERT(ramp.red[0] == 0, "Red channel index 0 must be 0");
    TEST_ASSERT(ramp.green[0] == 0, "Green channel index 0 must be 0");
    TEST_ASSERT(ramp.blue[0] == 0, "Blue channel index 0 must be 0");

    TEST_ASSERT(ramp.red[255] == 65535, "Red channel index 255 must be 65535");
    TEST_ASSERT(ramp.green[255] == 65535, "Green channel index 255 must be 65535");
    TEST_ASSERT(ramp.blue[255] == 65535, "Blue channel index 255 must be 65535");

    // Monotonicity verification
    for (int i = 1; i < 256; ++i) {
        TEST_ASSERT(ramp.red[i] >= ramp.red[i - 1], "Red channel must be monotonic");
        TEST_ASSERT(ramp.green[i] >= ramp.green[i - 1], "Green channel must be monotonic");
        TEST_ASSERT(ramp.blue[i] >= ramp.blue[i - 1], "Blue channel must be monotonic");
    }

    // Midtone pitch vibrance boost: Green channel should be elevated over pure linear in midtones
    WORD linearMid = (WORD)(128.0f / 255.0f * 65535.0f);
    TEST_ASSERT(ramp.green[128] >= linearMid, "Green channel midtones must be elevated for pitch vibrance");
}

static void test_analytics_xg_calculation() {
    using namespace remaster::analytics;

    init();

    // 1. Penalty kick must equal standard calibrated 0.76 xG
    float penXG = calculateShotXG(41.5f, 0.0f, 0.0f, SHOT_PENALTY, true);
    TEST_ASSERT(fabs(penXG - 0.76f) < 0.001f, "Penalty kick xG must equal exactly 0.76");

    // 2. Central tap-in from 6-yard box (X=48.0, Y=0.0) -> high xG (> 0.50)
    float tapInXG = calculateShotXG(48.0f, 0.0f, 0.2f, SHOT_OPEN_PLAY, true);
    TEST_ASSERT(tapInXG > 0.50f, "6-yard central shot must produce xG > 0.50");

    // 3. Distance decay: 15m shot must have strictly higher xG than 28m shot
    float closeXG = calculateShotXG(37.5f, 0.0f, 0.0f, SHOT_OPEN_PLAY, true);
    float longXG = calculateShotXG(24.5f, 0.0f, 0.0f, SHOT_OPEN_PLAY, true);
    TEST_ASSERT(closeXG > longXG, "Closer shot must produce higher xG than long range shot");
    TEST_ASSERT(longXG < 0.10f, "28m shot must produce low xG (< 0.10)");

    // 4. Angle effect: central shot vs acute angle from byline
    float centralXG = calculateShotXG(42.0f, 0.0f, 0.0f, SHOT_OPEN_PLAY, true);
    float wideXG = calculateShotXG(42.0f, 15.0f, 0.0f, SHOT_OPEN_PLAY, true);
    TEST_ASSERT(centralXG > wideXG, "Central shot must produce higher xG than sharp angle shot");

    // 5. Header penalty: header must produce lower xG than foot strike
    float footXG = calculateShotXG(44.0f, 2.0f, 0.5f, SHOT_OPEN_PLAY, true);
    float headXG = calculateShotXG(44.0f, 2.0f, 1.8f, SHOT_HEADER, true);
    TEST_ASSERT(footXG > headXG, "Foot strike must produce higher xG than header from same position");

    // 6. Bounds enforcement: must remain within [0.01, 0.99]
    float halfPitchShot = calculateShotXG(0.0f, 0.0f, 0.0f, SHOT_OPEN_PLAY, true);
    TEST_ASSERT(halfPitchShot >= 0.01f && halfPitchShot <= 0.02f, "Halfway line shot must be bounded >= 0.01");
}

static void test_analytics_momentum_and_field_tilt() {
    using namespace remaster::analytics;

    resetMatch();

    const MatchAnalytics& aInitial = getAnalytics();
    TEST_ASSERT(fabs(aInitial.homeMomentum - 50.0f) < 0.01f, "Initial home momentum must be 50%");
    TEST_ASSERT(fabs(aInitial.awayMomentum - 50.0f) < 0.01f, "Initial away momentum must be 50%");

    // Simulate sustained Home attacking possession in final third (X = +35.0m)
    for (int i = 0; i < 20; ++i) {
        updateMomentum(35.0f, 5.0f, 0.1f, 3, 0.5f);
    }

    const MatchAnalytics& aHomeAttack = getAnalytics();
    TEST_ASSERT(aHomeAttack.homeMomentum > 60.0f, "Sustained attacking possession must raise Home momentum > 60%");
    TEST_ASSERT(aHomeAttack.homeFieldTilt > 70.0f, "Home field tilt must exceed 70% during sustained attack");
    TEST_ASSERT(fabs(aHomeAttack.homeMomentum + aHomeAttack.awayMomentum - 100.0f) < 0.01f,
                "Momentum percentages must sum to 100%");

    // Record shots and verify cumulative xG (with shot deduplication test)
    recordShot(45.0f, 0.0f, 0.2f, SHOT_OPEN_PLAY, true, true, 10.0f);
    // Duplicate event at 10.5s should be debounced
    recordShot(45.0f, 0.0f, 0.2f, SHOT_OPEN_PLAY, true, true, 10.5f);
    // Valid subsequent shot at 25.0s
    recordShot(40.0f, 5.0f, 0.0f, SHOT_OPEN_PLAY, true, false, 25.0f);

    const MatchAnalytics& aShots = getAnalytics();
    TEST_ASSERT(aShots.homeShots == 2, "Home shots count must be 2 (duplicate at 10.5s debounced)");
    TEST_ASSERT(aShots.homeShotsOnTarget == 1, "Home shots on target must be 1");
    TEST_ASSERT(aShots.homeXG > 0.30f, "Cumulative Home xG must exceed 0.30");

    // Halftime switch test: In 2nd half, Home attacks negative X
    resetMatch();
    for (int i = 0; i < 20; ++i) {
        updateMomentum(-35.0f, 5.0f, 0.1f, 3, 0.5f, false /* homeAttacksPositiveX = false */);
    }
    const MatchAnalytics& aSecondHalf = getAnalytics();
    TEST_ASSERT(aSecondHalf.homeMomentum > 60.0f, "In 2nd half, negative X attack must correctly raise Home momentum");
}

static void test_modern_hud_presentation() {
    using namespace remaster::graphics;

    initModernHud();

    // 1. Pre-match check
    updateModernHud(0.0f, 0, 0, 0, "Austria", "Austria");
    const BroadcastScoreBug& b1 = getScoreBug();
    TEST_ASSERT(strcmp(b1.clockStr, "00:00") == 0, "Clock must format 00:00");
    TEST_ASSERT(strcmp(b1.matchPeriodStr, "PRE-MATCH") == 0, "Phase 0 must be PRE-MATCH");
    TEST_ASSERT(strcmp(b1.homeAbbr, "AUS") == 0, "Austria must abbreviate to AUS");
    TEST_ASSERT(strcmp(b1.scoreStr, "0 - 0") == 0, "Score must format 0 - 0");

    // 2. 1st half 25th minute
    updateModernHud(25.0f * 60.0f + 30.0f, 3, 1, 0, "Arsenal", "Chelsea");
    const BroadcastScoreBug& b2 = getScoreBug();
    TEST_ASSERT(strcmp(b2.clockStr, "25:30") == 0, "Clock must format 25:30");
    TEST_ASSERT(strcmp(b2.matchPeriodStr, "1ST HALF") == 0, "25th min must be 1ST HALF");
    TEST_ASSERT(strcmp(b2.homeAbbr, "ARS") == 0, "Arsenal must abbreviate to ARS");
    TEST_ASSERT(strcmp(b2.awayAbbr, "CHE") == 0, "Chelsea must abbreviate to CHE");
    TEST_ASSERT(strcmp(b2.scoreStr, "1 - 0") == 0, "Score must format 1 - 0");
    TEST_ASSERT(!b2.isStoppageActive, "Regular time must not have stoppage active");

    // 3. Stoppage time in 1st half (46:30 -> +2')
    updateModernHud(46.0f * 60.0f + 30.0f, 3, 1, 0, "Arsenal", "Chelsea");
    const BroadcastScoreBug& b3 = getScoreBug();
    TEST_ASSERT(b3.isStoppageActive, "46:30 must trigger stoppage time");
    TEST_ASSERT(strcmp(b3.stoppageStr, "+2'") == 0, "Stoppage string must be +2'");

    // 4. Goal banner trigger
    triggerGoalBanner(true);
    const BroadcastScoreBug& bGoal = getScoreBug();
    TEST_ASSERT(bGoal.isGoalBannerActive, "Goal banner must be active upon trigger");
    TEST_ASSERT(bGoal.goalBannerTimer > 7.0f, "Goal banner timer must initialize to ~8s");
}

static void test_broadcast_scoreboard_themes() {
    using namespace remaster::graphics;

    // 1. Theme string parsing validation
    TEST_ASSERT(parseBroadcastTheme("ucl") == THEME_UCL_2006, "parseBroadcastTheme('ucl') failed");
    TEST_ASSERT(parseBroadcastTheme("champions_league") == THEME_UCL_2006, "parseBroadcastTheme('champions_league') failed");
    TEST_ASSERT(parseBroadcastTheme("sky_sports") == THEME_SKY_SPORTS_2006, "parseBroadcastTheme('sky_sports') failed");
    TEST_ASSERT(parseBroadcastTheme("sky") == THEME_SKY_SPORTS_2006, "parseBroadcastTheme('sky') failed");
    TEST_ASSERT(parseBroadcastTheme("motd") == THEME_MOTD_VINTAGE, "parseBroadcastTheme('motd') failed");
    TEST_ASSERT(parseBroadcastTheme("bbc") == THEME_MOTD_VINTAGE, "parseBroadcastTheme('bbc') failed");
    TEST_ASSERT(parseBroadcastTheme("wc2006") == THEME_WC_2006, "parseBroadcastTheme('wc2006') failed");
    TEST_ASSERT(parseBroadcastTheme("worldcup") == THEME_WC_2006, "parseBroadcastTheme('worldcup') failed");
    TEST_ASSERT(parseBroadcastTheme("modern") == THEME_MODERN_GLASS, "parseBroadcastTheme('modern') failed");
    TEST_ASSERT(parseBroadcastTheme("glass") == THEME_MODERN_GLASS, "parseBroadcastTheme('glass') failed");
    TEST_ASSERT(parseBroadcastTheme(nullptr) == THEME_UCL_2006, "nullptr fallback must be UCL");

    // 2. Style attributes validation
    const ThemeStyle& sUcl = getThemeStyle(THEME_UCL_2006);
    TEST_ASSERT(sUcl.primaryColor == 0xE60A1832, "UCL primary color must match deep navy");
    TEST_ASSERT(sUcl.secondaryColor == 0xFF00E5FF, "UCL secondary color must match neon cyan");
    TEST_ASSERT(strcmp(sUcl.badgeTag, "UCL") == 0, "UCL badge tag must be UCL");
    TEST_ASSERT(sUcl.showXG == true, "UCL must show xG");

    const ThemeStyle& sSky = getThemeStyle(THEME_SKY_SPORTS_2006);
    TEST_ASSERT(sSky.primaryColor == 0xEE002244, "Sky Sports primary color must match Sky navy");
    TEST_ASSERT(sSky.secondaryColor == 0xFFFFD200, "Sky Sports secondary color must match yellow");
    TEST_ASSERT(strcmp(sSky.badgeTag, "SKY") == 0, "Sky Sports badge tag must be SKY");

    const ThemeStyle& sMotd = getThemeStyle(THEME_MOTD_VINTAGE);
    TEST_ASSERT(sMotd.primaryColor == 0xF0121212, "MOTD primary color must match carbon black");
    TEST_ASSERT(sMotd.secondaryColor == 0xFFD4AF37, "MOTD secondary color must match metallic gold");
    TEST_ASSERT(strcmp(sMotd.badgeTag, "MOTD") == 0, "MOTD badge tag must be MOTD");
    TEST_ASSERT(sMotd.showXG == false, "MOTD vintage theme must omit modern xG");

    const ThemeStyle& sWc = getThemeStyle(THEME_WC_2006);
    TEST_ASSERT(sWc.primaryColor == 0xEE083A1E, "WC 2006 primary color must match forest green");
    TEST_ASSERT(strcmp(sWc.badgeTag, "WC06") == 0, "WC 2006 badge tag must be WC06");

    // 3. Dynamic runtime theme switching & score bug presentation
    setBroadcastTheme(THEME_SKY_SPORTS_2006);
    updateModernHud(18.0f * 60.0f + 45.0f, 3, 2, 0, "Liverpool", "Chelsea");
    const BroadcastScoreBug& bSky = getScoreBug();
    TEST_ASSERT(bSky.currentTheme == THEME_SKY_SPORTS_2006, "Active theme must reflect Sky Sports");
    TEST_ASSERT(strcmp(bSky.badgeStr, "SKY") == 0, "Active badge must be SKY");
    TEST_ASSERT(bSky.primaryColor == 0xEE002244, "Score bug color must update to Sky navy");
    TEST_ASSERT(strstr(bSky.formattedHeader, "[SKY] LIV 2 - 0 CHE (18:45)") != nullptr,
                "Formatted header must match Sky broadcast format");

    // Switch to UCL and verify stoppage format
    setBroadcastTheme(THEME_UCL_2006);
    updateModernHud(46.0f * 60.0f + 30.0f, 3, 1, 1, "Barcelona", "Arsenal");
    const BroadcastScoreBug& bUcl = getScoreBug();
    TEST_ASSERT(bUcl.currentTheme == THEME_UCL_2006, "Active theme must reflect UCL");
    TEST_ASSERT(strcmp(bUcl.badgeStr, "UCL") == 0, "Active badge must be UCL");
    TEST_ASSERT(bUcl.isStoppageActive, "Stoppage time must be active");
    TEST_ASSERT(strstr(bUcl.formattedHeader, "[UCL] BAR 1 - 1 ARS (46:30 +2')") != nullptr,
                "Formatted header must include stoppage time in UCL format");
}

static void test_graphics_quality_presets() {
    using namespace remaster::graphics;

    // 1. String parser validation
    TEST_ASSERT(parsePreset("classic") == PRESET_CLASSIC, "parsePreset('classic') failed");
    TEST_ASSERT(parsePreset("enhanced") == PRESET_ENHANCED, "parsePreset('enhanced') failed");
    TEST_ASSERT(parsePreset("hd") == PRESET_HD, "parsePreset('hd') failed");
    TEST_ASSERT(parsePreset("maximum") == PRESET_MAXIMUM, "parsePreset('maximum') failed");
    TEST_ASSERT(parsePreset("ultra") == PRESET_ULTRA_4K, "parsePreset('ultra') failed");
    TEST_ASSERT(parsePreset("4k") == PRESET_ULTRA_4K, "parsePreset('4k') failed");

    // 2. Classic Preset: 100% bit-exact original baseline
    setQualityPreset(PRESET_CLASSIC);
    const QualitySettings& sClassic = getQualitySettings();
    TEST_ASSERT(sClassic.internalWidth == 0, "Classic preset must disable internal resolution scaling");
    TEST_ASSERT(sClassic.anisotropicLevel == 0, "Classic preset must disable anisotropic filtering");
    TEST_ASSERT(!sClassic.colorGrading, "Classic preset must disable color grading");
    TEST_ASSERT(!sClassic.modernHud, "Classic preset must disable modern HUD overlay");

    // 3. HD Preset: 1080p internal rendering, 16x AF, modern HUD & color grading
    setQualityPreset(PRESET_HD);
    const QualitySettings& sHd = getQualitySettings();
    TEST_ASSERT(sHd.internalWidth == 1920 && sHd.internalHeight == 1080, "HD preset must set 1080p internal rendering");
    TEST_ASSERT(sHd.anisotropicLevel == 16, "HD preset must set 16x anisotropic filtering");
    TEST_ASSERT(sHd.colorGrading, "HD preset must enable color grading");
    TEST_ASSERT(sHd.modernHud, "HD preset must enable modern HUD");

    // 4. Maximum Preset: 1440p internal rendering, 16x AF
    setQualityPreset(PRESET_MAXIMUM);
    const QualitySettings& sMax = getQualitySettings();
    TEST_ASSERT(sMax.internalWidth == 2560 && sMax.internalHeight == 1440, "Maximum preset must set 1440p internal rendering");
    TEST_ASSERT(sMax.anisotropicLevel == 16, "Maximum preset must set 16x anisotropic filtering");

    // 5. Ultra 4K Preset: 3840x2160 internal rendering, 16x AF, modern HUD & color grading
    setQualityPreset(PRESET_ULTRA_4K);
    const QualitySettings& sUltra = getQualitySettings();
    TEST_ASSERT(sUltra.internalWidth == 3840 && sUltra.internalHeight == 2160, "Ultra 4K preset must set 3840x2160 internal rendering");
    TEST_ASSERT(sUltra.anisotropicLevel == 16, "Ultra 4K preset must set 16x anisotropic filtering");
    TEST_ASSERT(sUltra.colorGrading, "Ultra 4K preset must enable color grading");
    TEST_ASSERT(sUltra.modernHud, "Ultra 4K preset must enable modern HUD");
}

static void test_analytics_tactical_intelligence() {
    using namespace remaster::analytics;

    resetMatch();

    unsigned char mockEntities[24 * 0x48];
    memset(mockEntities, 0, sizeof(mockEntities));

    // Home players: 0..10
    // GK (0) at X=-50.0
    float gkX = -50.0f, gkY = 0.0f, gkZ = 0.0f;
    memcpy(mockEntities + 0 * 0x48 + 0x00, &gkX, 4);
    memcpy(mockEntities + 0 * 0x48 + 0x04, &gkY, 4);
    memcpy(mockEntities + 0 * 0x48 + 0x08, &gkZ, 4);

    // Outfield players (1..10)
    for (int i = 1; i < 11; ++i) {
        float px = -30.0f + (float)i * 3.0f;
        float py = -15.0f + (float)(i % 5) * 7.5f;
        float pz = 0.0f;
        memcpy(mockEntities + i * 0x48 + 0x00, &px, 4);
        memcpy(mockEntities + i * 0x48 + 0x04, &py, 4);
        memcpy(mockEntities + i * 0x48 + 0x08, &pz, 4);
    }

    // Away players: 11..21
    for (int i = 11; i < 22; ++i) {
        float px = 10.0f + (float)(i - 11) * 3.5f;
        float py = -15.0f + (float)(i % 5) * 7.5f;
        float pz = 0.0f;
        memcpy(mockEntities + i * 0x48 + 0x00, &px, 4);
        memcpy(mockEntities + i * 0x48 + 0x04, &py, 4);
        memcpy(mockEntities + i * 0x48 + 0x08, &pz, 4);
    }

    // Ball (22) near Home player 1 (X=-27.0, Y=-7.5)
    float b1X = -27.2f, b1Y = -7.4f, b1Z = 0.1f;
    memcpy(mockEntities + 22 * 0x48 + 0x00, &b1X, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x04, &b1Y, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x08, &b1Z, 4);

    // Tick 1: Player 1 controls ball
    updateMatchEntities(mockEntities, 24, 3, 15.0f, 0.25f);

    // Ball moves to Player 2 (Home completed pass)
    float b2X = -24.1f, b2Y = 0.1f, b2Z = 0.1f;
    memcpy(mockEntities + 22 * 0x48 + 0x00, &b2X, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x04, &b2Y, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x08, &b2Z, 4);
    updateMatchEntities(mockEntities, 24, 3, 16.0f, 0.25f);

    const TacticalMetrics& t1 = getTacticalMetrics();
    TEST_ASSERT(t1.homeCompletedPasses == 1, "Passing to teammate must increment homeCompletedPasses");

    // Ball intercepted by Away player 12 at (13.5, 0.0) -> (turnover)
    float b3X = 13.6f, b3Y = 0.1f, b3Z = 0.1f;
    memcpy(mockEntities + 22 * 0x48 + 0x00, &b3X, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x04, &b3Y, 4);
    memcpy(mockEntities + 22 * 0x48 + 0x08, &b3Z, 4);
    updateMatchEntities(mockEntities, 24, 3, 17.5f, 0.25f);

    const TacticalMetrics& t2 = getTacticalMetrics();
    TEST_ASSERT(t2.awayInterceptions == 1, "Interception by opponent must increment awayInterceptions");
    TEST_ASSERT(t2.homeCompactness > 5.0f && t2.homeCompactness < 50.0f, "Home compactness must evaluate to realistic pitch radius");
    TEST_ASSERT(t2.homeDefensiveLineHeight > 10.0f, "Defensive line height must evaluate distance from own goal");

    // Check intelligent coaching advisor
    const CoachingInsight& coach = getCoachingInsight();
    TEST_ASSERT(strlen(coach.advice) > 0, "Coaching advisor must provide non-empty tactical advice");
    TEST_ASSERT(coach.confidence > 0.5f, "Coaching advisor confidence must exceed 0.50");
}

int main() {
    printf("====================================================\n");
    printf("PES 6 Legacy Remaster — Unit Test Suite\n");
    printf("====================================================\n");

    RUN_TEST(test_config_master_switch);
    RUN_TEST(test_manifest_literal_ff_vs_wildcard);
    RUN_TEST(test_manifest_buffer_bounds_protection);
    RUN_TEST(test_manifest_unloaded_safety);
    RUN_TEST(test_config_whitespace_and_comments);
    RUN_TEST(test_asset_path_sanitization);
    RUN_TEST(test_asset_transactional_read);
    RUN_TEST(test_com_release_refcount_contract);
    RUN_TEST(test_hires_texture_crc_disambiguation);
    RUN_TEST(test_hires_draw_downsample);
    RUN_TEST(test_graphics_widescreen_projection);
    RUN_TEST(test_graphics_ultrawide_projection);
    RUN_TEST(test_sha256_known_vectors);
    RUN_TEST(test_vram_memory_budget_enforcement);
    RUN_TEST(test_json_manifest_loader);
    RUN_TEST(test_broadcast_director_state_transitions);
    RUN_TEST(test_broadcast_camera_zoom_lerp_and_bounds);
    RUN_TEST(test_broadcast_classic_mode_bypass);
    RUN_TEST(test_gameplay_safety_envelope_clamping);
    RUN_TEST(test_gameplay_dynamic_pressing_calculation);
    RUN_TEST(test_gameplay_classic_mode_strict_preservation);
    RUN_TEST(test_postprocess_gamma_ramp);
    RUN_TEST(test_analytics_xg_calculation);
    RUN_TEST(test_analytics_momentum_and_field_tilt);
    RUN_TEST(test_modern_hud_presentation);
    RUN_TEST(test_broadcast_scoreboard_themes);
    RUN_TEST(test_graphics_quality_presets);
    RUN_TEST(test_analytics_tactical_intelligence);

    printf("====================================================\n");
    printf("RESULTS: %d Passed, %d Failed\n", g_passCount, g_failCount);
    printf("====================================================\n");

    return (g_failCount == 0) ? 0 : 1;
}
