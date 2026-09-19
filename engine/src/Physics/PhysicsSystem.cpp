#include "Nebrix/Physics/PhysicsSystem.h"

#include "Nebrix/ECS/Components.h"
#include "Nebrix/ECS/World.h"

#include <cmath>
#include <vector>

namespace nbx
{

    namespace
    {

        AABB colliderBox(const Transform &transform, const Collider &collider)
        {
            return AABB::fromCenterHalf(transform.position, collider.halfExtents);
        }

    } // namespace

    void PhysicsSystem::setCellSize(float cellSize) { m_grid.setCellSize(cellSize); }

    void PhysicsSystem::step(World &world, float dt)
    {
        // Broadphase: bucket every collider (static and dynamic).
        m_grid.clear();
        if (ComponentPool<Collider> *colliders = world.pool<Collider>())
        {
            for (uint32_t i = 0; i < colliders->size(); ++i)
            {
                const Entity entity = colliders->entityAt(i);
                if (!world.has<Transform>(entity))
                    continue;
                m_grid.insert(entity, colliderBox(world.get<Transform>(entity), colliders->get(entity)));
            }
        }

        // Move dynamics one axis at a time so walls on X and Y resolve cleanly.
        for (auto [entity, collider, velocity, transform] : world.view<Collider, Velocity, Transform>())
        {
            (void)collider;
            (void)transform;
            moveAxis(world, entity, true, dt);
            moveAxis(world, entity, false, dt);
        }
    }

    void PhysicsSystem::moveAxis(World &world, const Entity &entity, bool axisX, float dt)
    {
        Transform &transform = world.get<Transform>(entity);
        Velocity &velocity = world.get<Velocity>(entity);
        const Collider &collider = world.get<Collider>(entity);

        float &position = axisX ? transform.position.x : transform.position.y;
        float &speed = axisX ? velocity.value.x : velocity.value.y;
        const float delta = speed * dt;
        if (delta == 0.0f)
            return;

        // Move first, then resolve against the swept volume (start + end positions).
        position += delta;
        AABB swept = colliderBox(transform, collider);
        // Swept volume = union of start and end boxes. The leading edge already
        // moved with `position`, so only the trailing side is expanded by
        // |delta| (expanding both sides doubled the query area for no benefit).
        if (axisX)
            (delta > 0.0f ? swept.min.x : swept.max.x) -= delta;
        else
            (delta > 0.0f ? swept.min.y : swept.max.y) -= delta;

        // Resolve against one solid box; returns true when movement was blocked.
        const auto resolve = [&](const AABB &otherBox)
        {
            const AABB box = colliderBox(transform, collider);
            if (axisX)
            {
                if (box.overlapX(otherBox) > 0.0f && box.overlapY(otherBox) > 0.0f)
                {
                    position -= std::copysign(box.overlapX(otherBox), delta);
                    speed = 0.0f;
                    return true;
                }
            }
            else if (box.overlapY(otherBox) > 0.0f && box.overlapX(otherBox) > 0.0f)
            {
                position -= std::copysign(box.overlapY(otherBox), delta);
                speed = 0.0f;
                return true;
            }
            return false;
        };

        std::vector<Entity> candidates;
        m_grid.query(swept, candidates);
        for (const Entity other : candidates)
        {
            if (other == entity || !world.has<Collider>(other))
                continue;
            const Collider &otherCollider = world.get<Collider>(other);
            if (!otherCollider.solid || !world.has<Transform>(other))
                continue;
            if (resolve(colliderBox(world.get<Transform>(other), otherCollider)))
                break;
        }

        // Static tilemaps: solid cells overlapping the swept volume.
        for (auto [tilemapEntity, tilemap, mapTransform] : world.view<Tilemap, Transform>())
        {
            (void)tilemapEntity;
            const float originX = mapTransform.position.x;
            const float originY = mapTransform.position.y;
            int32_t minX = 0;
            int32_t minY = 0;
            int32_t maxX = 0;
            int32_t maxY = 0;
            if (!tilemap.cellRange(swept.min.x - originX, swept.min.y - originY,
                                   swept.max.x - originX, swept.max.y - originY, minX, minY, maxX,
                                   maxY))
                continue;

            for (int32_t cy = minY; cy <= maxY; ++cy)
            {
                for (int32_t cx = minX; cx <= maxX; ++cx)
                {
                    if (!tilemap.isSolid(static_cast<uint32_t>(cx), static_cast<uint32_t>(cy)))
                        continue;
                    const AABB cellBox = {
                        {originX + static_cast<float>(cx) * tilemap.tileSize,
                         originY + static_cast<float>(cy) * tilemap.tileSize},
                        {originX + static_cast<float>(cx + 1) * tilemap.tileSize,
                         originY + static_cast<float>(cy + 1) * tilemap.tileSize}};
                    if (resolve(cellBox))
                        return;
                }
            }
        }
    }

} // namespace nbx