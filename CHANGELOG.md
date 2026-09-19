# Nebrix Engine — Project Log

**Purpose:** This file is the single source of truth for project context. It records
decisions, completed work, the plan, how to run the project, and the codebase structure.
**It must be read at the start of every working session and updated after every change.**

---

## How to run

```bash
# Configure + build (Debug, default)
cmake -B build -G Ninja
ninja -C build

# Run the sandbox demo
./build/sandbox/NebrixSandbox

# Release build
cmake -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build-release
```

- The sandbox opens a window with a 100x100 tile world (10,000 sprites) + a moving player.
  Camera follows the player; WASD/arrows move, +/- zoom, C toggles culling.
  Run with `NEBRIX_CULLING=0` to disable culling at startup (stress test).
- **Window backend: GLFW.** The sandbox defaults to the X11 platform (Xwayland) on this
  machine because NVIDIA's EGL Wayland presentation is broken here (see bug log below).
  Override with `NBX_PLATFORM=wayland` or `NBX_PLATFORM=x11`; the engine defaults to Auto.
- Close the window (WM close button → Quit event) or `pkill -x NebrixSandbox` to exit.
  `timeout 5 ./build/sandbox/NebrixSandbox` works for headless-ish testing.
- Expected log output: `VSync: on/off`, GL renderer name, `fps=60`.
- Stats line every second: `fps=60 quads=121 drawCalls=1 culling=true` (culling on) or
  `quads=10000 drawCalls=1` (culling off — the Phase 1 milestone).
- Shaders live in `sandbox/assets/shaders/` and hot-reload when edited while running.
  Asset paths resolve from the executable location, so CWD doesn't matter.

---

## Project structure

```
Nebrix-Engine/
├── CMakeLists.txt          # top level: C++23, Ninja, finds GLFW, adds engine + sandbox
├── CHANGELOG.md            # project log (history, decisions, plan)
├── docs/
│   ├── ARCHITECTURE.md     # complete engine guide: every file, how/why it works
│   └── LEARNING_PATH.md    # solo study plan: every block -> its canonical resource
├── engine/                 # the Nebrix engine (static library)
│   ├── CMakeLists.txt
│   ├── include/Nebrix/
│   │   ├── Assets/         # AssetManager.h (root resolution, texture/shader cache)
│   │   ├── Core/           # Log.h, Assert.h, GameLoop.h, Events.h, Input.h
│   │   ├── ECS/            # World.h (sparse sets + views), Components.h
│   │   ├── Math/           # Math.h (vec2/3/4, mat4, Transform2D, ortho, translate, scale, lerp)
│   │   ├── Physics/        # AABB.h, SpatialHashGrid.h, PhysicsSystem.h
│   │   ├── Platform/       # Window.h (GLFW window + GL context)
│   │   └── Renderer/       # Shader.h, Buffer.h, Texture.h, TextureAtlas.h, Sprite.h,
│   │                       #   SpriteSheet.h, Camera2D.h, Renderer.h (batched)
│   └── src/
│       ├── Assets/         # AssetManager.cpp
│       ├── Core/           # Log.cpp, GameLoop.cpp
│       ├── ECS/            # World.cpp
│       ├── Physics/        # SpatialHashGrid.cpp, PhysicsSystem.cpp
│       ├── Platform/       # Window.cpp, Input.cpp, Stb.cpp (STB_IMAGE_IMPLEMENTATION)
│       └── Renderer/       # Shader.cpp, Buffer.cpp, Texture.cpp, TextureAtlas.cpp,
│                           #   Camera2D.cpp, Renderer.cpp
├── sandbox/                # demo game app that proves the engine works
│   ├── src/Main.cpp
│   ├── assets/
│   │   ├── generate_assets.py  # Pillow script -> textures/*.png
│   │   ├── shaders/            # batch.vert + batch.frag (hot-reloadable)
│   │   └── textures/           # tileset.png + player.png (generated)
├── vendor/                 # vendored deps (no package manager)
│   ├── glad/               # OpenGL 3.3 core loader (C), include/ + src/glad.c
│   └── stb/                # stb_image.h, stb_truetype.h (header-only)
└── CHANGELOG.md            # this file
```

---

## Decisions (stack & architecture)

| Decision | Choice | Rationale |
|---|---|---|
| Language | C++23 | g++ 16.2.1 installed, full support |
| Build | CMake + Ninja | already installed |
| Window/Input | GLFW 3.5 (system package) | dedicated GL windowing; own native Wayland backend; backend-agnostic `Event` type (KeyPressed/KeyReleased/WindowResized/Quit) |
| OpenGL | 3.3 Core via **glad** (vendored, C loader) | modern GL, no system dependency; sourced from arrayfire/glad repo (pre-generated, loader on) |
| Images/Fonts | stb_image + stb_truetype (vendored) | single-header, industry standard |
| Math | **own library** (no glm) | 2D needs only vec2/mat4; instructive; zero deps |
| Audio (Phase 3) | miniaudio (vendored, header-only) | SDL is gone (SDL_mixer no longer fits); miniaudio keeps zero-package philosophy |
| ECS (Phase 2) | own, sparse-set based | learn how engines really work |
| Platform target | Linux first, cross-platform later | matches user's machine |
| Error handling | NBX_ASSERT (log + abort), NBX_LOG_* macros with std::format + source_location | simple, visible in terminal |
| Frame rate | fixed timestep sim (default 1/60) + render interpolation + optional frame cap | vsync unreliable on Wayland; cap is required |

### Key conventions
- All engine public headers under `engine/include/Nebrix/<Module>/`, namespace `nbx`.
- GL types never leak into public headers (raw `uint32_t` ids instead).
- GL resources must be destroyed while the GL context is still alive (see bug log).
- `NEBRIX_DEBUG` compile definition (Debug builds only) enables GL debug output.

---

## What was done

