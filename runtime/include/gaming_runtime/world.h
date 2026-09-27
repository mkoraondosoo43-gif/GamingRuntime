#pragma once

#include "gaming_runtime/entity.h"
#include "gaming_runtime/transform.h"
#include "gaming_runtime/renderable.h"
#include "gaming_runtime/render.h"
#include "gaming_runtime/camera.h"

namespace gaming_runtime {

class GameWorld {
public:
    GameWorld(EntityManager& entities, TransformManager& transforms,
              RenderableManager& renderables, CameraManager& cameras)
        : entities_(entities), transforms_(transforms), renderables_(renderables), cameras_(cameras) {}

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
        cameras_.destroy(entity);
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

    std::size_t render(RenderFrame& frame) const;

    Renderable* renderable(EntityId entity) noexcept {
        return entities_.is_alive(entity) ? renderables_.get(entity) : nullptr;
    }

    const Renderable* renderable(EntityId entity) const noexcept {
        return entities_.is_alive(entity) ? renderables_.get(entity) : nullptr;
    }

    Camera* camera(EntityId entity) noexcept {
        return entities_.is_alive(entity) ? cameras_.get(entity) : nullptr;
    }

    const Camera* camera(EntityId entity) const noexcept {
        return entities_.is_alive(entity) ? cameras_.get(entity) : nullptr;
    }

    bool attach_camera(EntityId entity) {
        return entities_.is_alive(entity) && cameras_.create(entity);
    }

    bool detach_camera(EntityId entity) {
        return entities_.is_alive(entity) && cameras_.destroy(entity);
    }

    std::size_t camera_count() const noexcept {
        return cameras_.count();
    }

private:
    EntityManager& entities_;
    TransformManager& transforms_;
    RenderableManager& renderables_;
    CameraManager& cameras_;
};

} // namespace gaming_runtime
