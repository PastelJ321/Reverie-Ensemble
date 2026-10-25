// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <array>
namespace reverie {
struct BackendCapabilities {
    const char* name;
    bool controllers, pitchBend, streaming, soundFontPrograms;
};
// Construct/load/prepare and destroy on the control thread. Render <=256 frames.
// Events are submitted in chronological order, with offsets within the next block.
class InstrumentBackend {
  public:
    virtual ~InstrumentBackend() = default;
    virtual void noteOn(Id, int, float, int channel, int offset = 0) noexcept = 0;
    virtual void noteOff(Id, int offset = 0) noexcept = 0;
    virtual void controller(int channel, int cc, int value, int offset = 0) noexcept = 0;
    virtual void pitchBend(int channel, int value, int offset = 0) noexcept = 0;
    virtual void render(float* stereo, unsigned frames) noexcept = 0;
    virtual void reset() noexcept = 0;
    virtual void setOffline(bool) noexcept {}
    virtual std::uint64_t memoryBytes() const noexcept {
        return 0;
    }
    virtual std::string diagnostics() const {
        return {};
    }
    virtual BackendCapabilities capabilities() const noexcept = 0;
};
std::unique_ptr<InstrumentBackend> prepareBackend(const Preset&, double rate,
                                                  bool tolerateMissing = false);
bool sfizzAvailable();
bool fluidSynthAvailable();
struct SoundFontProgram {
    int bank, program;
    std::string name;
};
std::vector<SoundFontProgram> soundFontPrograms(const std::filesystem::path&);
Preset externalPreset(const std::filesystem::path&, InstrumentKind, int bank = 0, int program = 0);
} // namespace reverie
