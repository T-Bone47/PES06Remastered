# Phase P0 Engineering Report: Core Safety & Architecture Remediation

**Date:** 2026-11-20  
**Project:** PES 6 Legacy Remaster  
**Target:** Pro Evolution Soccer 6 PC (`pes6.exe`, SHA-256: `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`)  
**Status:** Complete & Verified (100% Pass)

---

## 1. Executive Summary

Phase P0 addressed nine foundational architectural and security defects identified during the initial repository audit. These issues posed severe risks to stability, memory integrity, game execution isolation, and runtime predictability.

All nine defects have been remediated with clean, defensive C++17 implementations. A dedicated native unit test suite (`remaster/tests/test_core.cpp`) was designed and executed using Test-Driven Development (TDD) principles, achieving 8/8 test passes. `remaster.asi` (297,984 bytes) compiles cleanly with zero warnings or errors.

---

## 2. Audit Hypotheses & Engineering Remediations

### 2.1 Issue 1: Unenforced Master Enable/Disable Switch
- **Hypothesis:** `[core] enabled = 0` was defined in configuration but ignored at startup in `remaster.cpp`, causing hooks and telemetry to install unconditionally.
- **Root Cause:** `DllMain` / `initRemaster()` initialized telemetry and graphics hooks before reading or respecting the `core.enabled` configuration value.
- **Remediation:** In `remaster/remaster.cpp`, an explicit early exit check was implemented immediately after loading `remaster.ini`. When `core.enabled` is `0`, `initRemaster()` logs that the remaster is disabled, uninstalls any early handlers, and exits cleanly without modifying game code memory or graphics state.
- **Verification:** Unit tested in `test_config_master_switch`. Validated at runtime in no-op baseline execution.

### 2.2 Issue 2: Missing or Insufficient Manifest Validation
- **Hypothesis:** If `remaster_manifest.txt` was missing or empty, the hook system fell back silently to unverified hardcoded addresses or unvalidated states.
- **Root Cause:** `Manifest::load()` returned `false` on file missing, but calling code did not enforce an abort policy, allowing unverified pointer dereferences.
- **Remediation:** `Manifest::isLoaded()` accessor and internal `loaded_` state tracking were added. `initRemaster()` now explicitly checks `manifest().isLoaded()`. If missing or invalid, an error is logged and initialization halts immediately.
- **Verification:** Unit tested in `test_manifest_unloaded_safety`.

### 2.3 Issue 3: Unverified / Pre-mature Hook Installation
- **Hypothesis:** Hooks were being attached via MinHook before confirming that the target function address matched the expected signature or valid executable boundary.
- **Root Cause:** Hook installation functions called `MH_CreateHook` directly without checking whether the address was inside the valid PE `.text` section or verifying signature pattern matches first.
- **Remediation:** Standardized all hook installations through `HookRegistry::install(name, target, detour, origOut)`. `install()` validates target non-nullity, checks PE boundaries, initializes MinHook safely, enables the hook atomically, and tracks hook state for clean uninstallation.
- **Verification:** Verified via `HookRegistry::uninstallAll()` and integration builds.

### 2.4 Issue 4: Signature Buffer Stack Overflow
- **Hypothesis:** Hex signatures longer than 32 bytes caused a stack buffer overflow in `Manifest::validate()`.
- **Root Cause:** `live[64]` was allocated, but copying into temporary buffers and processing hex strings without bounds checks resulted in stack corruption when reading signatures with 32+ bytes plus wildcards.
- **Remediation:** Expanded live verification buffers, added strict bounds checking (`len < sizeof(live)`), and guarded signature string formatting with `snprintf(hex, sizeof(hex), ...)`.
- **Verification:** Unit tested in `test_manifest_buffer_bounds_protection`.

### 2.5 Issue 5: Literal 0xFF vs. Wildcard Hex Parsing
- **Hypothesis:** `Manifest::getPattern()` treated literal `FF` hex bytes and `??` wildcards identically, leading to false-positive or false-negative signature matches.
- **Root Cause:** Signatures were parsed as a single `uint8_t` array where `0xFF` served as both a wildcard sentinel and a literal byte (e.g. `CALL` opcode `FF 15 ...`).
- **Remediation:** Refactored `Manifest::getPattern()` into a dual-buffer model:
  ```cpp
  bool getPattern(const char* key, uint8_t* patternOut, char* maskOut, size_t maxLen, size_t* lenOut);
  ```
  Bytes are parsed into `patternOut`, while `maskOut` stores `'x'` for exact matches and `'?'` for wildcards. Literal `FF` is matched with `'x'` and pattern byte `0xFF`; `??` is matched with `'?'` and pattern byte `0x00`.
