# Nebrix Engine - Learning Path

A study plan for building a 2D game engine from scratch, on your own, without AI
help. It is the map of where every decision in the Nebrix engine comes from:
each engine block below points to the canonical resource that teaches it, in
study order, with milestones.

This document is written in English, without em-dashes, and pairs with
`docs/ARCHITECTURE.md` (what the engine is) and `CHANGELOG.md` (how it got
built).

---

## 0. How to use this plan

The most efficient school that exists is this loop:

1. Study the resource listed for the topic (free online first).
2. Re-read the corresponding Nebrix file listed next to it.
3. Ask yourself: "Why is it written this way?" and "What would I change?"
4. Try to reproduce the file from memory, then diff against the real one.

Do not skip step 2 and 3. The engine is small (about 3,400 lines) and every
file maps to a well-known technique, which is what makes it a good teacher.

---

## 1. Level 0 - Foundations (before touching the engine)

- **C++17/20/23 basics**: references, RAII, move semantics, templates,
  `std::vector` and `std::unordered_map`.
  - Resource: "A Tour of C++" by Bjarne Stroustrup (short, 300 pages).
  - Milestone: you can write a small class with move semantics and a template
    function without looking anything up.
- **The core patterns**: Game Loop, Component, Data Locality, Update Method.
  - Resource: "Game Programming Patterns" by Robert Nystrom, free at
    gameprogrammingpatterns.com. Read at least: Game Loop, Update Method,
    Component, Data Locality.
  - Milestone: you can explain in one sentence why the engine separates
    fixed update from render.

---

## 2. Level 1 - The game loop

- **Topic**: fixed timestep, accumulator, render interpolation, frame cap,
  spiral of death.
- **Resource**: "Fix Your Timestep" by Glenn Fiedler, free at
  gafferongames.com. This is the single most important article in the whole
  plan.
- **Nebrix file**: `engine/include/Nebrix/Core/GameLoop.h` and
  `engine/src/Core/GameLoop.cpp`.
- **Questions to answer**: Why clamp frameTime? What does `interpolation`
  represent? Why does the FPS counter use a 0.5 s window?
- **Milestone**: write your own loop in a scratch file that runs a counter at
  30 Hz and renders at 60 Hz.

---

## 3. Level 2 - Math

- **Topic**: vectors, column-major matrices, transforms, orthographic
  projection, the Y-down world.
- **Resources**:
  - "Coordinate Systems" chapter of LearnOpenGL, free at learnopengl.com.
  - "3D Math Primer for Graphics and Game Development" by Dunn and Parberry
    (when you want the full treatment).
- **Nebrix file**: `engine/include/Nebrix/Math/Math.h`.
- **Questions to answer**: Why is the matrix column-major? How does passing
  `bottom > top` to `ortho` flip the Y axis? Why does `transformPoint` take
  local half-extents?
- **Milestone**: write `ortho` and a 2D rotation from scratch, then render
  something rotated with them.

---

## 4. Level 3 - Rendering (OpenGL)

- **Topic**: shaders, VAO/VBO, textures, UV coordinates, and the batch
  renderer (one draw call per texture).
- **Resources**:
  - LearnOpenGL, free at learnopengl.com: "Getting started" through
    "Textures", then study how batching works (a dynamic vertex buffer, a
    static index buffer, flush on texture switch).
  - The Cherno's OpenGL and Game Engine series on YouTube (C++, free; ignore
    the style choices you dislike, keep the concepts).
  - `docs.gl` and the Khronos OpenGL wiki as the API reference.
- **Nebrix files**:
  - `engine/include/Nebrix/Renderer/Buffer.h` + `engine/src/Renderer/Buffer.cpp`
  - `engine/include/Nebrix/Renderer/Texture.h` + `engine/src/Renderer/Texture.cpp`
  - `engine/include/Nebrix/Renderer/Shader.h` + `engine/src/Renderer/Shader.cpp`
  - `engine/include/Nebrix/Renderer/Renderer.h` + `engine/src/Renderer/Renderer.cpp`
- **Questions to answer**: Why is the color stored as 4 bytes, not 4 floats?
  Why does a texture switch force a flush? Why is the index buffer static?
- **Milestone**: render 10,000 colored quads in one draw call at 60 FPS. The
  Nebrix milestone line to match: `quads=10000 drawCalls=1 fps=60`.

---

## 5. Level 4 - The ECS

- **Topic**: entities as ids, sparse sets, cache-friendly iteration, views,
  generations, swap-and-pop.
- **Resources**:
  - "Game Programming Patterns": Component and Data Locality chapters.
  - "Overwatch Gameplay Architecture and Netcode" GDC talk (Tim Ford), free
    on YouTube. This is where the modern data-oriented ECS design comes from.
  - Austin Morlan's blog posts on ECS from scratch, free at austinmorlan.com.
  - The EnTT library (skypjack/entt on GitHub): read the README and the
    examples, do not read everything.
