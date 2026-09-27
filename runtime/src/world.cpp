#include "gaming_runtime/world.h"

namespace gaming_runtime {

std::size_t GameWorld::render(RenderFrame& frame) const {
    std::size_t submitted = 0;
    for (const EntityId entity : entities_.alive_entities()) {
        const Transform* transform = transforms_.get(entity);
        const Renderable* renderable = renderables_.get(entity);
        if (transform == nullptr || renderable == nullptr || !renderable->visible) {
            continue;
        }

        const float width = transform->scale.x > 0.0f ? transform->scale.x : 0.0f;
        const float height = transform->scale.y > 0.0f ? transform->scale.y : 0.0f;
        if (width <= 0.0f || height <= 0.0f) {
            continue;
        }

        if (!frame.draw_quad(transform->position.x, transform->position.y,
                             width, height, renderable->resource_id)) {
            break;
        }
        ++submitted;
    }
    return submitted;
}

} // namespace gaming_runtime
