// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Engine.h"
#include "core/Recorder.h"
namespace reverie {
class AudioDeviceManager {
  public:
    explicit AudioDeviceManager(AudioEngine&);
    ~AudioDeviceManager();
    bool startOutput(bool forceFailure = false);
    void stopOutput();
    std::vector<std::string> inputDevices();
    bool startCapture(int deviceIndex, AudioRecorder&);
    void stopCapture();
    std::string error() const;
    bool outputRunning() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
bool requestMicrophonePermission();
} // namespace reverie
