// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace reverie {
struct MidiMessage {
    std::uint64_t hostTime = 0;
    std::uint8_t status = 0, data1 = 0, data2 = 0;
    std::int32_t device = 0;
};
struct MidiDevice {
    std::int32_t id;
    std::string name;
};
// Callback only decodes bytes into a bounded queue. UI drains and dispatches to audio SPSC.
class MidiInput {
  public:
    MidiInput();
    ~MidiInput();
    std::vector<MidiDevice> devices();
    void select(std::int32_t id); // -1 all devices, 0 disabled
    bool pop(MidiMessage&) noexcept;
    void inject(MidiMessage) noexcept; // also used by hardware-free integration tests
    bool takeOverflow() noexcept;
    bool takeDevicesChanged() noexcept;
    double secondsSince(std::uint64_t hostTime) const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace reverie
