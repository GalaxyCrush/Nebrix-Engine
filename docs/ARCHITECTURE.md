# Nebrix Engine - Complete Architecture Guide

This document explains the Nebrix engine in full: every file, what it does, how it
works, and why it was built that way. It is the companion to `CHANGELOG.md` (the
project history) and describes the state of the engine as of Phase 2, Block 4
(physics + tilemaps).

The engine is a from-scratch 2D game engine written in C++23, built for learning
how real game engines work. It has zero runtime dependencies beyond the system
GLFW package: the OpenGL loader (glad) and the image library (stb) are vendored,
and the math library is written by hand.

---

## 1. Repository layout

```
Nebrix-Engine/
├── CMakeLists.txt              top-level build configuration
├── CHANGELOG.md                project log (history, decisions, plan)
├── docs/
│   └── ARCHITECTURE.md         this document
├── engine/                     the Nebrix engine (static library)
│   ├── CMakeLists.txt
│   ├── include/Nebrix/
│   │   ├── Assets/             AssetManager (root resolution, texture/shader cache)
│   │   ├── Core/               Log, Assert, Events, GameLoop, Input
│   │   ├── ECS/                World, Entity, ComponentPool, View, Components/
│   │   │                       (Transform, Velocity, Collider, Player, Tilemap,
│   │   │                       Animation + umbrella Components.h)
│   │   ├── Math/               Math.h (vec2/3/4, mat4, Transform2D)
│   │   ├── Physics/            AABB, SpatialHashGrid, PhysicsSystem
│   │   ├── Platform/           Window
│   │   └── Renderer/           Shader, Buffer, Texture, TextureAtlas, Sprite,
│   │                           SpriteSheet, Camera2D, Renderer
│   └── src/                    one .cpp per public header (same subfolders)
├── sandbox/                    demo game that proves the engine works
│   ├── CMakeLists.txt
│   ├── src/Main.cpp            world setup + loop wiring
│   ├── src/Maze.h/.cpp         buildMaze(): deterministic backtracker maze
│   └── assets/
│       ├── generate_assets.py  Pillow script generating textures/*.png
│       ├── shaders/            batch.vert + batch.frag (hot-reloadable)
│       └── textures/           tileset.png + player.png (generated, loaded via AssetManager)
└── vendor/
    ├── glad/                   OpenGL 3.3 core loader (C), include/ + src/glad.c
    └── stb/                    stb_image.h, stb_truetype.h (single header)
```

Every engine module has exactly one public header in `include/Nebrix/<Module>/`
and one implementation file in `src/<Module>/` with the same name. The only
exceptions are `Math.h` and `Components.h` (header-only, both by design: math is
tiny and components are plain data) and the ECS headers `Entity.h`,
`ComponentPool.h`, `View.h` (templates must live in headers).

Key conventions used everywhere:

- All public code lives in the `nbx` namespace.
- GL types never leak into public headers. `Texture` stores `uint32_t m_id`
  instead of `GLuint`, `Window` exposes `GLFWwindow*` only because the window
  handle is genuinely backend-specific (this one exception is documented in the
  top-level CMakeLists).
- GL resources must be destroyed while the GL context is still alive. The
  sandbox shuts down the renderer and resets texture/atlas objects before
  calling `window.shutdown()` (see section 11.9).
- Debug builds define `NEBRIX_DEBUG`, which enables the GL debug output callback
  and a debug GL context.
- Engine headers use same-line braces; `sandbox/src/Main.cpp` uses the user's
  style (braces on their own lines, indented namespace). Follow the style of the
  file you edit.

---

## 2. The big picture: how a frame happens

One rendered frame goes through four phases, coordinated by `GameLoop`:

1. `fixedUpdate(dt)`: the simulation step, always 1/30 of a second (see section
   3.4). Input is read, velocities are computed, `PhysicsSystem::step` moves the
   player against walls.
2. `update(dt)`: happens once per frame, as fast as possible. Events are
   polled, the camera follows the player, the shader hot-reload is checked.
3. `render(stats)`: builds a 4x4 matrix from the camera, clears the screen,
   submits all visible quads into a big vertex buffer, then flushes them to the
   GPU in one draw call per texture.
4. `swapBuffers()`: presents the finished frame.

The simulation runs at a constant rate so that physics is deterministic and
independent of the monitor refresh rate. The renderer runs at whatever rate the
machine can sustain, and uses the `interpolation` factor (how much of the next
fixed step has accumulated) to draw the player between its previous and current
positions. This is the classic fixed-timestep + interpolation pattern.

The whole pipeline is a pipeline of simple layers. Each layer only talks to the
layer below it:

```
sandbox (game logic)  ->  ECS (entities + components)
                      ->  Physics (grid + sweep resolution)
                      ->  Renderer (batch of quads)
                      ->  Buffer/Texture/Shader (GL wrappers)
                      ->  Window/Input (GLFW)
                      ->  OS
```

---

## 3. Core module

### 3.1 `Core/Log.h` + `src/Core/Log.cpp` - logging

What it is: a tiny logging system with levels, ANSI colors, and automatic
file:line reporting.

Why it exists: every engine needs to tell the developer what is happening.
`std::printf` gives no levels, no source location, and no colors.

How it works:

- `LogLevel` is an enum (`Trace`, `Info`, `Warn`, `Error`).
- `Log::setLevel` filters messages below the level (default `Info`).
- `Log::write(level, location, message)` prints to **stderr** (stdout is often
  captured or lost in game launchers; stderr survives), prefixed with a colored
  `[Nebrix][Level]` tag and suffixed with the source file name and line.
  Colors are plain ANSI escape sequences: gray for Trace, default for Info,
  yellow for Warn, red for Error.
- The file path is trimmed to its basename so messages stay short.

The macros `NBX_LOG_TRACE/INFO/WARN/ERROR(...)` are the public API. They use
`std::source_location::current()` to capture the call site and `std::format`
for formatting, which means the macros accept format strings: `NBX_LOG_INFO("x={}", x)`.

Why this design: `std::format` is type-safe (no printf format mismatches), and
`source_location` gives exact call sites with zero effort from the caller. The
whole logging system is 60 lines.

### 3.2 `Core/Assert.h` - assertions

What it is: `NBX_ASSERT(condition, ...)` and `NBX_UNREACHABLE()`.

How it works: when the condition fails, it logs an Error with the failing
expression text and an optional reason message, then calls `std::abort()`.
`NBX_UNREACHABLE()` always aborts with "reached unreachable code".

Why this design: asserts are not silent `#error`s and not exceptions. A failed
assert prints exactly what failed and where, then dies immediately, which is the
right behaviour for a bug that makes the rest of the run meaningless. The
`do { ... } while (false)` wrapper makes the macro safe inside `if/else`
without braces.

