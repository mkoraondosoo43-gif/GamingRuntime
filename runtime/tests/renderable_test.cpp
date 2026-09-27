#include "gaming_runtime/renderable.h"

#include <cassert>

int main() {
    gaming_runtime::EntityManager entities;
    gaming_runtime::RenderableManager renderables;

    const auto entity = entities.create();

    assert(renderables.count() == 0);
    assert(renderables.create(entity, 7));
    assert(renderables.has(entity));
    assert(renderables.count() == 1);
    assert(!renderables.create(entity));

    auto* renderable = renderables.get(entity);
    assert(renderable != nullptr);
    assert(renderable->resource_id == 7);
    assert(renderable->visible);

    renderable->visible = false;
    renderable->resource_id = 12;

    const auto* read_only = renderables.get(entity);
    assert(read_only != nullptr);
    assert(read_only->resource_id == 12);
    assert(!read_only->visible);

    assert(renderables.destroy(entity));
    assert(!renderables.has(entity));
    assert(renderables.get(entity) == nullptr);
    assert(renderables.count() == 0);

    return 0;
}
