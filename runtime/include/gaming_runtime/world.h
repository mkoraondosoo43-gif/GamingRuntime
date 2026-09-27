#pragma once

#include "gaming_runtime/entity.h"
#include "gaming_runtime/transform.h"
#include "gaming_runtime/renderable.h"

namespace gaming_runtime {

class GameWorld {
public:
    GameWorld(EntityManager& entities, TransformManager& transforms,
              RenderableManager& renderables)
        : entities_(entities), transforms_(transforms), renderables_(renderables) {}

    EntityId create_entity() {
        const EntityId entity = entities_.create();
        if (entity == kInvalidEntity ||
            !transforms_.create(entity) ||
            !renderables_.create(entity)) {
            if (entity != kInvalidEntity) {
                renderables_.destroy(entity);
                transforms_.destroy(entity);
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
        renderables_.destroy(entity);
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

    Renderable* renderable(EntityId entity) noexcept {
        return entities_.is_alive(entity) ? renderables_.get(entity) : nullptr;
    }

    const Renderable* renderable(EntityId entity) const noexcept {
        return entities_.is_alive(entity) ? renderables_.get(entity) : nullptr;
    }

private:
    EntityManager& entities_;
    TransformManager& transforms_;
    RenderableManager& renderables_;
};

} // namespace gaming_runtime
