#pragma once

#include <cstdint>

namespace gaming_runtime {

class MemoryManager {
public:
    explicit MemoryManager(std::uint64_t budget_mb = 1024);

    bool reserve(std::uint64_t bytes) noexcept;
    void release(std::uint64_t bytes) noexcept;
    void clear() noexcept;

    std::uint64_t budget_bytes() const noexcept;
    std::uint64_t used_bytes() const noexcept;
    std::uint64_t available_bytes() const noexcept;

private:
    std::uint64_t budget_bytes_ = 0;
    std::uint64_t used_bytes_ = 0;
};

} // namespace gaming_runtime
