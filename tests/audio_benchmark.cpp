// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Engine.h"
#include <array>
#include <chrono>
#include <iostream>
using namespace reverie;
int main() {
    Project p;
    p.master = .08;
    for (int t = 0; t < 16; ++t) {
        auto track = makeTrack(TrackKind::Midi, t);
        track.preset = factoryPresets()[7];
        Clip c;
        c.length = 8;
        for (int n = 0; n < 16; ++n) {
            Note note;
            note.key = 40 + n;
            note.duration = 8;
            c.notes.push_back(note);
        }
        track.clips.push_back(c);
        p.tracks.push_back(track);
    }
    AudioEngine e;
    e.publish(p);
    e.setPlaying(true);
    std::array<float, 512> output{};
    std::vector<double> times;
    times.reserve(800);
    for (int i = 0; i < 750; ++i) {
        auto start = std::chrono::steady_clock::now();
        e.process(output.data(), 256);
        auto stop = std::chrono::steady_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(stop - start).count());
    }
    std::sort(times.begin(), times.end());
    std::cout << "16 tracks / 256 held voices / 48 kHz / 256 frames\nCPU callback ms: p50="
              << times[times.size() / 2] << " p99=" << times[times.size() * 99 / 100]
              << " max=" << times.back() << "; buffer deadline=" << 256. / 48
              << " ms\nThis is a synthetic CPU test, not a device/glitch or round-trip latency "
                 "measurement.\n";
}
