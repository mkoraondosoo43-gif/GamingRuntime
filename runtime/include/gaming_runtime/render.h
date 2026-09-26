#pragma once

#include <cstdint>
#include <vector>

namespace gaming_runtime {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct Texture {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels;

    bool valid() const noexcept {
        return width != 0 && height != 0 &&
               pixels.size() == static_cast<std::size_t>(width) * height * 4U;
    }
};

struct RenderCommand {
    enum class Type : std::uint8_t {
        Clear,
        DrawQuad
    };

    Type type = Type::Clear;
    Color color{};
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    std::uint32_t resource_id = 0;
};

class RenderFrame {
public:
    explicit RenderFrame(std::size_t max_commands = 4096);

    void reset();
    bool clear(Color color);
    bool draw_quad(float x, float y, float width, float height,
                   std::uint32_t resource_id = 0);
    const std::vector<RenderCommand>& commands() const noexcept;
    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;

private:
    std::vector<RenderCommand> commands_;
    std::size_t max_commands_;
};

class Renderer {
public:
    virtual ~Renderer() = default;

    virtual bool initialize(std::uint32_t width, std::uint32_t height) = 0;
    virtual bool submit(const RenderFrame& frame) = 0;
    virtual void shutdown() = 0;
};

class NullRenderer final : public Renderer {
public:
    bool initialize(std::uint32_t width, std::uint32_t height) override;
    bool submit(const RenderFrame& frame) override;
    void shutdown() override;

    std::uint64_t submitted_frames() const noexcept;
    std::size_t last_command_count() const noexcept;

private:
    bool initialized_ = false;
    std::uint64_t submitted_frames_ = 0;
    std::size_t last_command_count_ = 0;
};

class SoftwareRenderer final : public Renderer {
public:
    bool initialize(std::uint32_t width, std::uint32_t height) override;
    bool submit(const RenderFrame& frame) override;
    void shutdown() override;

    bool register_texture(std::uint32_t resource_id, Texture texture);
    bool unregister_texture(std::uint32_t resource_id);
    bool has_texture(std::uint32_t resource_id) const noexcept;

    std::uint32_t width() const noexcept;
    std::uint32_t height() const noexcept;
    const std::vector<std::uint8_t>& pixels() const noexcept;
    std::uint64_t submitted_frames() const noexcept;

private:
    void fill(Color color);
    void draw_quad(const RenderCommand& command);

    bool initialized_ = false;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::vector<std::uint8_t> pixels_;
    std::vector<std::pair<std::uint32_t, Texture>> textures_;
    std::uint64_t submitted_frames_ = 0;
};

} // namespace gaming_runtime
