// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Dsp.h"
#include <numbers>
namespace reverie {
static constexpr double tau = 2. * std::numbers::pi;
static float blep(double t, double dt) noexcept {
    if (t < dt) {
        t /= dt;
        return float(t + t - t * t - 1.);
    }
    if (t > 1. - dt) {
        t = (t - 1.) / dt;
        return float(t * t + t + t + 1.);
    }
    return 0;
}
Synthesizer::Synthesizer(Preset p, double r, const Sample* s)
    : preset_(std::move(p)), rate_(r),
      filterAlpha_(float(1. - std::exp(-tau * std::min(double(preset_.cutoff), r * .45) / r))),
      sample_(s) {}
void Synthesizer::reset() noexcept {
    for (auto& v : voices_)
        v = Voice{};
    sequence_ = {};
}
void Synthesizer::noteOn(Id token, int key, float velocity) noexcept {
    Voice* target = &voices_[0];
    for (auto& v : voices_) {
        if (!v.stage) {
            target = &v;
            break;
        }
        if (v.env < target->env)
            target = &v;
    }
    *target = Voice{};
    auto& v = *target;
    v.token = token;
    v.key = key;
    v.stage = 1;
    v.velocity = velocity;
    v.increment = std::min(.45, 440. * std::exp2((key - 69) / 12.) / rate_);
    v.sampleStep =
        (sample_ ? sample_->sampleRate : rate_) / rate_ * std::exp2((key - preset_.rootNote) / 12.);
    if (preset_.kind == InstrumentKind::Sampler && !preset_.zones.empty()) {
        int best = -1, distance = 1000;
        int velocityKey = int(velocity * 127);
        auto sequence = sequence_[std::clamp(key, 0, 127)]++;
        for (int i = 0; i < int(preset_.zones.size()); ++i) {
            const auto& z = preset_.zones[i];
            if (z.sample && key >= z.lowKey && key <= z.highKey && velocityKey >= z.lowVelocity &&
                velocityKey <= z.highVelocity &&
                int(sequence % z.sequenceLength) + 1 == z.sequencePosition &&
                std::abs(key - z.rootKey) < distance) {
                best = i;
                distance = std::abs(key - z.rootKey);
            }
        }
        v.zone = best;
        if (best < 0) {
            v.stage = 0;
            return;
        }
        const auto& z = preset_.zones[best];
        v.samplePosition = double(z.start);
        v.sampleStep = z.sample->sampleRate / rate_ * std::exp2((key - z.rootKey) / 12.);
        v.velocity *= z.gain;
    }
    v.noise = std::uint32_t(token * 747796405ULL + 2891336453ULL) | 1u;
}
void Synthesizer::noteOff(Id token) noexcept {
    for (auto& v : voices_)
        if (v.stage && v.token == token && v.stage != 4) {
            if (v.zone >= 0 && preset_.zones[v.zone].oneShot)
                continue;
            v.stage = 4;
            v.releaseStep = v.env / std::max(1.f, preset_.release * float(rate_));
        }
}
void Synthesizer::releaseAll() noexcept {
    for (auto& v : voices_)
        if (v.stage && v.stage != 4) {
            v.stage = 4;
            v.releaseStep = v.env / std::max(1.f, preset_.release * float(rate_));
        }
}
float Synthesizer::oscillator(Voice& v) const noexcept {
    const double p = v.phase;
    float x = 0;
    switch (preset_.wave) {
    case Waveform::Sine:
        x = float(std::sin(tau * p));
        break;
    case Waveform::Saw:
        x = float(2 * p - 1) - blep(p, v.increment);
        break;
    case Waveform::Square:
        x = (p < .5 ? 1.f : -1.f) + blep(p, v.increment) - blep(std::fmod(p + .5, 1.), v.increment);
        break;
    case Waveform::Triangle:
        x = float(1. - 4. * std::abs(p - .5));
        break;
    }
    if (preset_.color > 0)
        x = x * (1.f - preset_.color * .4f) +
            preset_.color * .3f * float(std::sin(tau * p * 2.)) * float(std::exp(-v.age * 1.8));
    return x;
}
std::pair<float, float> Synthesizer::next() noexcept {
    float left = 0, right = 0;
    for (auto& v : voices_) {
        if (!v.stage)
            continue;
        if (v.stage == 1) {
            v.env += 1.f / (preset_.attack * float(rate_));
            if (v.env >= 1) {
                v.env = 1;
                v.stage = 2;
            }
        } else if (v.stage == 2) {
            v.env -= (1.f - preset_.sustain) / (preset_.decay * float(rate_));
            if (v.env <= preset_.sustain) {
                v.env = preset_.sustain;
                v.stage = 3;
            }
        } else if (v.stage == 4) {
            v.env -= v.releaseStep;
            if (v.env <= 0) {
                v.stage = 0;
                continue;
            }
        }
        float l = 0, r = 0;
        if (preset_.kind == InstrumentKind::Sampler) {
            const auto* z = v.zone >= 0 ? &preset_.zones[v.zone] : nullptr;
            const Sample* sample = z ? z->sample.get() : sample_;
            if (!sample) {
                v.stage = 0;
                continue;
            }
            const auto end =
                z && z->end ? std::min(z->end, std::uint64_t(sample->frames())) : sample->frames();
            if (z && z->loop && (v.stage != 4 || z->loopContinuous) &&
                v.samplePosition >= z->loopEnd)
                v.samplePosition = z->loopStart + std::fmod(v.samplePosition - z->loopStart,
                                                            double(z->loopEnd - z->loopStart));
            if (v.samplePosition >= double(end)) {
                v.stage = 0;
                continue;
            }
            const auto idx = std::size_t(v.samplePosition),
                       next = std::min(idx + 1, std::size_t(end) - 1);
            const float f = float(v.samplePosition - double(idx));
            auto interpolate = [&](int ch) {
                auto value = [&](std::int64_t frame) {
                    return sample
                        ->stereo[std::size_t(std::clamp(frame, std::int64_t(z ? z->start : 0),
                                                        std::int64_t(end) - 1)) *
                                     2 +
                                 ch];
                };
                if (!preset_.cubicInterpolation)
                    return std::lerp(value(idx), value(next), f);
                float a = value(std::int64_t(idx) - 1), b = value(idx), c = value(next),
                      d = value(std::int64_t(idx) + 2);
                return b +
                       .5f * f *
                           (c - a + f * (2 * a - 5 * b + 4 * c - d + f * (3 * (b - c) + d - a)));
            };
            l = interpolate(0);
            r = interpolate(1);
            v.samplePosition += v.sampleStep;
        } else if (preset_.kind == InstrumentKind::Drums) {
            v.noise ^= v.noise << 13;
            v.noise ^= v.noise >> 17;
            v.noise ^= v.noise << 5;
            const float noise = float(v.noise) / float(UINT32_MAX) * 2 - 1;
            if (v.key == 36 || v.key == 35) {
                double freq = 45 + 110 * std::exp(-v.age * 40);
                v.phase += freq / rate_;
                l = float(std::sin(tau * v.phase) * std::exp(-v.age * 16));
            } else if (v.key == 38 || v.key == 40)
                l = (noise * .8f + float(std::sin(tau * 180 * v.age)) * .2f) *
                    float(std::exp(-v.age * 23));
            else
                l = noise * float(std::exp(-v.age * (v.key == 46 ? 14 : 65)));
            r = l;
            if (v.age > 1.2)
                v.stage = 0;
        } else {
            l = r = oscillator(v);
        }
        v.filterL += filterAlpha_ * (l - v.filterL);
        v.filterR += filterAlpha_ * (r - v.filterR);
        // Preserve the original multiply/add contraction for uncalibrated legacy projects.
        if (preset_.calibrationGain == 1.f) {
            left += v.filterL * v.env * v.velocity * preset_.gain;
            right += v.filterR * v.env * v.velocity * preset_.gain;
        } else {
            left += v.filterL * v.env * v.velocity * preset_.gain * preset_.calibrationGain;
            right += v.filterR * v.env * v.velocity * preset_.gain * preset_.calibrationGain;
        }
        if (preset_.kind != InstrumentKind::Drums)
            v.phase += v.increment;
        v.phase -= std::floor(v.phase);
        v.age += 1. / rate_;
    }
    return {left, right};
}
} // namespace reverie