### Phase 0 — Foundation (COMPLETE)
- [x] Vendored deps: glad (GL 3.3 core, C loader) + stb_image/stb_truetype
- [x] CMake skeleton: top-level + `engine/` static lib + `sandbox/` app, C++23, warnings `-Wall -Wextra -Wpedantic`
- [x] `Log` (levels, ANSI colors, file:line via source_location) + `Assert` macros
- [x] `Window`: SDL window + GL 3.3 core context (debug flag in debug builds), resize handling, vsync attempt with warning fallback
- [x] `GameLoop`: fixed timestep + accumulator + render interpolation + FPS reporting + frame cap (sleep-based) + max-frame-time anti spiral-of-death
- [x] Math lib: `vec2/vec3/vec4`, column-major `mat4`, `identity/translate/scale/mul/ortho`, `lerp`
- [x] `Shader` (source/file load, uniform cache), `VertexBuffer/IndexBuffer/VertexArray` + layouts, `Texture` (stb_image load + raw create), `Renderer` (clear, setViewport, setProjection, drawQuad with texture or flat color; 1 draw call per quad — immediate mode, replaced in Phase 1)
- [x] Sandbox demo: checkerboard quad + red quad, interpolated movement with wraparound, resize-adaptive projection
- [x] Verified: builds clean (0 warnings, Debug + Release), runs at fps=60, clean shutdown

### Bugs found & fixed during Phase 0 verification
1. **Crash on exit (SIGSEGV)** — `Texture` destructor ran `glDeleteTextures` with no current GL context (context destroyed by `window.shutdown()` before local `Texture` went out of scope). Fix: sandbox destroys textures before `window.shutdown()`. Rule: GL resources die before the context.
2. **`stats.fps` reported 0 most frames** — frame time was ~60.1 fps, so the 0.5 s measurement window crossed one frame *after* each multiple of 30; the field was only filled on crossing frames. Fix: persist last measured fps in `m_lastFps`, always fill `stats.fps`.
3. **VSync unreliable on Wayland** — added `GameLoop::setFrameCap()` (sleep-based) as the fallback; `Window::init` logs whether vsync actually engaged.
4. **Wayland window size surprise** — compositor resized 1280x720 window to 955x1160; engine adapts (resize event → new projection/viewport). `Window::init` logs logical vs drawable size. Not a bug, but worth remembering.

### What was done (Phase 1)
- `Math`: added `Transform2D` (position, rotation, scale) + `transformPoint` (batch-friendly
  quad corner expansion; one sin/cos per quad).
- `ShaderDataType::UByte4` (normalized 4x8-bit color) in Buffer.h/.cpp.
- `Sprite` (texture id + UV rect + pixel size — id-based, survives texture moves).
- `TextureAtlas`: grid-based sprite sheet slicing; owns the texture; `loadFromFile` + `create`.
- **`Renderer` rewritten as a batch renderer**: dynamic vertex buffer (capacity 10,000 quads =
  40,000 verts, 24 B/vertex: pos3 + uv2 + color4), static index buffer, one `glDrawElements`
  per texture per frame, flush on texture switch, per-frame `Stats {quadCount, drawCalls}`,
  `setShader` for custom batch shaders. API changed to `Transform2D` + `Sprite` based
  `drawQuad` (old immediate-mode per-quad path removed).
- `Camera2D`: y-down world (1 unit = 1 px at zoom 1), `viewProjection` (ortho centered on
  position), `follow` (exponential smoothing), `screenToWorld`, `isVisible` (AABB culling
  with margin).
- `Shader` hot reload: `loadFromFile` keeps paths + mtimes; `update(dt)` polls every 0.5 s
  and reloads on change; move ctor/assign now carry the hot-reload state.
- Sandbox rewritten as a stress demo: 100x100 tile world (10k tiles from a procedural 4x4
  atlas), WASD/arrows player with rotation, camera follow + zoom (+/-), C toggles culling,
  `NEBRIX_CULLING=0` env var for startup stress mode, stats log every second, shaders in
  `sandbox/assets/shaders/` (hot-reloadable, paths resolved from executable location).

### Bugs found & fixed during Phase 1 verification
1. **SIGSEGV on early exit** — shader load failed (relative path from wrong CWD), sandbox
   `return 1` → static GL resources destroyed by `exit()` with no current context → crash.
   Two fixes: sandbox resolves asset paths from `/proc/self/exe` (CWD-independent) and falls
   back to the engine's default shader instead of exiting. Engine rule stands: GL resources
   must die while the context is alive.
2. **Constexpr misuse in batch submit** — `constexpr` arrays of `vec2` (ctor not constexpr)
   and an unused `textureId` param; made arrays runtime `const`, removed the param.
3. **Camera2D.cpp edit mishap** — an overzealous edit deleted the `isVisible` signature line;
   restored (compile caught it).

### Verification (Phase 1)
- Debug + Release builds: 0 warnings.
- Culling on: `fps=60 quads=121 drawCalls=1` (only visible tiles).
- Culling off: `fps=60 quads=10000 drawCalls=1` — **milestone verified**.
- Hot reload verified: edited `batch.frag` while running → "Shader files changed, reloading..."
  → sandbox notified; no crash.

### Build/toolchain notes
- g++ 16.2.1, clang++, CMake 4.4.2, ninja, GLFW 3.5.1 (system), no pip/glad CLI on this machine (glad files fetched pre-generated from GitHub).
- `find_package(glfw3 3.4 REQUIRED)` lives in the **top-level** CMakeLists; `Nebrix` links it `PUBLIC` (Window.h exposes `GLFWwindow*` in its public API).
- GLFW headers in engine/sandbox are included with `#define GLFW_INCLUDE_NONE` (no GL header conflicts with glad).
- `.gitignore` covers `build/` and `build-*/`.
- The `Write` tool strips trailing newlines → files must end with `\n` (GCC warns `backslash-newline at end of file` otherwise).

---

## Window/Input migration: SDL2 → GLFW (2026-08-16)

### Symptom
- The sandbox ran fine (60 fps, GL work done) but **no window ever appeared** on the user's
  Hyprland (Wayland) desktop — with SDL2 2.32.70 (`sdl2-compat`, SDL2 API on SDL3).

