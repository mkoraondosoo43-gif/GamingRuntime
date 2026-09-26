#pragma once

#include <cstdint>

namespace gaming_runtime {

class DisplayBackend {
public:
    virtual ~DisplayBackend() = default;

    virtual bool initialize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool resize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool present() = 0;
    virtual void shutdown() = 0;

    virtual std::uint32_t width() const noexcept = 0;
    virtual std::uint32_t height() const noexcept = 0;
    virtual std::uint64_t memory_bytes() const noexcept = 0;
};

class NullDisplay final : public DisplayBackend {
public:
    bool initialize(std::uint32_t width, std::uint32_t height) override;
    bool resize(std::uint32_t width, std::uint32_t height) override;
    bool present() override;
    void shutdown() override;

    std::uint32_t width() const noexcept override;
    std::uint32_t height() const noexcept override;
    std::uint64_t memory_bytes() const noexcept override;
    std::uint64_t presented_frames() const noexcept;

private:
    bool initialized_ = false;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint64_t presented_frames_ = 0;
};

} // namespace gaming_runtime
