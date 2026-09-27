#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gaming_runtime {

using EntityId = std::uint64_t;

constexpr EntityId kInvalidEntity = 0;

class EntityManager {
public:
    EntityManager() = default;

    EntityId create();
    bool destroy(EntityId entity);
    bool is_alive(EntityId entity) const noexcept;
    std::size_t alive_count() const noexcept;
    std::vector<EntityId> alive_entities() const;

private:
    static std::uint32_t index_from_id(EntityId entity) noexcept;
    static std::uint32_t generation_from_id(EntityId entity) noexcept;
    static EntityId make_id(std::uint32_t index, std::uint32_t generation) noexcept;

    std::vector<std::uint32_t> generations_;
    std::vector<std::uint8_t> alive_;
    std::vector<std::uint32_t> free_indices_;
    std::size_t alive_count_ = 0;
};

} // namespace gaming_runtime
