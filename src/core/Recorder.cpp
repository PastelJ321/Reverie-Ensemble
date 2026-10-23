// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Recorder.h"
#include "miniaudio.h"
#include <array>
#include <chrono>
#include <future>
#include <stdexcept>
namespace reverie {
AudioRecorder::AudioRecorder() : ring_(std::make_unique<float[]>(capacity * 2)) {}
AudioRecorder::~AudioRecorder() {
    stop();
}
void AudioRecorder::start(const std::filesystem::path& path, std::uint32_t rate) {
    stop();
    write_ = 0;
    read_ = 0;
    frames_ = 0;
    overflow_ = false;
    failure_ = false;
    running_ = true;
    std::promise<bool> ready;
    auto future = ready.get_future();
    worker_ = std::thread([this, path, rate, p = std::move(ready)]() mutable {
        auto cfg = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, 2, rate);
        ma_encoder encoder{};
        if (ma_encoder_init_file(path.string().c_str(), &cfg, &encoder) != MA_SUCCESS) {
            failure_ = true;
            running_ = false;
            p.set_value(false);
            return;
        }
        p.set_value(true);
        std::array<std::int16_t, 2048> pcm{};
        while (running_.load() || read_.load() != write_.load()) {
            const auto r = read_.load(std::memory_order_relaxed),
                       w = write_.load(std::memory_order_acquire);
            const auto n = std::min(std::uint64_t(1024), w - r);
            if (!n) {
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                continue;
            }
            for (std::uint64_t i = 0; i < n; ++i)
                for (int c = 0; c < 2; ++c) {
                    float v = ring_[((r + i) % capacity) * 2 + c];
                    pcm[i * 2 + c] = std::int16_t(
                        std::lrint(std::clamp(std::isfinite(v) ? v : 0, -1.f, 1.f) * 32767));
                }
            ma_uint64 written = 0;
            if (ma_encoder_write_pcm_frames(&encoder, pcm.data(), n, &written) != MA_SUCCESS ||
                written != n) {
                failure_ = true;
                running_ = false;
                read_.store(w);
                break;
            }
            frames_.fetch_add(written);
            read_.store(r + n, std::memory_order_release);
        }
        ma_encoder_uninit(&encoder);
    });
    if (!future.get()) {
        stop();
        throw std::runtime_error("Cannot open recording destination");
    }
}
void AudioRecorder::capture(const float* input, std::uint32_t frames) noexcept {
    if (!running_.load(std::memory_order_relaxed) || !input)
        return;
    const auto w = write_.load(std::memory_order_relaxed),
               r = read_.load(std::memory_order_acquire);
    if (w - r + frames > capacity) {
        overflow_ = true;
        running_ = false;
        return;
    }
    for (std::uint32_t i = 0; i < frames; ++i) {
        ring_[((w + i) % capacity) * 2] = input[i * 2];
        ring_[((w + i) % capacity) * 2 + 1] = input[i * 2 + 1];
    }
    write_.store(w + frames, std::memory_order_release);
}
void AudioRecorder::stop() {
    running_ = false;
    if (worker_.joinable())
        worker_.join();
}
} // namespace reverie