- **Nebrix files**:
  - `engine/include/Nebrix/ECS/Entity.h`
  - `engine/include/Nebrix/ECS/ComponentPool.h`
  - `engine/include/Nebrix/ECS/View.h`
  - `engine/include/Nebrix/ECS/World.h` + `engine/src/ECS/World.cpp`
- **Questions to answer**: Why store entities and components in parallel dense
  arrays? What does the generation counter protect against? Why does the View
  iterate the first component's array? Why is removal swap-and-pop?
- **Milestone**: rebuild a small ECS from memory (Entity, one pool, one view)
  and iterate a 100,000-entity scene without allocations.

---

## 6. Level 5 - Physics and collision

- **Topic**: AABB, overlap tests, spatial hash grid, swept movement,
  axis-separated resolution, tilemap collision.
- **Resources**:
  - "Real-Time Collision Detection" by Christer Ericson: the definitive
    reference for spatial partitioning (the grid is chapter 7 territory).
  - "2D Game Collision Detection" by Thomas Schwarzl (small, practical book;
    the sweep + axis-separated style is directly this).
  - Glenn Fiedler's older articles at gafferongames.com for the movement
    mindset.
- **Nebrix files**:
  - `engine/include/Nebrix/Physics/AABB.h`
  - `engine/include/Nebrix/Physics/SpatialHashGrid.h` + `engine/src/Physics/SpatialHashGrid.cpp`
  - `engine/include/Nebrix/Physics/PhysicsSystem.h` + `engine/src/Physics/PhysicsSystem.cpp`
- **Questions to answer**: Why move one axis at a time? Why expand the box
  into a swept volume before querying? Why rebuild the grid every step instead
  of updating it? How does the cell key pack signed coordinates?
- **Milestone**: a player box that slides along walls and cannot tunnel
  through them at any speed below one cell per step.

---

## 7. Level 6 - Tiles, grids, and game logic

- **Topic**: tilemaps as data, culling, pathfinding, procedural generation.
- **Resources**:
  - Red Blob Games, free at redblobgames.com: "Grids and Graphs", "A*",
    "Hexagonal grids" (even for square grids, the thinking transfers).
  - "Procedural Generation in Game Design" (if you want the maze/farming
    generator depths).
- **Nebrix files**: `engine/include/Nebrix/ECS/Components.h` (the Tilemap
  component) and `sandbox/src/Main.cpp` (visible-cell culling and the maze
  generator).
- **Milestone**: generate a connected maze and render only the visible cells.

---

## 8. Books to grow into (in order of value)

1. "Game Engine Architecture" by Jason Gregory: the bible. Covers loops, ECS,
   physics, audio, and how a real studio engine is organized. Read it twice,
   it pays for itself.
2. "Real-Time Collision Detection" by Christer Ericson: physics reference.
3. "Mathematics for 3D Game Programming and Computer Graphics" by Eric
   Lengyel: when the 3D door starts to tempt you.
4. "Game Physics Cookbook" by Gabor Szauer: a friendlier entry into rigid
   bodies than Ericson.
5. "Effective Modern C++" by Scott Meyers: the language, once the engine grows
   beyond 10k lines.
6. "Crafting Interpreters" by Robert Nystrom (free online): only if you ever
   decide to embed a scripting language (prefer embedding Lua over writing
   your own).

---

## 9. Reference engines to read (the best school after your own)

- **EnTT** (skypjack/entt): the state of the art in ECS. Read examples, not
  internals.
- **raylib**: small, readable, 2D and 3D. Compare its organization with the
  Nebrix module layout.
- **MonoGame**: read its `SpriteBatch` and compare it with the Nebrix batch
  renderer. This is the tech level behind Stardew Valley.
- **olcPixelGameEngine** (javidx9, OneLoneCoder): a single-header 2D engine;
  good for seeing an alternative design philosophy.
- **Godot**: never read it all. Read only the 2D renderer and the scene tree
  modules when you want to see how a grown engine structures systems.

---

## 10. The milestone map (checks to print and hang on the wall)

- [ ] Loop: 30 Hz fixed update, 60 FPS render, interpolation works
- [ ] Math: ortho and 2D rotation written by hand, no glm
- [ ] Renderer: 10,000 quads, 1 draw call, 60 FPS
- [ ] Texture: PNG loaded through stb, sprites with UVs
- [ ] ECS: entities, pools, views; 100k entities iterated without allocations
- [ ] Physics: swept movement, axis-separated resolution, no tunneling
- [ ] Tilemap: visible-cell rendering and O(1) solid-cell lookup
- [ ] Complete game: a player that walks, collides, and reaches a goal

Each check is a day-to-week of work for a beginner and a lesson that stays
with you. The Nebrix codebase is the answer key, and the CHANGELOG is the
diary of how it was written, including the bugs and the reasoning.

---

*Last updated: 2026-08-17. Companion to docs/ARCHITECTURE.md.*