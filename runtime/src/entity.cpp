#include "gaming_runtime/entity.h"

#include <limits>

namespace gaming_runtime {

EntityId EntityManager::create() {
    std::uint32_t index = 0;

    if (!free_indices_.empty()) {
        index = free_indices_.back();
        free_indices_.pop_back();
        alive_[index] = 1;
    } else {
        if (generations_.size() >= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
            return kInvalidEntity;
        }

        index = static_cast<std::uint32_t>(generations_.size());
        generations_.push_back(1);
        alive_.push_back(1);
    }

    ++alive_count_;
    return make_id(index, generations_[index]);
}

bool EntityManager::destroy(EntityId entity) {
    if (!is_alive(entity)) {
        return false;
    }

    const std::uint32_t index = index_from_id(entity);
    alive_[index] = 0;
    --alive_count_;

    std::uint32_t& generation = generations_[index];
    ++generation;
    if (generation == 0) {
        generation = 1;
    }

    free_indices_.push_back(index);
    return true;
}

bool EntityManager::is_alive(EntityId entity) const noexcept {
    if (entity == kInvalidEntity) {
        return false;
    }

    const std::uint32_t index = index_from_id(entity);
    if (index >= generations_.size() || index >= alive_.size() || !alive_[index]) {
        return false;
    }

    return generations_[index] == generation_from_id(entity);
}

std::size_t EntityManager::alive_count() const noexcept {
    return alive_count_;
}

std::uint32_t EntityManager::index_from_id(EntityId entity) noexcept {
    return static_cast<std::uint32_t>(entity & 0xffffffffULL);
}

std::uint32_t EntityManager::generation_from_id(EntityId entity) noexcept {
    return static_cast<std::uint32_t>(entity >> 32U);
}

EntityId EntityManager::make_id(
    std::uint32_t index,
    std::uint32_t generation) noexcept {
    return (static_cast<EntityId>(generation) << 32U) |
           static_cast<EntityId>(index);
}

} // namespace gaming_runtime
