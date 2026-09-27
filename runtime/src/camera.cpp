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

bool CameraProjection::project_point(const Camera& camera,
                                     const Transform& camera_transform,
                                     const Vec3& world_position,
                                     Vec3& screen_position) noexcept {
    if (camera.zoom <= 0.0f ||
        camera.viewport_width == 0 ||
        camera.viewport_height == 0) {
        return false;
    }

    screen_position.x =
        (world_position.x - camera_transform.position.x) * camera.zoom +
        static_cast<float>(camera.viewport_width) * 0.5f;
    screen_position.y =
        (world_position.y - camera_transform.position.y) * camera.zoom +
        static_cast<float>(camera.viewport_height) * 0.5f;
    screen_position.z = world_position.z - camera_transform.position.z;
    return true;
}

} // namespace gaming_runtime
