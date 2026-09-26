#include "gaming_runtime/render.h"

#include <utility>

namespace gaming_runtime {

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

} // namespace gaming_runtime
