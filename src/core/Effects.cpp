// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Effects.h"
#include <numbers>
namespace reverie {
EffectChain::EffectChain(const Effects& s, double r)
    : settings_(s), alpha_(float(1 - std::exp(-2 * std::numbers::pi * 800 / r))),
      lowGain_(std::pow(10.f, s.lowDb / 20)), highGain_(std::pow(10.f, s.highDb / 20)) {
    if (s.enabled && s.delayMix > 0)
        delay_.resize(std::max(std::size_t(1), std::size_t(std::llround(r * s.delaySeconds))) * 2);
    if (s.enabled && s.reverbMix > 0)
        for (int i = 0; i < 4; ++i)
            comb_[i].resize(std::size_t(r * (.0297 + i * .0071)) * 2 + 2);
}
void EffectChain::inherit(EffectChain& old) noexcept {
    const auto& a = settings_;
    const auto& b = old.settings_;
    if (a.enabled != b.enabled || a.lowDb != b.lowDb || a.highDb != b.highDb ||
        a.delayMix != b.delayMix || a.delaySeconds != b.delaySeconds || a.feedback != b.feedback ||
        a.reverbMix != b.reverbMix)
        return;
    delay_.swap(old.delay_);
    comb_.swap(old.comb_);
    std::swap(low_, old.low_);
    std::swap(cursor_, old.cursor_);
    std::swap(combCursor_, old.combCursor_);
}
void EffectChain::reset() noexcept {
    low_ = {};
    std::fill(delay_.begin(), delay_.end(), 0);
    for (auto& b : comb_)
        std::fill(b.begin(), b.end(), 0);
    cursor_ = 0;
    combCursor_ = {};
}
std::pair<float, float> EffectChain::next(float l, float r) noexcept {
    if (!settings_.enabled)
        return {l, r};
    float v[2] = {l, r};
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
    return {v[0], v[1]};
}
} // namespace reverie
