# Baseline re-proof

**Date:** 2026-10-20
**Build:** clean tree (`build/` and all `*.obj`/`*.asi` removed), rebuilt from
`remaster/build.bat` → `build\remaster.asi`, 281,600 bytes.
**Target:** `pes6.exe`, sha256
`0c7f804be8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f`
**Game folder:** `C:\Users\olive\Downloads\PES-6\PES-6-DownloadComputerGames.net`

This is the reference point every later change is measured against. If any
line here stops being true, the change that broke it is the bug.

## Results

| Check | Expected | Observed | Verdict |
|---|---|---|---|
| Clean rebuild | succeeds | `built build\remaster.asi`, 281,600 bytes | PASS |
| Address gate | 119/119 | `gate: 119 addresses checked, 119 matched, 0 mismatched, 0 unmapped` | PASS |
| Gameplay gate | 101/101 | `gate: gameplay 101 checked, 101 matched, 0 mismatched, 0 unmapped` | PASS |
| Hooks installed | 5 | `hooks installed: 5` (d3d.init, d3d.create_window, d3d.createdevice, tex.set_size, assets.unpack) | PASS |
| Window | windowed, 1536×864 client | `client area : 1536 x 864`, `window rect : 1536 x 864` | PASS |
| Game process | stays alive | `ALIVE  window='Pro Evolution Soccer 6'` | PASS |
| Texture substitution | one visible on screen | green/white checkerboard from the pack fills the screen, game's own menu bar still drawn over it | PASS |
| `.afs` archives | unmodified | all four sha256 identical before and after (see below) | PASS |

## How each was measured

### Clean rebuild

```powershell
Remove-Item "$new\remaster\build" -Recurse -Force
Get-ChildItem $new -Recurse -Include *.obj,*.asi | Remove-Item -Force
cmd /c "$new\remaster\build.bat"
```

```
built build\remaster.asi
20-10-2026  20:01            281600 remaster.asi
```

### Gates, hooks, window

```powershell
$p = Start-Process "$game\pes6.exe" -WorkingDirectory $game -PassThru
Start-Sleep -Seconds 38
Get-Content "$game\remaster.log" | Select-String "gate:|hooks installed|ready"
```

```
[INFO] gate: 119 addresses checked, 119 matched, 0 mismatched, 0 unmapped
[INFO] gate: gameplay 101 checked, 101 matched, 0 mismatched, 0 unmapped
[INFO] gameplay: upstream hooks installed
[INFO] hooks installed: 5
[INFO] remaster.asi ready
```

Window size is the **client** area, measured with `GetClientRect` on the
game's main window (not `GetWindowRect`, which includes the frame):

```
client area : 1536 x 864
window rect : 1536 x 864
```

Note the config asks for 1280×720 (`graphics: window 640x480 -> 1280x720`)
and the game settles on the display's native 1536×864 panel.

### Visible texture substitution

A single pack entry, 1024×1024 artwork for the game's 512×512 title texture:

```
remaster_pack/hires.txt : 512x512 baseline512.raw
```

```powershell
python tools\make_hires.py --image $art --size 512x512 --out "$game\remaster_pack" --name baseline512.raw
```

Result in the log:

```
[INFO] hires: 1 entries in the pack index
[INFO] hires: baseline512.raw is 1024x1024, for a 512x512 texture
```

and on screen the green/white checkerboard covers the whole 1536×864 client
area, with the game's own yellow selection bar and black menu panel drawn
over it — the pack artwork is underneath the game's own foreground elements,
which is the expected layering.

### Archives unchanged

Identical before and after the run:

```
0_sound.afs   D90C9D8A6712A5227B901BCF1E622DA592539961E78B02222A40922CCF75BE7B
0_text.afs    F611A065E47596E0A31C477A9E34B09E2CCF019A728EE6C223B2D6BC357DF985
e_sound.afs   3C61A3F26444FDFC2E83036E00937176B3C34143BA4C635D3A5FF9D7FB149E0D
e_text.afs    D90C9D8A6712A5227B901BCF1E622DA592539961E78B02222A40922CCF75BE7B
```

## Known-good environment for a re-proof

* ASI loader `dinput8.dll` present in the game folder; **no `d3d8.dll`**
* `remaster.asi`, `remaster.ini`, `remaster_manifest.txt` in the game folder
* `remaster_pack/` optional — without it the asset layer is a no-op
* the game's single-instance mutex must be free; a hung instance from an
  earlier experiment holds it and blocks every later launch