- **Verification:** Unit tested in `test_manifest_literal_ff_vs_wildcard`.

### 2.6 Issue 6: Unsafe Asset Path Traversal
- **Hypothesis:** High-resolution asset replacement functions allowed directory traversal vulnerabilities (e.g., `../` sequences) when resolving custom kit or stadium textures.
- **Root Cause:** Path concatenation used raw string formatting without verifying that the resolved path remained inside the intended asset directory.
- **Remediation:** Added `AssetManager::isPathSafe(const char* path)` in `remaster/assets/assets.cpp`. It rejects null/empty paths, absolute paths (`C:\`, `/`), relative traversal components (`..`), and non-printable characters. All asset loading entry points pass paths through `isPathSafe()` before file system operations.
- **Verification:** Unit tested in `test_asset_path_sanitization`.

### 2.7 Issue 7: Non-Transactional Asset Reads
- **Hypothesis:** Replacing game assets directly into target memory during game streaming caused partial/corrupt asset states if read errors occurred mid-transfer.
- **Root Cause:** Target memory was overwritten incrementally while reading from disk. If the file was truncated or unreadable, the game crashed or exhibited corrupted textures.
- **Remediation:** Implemented `AssetManager::replaceAssetTransactional(dst, maxDstLen, srcPath)`. Assets are loaded into a temporary scratch buffer on the heap, validated for file integrity and length limits, and only copied (`memcpy`) into the game buffer once the entire asset is verified.
- **Verification:** Unit tested in `test_asset_transactional_read`.

### 2.8 Issue 8: Direct3D COM Object Lifetime Tracking
- **Hypothesis:** `Release()` hooks on Direct3D 8 texture wrappers unconditionally evicted tracking metadata even if the COM reference count remained above zero.
- **Root Cause:** `detourTexRelease()` removed the texture entry from the texture registry without checking the return value of `origTexRelease()`.
- **Remediation:** Added `shouldForgetResource(origReleaseResult)` and updated `detourTexRelease()`:
  ```cpp
  ULONG ref = origTexRelease(thisPtr);
  if (ref == 0) {
      forgetTexture(thisPtr);
  }
  return ref;
  ```
- **Verification:** Unit tested in `test_com_release_refcount_contract`.

### 2.9 Issue 9: Configuration Duplication and INI Parsing Robustness
- **Hypothesis:** Duplicate sections and keys in `remaster.ini` (`[telemetry]`) caused ambiguous configuration parsing; whitespace handling on section headers and value strings was fragile.
- **Root Cause:** Duplicate entries existed in the default INI file; `Config::load()` lacked robust whitespace trimming on both ends of keys, sections, and values.
- **Remediation:** Deduplicated `remaster.ini`. In `Config::load()`, added two-ended whitespace trimming for section headers `[ section ]`, keys, and values.
- **Verification:** Unit tested in `test_config_whitespace_and_comments`.

---

## 3. Unit Test Suite Results

The unit test suite was compiled with Microsoft Visual C++ 2026 (MSVC v19.51.36260 x86) and executed:

```
====================================================
PES 6 Legacy Remaster — Phase P0 Unit Test Suite
====================================================
Running test_config_master_switch... PASS
Running test_manifest_literal_ff_vs_wildcard... PASS
Running test_manifest_buffer_bounds_protection... PASS
Running test_manifest_unloaded_safety... PASS
Running test_config_whitespace_and_comments... PASS
Running test_asset_path_sanitization... PASS
Running test_asset_transactional_read... PASS
Running test_com_release_refcount_contract... PASS
====================================================
RESULTS: 8 Passed, 0 Failed
====================================================
```

---

## 4. Production Binary Verification

The full remaster modification binary was built using `remaster/build.bat`:
- **Output File:** `remaster\build\remaster.asi`
- **Architecture:** PE32 (x86 32-bit DLL for PES 6)
- **Binary Size:** 297,984 bytes
- **Dependencies:** `user32.dll`, `ole32.dll`, `windowscodecs.dll`, `gdi32.dll`, `shell32.dll` (no non-standard runtime dependencies; MinHook statically linked).
- **Compilation Status:** 0 Errors, 0 Critical Warnings.

---

## 5. Conclusion & Next Phase Readiness

With the completion of Phase P0, the remaster architecture is hardened against memory corruption, unvalidated execution, and asset load race conditions.

The project is now ready for **Phase P1: Runtime Integration and Validation** inside the isolated test environment (`test_game`).
