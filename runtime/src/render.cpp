#include "gaming_runtime/render.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace gaming_runtime {
namespace {

std::uint8_t to_byte(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
}

bool valid_dimensions(std::uint32_t width, std::uint32_t height) {
    constexpr std::uint64_t max_pixels = 16ULL * 1024ULL * 1024ULL;
    return width != 0 && height != 0 &&
           static_cast<std::uint64_t>(width) * height <= max_pixels;
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

    RenderCommand command;
    command.type = RenderCommand::Type::Clear;
    command.color = color;
    commands_.push_back(command);
    return true;
}

bool RenderFrame::draw_quad(float x, float y, float width, float height,
                            std::uint32_t resource_id, float rotation) {
    if (commands_.size() >= max_commands_ ||
        width < 0.0f || height < 0.0f) {
        return false;
    }

    RenderCommand command;
    command.type = RenderCommand::Type::DrawQuad;
    command.x = x;
    command.y = y;
    command.width = width;
    command.height = height;
    command.rotation = rotation;
    command.resource_id = resource_id;
    commands_.push_back(command);
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

bool NullRenderer::initialize(std::uint32_t width, std::uint32_t height) {
    if (!valid_dimensions(width, height)) {
        return false;
    }

    initialized_ = true;
    width_ = width;
    height_ = height;
    submitted_frames_ = 0;
    last_command_count_ = 0;
    return true;
}

bool NullRenderer::resize(std::uint32_t width, std::uint32_t height) {
    if (!initialized_ || !valid_dimensions(width, height)) {
        return false;
    }

    width_ = width;
    height_ = height;
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

bool NullRenderer::upload_texture(std::uint32_t, const Texture& texture) {
    return initialized_ && texture.valid();
}

bool NullRenderer::unregister_texture(std::uint32_t) {
    return initialized_;
}

std::uint64_t NullRenderer::memory_bytes() const noexcept {
    return 0;
}

FramebufferView NullRenderer::framebuffer() const noexcept {
    return {};
}

void NullRenderer::shutdown() {
    initialized_ = false;
    width_ = 0;
    height_ = 0;
}

std::uint64_t NullRenderer::submitted_frames() const noexcept {
    return submitted_frames_;
}

std::size_t NullRenderer::last_command_count() const noexcept {
    return last_command_count_;
}

bool SoftwareRenderer::initialize(std::uint32_t width, std::uint32_t height) {
    if (!valid_dimensions(width, height)) {
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

bool SoftwareRenderer::resize(std::uint32_t width, std::uint32_t height) {
    if (!initialized_ || !valid_dimensions(width, height)) {
        return false;
    }

    if (width == width_ && height == height_) {
        return true;
    }

    std::vector<std::uint8_t> new_pixels;
    try {
        new_pixels.assign(static_cast<std::size_t>(
            static_cast<std::uint64_t>(width) * height * 4ULL), 0);
    } catch (...) {
        return false;
    }

    width_ = width;
    height_ = height;
    pixels_.swap(new_pixels);
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

bool SoftwareRenderer::upload_texture(std::uint32_t resource_id, const Texture& texture) {
    return register_texture(resource_id, texture);
}

void SoftwareRenderer::shutdown() {
    initialized_ = false;
    width_ = 0;
    height_ = 0;
    pixels_.clear();
    textures_.clear();
}

bool SoftwareRenderer::register_texture(std::uint32_t resource_id, Texture texture) {
    constexpr std::uint64_t max_pixels = 16ULL * 1024ULL * 1024ULL;
    const std::uint64_t pixel_count =
        static_cast<std::uint64_t>(texture.width) * texture.height;
    if (!initialized_ || resource_id == 0 || pixel_count == 0 ||
        pixel_count > max_pixels || !texture.valid()) {
        return false;
    }

    for (auto& entry : textures_) {
        if (entry.first == resource_id) {
            entry.second = std::move(texture);
            return true;
        }
    }

    textures_.push_back({resource_id, std::move(texture)});
    return true;
}

bool SoftwareRenderer::unregister_texture(std::uint32_t resource_id) {
    for (auto it = textures_.begin(); it != textures_.end(); ++it) {
        if (it->first == resource_id) {
            textures_.erase(it);
            return true;
        }
    }
    return false;
}

std::uint64_t SoftwareRenderer::memory_bytes() const noexcept {
    return static_cast<std::uint64_t>(pixels_.size());
}

FramebufferView SoftwareRenderer::framebuffer() const noexcept {
    if (!initialized_ || pixels_.empty()) {
        return {};
    }

    return {
        pixels_.data(),
        width_,
        height_,
        width_ * 4U,
        pixels_.size()
    };
}

bool SoftwareRenderer::has_texture(std::uint32_t resource_id) const noexcept {
    for (const auto& entry : textures_) {
        if (entry.first == resource_id) {
            return true;
        }
    }
    return false;
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
    const float cosine = std::cos(command.rotation);
    const float sine = std::sin(command.rotation);
    const float half_width = command.width * 0.5f;
    const float half_height = command.height * 0.5f;

    const float extent_x = std::fabs(half_width * cosine) +
                           std::fabs(half_height * sine);
    const float extent_y = std::fabs(half_width * sine) +
                           std::fabs(half_height * cosine);
    const float center_x = command.x + half_width;
    const float center_y = command.y + half_height;

    const int left = std::max(0, static_cast<int>(std::floor(center_x - extent_x)));
    const int top = std::max(0, static_cast<int>(std::floor(center_y - extent_y)));
    const int right = std::min(
        static_cast<int>(width_),
        static_cast<int>(std::ceil(center_x + extent_x)));
    const int bottom = std::min(
        static_cast<int>(height_),
        static_cast<int>(std::ceil(center_y + extent_y)));

    if (right <= left || bottom <= top) {
        return;
    }

    const Texture* texture = nullptr;
    for (const auto& entry : textures_) {
        if (entry.first == command.resource_id) {
            texture = &entry.second;
            break;
        }
    }

    if (texture == nullptr) {
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
        return;
    }

    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - center_x;
            const float dy = static_cast<float>(y) + 0.5f - center_y;
            const float local_x = dx * cosine + dy * sine;
            const float local_y = -dx * sine + dy * cosine;
            if (std::fabs(local_x) > half_width ||
                std::fabs(local_y) > half_height) {
                continue;
            }

            const float u = command.width > 0.0f
                ? (local_x + half_width) / command.width
                : 0.0f;
            const float v = command.height > 0.0f
                ? (local_y + half_height) / command.height
                : 0.0f;

            const auto tx = static_cast<std::uint32_t>(
                std::clamp(u, 0.0f, 0.999999f) * texture->width);
            const auto ty = static_cast<std::uint32_t>(
                std::clamp(v, 0.0f, 0.999999f) * texture->height);
            const std::size_t source =
                (static_cast<std::size_t>(ty) * texture->width + tx) * 4;
            const std::size_t destination =
                (static_cast<std::size_t>(y) * width_ +
                 static_cast<std::size_t>(x)) * 4;

            pixels_[destination] = texture->pixels[source];
            pixels_[destination + 1] = texture->pixels[source + 1];
            pixels_[destination + 2] = texture->pixels[source + 2];
            pixels_[destination + 3] = texture->pixels[source + 3];
        }
    }
}

} // namespace gaming_runtime
