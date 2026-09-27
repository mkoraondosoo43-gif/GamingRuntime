#include "gaming_runtime/camera.h"

namespace gaming_runtime {

bool CameraManager::create(EntityId entity) {
    if (entity == kInvalidEntity) {
        return false;
    }
    return cameras_.emplace(entity, Camera{}).second;
}

bool CameraManager::destroy(EntityId entity) {
    return cameras_.erase(entity) != 0;
}

bool CameraManager::has(EntityId entity) const noexcept {
    return cameras_.find(entity) != cameras_.end();
}

Camera* CameraManager::get(EntityId entity) noexcept {
    const auto it = cameras_.find(entity);
    return it == cameras_.end() ? nullptr : &it->second;
}

const Camera* CameraManager::get(EntityId entity) const noexcept {
    const auto it = cameras_.find(entity);
    return it == cameras_.end() ? nullptr : &it->second;
}

std::size_t CameraManager::count() const noexcept {
    return cameras_.size();
}

} // namespace gaming_runtime
