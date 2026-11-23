IMPORTANT: This is the corrected FLAT ZIP. Extract all files to one folder. The `collector` folder should be directly beside README.md.

# PES 6 — 2006-era player face photo reference collector

This package is a **curated source-and-download starter kit**, not a ZIP containing downloaded photos. The files in this package do **not** contain any player photos yet, because the export environment could not reach image hosting. The source catalogue contains 16 players from Arsenal, Barcelona, Chelsea and Real Madrid, with 2005–2007 Commons candidate pages. Each page is real, but all photos need **manual confirmation of player, image quality, date and facial angle** before being used.

## One command to collect the photos on Windows

Open PowerShell in the extracted folder and run:

```powershell
py -3 collector\collect_refs.py --limit-per-player 2
```

Requires only Python 3 and an internet connection. Alternatively use `python collector\collect_refs.py --limit-per-player 2` if `py` is unavailable. The script uses the Wikimedia Commons API to retrieve **original-resolution JPEG/PNG files** and file-specific licensing metadata. It skips unknown/restricted licenses instead of assuming every photo is free. The completed package appears at:

`PES6_2006_Licensed_Photo_References.zip`

The folder `downloaded_references/<team>/<player>/` also contains the photos, organized by team. Keep `CREDITS_AND_LICENSES.md` and `COLLECTION_RESULT.csv` with the images.

If a file cannot be accessed, check that source page manually and find an alternative: do not bypass login or license controls.

## Scope

- **Arsenal**: Thierry Henry, Cesc Fabregas, Gilberto Silva, Freddie Ljungberg, Robin van Persie.
- **Barcelona**: Ronaldinho, Samuel Eto'o, Deco, Lionel Messi.
- **Chelsea**: Didier Drogba, Frank Lampard, John Terry.
- **Real Madrid**: Zinedine Zidane, Ronaldo Nazário, David Beckham, Roberto Carlos.

**Roster caveat:** These are the previously discussed *2006-era* stars, not a claim about exact starting XI, ratings or player IDs in your installed PES 6 build. Antigravity must validate its actual roster and team IDs. Zidane's presence depends on the exact database/patch.

## Image-quality caveats

- Wikimedia has relatively few genuine frontal close-up photographs from 2006. Many provided references are action/celebration photos, some very small. Do not pretend they are production-ready projected texture maps.
- These photos show appearance (hairstyle, complexion, facial proportions) but **are not UV atlases**. Project only with verified PES6 mesh UVs, plus manual painting/cleanup.
- Focus first on a player-specific front or three-quarter image; prioritize clear eye/nose/mouth geometry and era-appropriate hairstyle. Do not hallucinate a missing profile.
- 2005–2007 is an intentionally permissive *era* window. Check each file's original date and whether it matches the target season before use.
- Attribution/share-alike obligations may apply to modified face art. Commercial use of player identity can raise separate rights questions beyond photo copyright; this pack does not grant likeness, club, logo or merchandising rights.
- Never download random Getty/AFP photos or build a redistributable game asset pack from unlicensed editorial photos. Use them as inspiration only where lawful and approved.

## Expected Antigravity workflow

Read `ANTIGRAVITY_PROMPT.md`. Import reference images, visually inspect and manually select candidates, preserve the original game, and iterate **one player at a time** starting with Henry. The downloader has not been tested against the user's Antigravity environment.

## Example primary source licenses

- Thierry Henry portrait: Wikimedia Commons, CC BY-SA 3.0 / earlier versions, 28 Oct 2006.
- Ronaldinho-7-5-2006: Darz Mol, CC BY-SA 3.0, 7 May 2006.
- Zidane World Cup final 2006: David Ruddell, CC BY 2.0.

Each image's individual license and creator metadata take precedence. See [Wikimedia reuse guidance](https://commons.wikimedia.org/wiki/Commons:Reusing_content_outside_Wikimedia/licenses/en).
