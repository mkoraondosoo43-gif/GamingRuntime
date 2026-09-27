#pragma once

#include "gaming_runtime/entity.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace gaming_runtime {

struct Renderable {
    std::uint32_t resource_id = 0;
    bool visible = true;
};

class RenderableManager {
public:
    bool create(EntityId entity, std::uint32_t resource_id = 0);
    bool destroy(EntityId entity);
    bool has(EntityId entity) const noexcept;

    Renderable* get(EntityId entity) noexcept;
    const Renderable* get(EntityId entity) const noexcept;

    std::size_t count() const noexcept;

private:
    std::unordered_map<EntityId, Renderable> renderables_;
};

} // namespace gaming_runtime
