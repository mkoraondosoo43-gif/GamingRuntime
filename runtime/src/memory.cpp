#include "gaming_runtime/memory.h"

#include <limits>

namespace gaming_runtime {

MemoryManager::MemoryManager(std::uint64_t budget_mb)
    : budget_bytes_(budget_mb > (std::numeric_limits<std::uint64_t>::max() / (1024ULL * 1024ULL))
                        ? std::numeric_limits<std::uint64_t>::max()
                        : budget_mb * 1024ULL * 1024ULL) {}

bool MemoryManager::reserve(std::uint64_t bytes) noexcept {
    if (bytes > budget_bytes_ - used_bytes_) {
        return false;
    }

    used_bytes_ += bytes;
    return true;
}

void MemoryManager::release(std::uint64_t bytes) noexcept {
    if (bytes >= used_bytes_) {
        used_bytes_ = 0;
        return;
    }

    used_bytes_ -= bytes;
}

void MemoryManager::clear() noexcept {
    used_bytes_ = 0;
}

std::uint64_t MemoryManager::budget_bytes() const noexcept {
    return budget_bytes_;
}

std::uint64_t MemoryManager::used_bytes() const noexcept {
    return used_bytes_;
}

std::uint64_t MemoryManager::available_bytes() const noexcept {
    return budget_bytes_ - used_bytes_;
}

} // namespace gaming_runtime