### 3.3 `Core/Events.h` - the event type

What it is: a minimal, backend-agnostic event struct.

```cpp
struct Event {
    enum class Type : uint8_t { None, Quit, WindowResized, KeyPressed,
                                KeyReleased, FocusGained, FocusLost };
    Type type = Type::None;
    int data1 = 0;
    int data2 = 0;
};
```

Why it exists: the windowing backend (GLFW) must be kept replaceable. Instead
of exposing GLFW callbacks to game code, `Window` translates them into these
`Event`s and hands them to a callback set by the app.

How the payload works: `data1`/`data2` carry the event-specific data. For
`WindowResized` they are framebuffer width and height. For `KeyPressed`,
`data1` is the `Key` and `data2` is the repeat count. This is intentionally
untyped and minimal; it keeps the struct small and the event loop fast, at the
cost of having to know what each field means per type. That trade-off is fine
for an engine this size.

### 3.4 `Core/GameLoop.h` + `src/Core/GameLoop.cpp` - the fixed-timestep loop

What it is: the heart of the engine. It runs `fixedUpdate`, `update`, and
`render` callbacks in the correct rhythm, measures FPS, and enforces a frame
cap.

Why fixed timestep: if physics used the variable real frame time, a slow frame
would make the player move further and a fast frame less far. Physics would be
non-deterministic and hard to debug. Running the simulation at a constant rate
makes it deterministic: on any machine, at any FPS, the same inputs produce the
same simulation.

How `run()` works:

1. Record the frame start time.
2. Compute `frameTime` (real time since the last frame) and clamp it to
   `m_maxFrameTime` (0.25 s default). This is the anti-spiral-of-death: after a
   long stall (debugger breakpoint, alt-tab, suspend), the accumulator would
   otherwise try to catch up with thousands of physics steps and freeze the
   app.
3. `accumulator += frameTime`. While `accumulator >= fixedTimestep`, run
   `fixedUpdate(fixedTimestep)` and subtract. Because the sim runs 30 Hz by
   default, an 60 FPS screen alternates between 2 and 3 fixed steps per frame.
4. Build `FrameStats`: `deltaTime` (real frame time), `interpolation`
   (`accumulator / fixedTimestep`, the [0,1] fraction used by the renderer to
   blend between fixed steps), and `fps`.
5. Call `update(frameTime)` once, then `render(stats)` once.
6. If a frame cap is set, sleep the remaining time.

FPS measurement detail: FPS is computed over a rolling 0.5 s window
(`frameCount / fpsWindow`), and the last measured value is kept in `m_lastFps`.
The `stats.fps` field always reports the last measurement, so it never shows 0
just because the current frame did not cross a measurement boundary (this was
an actual bug, see CHANGELOG Phase 0 bug 2).

Why the frame cap exists: vsync is unreliable on Wayland (see section 10), so
the loop has a sleep-based cap as a fallback. With a 60 FPS cap and a 30 Hz
fixed step, `interpolation` is never stale, so render interpolation stays
smooth.

### 3.5 `Core/Input.h` + `src/Platform/Input.cpp` - polling input + actions

What it is: a polling-based input layer over GLFW, plus a semantic action
mapping layer.

Why polling and not events: for game input, "is key X down right now" is the
question every frame; push events are only useful for edge detection. GLFW
itself is polling-oriented (`glfwGetKey`), so the engine stays polling.

What it provides:

- `Key` enum: mirrors GLFW's numeric key codes (ASCII for letters and digits,
  GLFW constants for the rest). The values are an implementation detail; game
  code always uses `Key::W`, `Key::Left`, and so on.
- `MouseButton` enum.
- `isKeyDown`, `isKeyDownAny`, `isMouseDown`: direct GLFW queries.
- `mousePosition()` / `mouseDelta()`: cursor position in framebuffer pixels
  (the same coordinate space as `Camera2D`). `beginFrame()` reads the raw
  cursor position from GLFW (logical window coordinates) and scales it by
  `drawableWidth / windowWidth` so HiDPI/retina scaling is handled.
- `setMouseCapture`: hides and locks the cursor (GLFW `CURSOR_DISABLED`).
- Action mapping: `mapAction("MoveLeft", {Key::A, Key::Left})` binds a semantic
  name to one or more keys; `isActionDown("MoveLeft")` returns true if any of
  them is pressed. Remapping a name replaces its keys.
- `translateKey(int)`: the single place where a backend key code becomes a
  `Key`. It validates the code ranges (printable ASCII minus the punctuation
  GLFW never emits, plus the 256..348 ranges) and returns `Key::Unknown` for
  anything else. The event layer uses it too, so events and polling agree on
  key identity.

Why actions: game logic should never ask for raw keys, because remapping
(rebindable controls) would touch gameplay code. Binding semantic names to key
sets in one place makes rebinding a one-line change.

`Input` is a static class (no instances): there is exactly one keyboard/mouse
in the system. `Window::init` calls `Input::bind(window)` and `shutdown` calls
`unbind`, so apps never touch the binding step.

---

## 4. Math module

### 4.1 `Math/Math.h` - the hand-written math library

What it is: everything the engine needs for 2D math, in one header:
`vec2`, `vec3`, `vec4`, `mat4`, `Transform2D`, `lerp`, `transformPoint`,
`ortho`, `translate`, `scale`, `mul`.

Why hand-written: the engine only needs a fraction of GLM, and writing it
teaches how matrices really work (the stated goal of the project). There is no
runtime cost difference.

Details worth knowing:

- `vec2` has the usual operators (`+`, `-`, scalar `*`, `+=`, `-=`) and a
  `lerp(a, b, t)` helper.
- `Transform2D` is the 2D transform used by the renderer:
  - `position`: the quad center in world coordinates.
  - `rotation`: in radians.
  - `scale`: the quad's world-space size (e.g. `{64, 64}` for a 64x64 tile).
  - `identity()` returns a unit transform.
- `transformPoint(transform, local)`: converts a local-space point (relative
  to the quad center, in half-extents [-0.5, 0.5]) into world space, applying
  scale, rotation, and translation. This is the quad-expansion workhorse used
  by the batch renderer. One sin/cos per quad.
- `vec3` and `vec4`: plain storage structs (no operators needed yet). `vec4` is
  used for colors and UV rectangles.
