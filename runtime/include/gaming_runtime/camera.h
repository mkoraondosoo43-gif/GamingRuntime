#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "gaming_runtime/entity.h"

namespace gaming_runtime {

struct Camera {
    float zoom = 1.0f;
    std::uint32_t viewport_width = 0;
    std::uint32_t viewport_height = 0;
    bool active = true;
};

class CameraManager {
public:
    bool create(EntityId entity);
    bool destroy(EntityId entity);
    bool has(EntityId entity) const noexcept;

    Camera* get(EntityId entity) noexcept;
    const Camera* get(EntityId entity) const noexcept;

    std::size_t count() const noexcept;

private:
    std::unordered_map<EntityId, Camera> cameras_;
};

} // namespace gaming_runtime
