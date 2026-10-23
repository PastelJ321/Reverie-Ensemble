// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Dsp.h"
#include "core/Effects.h"
#include <array>
#include <atomic>
#include <memory>
namespace reverie {
template <typename T, std::size_t N> class SpscQueue {
  public:
    bool push(const T& item) noexcept {
        auto w = write_.load(std::memory_order_relaxed);
        auto n = (w + 1) % N;
        if (n == read_.load(std::memory_order_acquire))
            return false;
        data_[w] = item;
        write_.store(n, std::memory_order_release);
        return true;
    }
    bool pop(T& item) noexcept {
        auto r = read_.load(std::memory_order_relaxed);
        if (r == write_.load(std::memory_order_acquire))
            return false;
        item = data_[r];
        read_.store((r + 1) % N, std::memory_order_release);
        return true;
    }
    bool full() const noexcept {
        return (write_.load(std::memory_order_relaxed) + 1) % N ==
               read_.load(std::memory_order_acquire);
    }

  private:
    std::array<T, N> data_{};
    alignas(64) std::atomic<std::size_t> write_{0};
    alignas(64) std::atomic<std::size_t> read_{0};
};
struct Event {
    std::int64_t frame;
    Id token;
    int key;
    float velocity;
    bool on;
    std::int64_t endFrame = 0;
};
struct AudioRegion {
    std::int64_t start, end;
    double offset, step;
    std::shared_ptr<const Sample> sample;
};
struct PreparedTrack {
    Id id;
    float gainL, gainR, targetL, targetR;
    int rampRemaining = 0;
    std::vector<Event> events;
    std::vector<AudioRegion> audio;
    std::vector<Event> spans;
    std::vector<std::int64_t> intervalMax;
    std::uint64_t scheduleKey = 0, soundKey = 0;
    std::shared_ptr<const Sample> sampleOwner;
    Synthesizer synth;
    EffectChain effects;
    float peakL = 0, peakR = 0;
    std::size_t cursor = 0;
    float peak = 0;
    PreparedTrack(const Track&, double, bool);
    void buildIndex();
    void seek(std::int64_t, bool clearEffects = true) noexcept;
    void restore(std::size_t, std::size_t, std::size_t, std::int64_t, int&) noexcept;
};
struct AudioProcessingGraph {
    double rate;
    TempoMap tempo;
    float master;
    std::int64_t end;
    bool loop = false, metronome = false;
    std::int64_t loopStart = 0, loopEnd = 0;
    int numerator = 4, denominator = 4;
    float clickLevel = .12f;
    std::vector<PreparedTrack> tracks;
    AudioProcessingGraph(const Project&, double);
    void seek(std::int64_t) noexcept;
    void render(float*, std::uint32_t, std::int64_t) noexcept;
    void inherit(AudioProcessingGraph*, std::int64_t) noexcept;
};
class AudioEngine {
  public:
    explicit AudioEngine(double rate = defaultRate, bool transportTools = true);
    void audition(const Track&, int key, float velocity = .7f, double duration = .45);
    void releaseAudition() {
        releasePreview_.store(true);
    }
    ~AudioEngine(); // Stop the device before destroying the engine.
    AudioEngine(const AudioEngine&) = delete;
    void publish(const Project&); // Control thread only; prepare/allocation here.
    void collect(); // Control thread only; graph reclamation never occurs in callback.
    void process(float*, std::uint32_t) noexcept; // Single audio consumer only.
    void setPlaying(bool play) {
        playing_.store(play);
    }
    void setFreeRunning(bool value) {
        freeRunning_.store(value);
    }
    bool playing() const {
        return playing_.load();
    }
    void seek(std::int64_t frame) {
        seek_.store(std::max(std::int64_t(0), frame));
    }
    std::int64_t position() const {
        return position_.load();
    }
    double sampleRate() const {
        return rate_;
    }
    float masterPeak() const {
        return peak_.load();
    }
    bool clipped() const {
        return clipped_.load();
    }
    void clearClip() {
        clipped_.store(false);
    }
    float trackPeakL(int i) const {
        return i >= 0 && i < maxTracks ? leftPeaks_[i].load() : 0;
    }
    float trackPeakR(int i) const {
        return i >= 0 && i < maxTracks ? rightPeaks_[i].load() : 0;
    }
    float trackPeak(int index) const {
        return index >= 0 && index < maxTracks ? trackPeaks_[index].load() : 0;
    }

  private:
    double rate_;
    bool transportTools_;
    std::atomic<AudioProcessingGraph*> previewPending_{nullptr};
    AudioProcessingGraph* preview_ = nullptr;
    std::int64_t previewPosition_ = 0, previewLimit_ = 0;
    std::atomic<bool> releasePreview_{false};
    std::atomic<AudioProcessingGraph*> pending_{nullptr};
    AudioProcessingGraph* active_ = nullptr;
    SpscQueue<AudioProcessingGraph*, 64> retired_;
    std::atomic<bool> playing_{false}, clipped_{false}, freeRunning_{false};
    bool wasPlaying_ = false;
    std::atomic<std::int64_t> position_{0}, seek_{-1};
    std::atomic<float> peak_{0};
    std::array<std::atomic<float>, maxTracks> trackPeaks_{}, leftPeaks_{}, rightPeaks_{};
};
} // namespace reverie