- `mat4` is column-major, the same layout OpenGL expects for uniforms.
  - `identity()`, `mul(a, b)` (result = a * b, meaning "apply b first, then
    a"), `translate`, `scale`.
  - `ortho(left, right, bottom, top, zNear, zFar)`: standard OpenGL orthographic
    projection. Note that `bottom > top` is legal and flips the Y axis; the
    camera uses exactly this trick to make world Y grow downward (see 8.6).

Design consequence for the future: the renderer consumes a `mat4` projection
and nothing else, so the camera object could produce a perspective matrix later
without touching the renderer. This is discussed in the CHANGELOG under
"Futuro possível: 2.5D / 3D".

---

## 5. ECS module (Entity Component System)

The engine has its own ECS, built from scratch to learn how data-oriented
engines store entities.

The core idea: an entity is just a number. All data lives in component pools,
one pool per component type. Iterating all entities that have, say,
`Transform` and `Sprite` means walking one dense array of `Transform`s and
following along the parallel arrays of entities and sprites. No object
pointers, no inheritance, no virtual calls on the hot path. This is what makes
"10,000 tiles in one draw call at 60 FPS" possible: the render path never
allocates, never jumps through objects, and touches contiguous memory.

### 5.1 `ECS/Entity.h` - the entity handle

```cpp
struct Entity {
    uint32_t index;      // position in the World's entity arrays
    uint32_t generation; // bumped on destroy; invalidates stale handles
};
```

An `Entity` is a handle, not an object. `index` locates the entity in the
world's arrays; `generation` protects against use-after-free: when an entity is
destroyed, its generation is incremented, so any old handle to it no longer
matches and `isAlive` returns false. `valid()` just checks the index is not the
sentinel max value.

### 5.2 `ECS/ComponentPool.h` - sparse-set storage

`ComponentPool<T>` stores all components of type `T` in two structures:

- **Dense arrays**: `m_components` (the actual components) and `m_entities`
  (which entity owns each component), kept in the same order. Iteration walks
  these contiguous vectors, which is cache friendly.
- **Sparse index**: `m_sparse`, one `SparseEntry {dense, generation}` per
  entity index, mapping entity -> position in the dense array. The generation
  is stored per entry so that a stale handle to a reused index does not alias
  a live component.

Key operations:

- `add`: if the entity already has the component, replace in place (no
  duplicate). Otherwise push to both dense arrays and set the sparse entry.
- `get`: assert the component exists, return the reference. `has` checks.
- `remove`: swap-and-pop. The last dense element moves into the freed slot and
  its sparse entry is updated. This is O(1) but makes iteration order unstable
  after removals (fine: games rarely rely on entity order).
- `onEntityDestroyed(index)`: called by the World when an entity dies, so every
  pool drops that entity's component without the World knowing the type.

`IComponentPool` is a type-erased base with a single virtual (the destroy
notification). The World stores pools as `unique_ptr<IComponentPool>` so it can
own pools of any type; the virtual is only called on entity destruction, never
on the hot path.

### 5.3 `ECS/View.h` - iterating entities with several components

`world.view<Transform, Sprite>()` returns a `View` that iterates every entity
having **all** requested components. Dereferencing an iterator yields
`std::tuple<Entity, Transform&, Sprite&>` (structured bindings unpack it).

The clever bit: the view iterates the **first** component's dense array
(`using Primary = tuple_element_t<0, ...>`), and for each entry checks that the
entity also has every other component (via the sparse arrays, which is O(1)).
This means the iteration itself walks contiguous memory, and only entities
missing components are skipped. A null pool (the type was never added to the
world) is handled gracefully: the iterator treats it as "no entity has it" and
the view is empty.

### 5.4 `ECS/World.h` + `src/ECS/World.cpp` - entities and pools

The World owns everything:

- `createEntity()`: pops an index from the free list (reusing destroyed
  indices with their current generation) or appends a fresh index with
  generation 0.
- `destroyEntity(e)`: no-ops if not alive; marks dead, bumps the generation,
  notifies every pool via `onEntityDestroyed`, pushes the index to the free
  list. Because stale handles are invalidated by the generation bump, a
  destroyed entity's handle can never touch the wrong data later.
- `isAlive(e)`: index in range + alive flag + generation match.
- `entityCount()`: `versions.size() - freeList.size()`.
- `add/has/get/remove<T>`: forwarded to the per-type pool.
- `pool<T>()`: returns the pool or null. `ensurePool<T>()` lazily creates it
  on first use. Pools are keyed by `std::type_index`.
- `view<Components...>()`: forwards pools (possibly null) to a `View`.

The template implementations live in the header because C++ templates must be
visible at the call site; `World.cpp` only holds the non-template functions
(destructor, create, destroy, isAlive).

### 5.5 `ECS/Components.h` - the engine's built-in components

Components are plain data structs; the engine provides a small set and games
can add their own in the same style.

- `Transform`: `position` (vec2), `rotation` (radians), `scale` (world size).
- `Velocity`: `value` (vec2, world units per second).
- `Collider`: `halfExtents` (vec2, half sizes of the collision box centered on
  the transform) and `solid` (bool). Semantics: an entity with
  `Collider + Velocity + Transform` is dynamic; with `Collider` only it is a
  static obstacle. Non-solid colliders never block; they are reserved for
  triggers (used from Block 10 onwards).
- `Player`: gameplay marker plus `previous` (position at the start of the
  current fixed step, used for render interpolation) and `speed`.
- `Tilemap`: a data-driven tile grid (see section 9). `width`/`height` in
  tiles, `tileSize` in world units, `tiles[]` (index into the app's tileset)
  and `solid[]` (0/1 per cell) vectors. Helpers: `indexAt`, `isSolid`,
  `cellX/cellY` (world coordinate relative to the map origin -> cell
  coordinate). The owning entity's `Transform.position` is the map origin
  (top-left of cell 0,0).

Why `Tilemap` is one component instead of 10,000 entities: entities are great
for things that move and change, but a static grid of 10,000 identical tiles
would cost 10,000 entity slots plus 10,000 sprite components. As plain data it
is one vector, iterated only over the visible range, and collided against by
direct cell lookup. This is the data-driven lesson of the project: structure
follows data.

---

## 6. Platform module

### 6.1 `Platform/Window.h` + `src/Platform/Window.cpp` - GLFW window + context

What it is: the only place in the engine that talks to GLFW. It owns the
window, the OpenGL context, and the event callbacks.

Why GLFW: the engine previously used SDL2; the SDL2 -> GLFW migration is
documented in the CHANGELOG (section 10 summarizes it). GLFW is a dedicated
windowing library, lighter than SDL, and its Wayland/X11 backends are
selectable at init time, which this machine needs.

How it works:

- `WindowProps` holds the settings: title, size, vsync, and a `Platform`
  selection (`Auto`, `Wayland`, `X11`). The `NBX_PLATFORM=x11|wayland`
  environment variable overrides the choice at init (with a warning for
  unknown values).
- `init()`:
  - Optionally sets `glfwInitHint(GLFW_PLATFORM, ...)` to force the backend.
  - `glfwInit()`.
  - Requests a GL 3.3 core context, **EGL context creation API**, double
    buffering, 24-bit depth buffer, and (in debug builds) a debug context.
    The EGL-forcing hint exists because on this machine GLX content does not
    present on Xwayland with the NVIDIA driver, while EGL does (see 10).
  - Creates the window, makes the context current, sets the swap interval
    (vsync on/off).
  - Registers GLFW callbacks that translate into `Event`s and call
    `emit(event)`: framebuffer resize -> `WindowResized`, close -> `Quit`,
    key press/release -> `KeyPressed`/`KeyReleased` (translated through
    `Input::translateKey`, with the repeat flag in `data2`), focus ->
    `FocusGained`/`FocusLost`.
  - Binds `Input::bind(window)`.
  - Logs everything interesting: logical vs drawable size, window position,
    GLFW version, and the actual platform in use.
- `pollEvents()`: `glfwPollEvents()` then `Input::beginFrame()` (so cursor
  position/delta are per-frame fresh).
- `swapBuffers()`: presents the frame.
- `shutdown()`: unbinds input, destroys the window, `glfwTerminate()`.
- `width()`/`height()`: **framebuffer** size in pixels (logical size times the
  HiDPI scale). Rendering and input work in framebuffer pixels, so everything
  stays consistent on scaled displays.

---

## 7. Renderer module

### 7.1 `Renderer/Buffer.h` + `src/Renderer/Buffer.cpp` - GL buffer wrappers

Thin wrappers over GL buffer objects with a vertex layout system:

- `ShaderDataType`: Float, Float2/3/4, UInt, and `UByte4` (a normalized
  4x8-bit color, used for per-vertex tinting without wasting bytes).
- `BufferElement` + `VertexBufferLayout`: describes one attribute (type, name)
  and computes its byte offset and the total stride. `VertexArray` uses this to
  set up `glVertexAttribPointer` for every element.
- `VertexBuffer`: `glGenBuffers`/`glBufferData` (STATIC_DRAW), `setData` via
  `glBufferSubData` (used every flush to re-upload the batch), move semantics
  that transfer the GL id without leaks.
- `IndexBuffer`: holds the count; used with an index array.
- `VertexArray`: bundles buffers + layout into one VAO. `addVertexBuffer`
  binds the buffer and configures the attributes from the layout;
  `setIndexBuffer` stores the index count.

Why the layout abstraction: the batch renderer's vertex format (pos3 + uv2 +
color4) is expressed as a layout, so a custom shader with a different format is
a different layout, not new GL code.

### 7.2 `Renderer/Texture.h` + `src/Renderer/Texture.cpp` - textures

- `loadFromFile(path)`: stb_image decodes the file (png, jpg, bmp, tga, gif,
  and more) into RGBA8. `stbi_set_flip_vertically_on_load(1)` flips the image
  so row 0 is the top of the image, matching how the engine treats UVs
  (v grows downward). A failure is logged with stb's reason and returns false.
- `create(w, h, pixels)`: uploads raw RGBA8 pixels. Filtering is LINEAR (bilinear
  smoothing when scaling) and wrapping is CLAMP_TO_EDGE (no bleeding outside
  the texture edges).
- Move-only (copy deleted): the GL id must be owned by exactly one object.
  Moves transfer the id and zero out the source, so the destructor never
  deletes a live id twice.
- `bind(slot)`: binds to a texture unit.

The flip decision: GL convention puts (0,0) at the bottom-left of a texture;
image files put row 0 at the top. Flipping on load means the engine can think
of UV v=0 as "top of image", which matches the y-down world.

### 7.3 `Renderer/TextureAtlas.h` + `src/Renderer/TextureAtlas.cpp` - sprite sheets

`TextureAtlas` slices a texture into a uniform `columns x rows` grid of cells
and hands out one `Sprite` per cell. It owns the texture (the atlas must not be
moved after sprites are handed out, because sprites store texture ids, not
pointers). `cell(column, row)` and `cell(index)` (row-major index) compute the
UV rectangle for the cell from the grid dimensions.

Why it exists: one texture with many cells is the standard way to render many
different tiles in one draw call; texture switching is what breaks batching
(see 7.7). Also the basis for sprite animation in Block 7.

### 7.4 `Renderer/Sprite.h` - a region of a texture

```cpp
struct Sprite {
    uint32_t textureId;              // NOT a pointer: survives texture moves
    math::vec4 uv;                   // minX, minY, maxX, maxY (normalized 0..1)
    float width, height;             // pixel size of the region
};
```

A sprite is just data: which texture, which normalized UV rectangle, and the
region's pixel size. `fullTexture` builds a whole-texture sprite. Storing the
id (not a pointer to the Texture object) means sprites stay valid even if the
owning texture is moved (e.g. into an atlas), as long as the GL object still
exists.

### 7.5 `Renderer/Shader.h` + `src/Renderer/Shader.cpp` - GLSL shaders

What it does: compiles and links a vertex + fragment shader pair, caches
uniform locations, and supports hot reload.

- `compile(type, source)`: compiles one stage; on failure logs the info log
  (vertex vs fragment) and returns 0.
- `loadFromSource(vs, fs)`: compiles both, links the program, logs errors, and
  clears the uniform cache (new program, new locations).
- `loadFromFile(vsPath, fsPath)`: reads the files, calls `loadFromSource`, and
  stores the paths + modification times for hot reload.
- `update(dt)`: polls the files' `last_write_time` every 0.5 s (the timer is
  accumulated from `dt`). If either changed, it reloads. If the reload fails
  (e.g. a syntax error mid-edit), it keeps the previous program and logs an
  error, so a broken edit does not kill the running app.
