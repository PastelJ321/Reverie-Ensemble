// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include <array>
namespace reverie {
// Instruments never allocate in noteOn/noteOff/next/reset. Optional backends must honor this
// contract.
class Instrument {
  public:
    virtual ~Instrument() = default;
    virtual void noteOn(Id token, int key, float velocity) noexcept = 0;
    virtual void noteOff(Id token) noexcept = 0;
    virtual std::pair<float, float> next() noexcept = 0;
    virtual void reset() noexcept = 0;
};
class Synthesizer final : public Instrument {
  public:
    Synthesizer(Preset preset, double rate, const Sample* sample = nullptr);
    void noteOn(Id token, int key, float velocity) noexcept override;
    void noteOff(Id token) noexcept override;
    std::pair<float, float> next() noexcept override;
    void reset() noexcept override;
    void copyStateFrom(const Synthesizer& source) noexcept {
        voices_ = source.voices_;
    }

  private:
    struct Voice {
        Id token = 0;
        int key = 60, stage = 0;
        double phase = 0, increment = 0, samplePosition = 0, sampleStep = 1, age = 0;
        float env = 0, releaseStep = 0, velocity = 0, filterL = 0, filterR = 0;
        std::uint32_t noise = 12345;
    };
    std::array<Voice, 32> voices_{};
    Preset preset_;
    double rate_;
    float filterAlpha_;
    const Sample* sample_;
    float oscillator(Voice&) const noexcept;
};
// Effect interface is deliberately independent of instrument scheduling and Qt.
class AudioEffect {
  public:
    virtual ~AudioEffect() = default;
    virtual void process(float* interleavedStereo, std::uint32_t frames) noexcept = 0;
    virtual void reset() noexcept = 0;
};
} // namespace reverie
