// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "platform/AudioDevice.h"
#include "miniaudio.h"
namespace reverie {
struct AudioDeviceManager::Impl {
    AudioEngine& engine;
    ma_context context{};
    ma_device output{}, input{};
    bool initialized = false, outputReady = false, inputReady = false;
    std::vector<ma_device_id> inputs;
    std::string error;
    AudioRecorder* recorder = nullptr;
    explicit Impl(AudioEngine& e) : engine(e) {}
    bool ensureContext() {
        if (initialized)
            return true;
        if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
            error = "No usable audio backend. Offline editing and export remain available.";
            return false;
        }
        initialized = true;
        return true;
    }
};
AudioDeviceManager::AudioDeviceManager(AudioEngine& e) : impl_(std::make_unique<Impl>(e)) {}
AudioDeviceManager::~AudioDeviceManager() {
    stopCapture();
    stopOutput();
    if (impl_->initialized)
        ma_context_uninit(&impl_->context);
}
bool AudioDeviceManager::startOutput(bool forceFailure) {
    stopOutput();
    auto& p = *impl_;
    if (forceFailure) {
        p.error = "Simulated unavailable output device";
        return false;
    }
    if (!p.ensureContext())
        return false;
    auto cfg = ma_device_config_init(ma_device_type_playback);
    cfg.playback.format = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate = ma_uint32(p.engine.sampleRate());
    cfg.periodSizeInFrames = 256;
    cfg.pUserData = &p;
    cfg.dataCallback = [](ma_device* d, void* output, const void*, ma_uint32 frames) {
        if (output)
            static_cast<Impl*>(d->pUserData)->engine.process(static_cast<float*>(output), frames);
    };
    if (ma_device_init(&p.context, &cfg, &p.output) != MA_SUCCESS) {
        p.error = "Output device initialization failed. You can edit, save and export offline.";
        return false;
    }
    p.outputReady = true;
    if (ma_device_start(&p.output) != MA_SUCCESS) {
        stopOutput();
        p.error = "Output device could not start.";
        return false;
    }
    p.error.clear();
    return true;
}
void AudioDeviceManager::stopOutput() {
    auto& p = *impl_;
    if (p.outputReady) {
        ma_device_uninit(&p.output);
        p.outputReady = false;
    }
}
std::vector<std::string> AudioDeviceManager::inputDevices() {
    auto& p = *impl_;
    std::vector<std::string> names;
    p.inputs.clear();
    if (!p.ensureContext())
        return names;
    ma_device_info* capture = nullptr;
    ma_uint32 count = 0;
    if (ma_context_get_devices(&p.context, nullptr, nullptr, &capture, &count) != MA_SUCCESS) {
        p.error = "Cannot enumerate input devices";
        return names;
    }
    for (ma_uint32 i = 0; i < count; ++i) {
        p.inputs.push_back(capture[i].id);
        names.emplace_back(capture[i].name);
    }
    return names;
}
bool AudioDeviceManager::startCapture(int index, AudioRecorder& recorder) {
    stopCapture();
    auto& p = *impl_;
    if (!p.ensureContext())
        return false;
    auto cfg = ma_device_config_init(ma_device_type_capture);
    cfg.capture.format = ma_format_f32;
    cfg.capture.channels = 2;
    cfg.sampleRate = ma_uint32(p.engine.sampleRate());
    cfg.periodSizeInFrames = 256;
    cfg.pUserData = &p;
    if (index >= 0 && index < int(p.inputs.size()))
        cfg.capture.pDeviceID = &p.inputs[index];
    p.recorder = &recorder;
    cfg.dataCallback = [](ma_device* d, void*, const void* input, ma_uint32 count) {
        auto* state = static_cast<Impl*>(d->pUserData);
        if (state->recorder)
            state->recorder->capture(static_cast<const float*>(input), count);
    };
    if (ma_device_init(&p.context, &cfg, &p.input) != MA_SUCCESS) {
        p.error = "Input device unavailable or microphone access denied";
        p.recorder = nullptr;
        return false;
    }
    p.inputReady = true;
    if (ma_device_start(&p.input) != MA_SUCCESS) {
        stopCapture();
        p.error = "Input device could not start";
        return false;
    }
    return true;
}
void AudioDeviceManager::stopCapture() {
    auto& p = *impl_;
    if (p.inputReady) {
        ma_device_uninit(&p.input);
        p.inputReady = false;
    }
    p.recorder = nullptr;
}
std::string AudioDeviceManager::error() const {
    return impl_->error;
}
bool AudioDeviceManager::outputRunning() const {
    return impl_->outputReady && ma_device_is_started(&impl_->output);
}
#ifndef __APPLE__
bool requestMicrophonePermission() {
    return true;
}
#endif
} // namespace reverie
