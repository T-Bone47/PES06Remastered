#!/usr/bin/env python3
"""
PES 6 Legacy Remaster — SHA-256 Engine Updater & Asset Hash Tool
Utility script to:
1. Ensure png_loader.cpp uses native Windows CryptoAPI (wincrypt.h, CALG_SHA_256)
   for bit-exact NIST-compliant hashing with zero third-party dependencies.
2. Validate and synchronize SHA-256 digests in manifest.json with files on disk.
"""

import os
import sys
import hashlib
import json

def get_candidates():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_dir = os.path.dirname(script_dir)
    workspace_root = os.path.dirname(repo_dir)
    return [
        script_dir,
        repo_dir,
        workspace_root,
        os.path.join(workspace_root, "PES06Remastered-main"),
        os.path.join(workspace_root, "test_game"),
        os.getcwd()
    ]

def find_file(relative_subpath):
    for base in get_candidates():
        candidate = os.path.normpath(os.path.join(base, relative_subpath))
        if os.path.isfile(candidate):
            return candidate
    return None

def update_png_loader_source():
    target_rel = os.path.join("remaster", "assets", "png_loader.cpp")
    file_path = find_file(target_rel)
    if not file_path:
        print(f"[ERROR] Could not locate {target_rel}")
        return False

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    start_marker = "#include <windows.h>"
    end_marker = "// ---------------------------------------------------------------- VRAM Budget"

    idx1 = content.find(start_marker)
    idx2 = content.find(end_marker)

    if idx1 == -1 or idx2 == -1:
        print(f"[ERROR] Markers not found in {file_path} (idx1={idx1}, idx2={idx2})")
        return False

    crypto_impl = """#include <windows.h>
#include <wincrypt.h>
#include <wincodec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace assets {

// ---------------------------------------------------------------- SHA-256 (Windows CryptoAPI)

void computeBufferSha256(const void* data, size_t len, char hexDigestOut[65]) {
    if (!hexDigestOut) return;
    hexDigestOut[0] = '\\0';

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return;
    }
    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        if (len > 0) {
            CryptHashData(hHash, (const BYTE*)data, (DWORD)len, 0);
        }
        BYTE hash[32];
        DWORD hashLen = sizeof(hash);
        if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
            for (DWORD i = 0; i < hashLen; i++) {
                snprintf(hexDigestOut + (i * 2), 3, "%02x", hash[i]);
            }
            hexDigestOut[64] = '\\0';
        }
        CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
}

bool computeFileSha256(const char* filePath, char hexDigestOut[65]) {
    if (!filePath || !hexDigestOut) {
        return false;
    }
    hexDigestOut[0] = '\\0';

    FILE* f = fopen(filePath, "rb");
    if (!f) {
        return false;
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        fclose(f);
        return false;
    }

    bool success = false;
    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        BYTE buffer[16384];
        size_t bytesRead = 0;
        bool readOk = true;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            if (!CryptHashData(hHash, buffer, (DWORD)bytesRead, 0)) {
                readOk = false;
                break;
            }
        }
        if (readOk) {
            BYTE hash[32];
            DWORD hashLen = sizeof(hash);
            if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                for (DWORD i = 0; i < hashLen; i++) {
                    snprintf(hexDigestOut + (i * 2), 3, "%02x", hash[i]);
                }
                hexDigestOut[64] = '\\0';
                success = true;
            }
        }
        CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
    fclose(f);
    return success;
}

"""
    new_content = content[:idx1] + crypto_impl + content[idx2:]
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"[OK] Successfully updated {file_path} with Windows CryptoAPI SHA-256 ({len(new_content)} bytes)")
    return True

def update_manifest_hashes():
    manifest_targets = [
        find_file(os.path.join("assets", "manifest.json")),
        find_file(os.path.join("remaster_pack", "manifest.json")),
    ]
    manifest_targets = [m for m in manifest_targets if m]

    pack_dir = find_file("remaster_pack")
    if not pack_dir:
        for b in get_candidates():
            cand = os.path.join(b, "remaster_pack")
            if os.path.isdir(cand):
                pack_dir = cand
                break

    for mf_path in manifest_targets:
        try:
            with open(mf_path, "r", encoding="utf-8") as f:
                data = json.load(f)

            assets = data.get("assets", [])
            updated = 0
            for item in assets:
                rel_file = item.get("file", "")
                if pack_dir and rel_file:
                    disk_file = os.path.normpath(os.path.join(pack_dir, rel_file))
                    if os.path.isfile(disk_file):
                        with open(disk_file, "rb") as bf:
                            actual_hash = hashlib.sha256(bf.read()).hexdigest()
                        if item.get("sha256") != actual_hash:
                            item["sha256"] = actual_hash
                            updated += 1

            if updated > 0:
                with open(mf_path, "w", encoding="utf-8") as f:
                    json.dump(data, f, indent=2)
                print(f"[OK] Updated {updated} SHA-256 digests in {mf_path}")
            else:
                print(f"[OK] All {len(assets)} hashes in {mf_path} match disk assets")
        except Exception as e:
            print(f"[WARN] Failed to process {mf_path}: {e}")

if __name__ == "__main__":
    print("==================================================================")
    print("PES 6 Legacy Remaster — SHA-256 Engine Updater")
    print("==================================================================")
    ok = update_png_loader_source()
    update_manifest_hashes()
    if ok:
        print("[SUCCESS] All SHA-256 components synchronized.")
        sys.exit(0)
    else:
        sys.exit(1)
