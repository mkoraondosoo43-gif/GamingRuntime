#include "gaming_runtime/transform.h"

#include <cassert>

int main() {
    gaming_runtime::EntityManager entities;
    gaming_runtime::TransformManager transforms;

    const auto entity = entities.create();

    assert(transforms.count() == 0);
    assert(!transforms.has(entity));
    assert(transforms.create(entity));
    assert(transforms.has(entity));
    assert(transforms.count() == 1);
    assert(!transforms.create(entity));

    auto* transform = transforms.get(entity);
    assert(transform != nullptr);
    assert(transform->position.x == 0.0f);
    assert(transform->position.y == 0.0f);
    assert(transform->position.z == 0.0f);
    assert(transform->scale.x == 1.0f);
    assert(transform->scale.y == 1.0f);
    assert(transform->scale.z == 1.0f);

    transform->position = {10.0f, 20.0f, 30.0f};
    transform->rotation = {0.0f, 45.0f, 90.0f};
    transform->scale = {2.0f, 3.0f, 4.0f};

    const auto* read_only = transforms.get(entity);
    assert(read_only != nullptr);
    assert(read_only->position.x == 10.0f);
    assert(read_only->position.y == 20.0f);
    assert(read_only->rotation.y == 45.0f);
    assert(read_only->scale.z == 4.0f);

    assert(transforms.destroy(entity));
    assert(!transforms.has(entity));
    assert(transforms.get(entity) == nullptr);
    assert(transforms.count() == 0);

    return 0;
}
