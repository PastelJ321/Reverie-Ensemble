// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include <atomic>
#include <thread>
namespace reverie {
class AudioRecorder {
  public:
    AudioRecorder();
    ~AudioRecorder();
    void start(const std::filesystem::path&, std::uint32_t rate);
    void capture(const float* stereo, std::uint32_t frames) noexcept;
    void stop(); // Stop capture device first, then drains/joins disk worker.
    bool overflowed() const {
        return overflow_.load();
    }
    bool failed() const {
        return failure_.load();
    }
    std::uint64_t framesWritten() const {
        return frames_.load();
    }

  private:
    static constexpr std::size_t capacity = 262144;
    std::unique_ptr<float[]> ring_;
    alignas(64) std::atomic<std::uint64_t> write_{0};
    alignas(64) std::atomic<std::uint64_t> read_{0};
    std::atomic<std::uint64_t> frames_{0};
    std::atomic<bool> running_{false}, overflow_{false}, failure_{false};
    std::thread worker_;
};
} // namespace reverie
