import os

path = r'C:\Users\dream\Downloads\PES06Remastered-main\PES06Remastered-main\remaster\tests\test_core.cpp'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Ensure headers are included
if '#include "../graphics/font_atlas.h"' not in content:
    content = content.replace(
        '#include "../graphics/hires.h"',
        '#include "../graphics/hires.h"\n#include "../graphics/font_atlas.h"\n#include "../telemetry/ipc_pipe.h"'
    )

new_tests = """
// Test 16: High-resolution font atlas generation
static void test_font_atlas_generation() {
    using namespace remaster::graphics;

    FontConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.fontFace = "Arial";
    cfg.pointSize = 24;
    cfg.weight = FW_BOLD;
    cfg.atlasWidth = 512;
    cfg.atlasHeight = 512;
    cfg.textColor = 0x00FFFFFF;

    FontAtlas atlas;
    bool ok = generateFontAtlas(cfg, &atlas);
    TEST_ASSERT(ok, "generateFontAtlas should succeed");
    TEST_ASSERT(atlas.width == 512, "atlas width should be 512");
    TEST_ASSERT(atlas.height == 512, "atlas height should be 512");
    TEST_ASSERT(atlas.bgraPixels != nullptr, "atlas pixel buffer must be allocated");
    TEST_ASSERT(atlas.glyphCount >= 90, "atlas should contain printable ASCII characters");

    // Check glyph 'A'
    const GlyphMetric* gA = getGlyphMetric(&atlas, 'A');
    TEST_ASSERT(gA != nullptr, "glyph 'A' should be mapped");
    TEST_ASSERT(gA->advance > 0, "'A' advance width should be positive");
    TEST_ASSERT(gA->u0 >= 0.0f && gA->u1 <= 1.0f, "'A' U texture coordinates must be in [0, 1]");
    TEST_ASSERT(gA->v0 >= 0.0f && gA->v1 <= 1.0f, "'A' V texture coordinates must be in [0, 1]");

    // Verify presence of non-zero alpha pixel values
    bool hasNonZeroAlpha = false;
    for (size_t i = 0; i < atlas.totalBytes; i += 4) {
        if (atlas.bgraPixels[i + 3] > 0) {
            hasNonZeroAlpha = true;
            break;
        }
    }
    TEST_ASSERT(hasNonZeroAlpha, "font atlas must contain rendered antialiased glyph pixels");

    freeFontAtlas(&atlas);
    TEST_ASSERT(atlas.bgraPixels == nullptr, "freeFontAtlas should deallocate pixel memory");
}

// Test 17: Text measurement and antialiased BGRA rendering
static void test_font_atlas_measure_and_render() {
    using namespace remaster::graphics;

    FontConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.fontFace = "Arial";
    cfg.pointSize = 20;
    cfg.atlasWidth = 512;
    cfg.atlasHeight = 512;

    FontAtlas atlas;
    generateFontAtlas(cfg, &atlas);

    const char* sampleText = "PES 6 REMASTER";
    int textWidth = measureTextWidth(&atlas, sampleText);
    TEST_ASSERT(textWidth > 50, "sample text width should be > 50 pixels");

    // Create a target surface 300x50 BGRA
    const int targetW = 300;
    const int targetH = 50;
    const int pitch = targetW * 4;
    size_t targetBytes = (size_t)targetW * targetH * 4;
    unsigned char* targetBgra = (unsigned char*)malloc(targetBytes);
    TEST_ASSERT(targetBgra != nullptr, "failed to allocate target BGRA buffer");
    memset(targetBgra, 0, targetBytes);

    bool rendered = renderTextToBgra(&atlas, sampleText, targetBgra, targetW, targetH, pitch, 10, 10);
    TEST_ASSERT(rendered, "renderTextToBgra should succeed");

    bool targetModified = false;
    for (size_t i = 0; i < targetBytes; i += 4) {
        if (targetBgra[i + 3] > 0) {
            targetModified = true;
            break;
        }
    }
    TEST_ASSERT(targetModified, "target buffer should contain rendered text glyphs");

    free(targetBgra);
    freeFontAtlas(&atlas);
}

// Test 18: Telemetry IPC packet framing and checksum verification
static void test_telemetry_ipc_packet_integrity() {
    using namespace remaster::telemetry;

    TelemetryPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.magic = TELEMETRY_MAGIC;
    pkt.header.version = 1;
    pkt.header.sequence = 100;
    pkt.header.wallMs = 9988776655ULL;
    pkt.header.mode = 2; // Active match
    pkt.header.phase = 3;
    pkt.header.matchType = 4;
    pkt.header.matchClock = 350.25f;
    pkt.header.entityCount = 23;

    // Populate home team forward
    pkt.entities[9].id = 9;
    pkt.entities[9].side = 0;
    pkt.entities[9].posX = 25.5f;
    pkt.entities[9].posY = -10.2f;
    pkt.entities[9].posZ = 0.0f;
    pkt.entities[9].stamina = 850;

    // Populate ball
    pkt.entities[22].id = 22;
    pkt.entities[22].side = -1;
    pkt.entities[22].posX = 26.0f;
    pkt.entities[22].posY = -9.8f;
    pkt.entities[22].posZ = 1.2f;

    unsigned int cs1 = computePacketChecksum(pkt);
    TEST_ASSERT(cs1 != 0, "checksum should be non-zero");

    pkt.checksum = cs1;

    // Verify sensitivity to coordinate changes
    pkt.entities[9].posX += 0.5f;
    unsigned int cs2 = computePacketChecksum(pkt);
    TEST_ASSERT(cs1 != cs2, "modifying coordinates must invalidate checksum");

    // Revert coordinate and verify checksum matches
    pkt.entities[9].posX -= 0.5f;
    unsigned int cs3 = computePacketChecksum(pkt);
    TEST_ASSERT(cs1 == cs3, "restoring data must restore identical checksum");
}

int main() {
    printf("====================================================\\n");
    printf("PES 6 Legacy Remaster — Unit Test Suite\\n");
    printf("====================================================\\n");

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
    RUN_TEST(test_font_atlas_generation);
    RUN_TEST(test_font_atlas_measure_and_render);
    RUN_TEST(test_telemetry_ipc_packet_integrity);

    printf("====================================================\\n");
    printf("RESULTS: %d Passed, %d Failed\\n", g_passCount, g_failCount);
    printf("====================================================\\n");

    return (g_failCount == 0) ? 0 : 1;
}
"""

main_idx = content.find("int main()")
if main_idx != -1:
    new_content = content[:main_idx] + new_tests
    with open(path, 'w', encoding='utf-8') as f:
        f.write(new_content)
    print(f"Updated {path}, new size: {len(new_content)}")
else:
    print("Could not find int main()!")
