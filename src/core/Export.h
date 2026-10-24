// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <functional>
namespace reverie {
enum class WavDepth { Pcm16 = 16, Pcm24 = 24, Float32 = 32 };
struct ExportOptions {
    int sampleRate = 48000, channels = 2;
    WavDepth depth = WavDepth::Pcm24;
    double startBeat = 0, endBeat = -1;
    bool includeTail = true, normalize = false, dither = false;
};
struct ExportResult {
    std::uint64_t frames = 0, clippedSamples = 0;
    double peak = 0, rms = 0, gain = 1;
};
// Return false to cancel; cancellation/failure removes only this operation's temporary files.
ExportResult renderAudio(const Project&, const std::filesystem::path&, const ExportOptions&,
                         const std::function<bool(double)>& progress = {});
} // namespace reverie
