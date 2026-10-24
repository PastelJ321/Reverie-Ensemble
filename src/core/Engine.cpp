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
    : id(t.id), sampleOwner(t.instrumentSample), synth(t.preset, rate, sampleOwner.get()),
      effects(t.effects, rate) {
    // Stereo balance: center leaves both channels unchanged; hard pan silences the opposite side.
    gainL = enabled ? t.volume * std::sqrt(1.f - std::max(0.f, t.pan)) : 0;
    gainR = enabled ? t.volume * std::sqrt(1.f + std::min(0.f, t.pan)) : 0;
    targetL = gainL;
    targetR = gainR;
    auto mix = [&](std::uint64_t v) {
        soundKey ^= v + 0x9e3779b97f4a7c15ULL + (soundKey << 6) + (soundKey >> 2);
    };
    const auto& p = t.preset;
    for (const auto& z : p.zones) {
        mix(reinterpret_cast<std::uintptr_t>(z.sample.get()));
        mix(z.lowKey);
        mix(z.highKey);
        mix(z.rootKey);
        mix(z.lowVelocity);
        mix(z.highVelocity);
        mix(z.start);
        mix(z.end);
        mix(z.loopStart);
        mix(z.loopEnd);
        mix(z.loop);
        mix(z.oneShot);
        mix(z.loopContinuous);
        mix(std::bit_cast<std::uint32_t>(z.gain));
        mix(z.sequenceLength);
        mix(z.sequencePosition);
    }
    mix(int(p.kind));
    mix(int(p.wave));
    mix(p.rootNote);
    mix(reinterpret_cast<std::uintptr_t>(sampleOwner.get()));
    mix(p.cubicInterpolation);
    for (float v :
         {p.attack, p.decay, p.sustain, p.release, p.cutoff, p.gain, p.color, p.calibrationGain})
        mix(std::bit_cast<std::uint32_t>(v));
}
AudioProcessingGraph::AudioProcessingGraph(const Project& p, double r)
    : rate([&] {
          validate(p);
          return checkedRate(r);
      }()),
      tempo{p.bpm, rate}, master(p.master),
      end(tempo.sampleAt(p.endBeat()) + std::int64_t(rate * p.tailSeconds())) {
    loop = p.loopEnabled;
    metronome = p.metronome;
    loopStart = tempo.sampleAt(p.loopStart);
    loopEnd = tempo.sampleAt(p.loopEnd);
    numerator = p.numerator;
    denominator = p.denominator;
    clickLevel = p.clickLevel;
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
void PreparedTrack::seek(std::int64_t frame, bool clearEffects) noexcept {
    synth.reset();
    if (clearEffects)
        effects.reset();
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
void AudioProcessingGraph::inherit(AudioProcessingGraph* previous, std::int64_t frame) noexcept {
    for (auto& t : tracks) {
        t.seek(frame, false);
        if (previous)
            for (auto& old : previous->tracks)
                if (t.id == old.id) {
                    t.effects.inherit(old.effects);
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
        t.peak = t.peakL = t.peakR = 0;
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
                auto processed = t.effects.next(scratch[i * 2], scratch[i * 2 + 1]);
                float l = processed.first * t.gainL, r = processed.second * t.gainR;
                t.peakL = std::max(t.peakL, std::abs(l));
                t.peakR = std::max(t.peakR, std::abs(r));
                t.peak = std::max(t.peak, std::max(std::abs(l), std::abs(r)));
                out[(offset + i) * 2] += l * master;
                out[(offset + i) * 2 + 1] += r * master;
            }
        }
    }
}
AudioEngine::AudioEngine(double rate, bool tools, bool protect)
    : rate_(checkedRate(rate)), transportTools_(tools) {
    protectOutput_ = protect;
}
AudioEngine::~AudioEngine() {
    delete pending_.exchange(nullptr);
    delete active_;
    delete previewPending_.exchange(nullptr);
    delete preview_;
    collect();
}
void AudioEngine::setSampleRateStopped(double rate) {
    auto nextRate = checkedRate(rate);
    playing_.store(false);
    wasPlaying_ = false;
    delete pending_.exchange(nullptr);
    delete active_;
    active_ = nullptr;
    delete previewPending_.exchange(nullptr);
    delete preview_;
    preview_ = nullptr;
    collect();
    rate_ = nextRate;
    position_.store(0);
    seek_.store(-1);
    peak_.store(0);
    clearClip();
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
    std::fill_n(out, std::size_t(frames) * 2, 0.f);
    if (play && active_) {
        std::uint32_t offset = 0;
        while (offset < frames) {
            bool looping = transportTools_ && active_->loop && !freeRunning_.load();
            if (looping && (pos >= active_->loopEnd || pos < active_->loopStart)) {
                pos = active_->loopStart;
                active_->seek(pos);
            }
            auto n = frames - offset;
            if (looping)
                n = std::uint32_t(std::min(std::int64_t(n), active_->loopEnd - pos));
            active_->render(out + offset * 2, n, pos);
            if (transportTools_ && active_->metronome) {
                double beatFrames = 60. * rate_ / active_->tempo.bpm * 4. / active_->denominator;
                for (std::uint32_t i = 0; i < n; ++i) {
                    double beat = (pos + i) / beatFrames;
                    auto index = std::int64_t(std::floor(beat));
                    double elapsed = (beat - index) * beatFrames / rate_;
                    if (elapsed < .025) {
                        float click =
                            active_->clickLevel *
                            float(std::sin(6.283185307179586 *
                                           (index % active_->numerator ? 1100 : 1700) * elapsed) *
                                  std::exp(-elapsed * 180));
                        out[(offset + i) * 2] += click;
                        out[(offset + i) * 2 + 1] += click;
                    }
                }
            }
            // Short loop-edge fades avoid a hard discontinuity. No allocations or graph rebuilds.
            if (looping)
                for (std::uint32_t i = 0; i < n; ++i) {
                    auto distance =
                        std::min(pos + i - active_->loopStart, active_->loopEnd - (pos + i));
                    float gain = float(std::clamp(double(distance) / 64., 0., 1.));
                    out[(offset + i) * 2] *= gain;
                    out[(offset + i) * 2 + 1] *= gain;
                }
            offset += n;
            pos += n;
        }
        if (transportTools_ && active_->loop && pos >= active_->loopEnd && !freeRunning_.load()) {
            pos = active_->loopStart;
            active_->seek(pos);
        }
        position_.store(pos);
        if (pos >= active_->end && !freeRunning_.load() && !(transportTools_ && active_->loop))
            playing_.store(false);
    }
    if (!retired_.full())
        if (auto* p = previewPending_.exchange(nullptr)) {
            if (preview_)
                retired_.push(preview_);
            preview_ = p;
            previewPosition_ = 0;
            previewLimit_ = std::min(p->end, std::int64_t(rate_ * 6));
        }
    if (preview_ && releasePreview_.exchange(false)) {
        for (auto& t : preview_->tracks)
            t.synth.releaseAll();
        previewLimit_ = std::min(previewLimit_, previewPosition_ + std::int64_t(rate_ * 2));
    }
    if (preview_ && previewPosition_ < previewLimit_) {
        std::array<float, 512> scratch{};
        for (std::uint32_t offset = 0; offset < frames; offset += 256) {
            auto n = std::min(std::uint32_t(256), frames - offset);
            preview_->render(scratch.data(), n, previewPosition_);
            for (std::uint32_t i = 0; i < n * 2; ++i)
                out[offset * 2 + i] += scratch[i] * (active_ ? active_->master : .8f);
            previewPosition_ += n;
        }
    }
    float peak = 0;
    for (std::size_t i = 0; i < std::size_t(frames) * 2; ++i) {
        if (!std::isfinite(out[i]))
            out[i] = 0;
        peak = std::max(peak, std::abs(out[i]));
        if (protectOutput_)
            out[i] = std::clamp(out[i], -1.f, 1.f);
    }
    peak_.store(peak);
    if (peak > 1)
        clipped_.store(true);
    for (int i = 0; i < maxTracks; ++i) {
        bool valid = play && active_ && i < int(active_->tracks.size());
        trackPeaks_[i].store(valid ? active_->tracks[i].peak : 0);
        leftPeaks_[i].store(valid ? active_->tracks[i].peakL : 0);
        rightPeaks_[i].store(valid ? active_->tracks[i].peakR : 0);
    }
}
void AudioEngine::audition(const Track& source, int key, float velocity, double duration) {
    if (key < 0 || key > 127 || !std::isfinite(velocity))
        return;
    Project p;
    p.master = .45f;
    auto t = source;
    t.id = nextId();
    t.kind = TrackKind::Midi;
    t.clips.clear();
    t.mute = t.solo = false;
    t.volume = std::min(t.volume, .8f);
    Clip c;
    c.length = std::max(.25, duration * 2);
    Note n;
    n.key = key;
    n.velocity = std::clamp(velocity, 0.f, 1.f);
    n.duration = std::max(.01, duration * 2);
    c.notes.push_back(n);
    t.clips.push_back(c);
    p.tracks.push_back(t);
    auto graph = std::make_unique<AudioProcessingGraph>(p, rate_);
    collect();
    releasePreview_.store(false);
    delete previewPending_.exchange(graph.release());
}
} // namespace reverie
