#pragma once

#include <cstdint>
#include <vector>

namespace gaming_runtime {

struct AudioCommand {
    enum class Type : std::uint8_t {
        Play,
        StopAll,
        SetMasterVolume
    };

    Type type = Type::StopAll;
    std::uint32_t resource_id = 0;
    float volume = 1.0f;
    bool loop = false;
};

class AudioFrame {
public:
    explicit AudioFrame(std::size_t max_commands = 256);

    bool play(std::uint32_t resource_id,
              float volume = 1.0f,
              bool loop = false);
    bool stop_all();
    bool set_master_volume(float volume);

    void reset() noexcept;
    std::size_t size() const noexcept;
    std::size_t max_commands() const noexcept;
    const std::vector<AudioCommand>& commands() const noexcept;

private:
    std::size_t max_commands_;
    std::vector<AudioCommand> commands_;
};

class AudioBackend {
public:
    virtual ~AudioBackend() = default;

    virtual bool initialize(std::uint32_t sample_rate,
                            std::uint32_t channels) = 0;
    virtual bool submit(const AudioFrame& frame) = 0;
    virtual void shutdown() = 0;

    virtual std::uint64_t memory_bytes() const noexcept = 0;
};

class NullAudio final : public AudioBackend {
public:
    bool initialize(std::uint32_t sample_rate,
                    std::uint32_t channels) override;
    bool submit(const AudioFrame& frame) override;
    void shutdown() override;

    std::uint32_t sample_rate() const noexcept;
    std::uint32_t channels() const noexcept;
    std::uint64_t submitted_frames() const noexcept;
    std::size_t last_command_count() const noexcept;
    std::uint64_t memory_bytes() const noexcept override;

private:
    bool initialized_ = false;
    std::uint32_t sample_rate_ = 0;
    std::uint32_t channels_ = 0;
    std::uint64_t submitted_frames_ = 0;
    std::size_t last_command_count_ = 0;
};

} // namespace gaming_runtime