### Investigation highlights
- `hyprctl clients` never listed the app's window although the app was alive and rendering.
- Wayland protocol trace (`WAYLAND_DEBUG=1`): the app created the xdg_toplevel, got
  `configure(955,1160)`, acked it — but **never attached a real buffer**: only
  `attach(nil)` + 1 `commit` total. No buffer → compositor never maps the surface.
- Even a minimal SDL2 window (no GL, `SDL_CreateWindow` + `SDL_RenderPresent`) failed
  identically → not a code bug, a system-level presentation failure.
- GLFW 3.5.1 (native Wayland, its own EGL path) **fails the same way** → the root cause is
  **NVIDIA EGL → wl-buffer presentation on Wayland** with this driver (610.57.04) +
  Hyprland 0.56.2, not any specific windowing lib.
- X11 (via Xwayland) presentation works: minimal SDL2 tests and the GLFW sandbox both map
  fine with the X11 backend. (`GLFW_PLATFORM` env var is ignored by this Arch glfw build,
  so the engine sets the platform via `glfwInitHint` — see Window.cpp.)

### Changes
- Replaced SDL2 with **GLFW 3.5.1**: new `Core/Events.h` (backend-agnostic `Event`),
  `Window.h/.cpp` rewritten around GLFW (window, GL 3.3 core context, framebuffer-size
  resize callback, close → Quit event, key callbacks), `Renderer::init` loads GL via
  `glfwGetProcAddress`, sandbox key polling switched to `glfwGetKey`.
- `WindowProps::platform` (Auto/Wayland/X11) + `NBX_PLATFORM=x11|wayland` env override.
- Sandbox defaults to **X11** until the NVIDIA Wayland issue is fixed system-side
  (`TODO(nvidia-wayland)` comment in Main.cpp).

### Suggested system fix (not applied)
- Check `cat /sys/module/nvidia_drm/parameters/modeset` (needs root): if `N`, add
  `nvidia_drm.modeset=1` to the kernel cmdline and reboot — Hyprland/NVIDIA EGL on Wayland
  generally requires GBM + modeset. Driver/Hyprland updates may also fix it.

---

## Plan — what's next

### Phase 1 — Batched 2D renderer + camera (COMPLETE)
- [x] Sprite / Texture / TextureAtlas (sprite sheets)
- [x] **Batch renderer**: all quads in one draw call (the core of a fast 2D engine)
- [x] `Camera2D` (orthographic, follow-target) + world/pixel coordinate system
- [x] Shader hot-reload; 2D culling for big worlds
- [x] Milestone: 10k sprites @ 60 fps in 1 draw call — **verified: `quads=10000 drawCalls=1 fps=60`**

### Phase 2 — Gameplay core
- [ ] Input abstraction (keyboard/mouse/gamepad + action mapping)
- [ ] Own ECS (sparse sets: Transform, Velocity, Sprite + system iteration)
- [ ] 2D physics for topdown: AABB vs tilemap, swept movement, collision resolution, spatial hash grid
- [ ] Tilemaps (load + render via atlas, layer sorting)
- [ ] Scene system + transform hierarchy
- [ ] Milestone: player walks with collisions on a tile map, camera follows

### Phase 3 — Content & UI
- [ ] AssetManager (shaders, textures, tilesets, sounds, spritesheets)
- [ ] Fonts (stb_truetype) + text rendering for UI
- [ ] Audio (SDL_mixer vs miniaudio — decide then): music + SFX, volume/pan
- [ ] Scene serialization; particle system

### Phase 4 — Debug & demo
- [ ] ImGui overlay (ECS inspector, camera control, physics debug draw)
- [ ] Basic profiling (timers on-screen)
- [ ] Topdown demo game: player, tiles, enemies, collisions, camera follow, UI, win condition

### Explicitly out of scope (for now)
3D/Vulkan, networking, scripting, full editor, code hot-reload.

---

## Session log

### 2026-08-16 — Phase 0 complete
- Planned the full engine (4 phases) with the user; agreed stack: C++23 + CMake/Ninja +
  SDL2 + glad (3.3 core) + stb + own math; Linux first; full engine scope.
- Built everything listed under Phase 0 above; found and fixed 3 real bugs (see bug log).
- Sandbox verified: fps=60, interpolated motion, clean shutdown, no warnings (Debug + Release).
- NOT committed yet — user asked to hold commits.
- Next action: start Phase 1 (batch renderer + atlas + Camera2D). Read this file first.

### 2026-08-16 — Phase 1 complete (batch renderer + camera + atlas + hot-reload)
- Read CHANGELOG before starting (as agreed).
- Implemented Phase 1 in full: batch renderer (10k quads/1 draw call), Sprite/TextureAtlas,
  Camera2D (follow/zoom/culling), Shader hot-reload, stress demo sandbox.
- **Milestone verified**: `quads=10000 drawCalls=1 fps=60` (Debug + Release, 0 warnings).
- Found/fixed 3 issues (see Phase 1 bug log), incl. an early-exit SIGSEGV.
- Sandbox controls: WASD/arrows move, +/- zoom, C toggles culling, `NEBRIX_CULLING=0` env.
- NOT committed yet — user asked to hold commits.
- Next action: Phase 2 (input abstraction, ECS, physics, tilemaps, scenes). Read this file first.

### 2026-08-16 — Window/Input migrated SDL2 → GLFW
- **Root cause of "no window opens"**: NVIDIA EGL Wayland presentation broken on this system
  (driver 610.57.04 + Hyprland 0.56.2): sdl2-compat and GLFW alike never attach a buffer to
  the wl_surface, so the compositor never maps the window. X11/Xwayland works.
- Switched the engine to GLFW 3.5.1 (own native Wayland backend), added `Core/Events.h`,
  `WindowProps::platform` + `NBX_PLATFORM` env override; sandbox defaults to X11.
- **Verified**: sandbox window now maps in Hyprland (`Window 55e62cf76830 -> Nebrix Sandbox`),
  `fps=60 quads=121 drawCalls=1`, Debug build 0 warnings.
