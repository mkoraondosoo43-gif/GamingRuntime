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

#if defined(__ANDROID__)
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <array>
#include <cmath>
#include <new>

namespace {
constexpr std::size_t kToneSamples = 4800;

SLuint32 to_sl_rate(std::uint32_t rate) {
    switch (rate) {
    case 8000: return SL_SAMPLINGRATE_8;
    case 11025: return SL_SAMPLINGRATE_11_025;
    case 16000: return SL_SAMPLINGRATE_16;
    case 22050: return SL_SAMPLINGRATE_22_05;
    case 24000: return SL_SAMPLINGRATE_24;
    case 32000: return SL_SAMPLINGRATE_32;
    case 44100: return SL_SAMPLINGRATE_44_1;
    case 48000: return SL_SAMPLINGRATE_48;
    default: return 0;
    }
}
}

struct AndroidAudio::Impl {
    SLObjectItf engine_object = nullptr;
    SLEngineItf engine = nullptr;
    SLObjectItf output_mix_object = nullptr;
    SLObjectItf player_object = nullptr;
    SLPlayItf play = nullptr;
    SLAndroidSimpleBufferQueueItf queue = nullptr;
    std::array<std::int16_t, kToneSamples> samples{};
    std::uint32_t sample_rate = 0;
    std::uint32_t channels = 0;
    float master_volume = 1.0f;
    bool initialized = false;
};

AndroidAudio::~AndroidAudio() {
    shutdown();
}

bool AndroidAudio::initialize(std::uint32_t sample_rate,
                              std::uint32_t channels) {
    shutdown();
    if ((channels != 1 && channels != 2) || to_sl_rate(sample_rate) == 0) {
        return false;
    }

    auto* impl = new (std::nothrow) Impl();
    if (!impl) return false;
    impl->sample_rate = sample_rate;
    impl->channels = channels;

    SLresult result = slCreateEngine(&impl->engine_object, 0, nullptr, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) { delete impl; return false; }
    result = (*impl->engine_object)->Realize(impl->engine_object, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) { shutdown(); delete impl; return false; }
    result = (*impl->engine_object)->GetInterface(
        impl->engine_object, SL_IID_ENGINE, &impl->engine);
    if (result != SL_RESULT_SUCCESS) { (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }

    result = (*impl->engine)->CreateOutputMix(
        impl->engine, &impl->output_mix_object, 0, nullptr, nullptr);
    if (result != SL_RESULT_SUCCESS) { (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }
    result = (*impl->output_mix_object)->Realize(impl->output_mix_object, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) { (*impl->output_mix_object)->Destroy(impl->output_mix_object); (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }

    SLDataLocator_AndroidSimpleBufferQueue locator_buffer = {
        SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2
    };
    SLDataFormat_PCM format = {
        SL_DATAFORMAT_PCM,
        static_cast<SLuint32>(channels),
        to_sl_rate(sample_rate),
        SL_PCMSAMPLEFORMAT_FIXED_16,
        SL_PCMSAMPLEFORMAT_FIXED_16,
        channels == 2 ? static_cast<SLuint32>(SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT)
                      : static_cast<SLuint32>(SL_SPEAKER_FRONT_CENTER),
        SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSource source = {&locator_buffer, &format};
    SLDataLocator_OutputMix locator_output = {
        SL_DATALOCATOR_OUTPUTMIX, impl->output_mix_object
    };
    SLDataSink sink = {&locator_output, nullptr};

    const SLInterfaceID ids[] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean required[] = {SL_BOOLEAN_TRUE};
    result = (*impl->engine)->CreateAudioPlayer(
        impl->engine, &impl->player_object, &source, &sink, 1, ids, required);
    if (result != SL_RESULT_SUCCESS) {
        (*impl->output_mix_object)->Destroy(impl->output_mix_object);
        (*impl->engine_object)->Destroy(impl->engine_object);
        delete impl;
        return false;
    }
    result = (*impl->player_object)->Realize(impl->player_object, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) { (*impl->player_object)->Destroy(impl->player_object); (*impl->output_mix_object)->Destroy(impl->output_mix_object); (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }
    result = (*impl->player_object)->GetInterface(impl->player_object, SL_IID_PLAY, &impl->play);
    if (result != SL_RESULT_SUCCESS) { (*impl->player_object)->Destroy(impl->player_object); (*impl->output_mix_object)->Destroy(impl->output_mix_object); (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }
    result = (*impl->player_object)->GetInterface(impl->player_object, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &impl->queue);
    if (result != SL_RESULT_SUCCESS) { (*impl->player_object)->Destroy(impl->player_object); (*impl->output_mix_object)->Destroy(impl->output_mix_object); (*impl->engine_object)->Destroy(impl->engine_object); delete impl; return false; }

    impl->initialized = true;
    impl_ = impl;
    (*impl->play)->SetPlayState(impl->play, SL_PLAYSTATE_PLAYING);
    return true;
}

bool AndroidAudio::submit(const AudioFrame& frame) {
    if (!impl_ || !impl_->initialized) return false;

    for (const auto& command : frame.commands()) {
        if (command.type == AudioCommand::Type::SetMasterVolume) {
            impl_->master_volume = command.volume;
            continue;
        }
        if (command.type == AudioCommand::Type::StopAll) {
            (*impl_->queue)->Clear(impl_->queue);
            continue;
        }
        if (command.type != AudioCommand::Type::Play) continue;

        const float frequency = 180.0f + static_cast<float>(command.resource_id % 12U) * 35.0f;
        for (std::size_t i = 0; i < impl_->samples.size(); ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(impl_->sample_rate);
            const float envelope = std::exp(-t * 18.0f);
            const float sample = std::sin(6.28318530718f * frequency * t) *
                                 command.volume * impl_->master_volume * envelope;
            impl_->samples[i] = static_cast<std::int16_t>(
                std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
        }

        const SLresult queued = (*impl_->queue)->Enqueue(
            impl_->queue,
            impl_->samples.data(),
            static_cast<SLuint32>(impl_->samples.size() * sizeof(std::int16_t)));
        if (queued != SL_RESULT_SUCCESS) {
            return false;
        }
        if (!command.loop) break;
    }
    return true;
}

void AndroidAudio::shutdown() {
    if (!impl_) return;
    if (impl_->player_object) (*impl_->player_object)->Destroy(impl_->player_object);
    if (impl_->output_mix_object) (*impl_->output_mix_object)->Destroy(impl_->output_mix_object);
    if (impl_->engine_object) (*impl_->engine_object)->Destroy(impl_->engine_object);
    delete impl_;
    impl_ = nullptr;
}

std::uint64_t AndroidAudio::memory_bytes() const noexcept {
    return impl_ ? sizeof(Impl) : 0;
}
#endif

} // namespace gaming_runtime
