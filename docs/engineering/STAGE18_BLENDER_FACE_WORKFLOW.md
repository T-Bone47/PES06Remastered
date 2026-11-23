# Stage 18 — Blender MCP Face & Hair Inspection Pipeline
**PES 6 Legacy Remaster: 3D Geometric Verification, UV Checking & Preview Rendering**
*Engine Target: Blender 5.3 MCP | Protocol Version: 16 | Addon Version: 2.0*

---

## 1. Overview & System Setup

Stage 18 integrates **Blender MCP (Blender 5.3.0)** directly into the asset development pipeline. Rather than inspecting textures as flat 2D images, every restored face texture is evaluated in real-time mapped onto a 3D anatomical head geometry mirroring the PES 6 cylindrical UV parameterization.

The Blender MCP server operates locally on `127.0.0.1:9876`. Antigravity communicates via tool calls to `blender.execute_blender_code`, generating procedural geometry, assigning Principled BSDF shader networks, applying normal maps and subsurface scattering, setting up studio cameras and three-point lighting, and rendering high-resolution preview stills.

---

## 2. Cylindrical UV Mapping Architecture

PES 6 face meshes use a continuous cylindrical unwrap around the vertical cranial axis:
* **U Coordinate ($X \in [0.0, 1.0]$):** Wraps circumferentially from the left ear ($U \approx 0.0$), across the left cheek ($U \approx 0.25$), nasal bridge and philtrum ($U = 0.50$), right cheek ($U \approx 0.75$), to the right ear and back of skull ($U \approx 1.0$).
* **V Coordinate ($Y \in [0.0, 1.0]$):** Extends vertically from the cranial vertex/crown ($V = 0.0$), forehead ($V \approx 0.20$), eyes ($V \approx 0.42$), nose ($V \approx 0.54$), mouth ($V \approx 0.66$), chin ($V \approx 0.85$), to the neck throat boundary ($V = 1.0$).

### 2.1 Blender Head Mesh Reconstruction
To validate UV layouts without importing non-compliant meshes into the game, a reference anatomical head mesh was constructed in Blender:
* **Topology:** 512 vertices, 480 quad faces, continuous manifold topology.
* **UV Layout:** Exact cylindrical unwrapping with zero seams across the central facial plane ($X=0.50$).
* **Verification Artifacts:**
  - `scratch/stage18_head_uv_checker_front.png` (Frontal UV calibration)
  - `scratch/stage18_head_uv_checker_side.png` (3/4 Profile UV calibration)

---

## 3. Shader Network & Material Configuration

In Blender, the face material is evaluated using an advanced PBR shading network:
1. **Base Color:** Connected to the companion $1024 \times 2048$ PNG texture via an `Image Texture` node set to `Linear` color space.
2. **Subsurface Scattering (SSS):**
   - Subsurface Weight: `0.18`
   - Subsurface Radius: `(1.0, 0.4, 0.25)` (mimicking human blood perfusion)
   - Subsurface Color: Warm epidermal tone matching the player's complexion.
3. **Roughness & Specularity:**
   - Base Roughness: `0.45`
   - Specular IOR Level: `0.50` (human skin IOR $\approx 1.40$)
   - Specular Tint: Slightly cool tone reflecting environmental sky/stadium light.
4. **Lighting Setup:**
   - Key Light: 800W warm stadium spotlight ($45^\circ$ elevation, $30^\circ$ azimuth)
   - Fill Light: 300W soft diffused blue-tinted fill ($30^\circ$ elevation, $-45^\circ$ azimuth)
   - Rim / Hair Light: 600W sharp rim light positioned behind the head to accentuate jaw and cranial contours.

---

## 4. Reusable Batch Rendering Workflow

The batch validation script in Blender executes the following automated sequence:
1. Initialize clean scene with camera at $(0, -1.8, 0.0)$ targeting the head center.
2. Load player face texture from `PES 6 game files/GDB/faces/<Team>/<Name>.png`.
3. Load matching hair texture from `PES 6 game files/GDB/hair/<Team>/<Name_hair>.png`.
4. Render frontal beauty shot ($1024 \times 1024$) -> `scratch/stage18_<player>_front.png`.
5. Rotate camera to $(1.2, -1.4, 0.3)$ for $3/4$ perspective portrait -> `scratch/stage18_<player>_side.png`.
6. Output render metrics and log confirmation.
