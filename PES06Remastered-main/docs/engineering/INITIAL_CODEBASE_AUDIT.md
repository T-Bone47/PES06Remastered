# PES 6 Legacy Remaster — Initial Codebase & Architecture Audit

**Date:** 2026-11-20  
**Target Executable:** `pes6.exe`  
**Target SHA-256:** `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`  
**Architecture:** Windows x86 (PE32)  
**Compiler:** MSVC 19.51 (Visual Studio 2026 BuildTools)  
**Status:** COMPLETE (All 9 Audit Hypotheses Evaluated & Verified Against Current Source)

---

## 1. Executive Summary

A comprehensive source inspection of `remaster/` and associated components was conducted. All 9 hypotheses identified in the project directives were audited against the actual source code (`remaster.cpp`, `core/core.cpp`, `graphics/graphics.cpp`, `graphics/d3d8.cpp`, `graphics/hires.cpp`, `assets/assets.cpp`, and `remaster.ini`).

Every single hypothesis represents a confirmed and unfixed defect in the active codebase. In addition, critical memory corruption bugs (including a stack buffer overflow in manifest validation and raw pointer format string mismatches in logging) were uncovered and documented below.

---

## 2. Revalidation of Engineering Audit Hypotheses

| ID | Audit Item / Hypothesis | Source Location | Verified Status | Detailed Technical Analysis |
|---|---|---|---|---|
| **H1** | **Unenforced master enable/disable switch** | `remaster/remaster.cpp:176-186` | **CONFIRMED & UNFIXED** | `DllMain` never evaluates `config().getBool("core", "enabled", true)`. Even if `[core] enabled = 0` is specified in `remaster.ini`, `installGameplay()` and `installRemasterLayers()` are still invoked unconditionally. |
| **H2** | **Missing or insufficient hook manifest validation** | `remaster/remaster.cpp:159-161`, `remaster/remaster.cpp:71-79` | **CONFIRMED & UNFIXED** | When `remaster_manifest.txt` is missing, `manifest().validate("game.")` returns `total = 0`. The condition `if (g.total > 0 && g.matched < g.total)` evaluates to false, causing `ModInitialise` to execute and install 35 unverified hardcoded hooks despite the manifest being absent. |
| **H3** | **Hooks enabled before complete verification** | `remaster/graphics/graphics.cpp:97-115` | **CONFIRMED & UNFIXED** | `MH_CreateHook` and `MH_EnableHook` are called *prior* to `hooks().install()`. Furthermore, `graphics.cpp` requests `sig.d3d_init` and `sig.create_window` (underscores), whereas the manifest defines `sig.d3d.init` and `sig.d3d.createwindow` (dots), meaning the signature verification in `hooks().install()` was never even executed. |
| **H4** | **Unsafe or incomplete asset replacement reads** | `remaster/assets/assets.cpp:168-193` | **CONFIRMED & UNFIXED** | `fread(dest, 1, n, f)` does not check for partial reads (`got != n`), does not implement transactional fallback/rollback on read failure, leaves trailing original bytes when `n < size`, and `e->file` is appended without path sanitization (allowing directory traversal). |
| **H5** | **Direct3D COM object lifetime tracking** | `remaster/graphics/d3d8.cpp:176-186`, `373-377`, `394-413` | **CONFIRMED & UNFIXED** | `detourTexRelease` calls `forgetWatched(texture)` on *every* call to `Release()`, even when the reference count `r > 0`. `GetSurfaceLevel` acquires surfaces whose lifetimes are not tracked; and `g_watched` has a hard 512-entry cap without evicting or handling overflow safely. |
| **H6** | **Width-and-height-only texture identification** | `remaster/graphics/hires.cpp:230-267` | **CONFIRMED & UNFIXED** | `findHiResArt(width, height)` matches strictly on `width` and `height`. Any texture sharing the same dimensions (e.g. 512x256 pitch texture, 512x256 stadium texture, 512x256 menu background) will collide and be replaced by the first matching entry in `hires.txt`. |
| **H7** | **Duplicate configuration settings** | `remaster/remaster.ini:64-100` | **CONFIRMED & UNFIXED** | In `remaster.ini`, the `[assets]` section header was omitted, causing asset settings to merge into `[telemetry]`. Lines 70-100 duplicate the entire `[graphics]` section (`enabled = 1`, `log_textures`, `log_locks`, `observe_textures`, `texture_scale`) under `[telemetry]`. |
| **H8** | **Missing automated tests** | Entire repository | **CONFIRMED & UNFIXED** | No native C++ unit tests, harness tests, or regression test suites exist for `core`, `manifest`, `config`, `assets`, or `graphics`. Verification historically relied on manual game boots. |
| **H9** | **Signature-buffer capacity and wildcard parsing** | `remaster/core/core.cpp:257-279`, `313-344` | **CONFIRMED & UNFIXED** | **CRITICAL DEFECTS:**<br>1. **Stack Buffer Overflow:** `getBytes` decodes up to 64 bytes into `buf[64]`. However, in `validate()`, `live` is declared as `unsigned char live[16]`. `memcpy(live, (void*)va, (size_t)wantLen)` causes an immediate stack buffer overflow when `wantLen > 16`.<br>2. **Wildcard Collision:** Wildcards `??` are stored as literal `0xFF` in `buf`, causing literal `0xFF` opcodes to be treated as wildcards.<br>3. **Format String Crash:** `log().write(..., want, live)` passes raw binary byte arrays (`const unsigned char*`) to `%s`, causing undefined behavior and potential crashes reading past buffer bounds. |

---

## 3. Toolchain & Build Environment Status

- **Toolchain:** MSVC x86 (`vcvars32.bat` in `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars32.bat`).
- **Build Status:** Verified. `remaster\build.bat` was updated to support Visual Studio 18 / 2026 BuildTools and cleanly built `remaster.asi` (294,400 bytes).
- **Static Linking:** MinHook is statically compiled in; CRT is statically linked (`/MT`); no external runtime DLLs required.

---

## 4. Remediation Plan for Phase P0

1. **Implement `[core] enabled` gate:** Check `config().getBool("core", "enabled", true)` at the very top of `DllMain`. If 0, do not initialize MinHook, do not install any hooks, and return `TRUE` immediately.
2. **Fix Manifest Gate & Safety:** Require `manifest().isLoaded()` and `g.total > 0` before allowing hook installation. If the manifest is absent or fails validation, keep the remaster inert.
3. **Correct Hook Registration Order:** Check signatures against live memory *before* calling `MH_CreateHook` and `MH_EnableHook`. Align manifest key names (`sig.d3d.init`, `sig.d3d.createwindow`).
4. **Fix Stack Buffer Overflow in `validate()`:** Allocate `live` dynamically or match `wantLen` bounds safely (`unsigned char live[64]`).
5. **Fix Wildcard Parsing:** Separate pattern bytes from wildcard bitmasks (`mask[b]`), so literal `0xFF` opcodes match strictly.
6. **Fix Logging Format Strings:** Format binary signatures into hex strings before passing to `log().write()`.
7. **Fix Asset Replacement:** Ensure `fread` checks `got == n`. If read fails, abort replacement and preserve original buffer intact. Add path traversal sanitization to prevent directory escapes.
8. **Fix COM Lifetime:** Only invoke `forgetWatched(texture)` when `Release()` returns 0.
9. **Clean Configuration Schema:** Repair `remaster.ini` section headers and eliminate duplicated keys.
10. **Build Native Test Suite:** Add an isolated native test runner `test_core.exe` to verify config, manifest, wildcard parsing, and asset replacement integrity without requiring the game process.
