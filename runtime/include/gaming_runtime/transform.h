#pragma once

#include "gaming_runtime/entity.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace gaming_runtime {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Transform {
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

class TransformManager {
public:
    bool create(EntityId entity);
    bool destroy(EntityId entity);
    bool has(EntityId entity) const noexcept;

    Transform* get(EntityId entity) noexcept;
    const Transform* get(EntityId entity) const noexcept;

    std::size_t count() const noexcept;

private:
    std::unordered_map<EntityId, Transform> transforms_;
};

} // namespace gaming_runtime