- Still open: fix Wayland for real (check `nvidia_drm.modeset=1`, driver update) so the
  sandbox can go back to platform Auto/Wayland.
- NOT committed — user asked to hold commits.
### 2026-08-16 — Sandbox presents again (missing swapBuffers) + R/B texture fix
- **Bug (migration regression)**: sandbox rendered frames but NEVER called
  `window.swapBuffers()` — lost during the SDL2→GLFW rewrite. The loop ran at 60 fps
  (frame-cap sleep) but nothing was ever presented → window stayed transparent (desktop
  showed through; grim showed the wallpaper color, not the clear color). Found by bisecting
  the real `Main.cpp` against a working replica (`t9`): the replica called `swapBuffers()`,
  the sandbox didn't. Fixed: `window.swapBuffers()` after `Renderer::endFrame()`.
- **Bug (color channels)**: tile/palette constants were `0xAARRGGBB` but memory is
  little-endian, so GL_RGBA upload read them as BGR → R/B swapped on screen (verified via
  grim + magick pixel sampling, e.g. border showed (28,21,17) instead of (17,21,28)).
  Fixed all 17 constants in `sandbox/src/Main.cpp` to `0xAABBGGRR`.
- **Verified**: window presents clear color + tiles + exact palette colors
  ((94,129,172)=#5E81AC, (209,213,219)=#D1D5DB, borders (17,21,28)); `fps=60`,
  `drawCalls=1`, camera follow + culling working (quad count varies as the view pans).
- Open items unchanged: real Wayland fix (`nvidia_drm.modeset` check), commits held per user.
- Next action: Phase 2 (input abstraction, ECS, physics, tilemaps, scenes).

### 2026-08-16 — Phase 2, Block 1: Input abstraction (polling + action mapping)
- New `Core/Input.h` + `Platform/Input.cpp`: backend-agnostic polling layer over GLFW.
  - `Key` enum mirrors GLFW codes (ASCII for letters/digits; the values are an
    implementation detail — apps use `Key::A`, `Key::Left`, ...). `MouseButton` likewise.
  - Queries: `isKeyDown`, `isKeyDownAny`, `isMouseDown`, `mousePosition()` /
    `mouseDelta()` (framebuffer px, y-down — same space as Camera2D), `setMouseCapture`.
  - Action mapping: `mapAction("MoveLeft", {Key::A, Key::Left})` + `isActionDown("MoveLeft")`
    — game logic binds to semantic actions, never to raw keys.
  - `translateKey()` is the single translation point (also used by the event layer).
- `Window` integration: binds the GLFW window in `init()`, unbinds in `shutdown()`,
  `pollEvents()` also pumps `Input::beginFrame()` (cursor + delta per frame); key
  events now carry `Key` (via `translateKey`) instead of raw GLFW codes.
- Sandbox migrated: `fixedUpdateFn`/`updateFn` use actions (Move*/ToggleCulling/ZoomIn/Out);
  GLFW includes and `keyDown()` helper removed from `Main.cpp` — the app no longer sees GLFW.
- Verified: Debug + Release build clean (0 warnings), sandbox runs, window presents
  (exact colors via grim+magick), fps=60, no errors/warnings in logs.
- Open items unchanged: real Wayland fix (`nvidia_drm.modeset`), commits held per user.
- Next action: Phase 2, Block 2 — own ECS (sparse sets: Transform, Velocity, Sprite,
  Player; system iteration); migrate player/tiles gradually.

### 2026-08-16 — Phase 2, Block 2: own ECS (sparse sets)
- New `ECS/World.h` + `ECS/World.cpp` (header-heavy, templates in World.h):
  - `Entity`: index + generation (stale handles invalid after destroy/reuse).
  - `ComponentPool<T>`: sparse-set storage — dense arrays (components + entities)
    for cache-friendly iteration, sparse index (entity -> dense) with generation
    validation; swap-and-pop removal; `add` replaces in place, `get` asserts.
  - `View<Components...>`: range-for over all entities having every requested
    component (iterates the FIRST type's dense array; structured bindings yield
    `(Entity, T&...)`). `world.view<Transform, Sprite>()` etc.
  - `World`: create/destroy with free-list + generation bump, `entityCount()`,
    per-type pools via `std::type_index`, `pool<T>()` for cached lookups.
- New `ECS/Components.h`: `Transform` (position/rotation/scale), `Velocity`,
  `Player` (interpolation state `previous` + `speed`). Sprite component reuses
  the renderer's `Sprite`.
- Sandbox migrated (as agreed, gradually): 10,000 tile entities + player entity;
  fixed update integrates via `view<Player, Transform, Velocity>`, render draws
  tiles via `view<Transform, Sprite>` (culling + player skip via cached pool) and
  the player via `view<Player, Transform, Sprite>` with interpolated position.
- Verified: standalone ECS test (`/tmp/opencode/ecs_test.cpp` — create/destroy/
  reuse, generations, add-replace, remove, multi-component views, pool cleanup
  on destroy; all assertions pass); sandbox logs `World ready: 10001 entities
  (1 player + 10000 tiles)`, `fps=60 quads=121 drawCalls=1`, exact palette colors
  via grim+magick; Debug + Release 0 warnings. (Note: user re-formatted Main.cpp —
  edits now follow that style.)
- Open items unchanged: real Wayland fix (`nvidia_drm.modeset`), commits held.
- Next action: Phase 2, Block 3 — 2D physics (AABB vs tilemap, swept movement,
  collision resolution, spatial hash grid).

### 2026-08-16 — ECS refactor: one class per file
- Split `ECS/World.h` (was ~300 lines holding every ECS type) into one header per
  class: `ECS/Entity.h` (Entity), `ECS/ComponentPool.h` (IComponentPool +
  ComponentPool<T>), `ECS/View.h` (View<Components...>), `ECS/World.h` (World +
  template implementations). `World.h` still includes the others, so app code
  keeps using `#include <Nebrix/ECS/World.h>` unchanged.
- Behavior unchanged: `ecs_test` passes, Debug + Release 0 warnings, sandbox
  identical (exact colors via grim+magick, 0 errors).

### 2026-08-17 — Phase 2, Block 3: 2D physics (AABB, spatial hash grid)
- New `Physics/AABB.h`: `AABB` (min/max), `fromCenterHalf`, `overlaps`,
  `overlapX/overlapY` (interpenetration amounts).
- New `Physics/SpatialHashGrid.h` + `.cpp`: broadphase bucketing AABBs into
  uniform cells (cell key packs signed cell coords into uint64; an entity
  spanning cells is inserted into each, so queries may return duplicates —
  harmless, resolution stops at the first hit). `setCellSize` (default 128),
  `clear`/`insert`/`query`.
- New `Physics/PhysicsSystem.h` + `.cpp`: `step(world, dt)` rebuilds the grid
  from every Collider (static = Collider without Velocity), then moves each
  dynamic entity (Collider + Velocity + Transform) axis-separated — move on X,
  resolve against the swept volume, zero the axis speed; then Y. Resolution
  pushes out by the exact overlap and stops movement (no tunneling at
  velocities < cell size per step).
- `ECS/Components.h`: new `Collider` component (`halfExtents` + `solid` flag;
  non-solid colliders never block).
- Sandbox: tiles become a maze — 2x2 solid wall blocks separated by 2x2 open
  corridors; wall tiles carry a Collider (32x32 half extents) and render the
  bright palette, open cells render dark (floor, cell 0). Player has a Collider
  (24x24); fixed update now sets velocity and calls `physics.step()` instead of
  integrating manually; defensive world-bounds clamp kept.
- Verified: standalone physics test (`/tmp/opencode/physics_test.cpp` — wall
  face stop flush at 85.0, free movement on the other axis, diagonal corner
  blocking both axes with no interpenetration, non-solid pass-through; all
  pass); sandbox `fps=60 drawCalls=1`, maze renders floor `(64,52,46)` + wall
  colors exact via grim+magick, 0 warnings Debug + Release.
- Note: the user's Main.cpp reformat had reverted the earlier R/B-swap fix
  (constants back to 0xAARRGGBB); re-applied the swap to the 16 palette colors
  + border (now 0xAABBGGRR) so colors render correctly again.
- Open items unchanged: real Wayland fix (`nvidia_drm.modeset`), commits held.
- Next action: Phase 2, Block 4 — tilemaps (or dynamic-vs-dynamic collisions).

### 2026-08-17 — Phase 2, Block 4: tilemaps (data-driven)
- `ECS/Components.h`: new `Tilemap` component — plain-data grid (`width`,
  `height`, `tileSize`, `tiles[]` indices into the app's tileset, `solid[]`
  flags), no per-tile entities. Helpers: `indexAt`, `isSolid`, `cellX/cellY`
  (local coordinate -> cell). The owning entity's Transform position is the map
  origin (top-left of cell 0,0).
- `Physics/PhysicsSystem.cpp`: `moveAxis` now also resolves against every
  `Tilemap` in the world — computes the solid-cell range overlapped by the
  swept volume (clamped to the map bounds) and tests each solid cell's AABB
  (shared `resolve` lambda with the entity-collider path). Out-of-map is
  non-colliding; per-cell O(1) lookup, no grid insert needed.
- Sandbox: the 10,000 tile entities are gone — one tilemap entity holds the
  maze (2x2 solid blocks / 2x2 corridors; wall cells index 4..15 of the atlas,
  floor cells index 0). Render draws only the cells inside the camera's view
  (all 10k when culling is off), computing the visible cell range from
  `camera.position()` + `viewWidth/viewHeight`. Player + tilemap = 2 entities.
- Verified: physics test extended to 6 checks (tilemap wall face stop at 59.0,
  non-solid cells pass-through; all pass); sandbox logs `World ready: 2
  entities (1 player + 1 tilemap 100x100)`, `fps=60 quads=111 drawCalls=1`,
  identical rendering (floor (64,52,46), borders, wall colors exact via
  grim+magick); Debug + Release 0 warnings.
- Note: entities that spawn embedded in a solid cell get pushed out on the
  first step (expected resolution behavior; the player spawns on open floor).
- Open items unchanged: real Wayland fix (`nvidia_drm.modeset`), commits held.
- Next action: Phase 2, Block 5 — scene system (or dynamic-vs-dynamic
  collisions; the milestone "player walks with collisions, camera follows, all
  ECS" is already met).

### 2026-08-17 — Sandbox fix: connected maze (recursive backtracker)
- The previous tilemap layout (2x2 solid blocks / 2x2 corridors in a checkerboard)
  had disconnected pockets: the open areas only touched diagonally, so the player
  was trapped in the 128x128 spawn square.
- Replaced with a generated maze: 33x33 cells of 2x2 tiles with 1-tile walls,
  carved by recursive backtracker (deterministic seed 20260817). Every carved
  cell is guaranteed reachable; player spawn (96,96) is in cell (0,0).
- Verified: Python replica of the generator + flood fill from the spawn reaches
  all 6,532 floor tiles (4,356 carved + 2,176 knocked walls); sandbox renders
  the maze (floor (64,52,46), walls, fps=60 drawCalls=1); Debug 0 warnings.
  (The open top/left maze edges are covered by the existing world-bounds clamp.)

### 2026-08-17 — Análise de engines + decisão 2.5D/3D (futuro)
- Comparadas OGL-Game-Engine (GalaxyCrush/OGL-Game-Engine, projeto universitário
  do utilizador) e cubos (GameDevTecnico/cubos) via árvores do GitHub API
  (salvas em ~/.local/share/opencode/tool-output/). Resumo:
  - OGL: 39 ficheiros de engine (19 .cpp + 20 .hpp), C++17, GLFW+GLM+GLEW+
    Assimp+stb; OOP com managers (Mesh/Texture/Shader/Material/Light) + scene
    graph; Blinn-Phong forward + partículas (geometry shader) + Perlin; demo
    `app.cpp` 70 KB (dungeon). Sem física/áudio/UI/ECS/serialização.
  - cubos: ~870 ficheiros C++ (core 535 + engine 849 entradas); ECS
    data-oriented (archetypes/tables), reflection, serialização, 22 plugins de
    engine (renderer deferred completo, física com constraints, colisões com
    eventos, UI canvas, áudio, scenes, assets+bridges, voxels), editor ImGui
    (tesseratos), bindings Lua, C API, 106+ testes, CI multi-plataforma.
  - Veredicto para o utilizador: a Nebrix já é estruturalmente superior ao OGL
    (ECS a sério, matemática própria, física com grid espacial, tilemaps,
    testes); vs cubos a comparação é de escala (equipa vs uma pessoa) — o teto
    2D completo é alcançável por uma pessoa.
- Confirmada (e registada na secção "Futuro possível") a viabilidade de 2.5D/3D
  na arquitetura atual: renderer agnóstico de projeção (beginFrame só vê mat4),
  vec3/mat4 já existem, ECS agnóstico de dimensão. Definidas 4 regras para
  manter a porta aberta (ver secção).
- Nenhuma alteração de código nesta sessão. Próximo passo: Block 6 (Assets).

### 2026-08-17 — Full architecture documentation (docs/ARCHITECTURE.md)
- Read every file in the repo (all engine headers/sources, Main.cpp, shaders,
  all 3 CMakeLists) and wrote `docs/ARCHITECTURE.md`: the complete guide to the
  engine, in English, no em-dashes, per the user's request.
- The doc covers: repo layout + conventions; how a frame happens (fixed
  timestep + interpolation pipeline); every module file by file (Core: Log,
  Assert, Events, GameLoop, Input; Math; ECS: Entity, ComponentPool, View,
  World, Components; Platform: Window; Renderer: Buffer, Texture, TextureAtlas,
  Sprite, Shader, Camera2D, Renderer; Physics: AABB, SpatialHashGrid,
  PhysicsSystem); the sandbox explained end to end (palette byte order,
  procedural atlas, assetPath, maze generator, loop wiring, shutdown order);
  the Wayland saga; the build system (all 3 CMakeLists + vendor); a decisions
  table; one-page history; and the 4 future-proofing design rules.
- CHANGELOG project-structure tree updated to include `docs/`.
- No code changed; builds unaffected. Next action: Block 6 (Assets).

### 2026-08-17 — Learning path doc (docs/LEARNING_PATH.md)
- Wrote `docs/LEARNING_PATH.md`: a solo study plan (English, no em-dashes) that
  maps every Nebrix block to its canonical learning resource, in study order:
  C++ foundations + Game Programming Patterns -> "Fix Your Timestep"
  (gafferongames.com) -> LearnOpenGL coordinate systems -> LearnOpenGL
  rendering + batching -> ECS (Nystrom, Overwatch GDC talk, Austin Morlan,
  EnTT) -> physics (Ericson, Schwarzl) -> tiles/grids (Red Blob Games).
- Includes: the golden "resource -> re-read the Nebrix file -> why?" cycle,
  questions per topic, books in order of value, reference engines to read
  (EnTT, raylib, MonoGame, olcPixelGameEngine, Godot), and a printable
  milestone checklist matching the engine's verified milestones (10k quads,
  1 draw call, no tunneling, etc.).
- CHANGELOG project-structure tree updated to include `docs/LEARNING_PATH.md`.
- No code changed; builds unaffected. Next action: Block 6 (Assets).

### 2026-08-21 — Post-audit hardening: Phases A/B/C (bug fixes + tuning)
- Ran a 4-agent audit of every module before starting Block 6. It found 6 real
  high-severity bugs plus robustness/perf issues; all fixed in three phases.
  Note: ECS headers were found reformatted (user style); edits follow the file
  on disk.
- **Phase A (critical bugs)**:
  1. `World::add` now asserts `isAlive` — adding via a stale handle used to
     push a ghost dense entry that `isAlive` disagreed with.
  2. Batch overflow no longer drops quads: `submit()` flushes mid-frame and
     retries. Previously the 10,001st quad (the player!) was silently dropped
     when culling was off. Verified: `NEBRIX_CULLING=0` now reports
     `quads=10001 drawCalls=2 fps=60`.
  3. `Shader::loadFromSource` zeroes `m_id` right after deleting the old
     program, so a compile failure can't leave a dangling id (double-delete).
  4. `Renderer::setShader` flushes pending quads built for the previous shader
     before switching.
  5. Window key callback maps only `GLFW_RELEASE` to KeyReleased;
     `GLFW_REPEAT` is KeyPressed with data2=1 (was misclassified → spurious
     releases while holding keys).
  6. `Window::shutdown` terminates GLFW only if init succeeded (static flag);
     width()/height() return 0 without a window instead of dereferencing null.
- **Phase B (robustness)**:
  - `Log`: macros pass format args through; formatting happens only after the
    level filter (filtered Trace no longer pays std::format). Level storage is
    `std::atomic<LogLevel>`; `%.*s` for non-null-terminated string_view.
    NBX_ASSERT supports zero args (`NBX_ASSERT(cond)`) via an assertMessage()
    overload pair using vformat (compile-time fmt checking traded away).
  - `GameLoop`: setFixedTimestep/setMaxFrameTime reject <=0/non-finite;
    m_running is atomic; hard cap of kMaxStepsPerFrame (8) fixed steps then
    accumulator dump (no multi-second catch-up loops).
  - `Input`: transparent hashing on the action map (isActionDown takes
    string_view, no per-frame allocation); bind() seeds mouse position from
    the current cursor (no first-frame delta spike); unbind() restores the
    cursor mode.
  - `Texture::loadFromFile` no longer sets stb vertical flip — memory row 0 is
    the image top, matching create() and the renderer's uv.y-is-top convention.
    The flip would have rendered all future PNGs upside down (latent).
  - `Shader::loadFromFile` stats mtimes with error_code; a vanished file
    disables hot reload with a warning instead of throwing.
- **Phase C (tuning)**:
  - `View.h` documents rarest-component-first ordering (primary pool drives
    iteration cost); runtime smallest-pool selection deferred.
  - `World::m_alive` is vector<uint8_t> (vector<bool> is bit-packed/slow).
  - New `BufferUsage` enum; the batch vertex buffer uses GL_DYNAMIC_DRAW
    (was STATIC_DRAW despite per-flush sub-updates).
  - `math::ortho` asserts non-degenerate ranges.
  - Physics swept volume expands only the trailing side by |delta| (union of
    start/end boxes) — half the previous query area, same correctness.
  - `Collider::fromScale(scale)` helper keeps collider size in sync with
    sprite scale by construction.
- Verified after each phase: Debug + Release 0 warnings; physics_test all
  checks pass (incl. exact face-stop positions with the new swept volume);
  ecs_test passes; sandbox `fps=60 quads=100 drawCalls=1 culling=true`, exact
  colors via grim+magick (player srgb(173,142,180) = #B48EAD swapped, bg =
  clear color). Commits still held. Next action: Block 6 (Assets).

### 2026-08-21 — Phase 3, Block 6: assets & content pipeline
- New `Assets/AssetManager.h/.cpp` (engine): resolves the assets root from
  /proc/self/exe (`<repo>/build*/sandbox/` -> `<repo>/sandbox/assets`),
  CWD-independent; `getTexture(relative)` caches per name and returns a cached
  1x1 magenta fallback on missing files (error logged once, failure cached so
  retries don't spam); `getShader(name, vert, frag)` caches shaders and keeps
  hot reload, returning nullptr on compile failure (also cached);
  `shutdown()` frees all GL resources before the context dies.
- New `Renderer/SpriteSheet.h`: non-owning uniform grid view over a texture id
  (pairs with manager-owned textures); row-major top-left indexing matching
  image layout and the uv.y-is-top convention; explicit cell pixel size.
- `sandbox/assets/generate_assets.py` (Pillow, deterministic): generates
  `textures/tileset.png` (256x256, 4x4 cells of 64px, true-RGB palette +
  #1C1511 borders) and `textures/player.png` (48px, face notch so rotation is
  visible).
- Sandbox migrated off the procedural atlas: tileset + player load from disk
  via AssetManager; shader comes from `getShader("batch", ...)`; collider now
  uses `Collider::fromScale({48,48})`; shutdown order Renderer ->
  AssetManager -> window.
- Note: stats now show `drawCalls=2` (tileset.png and player.png are separate
  textures; flush-per-texture working as designed). Can be merged into one
  sheet later if it ever matters.
- Verified: Debug + Release 0 warnings; exact colors via grim+magick (floor/
  player unchanged vs procedural era); each texture loaded exactly once (cache
  dedup in log); missing-tileset run renders pure magenta tiles
  (srgb(255,0,255)) with the error logged; physics_test + ecs_test pass;
  fps=60. Commits still held. Next action: Block 7 (Animations).

### 2026-08-21 — Phase 3, Block 7: animations
- New `Animation` component (`ECS/Components.h`): pre-resolved `frames`
  (vector<Sprite>, no sheet coupling), `fps`, `loop`, `playing`, `timer`,
  `index`. Frame 0 is the idle pose by convention.
- New `Renderer/Animation.h` + `Animation.cpp` (`AnimationSystem::update`,
  called in fixedUpdate before physics): advances the timer, wraps it with
  fmod on loop (float precision never degrades), clamps on non-loop, and
  always writes the current frame into the entity's Sprite (paused = frozen
  pose). Entities without Sprite or with empty frames are skipped.
- `generate_assets.py` now also emits `textures/player_walk.png` (192x48, 4
  frames: standing + alternating stride legs + 2px bob on frames 1/3, face
  notch kept so facing stays readable).
- Sandbox: player gets `Animation{walkFrames, 8 fps}`; fixedUpdate sets
  `playing` from input and resets timer/index on stop; the rotation-to-face
  behavior is kept (single walk strip + rotation, no directional frames yet).
- Verified: Debug + Release 0 warnings; anim_test 6/6 (advance, loop wrap,
  pause freeze, non-loop clamp, missing-Sprite skip, empty-frames skip);
  recreated physics_test.cpp (lost to /tmp cleanup) with the same 6
  documented checks incl. exact face stops 85.0/59.0 — all pass; sandbox
  `fps=60 drawCalls=2` (tileset + walk strip), exact colors via grim+magick
  (body srgb(173,142,180), legs srgb(128,114,107) = walk frame 0 on screen,
  floor srgb(64,52,46), bg = clear).
- Lesson: window tiling moves between runs — always query `hyprctl clients`
  for the live geometry before grim (an earlier screenshot sampled the wrong
  region and looked "broken" when nothing was).
- Commits still held. Next action: Block 8 (Text & UI).

### 2026-08-21 — Abstraction pass (components split, shared helpers, maze out)
- Split `ECS/Components.h` (90 lines, 6 structs) into one header per
  component under `ECS/Components/` (Transform, Velocity, Collider, Player,
  Tilemap, Animation) plus an umbrella `Components.h` that includes all six —
  same pattern as the earlier World.h split. Existing includes keep working;
  each header pulls only what it needs.
- New `Tilemap::cellRange(localMin, localMax, ...)`: map-local rect ->
  clamped cell range, false when the rect misses the map. Single
  implementation behind both physics sweeps (`PhysicsSystem.cpp`) and render
  culling (`Main.cpp`), which previously duplicated the clamp logic and could
  drift apart.
- `TextureAtlas` now delegates cell UV math to an internal `SpriteSheet`:
  one UV implementation in the codebase (the flip saga proved this is where
  bugs hide). Bonus: atlas cells gain the Sheet's range asserts.
- Maze generator extracted from `Main.cpp` (~70 lines) to
  `sandbox/src/Maze.h/.cpp` as `buildMaze(tilesPerSide, tileSize)` (pure
  function, deterministic seed); Main.cpp keeps world setup + loop wiring.
  Sandbox CMake now builds `src/Maze.cpp`; `<random>` left Main.cpp.
- Deliberately NOT abstracted: Renderer internals (batch statics are cohesive,
  splitting adds indirection without reuse), Event as variant (public API
  churn for minimal gain now), Transform2D/Transform merge (math lib must
  stay standalone for a future 2.5D).
- Verified: Debug + Release 0 warnings; physics_test 6/6 (tilemap checks go
  through cellRange); anim_test passes; sandbox `fps=60`, exact colors via
  grim+magick with live hyprctl geometry (player srgb(173,142,180), floor
  srgb(64,52,46)). Behavior identical by construction. Next: Block 8.

### Beyond Block 11 — long-term vision (Stardew Valley / Graveyard Keeper)
- Declared target (2026-08-21): the engine must grow into something a solo
  dev can use to build Stardew Valley / Graveyard Keeper class games
  (farming/management sims). This validates the current blocks and scopes
  what comes after Block 11.
- **External asset import pipeline** (requested): drop-in PNG packs resolved
  through AssetManager; Aseprite support via its JSON export (frame tags ->
  Animation clips with per-frame durations; layers ignored); TMX tilemaps as
  a possible Tilemap source. Direct .aseprite binary parsing is explicitly
  out (use the Aseprite CLI export instead).
- Needed for the genre after Block 11: save/load system, inventory + grid UI
  building blocks on top of Block 8, day/night + calendar as game-side
  patterns (not engine systems), NPC scheduling helpers, and eventually an
  editor-lite (tilemap/scene editing) — the biggest single future block.

## Roadmap — Nebrix engine 2D completa (decidido 2026-08-17)
- Goal: turn the engine into a complete 2D game engine (current state: solid
  core — loop, ECS, physics, batch renderer, camera, shaders, input; missing
  asset pipeline, animations, text/UI, scenes, combat, audio, polish).
- Order chosen by the user: Assets -> Animations -> Text/UI -> Scenes ->
  Combat -> Audio.
- Sandbox decision: use real PNG assets from disk (generated by script) to
  prove the asset pipeline, instead of the procedural atlas.

### Block 6 — Assets & content (DONE 2026-08-21)
- [x] `Texture::loadFromFile` via stb (RGBA8, no flip — uv.y is top; magenta
    1x1 fallback lives in the AssetManager, not in Texture).
- [x] `Assets/AssetManager` (/proc/self/exe root, per-name cache, shader
    cache with hot reload, magenta fallback, shutdown()).
- [x] `Renderer/SpriteSheet.h`: PNG cell grid -> sprite(frame) with UVs.
- [x] Sandbox: `generate_assets.py` generates tileset.png + player.png;
    sandbox loads them via AssetManager + SpriteSheet.
- [x] Verified: exact colors, cache dedup, missing-file fallback, 0 warnings.

### Block 7 — Animations (DONE 2026-08-21)
- [x] `Animation` component (frames, fps, loop, playing, timer, index) + system
    in fixedUpdate; generated player walk spritesheet; walk on input, idle
    frame 0 otherwise.

### Block 8 — Text & UI
- Bitmap font (character atlas PNG) -> renderable `Text`; then UI: panels +
  buttons (hover/click via mouse Input) — menu in the sandbox.

### Block 9 — Scenes
- `Scene` (World + update/render + lifecycle) and `SceneManager` (transitions);
  sandbox: MenuScene -> GameScene (maze), restart with R.

### Block 10 — Combat & gameplay
- `Health`/damage, physics triggers/overlaps (enter/exit without blocking),
  collision layers, projectiles, knockback, enemies with AI chasing the player
  through the maze.

### Block 11 — Audio (miniaudio)
- `AudioSystem`: SFX + looping background music, audio assets; then polish:
  particles, post-processing, save/load.

### Futuro possível: 2.5D / 3D (decidido 2026-08-17)
- Analisados OGL-Game-Engine (projeto universitário do utilizador: 39 ficheiros,
  OOP + managers, demo 3D Blinn-Phong, sem física/áudio/UI/ECS) e cubos (equipa
  IST: ~870 ficheiros C++, ECS data-oriented, reflection, renderer deferred,
  física com constraints, editor, Lua — outra dimensão de escala).
- Veredicto: a Nebrix já é estruturalmente superior ao OGL; vs cubos a comparação
  justa é arquitetural (não de escala) — um motor 2D completo (Blocks 6-11) é
  alcançável por uma pessoa; em 2D, o cubos seria overkill.
- **A porta 2.5D/3D já está aberta pela arquitetura atual**:
  - `Camera2D::viewProjection()` devolve `mat4` e `Renderer::beginFrame` só
    consome essa matriz → o renderer é agnóstico de projeção (trocar ortho por
    perspective é mudança de câmara, não de renderer).
  - `vec3` + `mat4` já existem em `Math.h` (falta `perspective()`, `quat`,
    rotação de matriz — tudo aditivo).
  - ECS (`World`/`View`/`ComponentPool`) é agnóstico de dimensão; os componentes
    é que são 2D.
- **Custo futuro**: 2.5D = aditivo pequeno (perspective + billboards + z-sort no
  batch + física no plano XZ porta quase 1:1). 3D completo = renderer de meshes
  novo ao lado do batch (VAO com triângulos indexados + depth buffer + luz),
  física 3D (grid 2D generaliza mas é reescrita), matemática 3D. Nenhum destes
  toca no ECS.
- **Regras para manter a porta aberta (custo zero agora, caro se ignoradas)**:
  1. Nunca hardcodar ortho nos shaders/renderer — tudo via matriz da câmara.
  2. O renderer só consome `mat4 viewProjection` — nunca ligá-lo à Camera2D.
  3. Componentes como plain data com nomes genéricos — 3D adiciona Transform3D,
     não mexe no Transform.
  4. Z-sorting como modo opcional do renderer, não entranhado no caminho 2D.
- Onde encaixa: 2.5D como Block opcional 12 (depois do Combat/Audio); 3D como
  Fase 3 separada. Não muda nada nos Blocks 6-11.

### Outside the roadmap (pending)
- Real Wayland fix (`nvidia_drm.modeset`, needs the user's root terminal).
- Commits (held per user).
