#include "gaming_runtime/camera.h"

#include <cassert>

int main() {
    gaming_runtime::EntityManager entities;
    gaming_runtime::CameraManager cameras;

    const auto entity = entities.create();
    assert(entity != gaming_runtime::kInvalidEntity);
    assert(cameras.count() == 0);

    assert(cameras.create(entity));
    assert(cameras.has(entity));
    assert(cameras.count() == 1);
    assert(!cameras.create(entity));

    auto* camera = cameras.get(entity);
    assert(camera != nullptr);
    assert(camera->zoom == 1.0f);
    assert(camera->viewport_width == 0);
    assert(camera->viewport_height == 0);
    assert(camera->active);

    camera->zoom = 2.0f;
    camera->viewport_width = 1280;
    camera->viewport_height = 720;
    camera->active = false;

    const auto* read_only = cameras.get(entity);
    assert(read_only != nullptr);
    assert(read_only->zoom == 2.0f);
    assert(read_only->viewport_width == 1280);
    assert(read_only->viewport_height == 720);
    assert(!read_only->active);

    gaming_runtime::Transform camera_transform{};
    camera_transform.position = {10.0f, 20.0f, 30.0f};
    camera->active = true;

    gaming_runtime::Vec3 screen_position{};
    assert(gaming_runtime::CameraProjection::project_point(
        *camera, camera_transform, {15.0f, 25.0f, 35.0f}, screen_position));
    assert(screen_position.x == 650.0f);
    assert(screen_position.y == 410.0f);
    assert(screen_position.z == 5.0f);

    camera->zoom = 0.0f;
    assert(!gaming_runtime::CameraProjection::project_point(
        *camera, camera_transform, {15.0f, 25.0f, 35.0f}, screen_position));

    camera->zoom = 2.0f;
    camera->viewport_width = 0;
    assert(!gaming_runtime::CameraProjection::project_point(
        *camera, camera_transform, {15.0f, 25.0f, 35.0f}, screen_position));

    assert(cameras.destroy(entity));
    assert(!cameras.has(entity));
    assert(cameras.count() == 0);
    assert(cameras.get(entity) == nullptr);
    assert(!cameras.destroy(entity));

    assert(entities.destroy(entity));
    return 0;
}
