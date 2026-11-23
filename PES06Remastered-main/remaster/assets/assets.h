#ifndef REMASTER_ASSETS_H
#define REMASTER_ASSETS_H

namespace remaster {
namespace assets {

// Installs the asset replacement hook. Returns false if the hook could not be
// installed; the game is unaffected in that case.
bool install();

// Validates that a pack relative filename does not attempt directory traversal.
bool isPathSafe(const char* file);

// Performs a transactional replacement from disk into dest buffer.
// If the file is missing, oversized, or read is partial, dest is unmodified and returns false.
bool replaceAssetTransactional(void* dest, unsigned int destSize, const char* fullPath, unsigned int* bytesWritten);

// Logs how many assets were inflated and how many were replaced.
void report();

}  // namespace assets
}  // namespace remaster

#endif  // REMASTER_ASSETS_H
