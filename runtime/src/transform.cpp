#include "gaming_runtime/transform.h"

namespace gaming_runtime {

bool TransformManager::create(EntityId entity) {
    if (entity == kInvalidEntity || transforms_.contains(entity)) {
        return false;
    }

    transforms_.emplace(entity, Transform{});
    return true;
}

bool TransformManager::destroy(EntityId entity) {
    return transforms_.erase(entity) != 0;
}

bool TransformManager::has(EntityId entity) const noexcept {
    return transforms_.find(entity) != transforms_.end();
}

Transform* TransformManager::get(EntityId entity) noexcept {
    const auto it = transforms_.find(entity);
    return it == transforms_.end() ? nullptr : &it->second;
}

const Transform* TransformManager::get(EntityId entity) const noexcept {
    const auto it = transforms_.find(entity);
    return it == transforms_.end() ? nullptr : &it->second;
}

std::size_t TransformManager::count() const noexcept {
    return transforms_.size();
}

} // namespace gaming_runtime
