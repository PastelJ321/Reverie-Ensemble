// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <array>
namespace reverie {
// Delay/reverb storage is allocated when a graph is prepared, never while rendering.
class EffectChain {
  public:
    EffectChain(const Effects&, double);
    std::pair<float, float> next(float, float) noexcept;
    void reset() noexcept;
    void inherit(EffectChain&) noexcept;

  private:
    Effects settings_;
    float alpha_, lowGain_, highGain_;
    float inputGain_ = 1, outputGain_ = 1, envelope_ = 0, limitEnvelope_ = 0, attack_ = 0,
          release_ = 0;
    std::array<double, 5> eq_{};
    std::array<std::array<double, 2>, 2> eqState_{};
    std::array<float, 2> low_{};
    std::vector<float> delay_;
    std::array<std::vector<float>, 4> comb_;
    std::size_t cursor_ = 0;
    std::array<std::size_t, 4> combCursor_{};
};
} // namespace reverie
