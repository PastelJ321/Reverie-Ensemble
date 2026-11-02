// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Effects.h"
#include <numbers>
namespace reverie {
EffectChain::EffectChain(const Effects& s, double r)
    : settings_(s), alpha_(float(1 - std::exp(-2 * std::numbers::pi * 800 / r))),
      lowGain_(std::pow(10.f, s.lowDb / 20)), highGain_(std::pow(10.f, s.highDb / 20)) {
    inputGain_ = std::pow(10.f, s.inputDb / 20);
    outputGain_ = std::pow(10.f, (s.outputDb + s.makeupDb) / 20);
    attack_ = float(std::exp(-1 / (std::max(.1f, s.attackMs) * .001 * r)));
    release_ = float(std::exp(-1 / (std::max(1.f, s.releaseMs) * .001 * r)));
    double w = 2 * std::numbers::pi * std::min(double(s.eqHz), r * .45) / r,
           A = std::pow(10., s.eqDb / 40.), alpha = std::sin(w) / (2 * s.eqQ), a0 = 1 + alpha / A;
    eq_ = {(1 + alpha * A) / a0, -2 * std::cos(w) / a0, (1 - alpha * A) / a0, -2 * std::cos(w) / a0,
           (1 - alpha / A) / a0};
    if (s.enabled && s.delayMix > 0)
        delay_.resize(std::max(std::size_t(1), std::size_t(std::llround(r * s.delaySeconds))) * 2);
    if (s.enabled && s.reverbMix > 0)
        for (int i = 0; i < 4; ++i)
            comb_[i].resize(std::size_t(r * (.0297 + i * .0071)) * 2 + 2);
}
void EffectChain::inherit(EffectChain& old) noexcept {
    const auto& a = settings_;
    const auto& b = old.settings_;
    if (!(a == b))
        return;
    std::swap(envelope_, old.envelope_);
    std::swap(limitEnvelope_, old.limitEnvelope_);
    std::swap(eqState_, old.eqState_);
    delay_.swap(old.delay_);
    comb_.swap(old.comb_);
    std::swap(low_, old.low_);
    std::swap(cursor_, old.cursor_);
    std::swap(combCursor_, old.combCursor_);
}
void EffectChain::reset() noexcept {
    low_ = {};
    envelope_ = limitEnvelope_ = 0;
    eqState_ = {};
    std::fill(delay_.begin(), delay_.end(), 0);
    for (auto& b : comb_)
        std::fill(b.begin(), b.end(), 0);
    cursor_ = 0;
    combCursor_ = {};
}
std::pair<float, float> EffectChain::next(float l, float r) noexcept {
    if (!settings_.enabled)
        return {l, r};
    float v[2] = {l * inputGain_, r * inputGain_};
    if (settings_.peq)
        for (int c = 0; c < 2; ++c) {
            double y = eq_[0] * v[c] + eqState_[c][0];
            eqState_[c][0] = eq_[1] * v[c] - eq_[3] * y + eqState_[c][1];
            eqState_[c][1] = eq_[2] * v[c] - eq_[4] * y;
            v[c] = float(y);
        }
    if (settings_.compressor) {
        float peak = std::max(std::abs(v[0]), std::abs(v[1]));
        float coeff = peak > envelope_ ? attack_ : release_;
        envelope_ = coeff * envelope_ + (1 - coeff) * peak;
        float db = 20 * std::log10(std::max(envelope_, 1e-9f));
        float reduction = std::max(0.f, db - settings_.thresholdDb) * (1 - 1 / settings_.ratio);
        float gain = std::pow(10.f, -reduction / 20);
        v[0] *= gain;
        v[1] *= gain;
    }
    for (int c = 0; c < 2; ++c) {
        low_[c] += alpha_ * (v[c] - low_[c]);
        v[c] = low_[c] * lowGain_ + (v[c] - low_[c]) * highGain_;
    }
    if (!delay_.empty()) {
        for (int c = 0; c < 2; ++c) {
            float old = delay_[cursor_ + c];
            delay_[cursor_ + c] = v[c] + old * settings_.feedback;
            v[c] += old * settings_.delayMix;
        }
        cursor_ = (cursor_ + 2) % delay_.size();
    }
    if (!comb_[0].empty()) {
        float wet[2] = {};
        for (int i = 0; i < 4; ++i) {
            for (int c = 0; c < 2; ++c) {
                float old = comb_[i][combCursor_[i] + c];
                comb_[i][combCursor_[i] + c] = v[c] + old * (.65f - i * .03f);
                wet[c] += old * .25f;
            }
            combCursor_[i] = (combCursor_[i] + 2) % comb_[i].size();
        }
        v[0] += wet[0] * settings_.reverbMix;
        v[1] += wet[1] * settings_.reverbMix;
    }
    v[0] *= outputGain_;
    v[1] *= outputGain_;
    if (settings_.limiter) {
        limitEnvelope_ = std::max(std::max(std::abs(v[0]), std::abs(v[1])), limitEnvelope_ * .999f);
        float gain = std::min(1.f, .8912509f / std::max(limitEnvelope_, 1e-9f));
        v[0] *= gain;
        v[1] *= gain;
    }
    return {v[0], v[1]};
}
} // namespace reverie