- `setInt/Float/Vec2/Vec4/Mat4`: look up the uniform (cached in an
  unordered_map; `glGetUniformLocation` is not free) and set it.

Why hot reload: shaders are the fastest thing to iterate in a 2D engine, and
editing a shader file while the app runs (it picks up the change within half a
second) is a huge quality-of-life win.

### 7.6 `Renderer/Camera2D.h` + `src/Renderer/Camera2D.cpp` - the 2D camera

- World coordinates are **y-down** (screen-like: y grows downward), and 1 world
  unit = 1 pixel at zoom 1.
- `viewProjection()` returns the orthographic matrix centered on the camera
  position: `ortho(pos.x - halfWidth, pos.x + halfWidth, pos.y + halfHeight,
  pos.y - halfHeight, -1, 1)`. Passing `bottom = pos.y + halfHeight` and
  `top = pos.y - halfHeight` (bottom > top) makes the projection flip Y, which
  is how the y-down world becomes screen space. This is the matrix handed to
  `Renderer::beginFrame`; the renderer never knows it is orthographic.
- `follow(target, smoothing, dt)`: frame-rate-independent exponential smoothing
  towards the target: `factor = 1 - exp(-smoothing * dt)`, then
  `position = lerp(position, target, factor)`. Using `exp` makes the smoothing
  behave identically at 30 and 144 FPS (a naive `1 - smoothing*dt` would not).
