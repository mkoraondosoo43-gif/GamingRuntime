#include "gaming_runtime/renderable.h"

namespace gaming_runtime {

bool RenderableManager::create(EntityId entity, std::uint32_t resource_id) {
    if (entity == kInvalidEntity || renderables_.find(entity) != renderables_.end()) {
        return false;
    }

    renderables_.emplace(entity, Renderable{resource_id, true});
    return true;
}

bool RenderableManager::destroy(EntityId entity) {
    return renderables_.erase(entity) != 0;
}

bool RenderableManager::has(EntityId entity) const noexcept {
    return renderables_.find(entity) != renderables_.end();
}

Renderable* RenderableManager::get(EntityId entity) noexcept {
    const auto it = renderables_.find(entity);
    return it == renderables_.end() ? nullptr : &it->second;
}

const Renderable* RenderableManager::get(EntityId entity) const noexcept {
    const auto it = renderables_.find(entity);
    return it == renderables_.end() ? nullptr : &it->second;
}

std::size_t RenderableManager::count() const noexcept {
    return renderables_.size();
}

} // namespace gaming_runtime
