# Antigravity — 2006 football photo reference integration

You are working in the existing PES06Remastered repository. This attached archive is a photo **reference-source starter kit**, not a finished character asset pack.

1. Read `README.md` and `collector/sources.json`. Confirm the roster in the actual installed PES 6 database. Do not assume every player ID or team assignment.
2. Run the bundled script on Windows with `py -3 collector/collect_refs.py --limit-per-player 2` if the user has approved downloading the free-license photographs. Record any failures.
3. Verify actual source, photo date, licensing, facial view, image dimensions and relevance to the 2006-era appearance for each player. Flag any images that lack a usable face, are too small, or are not frontal/three-quarter.
4. Retain all original images and attribution records untouched. Never add downloaded photos to the public game repository by default. Keep licensed reference media in a separate, ignored working directory.
5. Prioritize Arsenal and Thierry Henry for the first proof of concept. Use real period reference images for likeness, the original PES 6 head geometry, and the **actual mesh UV atlas**. A photograph is not a texture map and should not be directly pasted over the entire atlas.
6. Use Blender to project/refine a plausible face texture and inspect front, three-quarter and side views. Improve hair and seams, but do not sculpt a new modern character by default.
7. Use verified Faceserver mapping. Capture genuine in-game front/three-quarter/replay comparison. Evaluate likeness manually. Do not mark success based on image size or Blender render alone.
8. Once Henry passes, move to Arsenal starters, then Barcelona, Chelsea, and Real Madrid stars. Keep each team in a separate progress report.
9. Build a quality register with player, source images, permissions, UV atlas, generated texture, mapping ID, in-game screenshot, visual QA result, and remaining defects. Preserve classic gameplay and original archives.
10. If a chosen photo is too low-quality, seek better **appropriately licensed** 2005–2007 references; request manual approved photography if needed. Never fabricate a missing camera angle.

Final outputs: a 16-player reference manifest, resolved downloaded image paths (only if downloads actually completed), rights/credit ledger, quality shortlist, and one verified Henry remaster proof of concept before mass production.
