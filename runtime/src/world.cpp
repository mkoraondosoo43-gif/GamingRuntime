#include "gaming_runtime/world.h"

namespace gaming_runtime {

std::size_t GameWorld::render(RenderFrame& frame) const {
    const auto entities = entities_.alive_entities();

    const Camera* active_camera = nullptr;
    const Transform* camera_transform = nullptr;

    for (const EntityId entity : entities) {
        const Camera* camera = cameras_.get(entity);
        const Transform* transform = transforms_.get(entity);
        if (camera != nullptr && camera->active &&
            camera->zoom > 0.0f &&
            camera->viewport_width > 0 &&
            camera->viewport_height > 0 &&
            transform != nullptr) {
            active_camera = camera;
            camera_transform = transform;
            break;
        }
    }

    std::size_t submitted = 0;
    for (const EntityId entity : entities) {
        const Transform* transform = transforms_.get(entity);
        const Renderable* renderable = renderables_.get(entity);
        if (transform == nullptr || renderable == nullptr || !renderable->visible) {
            continue;
        }

        float x = transform->position.x;
        float y = transform->position.y;
        float width = transform->scale.x > 0.0f ? transform->scale.x : 0.0f;
        float height = transform->scale.y > 0.0f ? transform->scale.y : 0.0f;
        if (width <= 0.0f || height <= 0.0f) {
            continue;
        }

        if (active_camera != nullptr && camera_transform != nullptr) {
            Vec3 screen_position{};
            if (!CameraProjection::project_point(
                    *active_camera, *camera_transform,
                    {x, y, transform->position.z}, screen_position)) {
                continue;
            }
            x = screen_position.x;
            y = screen_position.y;
            width *= active_camera->zoom;
            height *= active_camera->zoom;
        }

        if (!frame.draw_quad(x, y, width, height, renderable->resource_id)) {
            break;
        }
        ++submitted;
    }
    return submitted;
}

} // namespace gaming_runtime
