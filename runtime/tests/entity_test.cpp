#include "gaming_runtime/entity.h"

#include <cassert>

int main() {
    gaming_runtime::EntityManager entities;

    assert(entities.alive_count() == 0);
    assert(!entities.is_alive(gaming_runtime::kInvalidEntity));

    const auto first = entities.create();
    assert(first != gaming_runtime::kInvalidEntity);
    assert(entities.is_alive(first));
    assert(entities.alive_count() == 1);

    const auto second = entities.create();
    assert(second != gaming_runtime::kInvalidEntity);
    assert(second != first);
    assert(entities.is_alive(second));
    assert(entities.alive_count() == 2);

    assert(entities.destroy(first));
    assert(!entities.is_alive(first));
    assert(entities.alive_count() == 1);
    assert(!entities.destroy(first));

    const auto replacement = entities.create();
    assert(replacement != gaming_runtime::kInvalidEntity);
    assert(replacement != first);
    assert(entities.is_alive(replacement));
    assert(!entities.is_alive(first));
    assert(entities.alive_count() == 2);

    assert(entities.destroy(second));
    assert(entities.destroy(replacement));
    assert(entities.alive_count() == 0);

    return 0;
}
