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
├── CMakeLists.txt          # top level: C++23, Ninja, finds SDL2, adds engine + sandbox
├── engine/                 # the Nebrix engine (static library)
│   ├── CMakeLists.txt
│   ├── include/Nebrix/
│   │   ├── Core/           # Log.h, Assert.h, GameLoop.h, Events.h
│   │   ├── Math/           # Math.h (vec2/3/4, mat4, Transform2D, ortho, translate, scale, lerp)
│   │   ├── Platform/       # Window.h (GLFW window + GL context)
│   │   └── Renderer/       # Shader.h, Buffer.h, Texture.h, TextureAtlas.h, Sprite.h,
│   │                       #   Camera2D.h, Renderer.h (batched)
│   └── src/
│       ├── Core/           # Log.cpp, GameLoop.cpp
│       ├── Platform/       # Window.cpp, Stb.cpp (STB_IMAGE_IMPLEMENTATION)
│       └── Renderer/       # Shader.cpp, Buffer.cpp, Texture.cpp, TextureAtlas.cpp,
│                           #   Camera2D.cpp, Renderer.cpp
├── sandbox/                # demo game app that proves the engine works
│   ├── src/Main.cpp
│   └── assets/shaders/     # batch.vert + batch.frag (hot-reloadable)
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