- `setZoom` clamps to >= 0.01. `viewWidth()/viewHeight()` return the visible
  world size (viewport divided by zoom).
- `screenToWorld(screen)`: inverts the projection for mouse picking.
- `isVisible(min, max)`: AABB vs the view rectangle plus a cull margin; used to
  skip off-screen tiles.

### 7.7 `Renderer/Renderer.h` + `src/Renderer/Renderer.cpp` - the batch renderer

This is the performance core of the engine: everything visible is drawn with
one `glDrawElements` call per texture per frame.

The fixed capacity is 10,000 quads (40,000 vertices, 60,000 indices). The
vertex format is 24 bytes per vertex:

```cpp
BatchVertex { float x, y, z; float u, v; uint8_t r, g, b, a; }
```

Three float positions, two UV floats, four normalized color bytes. The color is
a per-vertex tint; flat-colored quads use a 1x1 white texture multiplied by the
color in the shader.

The index buffer is built once at init and covers the whole capacity (each quad
is two triangles: 0-1-2, 2-3-0 with a 4-vertex offset per quad). The vertex
buffer is dynamic: `flush()` uploads exactly the used bytes with
`glBufferSubData` and issues the draw.

The frame flow:

1. `beginFrame(projection)`: stores the projection matrix, resets the quad
   counter and stats. Nothing GL happens here.
2. `drawQuad(transform, sprite, tint)` / `drawQuad(transform, color)`: if the
   sprite's texture differs from the currently batched texture, `flush()` first
   (texture switches break batching, so they are minimized). Then `submit`
   expands the quad: for each of the 4 corners, `transformPoint` maps the local
   half-extent into world space, and the corner's UV and tint are written into
   the next free vertex slot.
3. `endFrame()`: `flush()` the remaining batch: bind shader, set `u_Projection`
   and `u_Texture`, bind the texture, draw all submitted indices in one call.
   `s_stats.drawCalls` increments per flush; `quadCount` counts submitted quads.

Init-time setup:

- `gladLoadGLLoader(glfwGetProcAddress)`: loads all GL functions from the
  running context.
- In debug builds, enables `GL_DEBUG_OUTPUT` with a callback that maps GL
  severities to Log levels (High -> Error, Medium -> Warn). This surfaces
  driver-level mistakes immediately.
- `glEnable(GL_BLEND)` with `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`: standard alpha
  blending (transparent PNGs work). `glDisable(GL_DEPTH_TEST)`: 2D does not
  need depth; quads draw in submission order (painter's algorithm by draw
  order).
- A default shader pair is compiled from embedded source strings, identical to
  the sandbox's `batch.vert`/`batch.frag`. If the app never calls `setShader`,
  this is what renders.
- The 1x1 white texture, VAO, and buffers are created once.

`setShader(shader)`: switches the whole batch to a custom shader. The contract
is documented in the header: the shader must declare `u_Projection` (mat4),
`u_Texture` (sampler2D), and use the same vertex layout (locations 0, 1, 2).
The sandbox uses this with hot reload.

`shutdown()`: destroys everything in reverse order. Because all GL objects are
destroyed here, the context must still be alive when it is called (see the
conventions in section 1).

---

## 8. Physics module

### 8.1 `Physics/AABB.h` - axis-aligned boxes

`AABB {min, max}` with:

- `fromCenterHalf(center, half)`: builds a box from a center and half-extents.
- `overlaps(other)`: strict AABB overlap test.
- `overlapX/overlapY(other)`: the interpenetration amount on each axis
  (positive while overlapping). Used to compute the exact push-out distance.

The collision model of the engine is AABB only: no circles, no polygons. For a
top-down tile game this is the right shape (tiles are squares, the player is a
box) and it keeps collision math trivial and fast.

### 8.2 `Physics/SpatialHashGrid.h` + `src/Physics/SpatialHashGrid.cpp` - broadphase

Why it exists: checking a moving box against every other collider is O(n).
Instead, the world is divided into uniform cells (default 128 px) and each
collider is inserted into every cell it touches. A query on a swept box only
looks at the cells the box spans, which is O(cells touched + candidates).

How it works:

- `insert(entity, box)`: computes the cell range covered by the box
  (`floor(min / cellSize)` .. `floor(max / cellSize)`) and pushes the entity
  into each cell's list.
- `query(box, out)`: walks the same cell range and appends every entity found.
  Entities spanning several cells can appear multiple times; the physics system
  accepts that (resolution stops at the first hit, so duplicates are harmless).
- `cellKey(cx, cy)`: packs both signed cell coordinates into one uint64 by
  casting each int32 through uint32 (two's complement, so negative coords map
  to the high half of the range) and shifting X up 32 bits. The result is
  unique per (cx, cy) pair and used as the unordered_map key.
- The grid is **rebuilt every step** (`clear()` + re-insert all colliders).
  Rebuilding is simpler and just as fast as incremental updates for the small
  entity counts of this engine; correctness never depends on stale positions.

### 8.3 `Physics/PhysicsSystem.h` + `src/Physics/PhysicsSystem.cpp` - movement + resolution

`step(world, dt)` does two things:

1. **Rebuild the broadphase**: insert every entity with `Collider + Transform`
   into the grid (both static and dynamic; static entities are those without
   `Velocity`).
2. **Move every dynamic entity** (`Collider + Velocity + Transform`) one axis
   at a time.

`moveAxis(world, entity, axisX, dt)` is where the collision resolution happens:

1. Compute `delta = speed * dt`. If zero, skip (nothing to resolve).
2. Move the position by delta, then build the **swept volume**: the box at the
   new position expanded by |delta| along the movement axis. This covers the
   whole path from start to end, so nothing can tunnel through a thin wall at
   high speed (as long as the step distance is smaller than a grid cell).
3. Query the grid for candidates overlapping the swept volume.
4. For each candidate (skipping self, entities without Collider, non-solid
   colliders, and entities without Transform):
   - `resolve(otherBox)` tests the actual current box against the candidate.
     If they overlap on **both** axes, push the position back by exactly the
     overlap on the movement axis (`position -= copysign(overlap, delta)`) and
     zero the axis speed. Then stop checking further candidates for this axis
     (one blocking wall is enough; checking more could double-push).
