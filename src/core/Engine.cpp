// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Engine.h"
#include <bit>
#include <numbers>
#include <stdexcept>
namespace reverie {
static double checkedRate(double rate) {
    if (!std::isfinite(rate) || rate < 8000 || rate > 192000)
        throw std::runtime_error("Unsupported sample rate");
    return rate;
}
static_assert(std::atomic<void*>::is_always_lock_free);
static_assert(std::atomic<float>::is_always_lock_free);
static_assert(std::atomic<std::int64_t>::is_always_lock_free);
PreparedTrack::PreparedTrack(const Track& t, double rate, bool enabled)
    : id(t.id), sampleOwner(t.instrumentSample), synth(t.preset, rate, sampleOwner.get()) {
    // Stereo balance: center leaves both channels unchanged; hard pan silences the opposite side.
    gainL = enabled ? t.volume * std::sqrt(1.f - std::max(0.f, t.pan)) : 0;
    gainR = enabled ? t.volume * std::sqrt(1.f + std::min(0.f, t.pan)) : 0;
    targetL = gainL;
    targetR = gainR;
    auto mix = [&](std::uint64_t v) {
        soundKey ^= v + 0x9e3779b97f4a7c15ULL + (soundKey << 6) + (soundKey >> 2);
    };
    const auto& p = t.preset;
    mix(int(p.kind));
    mix(int(p.wave));
    mix(p.rootNote);
    mix(reinterpret_cast<std::uintptr_t>(sampleOwner.get()));
    for (float v : {p.attack, p.decay, p.sustain, p.release, p.cutoff, p.gain, p.color})
        mix(std::bit_cast<std::uint32_t>(v));
}
AudioProcessingGraph::AudioProcessingGraph(const Project& p, double r)
    : rate([&] {
          validate(p);
          return checkedRate(r);
      }()),
      tempo{p.bpm, rate}, master(p.master),
      end(tempo.sampleAt(p.endBeat()) + std::int64_t(rate * p.tailSeconds())) {
    const bool anySolo =
        std::any_of(p.tracks.begin(), p.tracks.end(), [](const Track& t) { return t.solo; });
    tracks.reserve(p.tracks.size());
    for (const auto& t : p.tracks) {
        tracks.emplace_back(t, r, !t.mute && (!anySolo || t.solo));
        auto& rt = tracks.back();
        for (const auto& c : t.clips) {
            if (t.kind == TrackKind::Audio) {
                if (c.sample)
                    rt.audio.push_back({tempo.sampleAt(c.beat), tempo.sampleAt(c.beat + c.length),
                                        c.sourceOffset * c.sample->sampleRate,
                                        c.sample->sampleRate / r, c.sample});
                continue;
            }
            for (const auto& n : c.notes) {
                if (n.beat >= c.length || n.velocity <= 0)
                    continue;
                const auto begin = tempo.sampleAt(c.beat + n.beat),
                           finish = std::max(
                               begin + 1,
                               tempo.sampleAt(c.beat + std::min(c.length, n.beat + n.duration)));
                rt.events.push_back({begin, n.id, n.key, n.velocity, true, finish});
                rt.events.push_back({finish, n.id, n.key, n.velocity, false});
            }
        }
        std::sort(rt.events.begin(), rt.events.end(), [](const Event& a, const Event& b) {
            return a.frame != b.frame ? a.frame < b.frame : int(a.on) < int(b.on);
        });
        rt.buildIndex();
    }
}
void PreparedTrack::buildIndex() {
    for (const auto& e : events) {
        auto mix = [&](std::uint64_t v) {
            scheduleKey ^= v + 0x9e3779b97f4a7c15ULL + (scheduleKey << 6) + (scheduleKey >> 2);
        };
        mix(e.frame);
        mix(e.token);
        mix(e.key);
        mix(std::bit_cast<std::uint32_t>(e.velocity));
        mix(e.on);
        if (e.on)
            spans.push_back(e);
    }
    intervalMax.resize(spans.size() * 4 + 4);
    auto build = [&](auto&& self, std::size_t node, std::size_t a, std::size_t b) -> std::int64_t {
        if (b - a == 1)
            return intervalMax[node] = spans[a].endFrame;
        auto mid = a + (b - a) / 2;
        auto left = self(self, node * 2, a, mid);
        auto right = self(self, node * 2 + 1, mid, b);
        return intervalMax[node] = std::max(left, right);
    };
    if (!spans.empty())
        build(build, 1, 0, spans.size());
}
void PreparedTrack::restore(std::size_t node, std::size_t a, std::size_t b, std::int64_t frame,
                            int& voices) noexcept {
    if (voices >= 32 || intervalMax[node] <= frame || spans[a].frame >= frame)
        return;
    if (b - a == 1) {
        const auto& e = spans[a];
        synth.noteOn(e.token, e.key, e.velocity);
        ++voices;
        return;
    }
    auto mid = a + (b - a) / 2;
    restore(node * 2, a, mid, frame, voices);
    restore(node * 2 + 1, mid, b, frame, voices);
}
void PreparedTrack::seek(std::int64_t frame) noexcept {
    synth.reset();
    cursor =
        std::size_t(std::lower_bound(events.begin(), events.end(), frame,
                                     [](const Event& e, std::int64_t f) { return e.frame < f; }) -
                    events.begin());
    int voices = 0;
    if (!spans.empty())
        restore(1, 0, spans.size(), frame, voices);
}
void AudioProcessingGraph::seek(std::int64_t frame) noexcept {
    for (auto& t : tracks)
        t.seek(frame);
}
void AudioProcessingGraph::inherit(const AudioProcessingGraph* previous,
                                   std::int64_t frame) noexcept {
    for (auto& t : tracks) {
        t.seek(frame);
        if (previous)
            for (const auto& old : previous->tracks)
                if (t.id == old.id) {
                    t.gainL = old.gainL;
                    t.gainR = old.gainR;
                    t.rampRemaining = 128;
                    if (t.soundKey == old.soundKey && t.scheduleKey == old.scheduleKey) {
                        t.synth.copyStateFrom(old.synth);
                        t.cursor = old.cursor;
                    }
                    break;
                }
    }
}
void AudioProcessingGraph::render(float* out, std::uint32_t frames, std::int64_t start) noexcept {
    std::fill_n(out, std::size_t(frames) * 2, 0.f);
    // Fixed scratch space also handles devices which negotiate a larger callback block.
    constexpr std::uint32_t blockSize = 256;
    std::array<float, blockSize * 2> scratch{};
    for (auto& t : tracks) {
        t.peak = 0;
        for (std::uint32_t offset = 0; offset < frames; offset += blockSize) {
            auto count = std::min(blockSize, frames - offset);
            auto blockStart = start + offset;
            for (std::uint32_t i = 0; i < count; ++i) {
                const auto pos = blockStart + i;
                while (t.cursor < t.events.size() && t.events[t.cursor].frame <= pos) {
                    const auto& e = t.events[t.cursor++];
                    if (e.on)
                        t.synth.noteOn(e.token, e.key, e.velocity);
                    else
                        t.synth.noteOff(e.token);
                }
                auto [l, r] = t.synth.next();
                scratch[i * 2] = l;
                scratch[i * 2 + 1] = r;
            }
            for (const auto& c : t.audio) {
                const auto first = std::max(blockStart, c.start),
                           last = std::min(blockStart + count, c.end);
                for (auto pos = first; pos < last; ++pos) {
                    const double source = double(pos - c.start) * c.step + c.offset;
                    const auto idx = std::size_t(source);
                    if (idx >= c.sample->frames())
                        break;
                    const auto next = std::min(idx + 1, c.sample->frames() - 1);
                    float f = float(source - double(idx));
                    auto i = std::size_t(pos - blockStart);
                    scratch[i * 2] +=
                        std::lerp(c.sample->stereo[idx * 2], c.sample->stereo[next * 2], f);
                    scratch[i * 2 + 1] +=
                        std::lerp(c.sample->stereo[idx * 2 + 1], c.sample->stereo[next * 2 + 1], f);
                }
            }
            for (std::uint32_t i = 0; i < count; ++i) {
                if (t.rampRemaining > 0) {
                    t.gainL += (t.targetL - t.gainL) / float(t.rampRemaining);
                    t.gainR += (t.targetR - t.gainR) / float(t.rampRemaining);
                    --t.rampRemaining;
                }
                float l = scratch[i * 2] * t.gainL, r = scratch[i * 2 + 1] * t.gainR;
                t.peak = std::max(t.peak, std::max(std::abs(l), std::abs(r)));
                out[(offset + i) * 2] += l * master;
                out[(offset + i) * 2 + 1] += r * master;
            }
        }
    }
}
AudioEngine::AudioEngine(double rate) : rate_(checkedRate(rate)) {}
AudioEngine::~AudioEngine() {
    delete pending_.exchange(nullptr);
    delete active_;
    collect();
}
void AudioEngine::collect() {
    AudioProcessingGraph* p;
    while (retired_.pop(p))
        delete p;
}
void AudioEngine::publish(const Project& p) {
    auto graph = std::make_unique<AudioProcessingGraph>(p, rate_);
    collect();
    delete pending_.exchange(graph.release(), std::memory_order_acq_rel);
}
void AudioEngine::process(float* out, std::uint32_t frames) noexcept {
    auto pos = position_.load(std::memory_order_relaxed);
    if (!retired_.full())
        if (auto* next = pending_.exchange(nullptr, std::memory_order_acq_rel)) {
            next->inherit(active_, pos);
            if (active_)
                retired_.push(active_);
            active_ = next;
        }
    auto request = seek_.exchange(-1);
    if (request >= 0) {
        pos = request;
        position_.store(pos);
        if (active_)
            active_->seek(pos);
    }
    const bool play = playing_.load();
    if (play && !wasPlaying_ && active_)
        active_->seek(pos);
    wasPlaying_ = play;
    if (!play || !active_) {
        std::fill_n(out, std::size_t(frames) * 2, 0.f);
        peak_.store(0);
        for (auto& meter : trackPeaks_)
            meter.store(0);
        return;
    }
    active_->render(out, frames, pos);
    float peak = 0;
    for (std::size_t i = 0; i < std::size_t(frames) * 2; ++i) {
        if (!std::isfinite(out[i]))
            out[i] = 0;
        peak = std::max(peak, std::abs(out[i]));
        out[i] = std::clamp(out[i], -1.f, 1.f);
    }
    peak_.store(peak);
    if (peak > 1)
        clipped_.store(true);
    for (int i = 0; i < maxTracks; ++i)
        trackPeaks_[i].store(i < int(active_->tracks.size()) ? active_->tracks[i].peak : 0);
    pos += frames;
    position_.store(pos);
    if (pos >= active_->end && !freeRunning_.load())
        playing_.store(false);
}
} // namespace reverie
