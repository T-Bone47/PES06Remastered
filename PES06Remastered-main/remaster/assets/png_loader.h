#ifndef REMASTER_PNG_LOADER_H
#define REMASTER_PNG_LOADER_H

#include <stddef.h>

namespace remaster {
namespace assets {

// Maximum number of assets tracked in a single manifest pack
#define MAX_MANIFEST_ASSETS 512

struct AssetManifestEntry {
    char id[64];
    char category[32];      // "kit", "face", "hair", "pitch", "stadium", "ball", "ui"
    unsigned int origWidth;
    unsigned int origHeight;
    unsigned int crc32;
    char file[256];
    char sha256[65];
    size_t memoryBytes;
    bool verified;
};

struct AssetMemoryBudget {
    size_t maxBytes;
    size_t allocatedBytes;
    size_t peakBytes;
    int assetCount;
    int rejectedCount;
};

// Initializes the VRAM budget (in megabytes). Clamped between 128 MB and 1800 MB.
void initMemoryBudget(size_t maxMegabytes);

// Queries the current memory budget and allocation metrics.
AssetMemoryBudget getMemoryBudget();

// Requests an allocation of `bytes`. Returns true if within budget.
bool requestMemoryAllocation(size_t bytes);

// Releases an allocation of `bytes`.
void releaseMemoryAllocation(size_t bytes);

// Computes the SHA-256 hex digest of a file on disk.
bool computeFileSha256(const char* filePath, char hexDigestOut[65]);

// Computes the SHA-256 hex digest of an in-memory buffer.
void computeBufferSha256(const void* data, size_t len, char hexDigestOut[65]);

// Loads a PNG companion image using WIC, verifying VRAM budget.
// Output format is top-down 32-bit BGRA (D3DFMT_A8R8G8B8).
unsigned char* loadCompanionPng(const char* fullPath, unsigned int* outWidth,
                                unsigned int* outHeight, size_t* outBytes);

// Parses a JSON asset manifest from disk, verifies SHA-256 hashes and memory budgets,
// and registers valid replacement entries into the hi-res texture engine.
// Returns the number of successfully loaded and registered assets.
int loadAssetManifest(const char* jsonPath);

}  // namespace assets
}  // namespace remaster

#endif  // REMASTER_PNG_LOADER_H