5. **Tilemap collision**: for every `Tilemap + Transform` in the world, compute
   the solid-cell range overlapped by the swept volume (clamped to the map
   bounds; out-of-map is non-colliding). Each solid cell becomes an AABB
   (cell origin + tileSize square) and goes through the same `resolve`. This is
   per-cell O(1) lookup, no grid insert needed.

Why axis-separated movement: moving and resolving X first, then Y, is what
lets a box slide along a wall instead of sticking into corners. If both axes
were resolved at once against both walls of a corner, the box would get stuck.
The X pass stops the X speed; the Y pass then moves freely and stops on the
other wall.

Why push-out by exact overlap: it guarantees zero interpenetration at rest, and
it works identically for the player walking into a wall and for an entity that
spawns already embedded in solid geometry (it is pushed out on the first step;
the sandbox spawns the player on open floor, so this is only a safety net).

`setCellSize` forwards to the grid; the sandbox uses 128 px (two tile widths),
which keeps queries small while rarely splitting a 48x48 player.

---

## 9. The sandbox (the demo game)

`sandbox/src/Main.cpp` is a complete game built on the engine: a maze with a
player that walks around, a following camera, zoom, and culling. It is the
proof that the engine works and the test bed for every feature.

### 9.1 The palette

`kTileColors` holds 16 colors as `0xAABBGGRR` constants. The byte order looks
backwards because of a real bug fixed during Phase 0: the constants used to be
`0xAARRGGBB`, but memory is little-endian and `glTexImage2D` with `GL_RGBA`
reads bytes in memory order, so the red and blue channels swapped on screen
(verified by pixel-sampling the actual window with `grim` + `magick`). The
fix: write the constants as `0xAABBGGRR` so the bytes land in RGBA order.

### 9.2 `makeTileAtlasTexture()` - the procedural tileset

Builds a 256x256 RGBA image in memory: a 4x4 grid of 64x64 cells, each filled
with one palette color and a 4 px dark border (`0xFF11151C`) so adjacent tiles
stay visually distinct. This procedural texture stands in for a real PNG
tileset until Block 6 (assets) lands.

### 9.3 `assetPath()` - CWD-independent asset resolution

The executable lives at `<repo>/build/sandbox/` (or `build-release/sandbox/`).
`assetPath` reads `/proc/self/exe` (a symlink to the running executable), takes
its parent directory, then the parent of that (the repo root), and appends
`sandbox/assets/<relative>`. Shaders and textures therefore resolve correctly
no matter what the current working directory is. Block 6 moves this hack into
the engine's AssetManager.

### 9.4 Startup sequence

1. `Log::setLevel(Info)`.
2. Create the window: 1280x720, vsync on, **platform forced to X11** with a
   TODO comment explaining why (the NVIDIA EGL Wayland presentation bug; the
   engine itself still defaults to Auto, and `NBX_PLATFORM=wayland` overrides).
3. `Renderer::init()`.
4. Load `batch.vert` + `batch.frag` through the hot-reloadable `Shader`;
   `Renderer::setShader` only if loading succeeded, otherwise keep the engine's
   internal default (the sandbox never exits because a shader file is missing).
5. Viewport + camera at the world center.
6. `TextureAtlas` created from the procedural texture (4x4 cells); the 16
   `tileSprites` are the atlas cells. Comment warns: do not move the atlas
   after this point.
7. The ECS world is created with exactly 2 entities (logged at startup):
   - The player: `Transform` at (96,96) with 48x48 scale, `Sprite` = atlas cell
     12 (the red tile), `Velocity`, `Collider` with 24x24 half-extents,
     `Player` with `previous` = spawn.
   - The tilemap entity: `Transform` at the origin + the `Tilemap` below.

### 9.5 The maze generator

`Tilemap` is 100x100 tiles of 64 px (6400x6400 world units), all solid by
default. The maze is carved by a **recursive backtracker**:

- The grid is divided into 33x33 cells of 2x2 tiles (100 / 3), separated by
  1-tile walls.
- Start at cell (0,0): carve the 2x2 tile block (solid = 0), mark visited, push
  on the stack.
- While the stack is not empty: shuffle the 4 directions with a seeded
  `std::mt19937` (seed 20260817, so the maze is deterministic across runs);
  take the first unvisited neighbor, knock down the 1-tile wall between the
  two cells (the wall column/row at `min(cx,nx)*3 + 2`), carve the neighbor,
  push it; if no unvisited neighbor exists, pop.

Why recursive backtracker and not the previous checkerboard: the checkerboard
of 2x2 blocks had open areas that only touched diagonally, so the player was
trapped inside a 128 px pocket. Backtracker guarantees every carved cell is
reachable from every other (verified with a Python replica + flood fill: all
6,532 floor tiles reachable from spawn).

Finally, floor cells keep tile index 0 (the dark cell); wall cells get
`4 + (tx + ty) % 12` (bright palette cells), so the maze reads visually.

### 9.6 Input mapping

Actions map semantic names to keys: Move* to WASD + arrows, ToggleCulling to
C, ZoomIn to `=` + numpad +, ZoomOut to `-` + numpad -. Game code only ever
asks `isActionDown("MoveLeft")`.

### 9.7 The game loop wiring

- `setFixedTimestep(1/30)` and `setFrameCap(60)`: the simulation runs at 30 Hz,
  rendering capped at 60 FPS (vsync normally handles it; the cap is the
  fallback).
- The event callback: `Quit` -> `loop.stop()`; `WindowResized` -> update
  viewport and camera size (HiDPI-aware resize).
- `fixedUpdateFn`:
  1. Read the four Move actions into a direction vector; normalize it so
     diagonal movement is not faster than axis-aligned (this matters: raw
     (1,1) would be sqrt(2) times faster).
  2. For the player: store `previous = position`, set `velocity = direction *
     speed` (or zero), and set `rotation = atan2(dy, dx)` so the sprite faces
     the movement direction.
  3. `physics.step(world, dt)` moves and resolves.
  4. Defensive world-bounds clamp (the maze walls already cover the edges;
     this guarantees the player can never escape even if physics is disabled).
- `updateFn`:
  1. `window.pollEvents()`.
  2. `shader.update(dt)`: hot reload check; logs when a reload happened.
  3. C toggles culling on the falling edge (the static `cWasDown` remembers the
     previous frame; holding C must not flip continuously).
  4. +/- adjust a zoom *target* (1.02x per frame, clamped 0.25..4); the camera
     zoom eases toward the target (0.1 factor), which feels smooth.
  5. `camera.follow(player.position, 8.0, dt)`.
