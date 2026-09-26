#include "gaming_runtime/audio.h"

#include <algorithm>

namespace gaming_runtime {

AudioFrame::AudioFrame(std::size_t max_commands)
    : max_commands_(max_commands) {
    commands_.reserve(max_commands_);
}

bool AudioFrame::play(std::uint32_t resource_id,
                      float volume,
                      bool loop) {
    if (resource_id == 0 || commands_.size() >= max_commands_) {
        return false;
    }

    AudioCommand command;
    command.type = AudioCommand::Type::Play;
    command.resource_id = resource_id;
    command.volume = std::clamp(volume, 0.0f, 1.0f);
    command.loop = loop;
    commands_.push_back(command);
    return true;
}

bool AudioFrame::stop_all() {
    if (commands_.size() >= max_commands_) {
        return false;
    }

    AudioCommand command;
    command.type = AudioCommand::Type::StopAll;
    commands_.push_back(command);
    return true;
}

bool AudioFrame::set_master_volume(float volume) {
    if (commands_.size() >= max_commands_) {
        return false;
    }

    AudioCommand command;
    command.type = AudioCommand::Type::SetMasterVolume;
    command.volume = std::clamp(volume, 0.0f, 1.0f);
    commands_.push_back(command);
    return true;
}

void AudioFrame::reset() noexcept {
    commands_.clear();
}

std::size_t AudioFrame::size() const noexcept {
    return commands_.size();
}

std::size_t AudioFrame::max_commands() const noexcept {
    return max_commands_;
}

const std::vector<AudioCommand>& AudioFrame::commands() const noexcept {
    return commands_;
}

bool NullAudio::initialize(std::uint32_t sample_rate,
                           std::uint32_t channels) {
    if (sample_rate == 0 || channels == 0) {
        return false;
    }

    sample_rate_ = sample_rate;
    channels_ = channels;
    submitted_frames_ = 0;
    last_command_count_ = 0;
    initialized_ = true;
    return true;
}

bool NullAudio::submit(const AudioFrame& frame) {
    if (!initialized_) {
        return false;
    }

    ++submitted_frames_;
    last_command_count_ = frame.size();
    return true;
}

void NullAudio::shutdown() {
    initialized_ = false;
    sample_rate_ = 0;
    channels_ = 0;
    submitted_frames_ = 0;
    last_command_count_ = 0;
}

std::uint32_t NullAudio::sample_rate() const noexcept {
    return sample_rate_;
}

std::uint32_t NullAudio::channels() const noexcept {
    return channels_;
}

std::uint64_t NullAudio::submitted_frames() const noexcept {
    return submitted_frames_;
}

std::size_t NullAudio::last_command_count() const noexcept {
    return last_command_count_;
}

std::uint64_t NullAudio::memory_bytes() const noexcept {
    return 0;
}

} // namespace gaming_runtime
