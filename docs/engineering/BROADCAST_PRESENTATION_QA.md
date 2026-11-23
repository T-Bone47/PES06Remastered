# Broadcast Presentation & Match Experience QA Report
**PES 6 Legacy Remaster: Modern TV Score Bug, Dynamic Camera & Analytics QA**
*Subsystem: remaster/broadcast & remaster/graphics/modern_hud | Date: November 2026*

---

## 1. Executive Summary

Phase 4 evaluates and validates the **Modern Broadcast Presentation Engine**. By refining the broadcast camera director, scoreboard overlay, instant replay system, and live football analytics, the Remaster creates a television-broadcast match atmosphere reminiscent of premium Champions League and Premier League coverage.

All broadcast components have been audited for stability, visual clarity, resolution scaling, and menu compatibility.

---

## 2. Scoreboard & Modern Match HUD (`modern_hud = 1`)

```
+-----------------------------------------------------------------------------------------+
| [ARS]  2  -  1  [BAR]    78:24 (+3')    [xG: 1.84 vs 1.12]   [MOMENTUM: >>>> (ARS 62%)] |
+-----------------------------------------------------------------------------------------+
```

### 2.1 Visual Hierarchy & Component Polish
1. **Team Badges & Abbreviations:** 3-letter uppercase team codes (`[ARS]`, `[BAR]`, `[CHE]`, `[RMA]`) rendered with vector-crisp contrast and club primary colors.
2. **Score Counter:** High-contrast numerical font with subtle drop shadows ensuring instant readability over floodlit turf or bright stadium stands.
3. **Match Clock & Stoppage Time:** Live game minute and second display (`MM:SS`), automatically switching to orange stoppage time badge (`+3'`) when entering injury time.
4. **Resolution-Aware Anchoring:** Automatically anchors to the top-left screen margin with safe-zone offsets ($40\text{ px}$ margin at 1080p, dynamically scaled for 720p, 1440p, and 4K), preventing overlap with pause menus, substitution alerts, or radar elements.

---

## 3. Dynamic Camera Director (`dynamic_camera = 1`)

The Camera Director intercepts PES 6's primary match camera and modulates zoom distance based on real-time pitch coordinates of the ball:

$$\text{CurrentZoom} = \text{CurrentZoom} + (\text{TargetZoom} - \text{CurrentZoom}) \cdot \text{zoom\_lerp\_speed}$$

| Pitch Zone / Match Event | Ball Coordinate Range | Zoom Multiplier | Resulting Camera Framing |
| :--- | :--- | :--- | :--- |
| **Midfield Overview** | $|X_{\text{ball}}| \le 25.0\text{ m}$ | $1.15\times$ ($1644.5\text{ units}$) | Widescreen tactical view showing wingers, passing lanes, and fullback overlaps. |
| **Penalty Box Danger** | $|X_{\text{ball}}| > 36.0\text{ m}$ | $0.85\times$ ($1215.5\text{ units}$) | Tight broadcast focus on attacker movements, goalkeeper positioning, and goalmouth battles. |
| **Goal Celebration** | Goal event flag active | $0.65\times$ ($929.5\text{ units}$) | Cinematic low-angle close-up framing scorer celebration. |
| **Transition Smoothing** | Across all zones | $\text{Lerp Speed} = 0.08$ | Smooth non-jarring camera glide ($< 1.5\text{ s}$ complete transition). |

### Instant Replay Bypass (`instant_replay_skip = 1`)
- **Keybinds:** Pressing `Spacebar`, `Enter`, or `Escape` immediately bypasses offside, foul, or missed shot replays, returning gameplay to kickoff or free kick instantaneously and eliminating pacing downtime.

---

## 4. Live Football Intelligence & Match Analytics

### 4.1 Calibrated Expected Goals (`xg_model = calibrated`)
- Evaluates shot distance to center of goalmouth, shot angle, defensive proximity, and shot type (header vs foot).
- Displayed cleanly as an estimated metric: `xG: 1.84 vs 1.12`.

### 4.2 Match Momentum & Field Tilt (`momentum_decay = 0.15`)
- Measures territory possession, final-third passes, shot frequency, and box touches over the preceding 5 minutes of game time.
- Renders a subtle directional momentum arrow indicator (`>>>>`) reflecting team ascendancy without distracting from ball tracking.

---

## 5. QA Stability & Acceptance Results

| Test Scenario | Verification Method | Result | Notes |
| :--- | :--- | :--- | :--- |
| **Scoreboard Safe Zones** | Verified at 720p, 1080p, 1440p, 4K | `PASS` | No clipping or menu overlap |
| **Camera Zoom Bounds** | `test_broadcast_camera_zoom_lerp_and_bounds()` | `PASS` | Clamped strictly to $[0.50, 1.50]$ |
| **State Transitions** | `test_broadcast_director_state_transitions()` | `PASS` | Correct kickoff/penalty/goal transitions |
| **Classic Mode Bypass** | `test_broadcast_classic_mode_bypass()` | `PASS` | Zoom returns strictly to $1.0\times$ |
| **Full Match Stability** | 90-minute simulated fixture | `PASS` | Zero memory leaks, steady 60 FPS |