- `renderFn`:
  1. `clear` with a dark blue-grey, `beginFrame(camera.viewProjection())`.
  2. Tilemap: compute the cell range covered by the view rectangle (via
     `camera.position()` and `viewWidth/viewHeight`, clamped to the map), or
     the whole map when culling is off. Draw one quad per visible cell from
     the corresponding `tileSprites` entry. With culling on this is ~111
     quads; off, 10,000.
  3. Player: draw at `lerp(previous, position, interpolation)` with the
     rotation and scale. The interpolation factor comes from `FrameStats`; this
     is what keeps 30 Hz physics looking smooth on a 60 Hz display.
  4. `endFrame()` (flushes the batch), `swapBuffers()`.
  5. Every 60 frames, log `fps=60 quads=... drawCalls=... culling=...`.

### 9.8 Shutdown order (important)

```cpp
Renderer::shutdown();   // destroys all GL objects while the context lives
shader = Shader{};      // releases the custom shader program
atlas = TextureAtlas{}; // releases the atlas texture
window.shutdown();      // only now the context dies
```

The rule from section 1: GL resources die before the context. The sandbox had a
real SIGSEGV from violating this (the window was destroyed before a local
`Texture` went out of scope; the destructor then called `glDeleteTextures`
with no context). The reset-to-empty idiom (`x = T{}`) releases the resource
eagerly while the context is guaranteed alive.

---

## 10. The Wayland saga (why the platform code looks the way it does)

The windowing history explains several "weird" details in the code:

1. The engine originally used SDL2. The sandbox ran fine (60 FPS, all GL work
   done) but **no window ever appeared** on Hyprland (Wayland). `hyprctl
   clients` never listed it. A `WAYLAND_DEBUG=1` trace showed the app created
   the toplevel and acked the configure, but **never attached a real buffer**
   (only `attach(nil)` + one `commit`). No buffer, no mapping.
2. Even a minimal SDL2 window with no GL failed identically, and GLFW 3.5.1's
   native Wayland backend failed the same way. Conclusion: the root cause is
   NVIDIA's EGL -> wl-buffer presentation on this driver (610.57.04) +
   Hyprland 0.56.2, not any windowing library.
3. X11 (via Xwayland) presents fine. So the engine sets its platform via
   `glfwInitHint` (the `GLFW_PLATFORM` env var is ignored by this Arch GLFW
   build), and the sandbox defaults to X11 with a `TODO(nvidia-wayland)`
   comment.
4. On X11, GLX content did not present either (NVIDIA on Xwayland), but EGL
   did. Hence `glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API)`
   in `Window::init`: it selects the EGL path on both Wayland and X11.
5. The suggested system-side fix (not applied, needs root): enable
   `nvidia_drm.modeset=1` on the kernel command line. The engine itself
   already defaults to platform Auto, so once the driver is fixed, the sandbox
   can drop the X11 override.

---

## 11. The build system

### 11.1 Top-level `CMakeLists.txt`

- `project(Nebrix VERSION 0.1.0 LANGUAGES C CXX)` (glad.c needs C).
- C++23, extensions off.
- Default build type Debug if none given.
- `NEBRIX_BUILD_SANDBOX` option (ON by default).
- `find_package(glfw3 3.4 REQUIRED)`: the only external package; the top level
  finds it because `Nebrix` links it `PUBLIC` (Window.h exposes
  `GLFWwindow*`).
- Adds `engine` and (optionally) `sandbox`.

### 11.2 `engine/CMakeLists.txt`

- `add_library(Nebrix STATIC ...)`: the 14 engine .cpp files plus the vendored
  `glad.c`.
- `target_include_directories`: PUBLIC `engine/include` (the public API), PRIVATE
  `vendor/glad/include` and `vendor/stb` (so GL and stb headers never leak into
  user code).
- Links `glfw` PUBLIC.
- `-Wall -Wextra -Wpedantic` on the engine code; `NEBRIX_DEBUG` defined for
  Debug configs only.
- `glad.c` is compiled with `-w` (warnings suppressed): it is third-party
  generated code.

### 11.3 `sandbox/CMakeLists.txt`

- `add_executable(NebrixSandbox src/Main.cpp)`, links `Nebrix` PRIVATE, same
  warning flags. The sandbox gets the engine's PUBLIC includes transitively and
  never sees glad/stb/glfw headers directly.

### 11.4 Vendor dependencies

- `vendor/glad`: a pre-generated GL 3.3 core loader (loader functions on),
  sourced from the arrayfire/glad repo because this machine has no pip/glad
  CLI. `glad.c` implements `gladLoadGLLoader`, which `Renderer::init` feeds
  with `glfwGetProcAddress`.
- `vendor/stb`: `stb_image.h` (used by `Texture`) and `stb_truetype.h` (for the
  Block 8 font work). `src/Platform/Stb.cpp` is the single translation unit
  that defines `STB_IMAGE_IMPLEMENTATION`, so the implementation is compiled
  exactly once.

---

## 12. Why each design decision was made (summary)

| Area | Decision | Reason |
|---|---|---|
| Language | C++23 | available compiler, modern standard |
| Build | CMake + Ninja | simple, fast, installed |
| Windowing | GLFW 3.5, backend-agnostic `Event` | dedicated windowing; selectable Wayland/X11 backend; no buffer on native Wayland with this driver |
| GL | 3.3 core via glad (vendored) | modern GL without system deps |
| Images/Fonts | stb_image + stb_truetype (vendored) | single-header, industry standard |
| Math | hand-written | 2D needs little; instructive; zero deps |
| Audio (later) | miniaudio (vendored) | keeps the zero-package philosophy |
| ECS | hand-written sparse sets | learn how engines really store data; cache-friendly iteration |
| Rendering | batch renderer, one draw call per texture | 10k sprites at 60 FPS requires it |
| Physics | AABB + spatial hash + axis-separated sweep | simple, fast, tile-perfect, no tunneling |
| Tilemaps | data component, not entities | 10k tiles as one vector beats 10k entities |
| Frame rate | fixed 30 Hz sim + interpolation + 60 FPS cap | deterministic physics; smooth rendering; vsync unreliable on Wayland |
| Input | polling + semantic actions | game code never binds raw keys |
| Errors | NBX_ASSERT (log + abort) + typed logging | visible, simple, immediate |
| Shaders | file-based with hot reload | iterate without restarting |

---

## 13. History in one page

- **Phase 0**: foundation. Vendored deps, CMake skeleton, Log/Assert, Window,
  GameLoop, math library, first Shader/Buffer/Texture/Renderer, checkerboard
  demo. Three real bugs found and fixed (GL resources outliving the context,
  FPS reported 0 most frames, vsync unreliable on Wayland).
