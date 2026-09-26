#include "gaming_runtime/render.h"

#include <algorithm>
#include <cmath>

namespace gaming_runtime {
namespace {

std::uint8_t to_byte(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

} // namespace

RenderFrame::RenderFrame(std::size_t max_commands)
    : max_commands_(max_commands) {
    commands_.reserve(max_commands_);
}

void RenderFrame::reset() {
    commands_.clear();
}

bool RenderFrame::clear(Color color) {
    if (commands_.size() >= max_commands_) {
        return false;
    }

    commands_.push_back({
        .type = RenderCommand::Type::Clear,
        .color = color
    });
    return true;
}

bool RenderFrame::draw_quad(float x, float y, float width, float height,
                            std::uint32_t resource_id) {
    if (commands_.size() >= max_commands_ ||
        width < 0.0f || height < 0.0f) {
        return false;
    }

    commands_.push_back({
        .type = RenderCommand::Type::DrawQuad,
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .resource_id = resource_id
    });
    return true;
}

const std::vector<RenderCommand>& RenderFrame::commands() const noexcept {
    return commands_;
}

std::size_t RenderFrame::size() const noexcept {
    return commands_.size();
}

std::size_t RenderFrame::capacity() const noexcept {
    return max_commands_;
}

bool NullRenderer::initialize(std::uint32_t, std::uint32_t) {
    initialized_ = true;
    submitted_frames_ = 0;
    last_command_count_ = 0;
    return true;
}

bool NullRenderer::submit(const RenderFrame& frame) {
    if (!initialized_) {
        return false;
    }

    ++submitted_frames_;
    last_command_count_ = frame.size();
    return true;
}

void NullRenderer::shutdown() {
    initialized_ = false;
}

std::uint64_t NullRenderer::submitted_frames() const noexcept {
    return submitted_frames_;
}

std::size_t NullRenderer::last_command_count() const noexcept {
    return last_command_count_;
}

bool SoftwareRenderer::initialize(std::uint32_t width, std::uint32_t height) {
    constexpr std::uint64_t max_pixels = 16ULL * 1024ULL * 1024ULL;

    if (width == 0 || height == 0 ||
        static_cast<std::uint64_t>(width) * height > max_pixels) {
        return false;
    }

    width_ = width;
    height_ = height;
    pixels_.assign(static_cast<std::size_t>(
        static_cast<std::uint64_t>(width) * height * 4ULL), 0);
    submitted_frames_ = 0;
    initialized_ = true;
    return true;
}

bool SoftwareRenderer::submit(const RenderFrame& frame) {
    if (!initialized_) {
        return false;
    }

    for (const auto& command : frame.commands()) {
        switch (command.type) {
        case RenderCommand::Type::Clear:
            fill(command.color);
            break;
        case RenderCommand::Type::DrawQuad:
            draw_quad(command);
            break;
        }
    }

    ++submitted_frames_;
    return true;
}

void SoftwareRenderer::shutdown() {
    initialized_ = false;
    width_ = 0;
    height_ = 0;
    pixels_.clear();
}

std::uint32_t SoftwareRenderer::width() const noexcept {
    return width_;
}

std::uint32_t SoftwareRenderer::height() const noexcept {
    return height_;
}

const std::vector<std::uint8_t>& SoftwareRenderer::pixels() const noexcept {
    return pixels_;
}

std::uint64_t SoftwareRenderer::submitted_frames() const noexcept {
    return submitted_frames_;
}

void SoftwareRenderer::fill(Color color) {
    const std::uint8_t rgba[] = {
        to_byte(color.r), to_byte(color.g), to_byte(color.b), to_byte(color.a)
    };

    for (std::size_t i = 0; i < pixels_.size(); i += 4) {
        pixels_[i] = rgba[0];
        pixels_[i + 1] = rgba[1];
        pixels_[i + 2] = rgba[2];
        pixels_[i + 3] = rgba[3];
    }
}

void SoftwareRenderer::draw_quad(const RenderCommand& command) {
    const int left = std::max(0, static_cast<int>(std::floor(command.x)));
    const int top = std::max(0, static_cast<int>(std::floor(command.y)));
    const int right = std::min(
        static_cast<int>(width_),
        static_cast<int>(std::ceil(command.x + command.width)));
    const int bottom = std::min(
        static_cast<int>(height_),
        static_cast<int>(std::ceil(command.y + command.height)));

    const std::uint8_t r = to_byte(command.resource_id == 0 ? 1.0f : 0.2f);
    const std::uint8_t g = to_byte(command.resource_id == 0 ? 1.0f : 0.7f);
    const std::uint8_t b = to_byte(command.resource_id == 0 ? 1.0f : 1.0f);

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const std::size_t index =
                (static_cast<std::size_t>(y) * width_ +
                 static_cast<std::size_t>(x)) * 4;

            pixels_[index] = r;
            pixels_[index + 1] = g;
            pixels_[index + 2] = b;
            pixels_[index + 3] = 255;
        }
    }
}

} // namespace gaming_runtime
