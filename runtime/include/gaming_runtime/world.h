#pragma once

#include "gaming_runtime/entity.h"
#include "gaming_runtime/transform.h"

namespace gaming_runtime {

class GameWorld {
public:
    GameWorld(EntityManager& entities, TransformManager& transforms)
        : entities_(entities), transforms_(transforms) {}

    EntityId create_entity() {
        const EntityId entity = entities_.create();
        if (entity == kInvalidEntity || !transforms_.create(entity)) {
            if (entity != kInvalidEntity) {
                entities_.destroy(entity);
            }
            return kInvalidEntity;
        }
        return entity;
    }

    bool destroy_entity(EntityId entity) {
        if (!entities_.is_alive(entity)) {
            return false;
        }
        transforms_.destroy(entity);
        return entities_.destroy(entity);
    }

    bool alive(EntityId entity) const noexcept {
        return entities_.is_alive(entity);
    }

    Transform* transform(EntityId entity) noexcept {
        return entities_.is_alive(entity) ? transforms_.get(entity) : nullptr;
    }

    const Transform* transform(EntityId entity) const noexcept {
        return entities_.is_alive(entity) ? transforms_.get(entity) : nullptr;
    }

    std::size_t entity_count() const noexcept {
        return entities_.alive_count();
    }

private:
    EntityManager& entities_;
    TransformManager& transforms_;
};

} // namespace gaming_runtime