- **SDL2 -> GLFW migration**: the "no window" investigation ended at the NVIDIA
  EGL/Wayland driver bug; GLFW replaced SDL2, platform selection and
  `NBX_PLATFORM` added, sandbox defaults to X11. Two follow-up bugs fixed:
  missing `swapBuffers()` (window stayed transparent) and the R/B color swap
  (`0xAABBGGRR` constants).
- **Phase 1**: batch renderer (the 10k milestone: `quads=10000 drawCalls=1
  fps=60`), Sprite/TextureAtlas, Camera2D with follow/zoom/culling, shader hot
  reload, stress-demo sandbox.
- **Phase 2, Block 1**: input abstraction (polling + action mapping).
- **Phase 2, Block 2**: the ECS (Entity/ComponentPool/View/World), sandbox
  migrated to 10,001 entities, then refactored to one header per class.
- **Phase 2, Block 3**: physics (AABB, spatial hash grid, axis-separated sweep
  resolution). 6/6 standalone tests pass.
- **Phase 2, Block 4**: Tilemap component, tilemap collisions in physics,
  sandbox shrunk to 2 entities, visible-cell rendering.
- **Sandbox fix**: maze replaced by a recursive-backtracker maze (connected by
  construction, verified by flood fill).
- **Roadmap**: Assets -> Animations -> Text/UI -> Scenes -> Combat -> Audio
  (Block 6 through 11), recorded in CHANGELOG. The engine was compared against
  the user's university engine (OGL-Game-Engine) and cubos; the architecture
  already keeps the door open for 2.5D/3D (projection-agnostic renderer,
  dimension-agnostic ECS) with four design rules recorded in the CHANGELOG.

---

## 14. Design rules for the future (keep the 2.5D/3D door open)

The current architecture already supports a future 2.5D or 3D mode without
rework, provided these four rules are never violated:

1. Never hardcode orthographic projection into shaders or the renderer. The
   projection always arrives as the `u_Projection` matrix from the camera.
2. The renderer only consumes a `mat4` from `beginFrame`; never bind the
   renderer to `Camera2D` specifically.
3. Keep components as plain data with generic names. A future 3D mode adds
   `Transform3D`/`Camera3D`; it does not modify `Transform`.
4. Z-sorting must stay an optional renderer mode, not something baked into the
   2D path, so a depth-buffer path can coexist later.

---

## 15. The assets pipeline (Block 6)

Content no longer lives in code. Three pieces work together:

### 15.1 `Assets/AssetManager.h` + `src/Assets/AssetManager.cpp`

- Resolves the assets root from `/proc/self/exe` (executable at
  `<repo>/build*/sandbox/` implies root `<repo>/sandbox/assets`), so the game
  runs from any working directory.
- `getTexture(relative)` caches one `Texture` per name. A missing or unreadable
  file logs an error once and returns a cached 1x1 magenta texture, keeping the
  app alive and the problem impossible to miss on screen.
- `getShader(name, vertRelative, fragRelative)` caches compiled shaders and
  keeps hot reload alive; compile failures return nullptr (cached) so callers
  can fall back to the renderer's internal default shader.
- `shutdown()` frees every cached GL resource; it must run before
  `Window::shutdown()` (the context-death rule from section 1).

### 15.2 `Renderer/SpriteSheet.h`

A non-owning uniform grid view over a texture id: `sprite(index)` returns the
`Sprite` for row-major cells counted from the top-left of the image. It pairs
with manager-owned textures (the manager owns, the sheet only references), and
takes explicit cell pixel sizes.

### 15.3 `sandbox/assets/generate_assets.py`

A deterministic Pillow script that generates `textures/tileset.png` (256x256,
4x4 cells of 64px with dark borders, the same palette the procedural atlas
used, in true RGB now that colors travel inside a file) and
`textures/player.png` (48px with a face notch so rotation is visible). The
sandbox loads both through the AssetManager; tile colors are byte-identical to
the old procedural ones, which is what the grim+magick verification checks.

### 15.4 Animation (Block 7)

- `Animation` component: pre-resolved `frames` (a `vector<Sprite>` built once
  from a `SpriteSheet`), `fps`, `loop`, `playing`, `timer`, `index`. Frame 0
  is the idle pose by convention; no sheet coupling inside the component.
- `AnimationSystem::update(world, dt)` runs in fixedUpdate before physics:
  advances the timer (fmod-wrapped on loop so float precision never degrades),
  clamps on non-loop, and always copies the current frame into the entity's
  `Sprite` (paused means a frozen pose, still written).
- `player_walk.png` (192x48, 4 frames) is generated alongside the other
  assets; the sandbox sets `playing` from input and resets to frame 0 on stop.

### 15.5 Text, UI and menu (Block 8)

- `Font` bakes a vendored TTF (`sandbox/assets/fonts/`, OFL-licensed) into a
  512x512 RGBA atlas with stb_truetype: white glyphs, coverage in alpha, so
  text renders through the standard batch as texture * tint. Glyph metrics
  (advance, bearings, UV) plus `measure()` and `ascent()`; unknown chars fall
  back to '?'.
- `Renderer::drawText` draws a top-left-anchored block, one batched quad per
  glyph, with baselines at block-top + ascent (stb yoff is negative-up from
  the baseline — anchoring at the block top renders a line too high).
- `UI/` is a minimal immediate-mode layer in screen-space pixels: `panel`,
  `label`, `labelCentered`, and `button` (hover tint + press-inside /
  release-inside click via per-id arm state, mouse snapshotted once per frame
  in `ui::beginFrame`). The click edge core is pure (`detail::clickEdge`) and
  unit-tested without GL.
- `AssetManager::getFont` caches baked fonts like textures and shaders. The
  sandbox boots into a menu (own screen-space ortho pass — the first
  multi-pass frame), START enters the maze, ESC returns, and an HUD pass
  shows fps + hints over the game.

### 15.6 Scenes (Block 9)

- `Scene` is a lifecycle base (onEnter/onExit/onResize + fixedUpdate/update/
  render); `SceneManager` owns the current scene and forwards everything.
  `switchTo<T>()` exits the old scene before entering the new one, so a
  restart is a fresh instance. Rule: never touch `this` after switchTo (it
  destroys the caller).
- Sandbox: `MenuScene` (title + START/QUIT), `GameScene` (own World, camera,
  physics, animations, maze built in onEnter; R restarts, ESC to menu), and a
  slim `Main.cpp` (init, action map, loop forwarding, shutdown). Window
  resizes reach scenes through `onResize`.

---

*Last updated: 2026-08-21 (Blocks 1-7 done; see section 15 for the parts
added after the original Phase 2 write-up). Written alongside the CHANGELOG;
when in doubt about history, read CHANGELOG.md.*
