#pragma once

#include "Nebrix/Physics/SpatialHashGrid.h"

namespace nbx {

class World;

// Moves dynamic entities (Collider + Velocity + Transform) against static ones
// (Collider without Velocity), one axis at a time, with overlap resolution.
// Broadphase via SpatialHashGrid.
class PhysicsSystem {
public:
    void setCellSize(float cellSize);

    // Integrates position from Velocity and resolves collisions. Call once per
    // fixed step. Only solid colliders (Collider::solid) block movement.
    void step(World& world, float dt);

private:
    void moveAxis(World& world, const Entity& entity, bool axisX, float dt);

    SpatialHashGrid m_grid;
};

} // namespace nbx