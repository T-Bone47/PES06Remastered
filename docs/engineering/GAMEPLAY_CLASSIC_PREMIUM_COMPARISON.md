# Classic vs Premium Gameplay Comparison Matrix
**PES 6 Legacy Remaster: Mode Differentiation & Technical Verification**
*Engine Target: Pro Evolution Soccer 6 | Date: November 2026*

---

## 1. Overview of Operating Modes

The PES 6 Legacy Remaster maintains two distinct, user-selectable gameplay architectures:
* **Classic Mode:** Bit-exact preservation of original 2006 Konami football mechanics. Ideal for purists and tournament play.
* **Premium Gameplay Mode:** Refined, bounded tactical spacing and responsiveness enhancements within a verified $\pm 15\%$ safety envelope.

Both modes enjoy the full suite of **Graphics Enhancements** (1080p/4K supersampling, widescreen aspect ratio fix, 16x anisotropic filtering, cinematic color grading) and **Broadcast Presentation** (dynamic camera director, modern TV score bug, instant replay skip).

---

## 2. Feature-by-Feature Comparison Matrix

| Gameplay Dimension | Classic Mode (`enabled = 0`) | Premium Gameplay Mode (`enabled = 1`) | Technical Implementation |
| :--- | :--- | :--- | :--- |
| **Tactical Team Width** | $1.00\times$ (Vanilla 2006 block) | $1.05\times$ (+5% off-ball spacing) | `remaster::gameplay::getTeamWidthMultiplier()` |
| **Dynamic Pressing** | Disabled ($1.00\times$) | Active late-game ($1.00\times \to 1.10\times$) | Scaled on match clock ($> 75\%$) and score deficit |
| **Through-Ball Lead** | $1.00\times$ (Vanilla lead distance) | $1.05\times$ (+5% forward channel lead) | `remaster::gameplay::getPassLeadMultiplier()` |
| **Ball Air Drag** | $1.00\times$ (Vanilla flight curve) | $1.00\times$ (Vanilla flight curve) | Protected against artificial alteration |
| **Player Face Rendering** | 100% Original Konami faces (`0_text.afs`) | 100% Original Konami faces (`0_text.afs`) | Zero custom face overrides active in either mode |
| **Input Responsiveness** | Standard DirectInput | Optimized flip buffers, zero lag | Direct3D 9 present parameters sync |
| **Simulation Timing** | 60 Hz tick-rate | 60 Hz tick-rate | Unaltered game clock and physics cycles |
| **Safety Envelope** | Unconditional vanilla clamp | Clamped strictly to $[0.85, 1.15]$ | Mathematical bounds protection |

---

## 3. How to Switch Modes

To switch modes, edit [PES 6 game files/remaster.ini](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES%206%20game%20files/remaster.ini):

### To Activate Classic Mode:
```ini
[gameplay_enhanced]
enabled = 0
```
*(All multipliers immediately lock to 1.000000)*

### To Activate Premium Gameplay Mode:
```ini
[gameplay_enhanced]
enabled = 1
team_width_factor = 1.05
dynamic_pressing = 1
pressing_aggression_cap = 1.10
pass_lead_factor = 1.05
ball_drag_factor = 1.00
enforce_safety_envelope = 1
```

---

## 4. Test Verification Evidence

In [remaster/tests/test_core.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/tests/test_core.cpp):
* `test_gameplay_classic_mode_strict_preservation()` confirms that when `enabled = false`, even if `teamWidthFactor` is set to `1.15`, the engine returns `1.0f` for all parameters.
* `test_gameplay_safety_envelope_clamping()` confirms that values exceeding the safety envelope (e.g. `2.50f` or `0.50f`) are clamped strictly to `1.15f` and `0.85f`.
* `test_gameplay_dynamic_pressing_calculation()` confirms correct escalation when trailing in late game phases.

All tests pass 100%.
