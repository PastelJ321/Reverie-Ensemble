// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Engine.h"
#include <unordered_map>
namespace reverie {
// Control-thread capture. Input timestamps are transport frames, never wall-clock durations.
class MidiRecording {
    TempoMap tempo_;
    std::int64_t start_ = 0;
    std::unordered_map<Id, std::size_t> open_;
    Clip clip_;

  public:
    void start(TempoMap tempo, std::int64_t startFrame) {
        tempo_ = tempo;
        start_ = startFrame;
        open_.clear();
        clip_ = Clip{};
        clip_.name = "MIDI take";
        clip_.beat = tempo.beatAt(startFrame);
        clip_.length = .25;
    }
    void capture(const Event& e) {
        if (e.frame < start_)
            return;
        double beat = tempo_.beatAt(e.frame - start_);
        if (e.controller >= 0)
            clip_.controllers.push_back({beat, e.channel, e.controller, e.value});
        else if (e.on) {
            if (open_.contains(e.token))
                return;
            Note n;
            n.id = e.token;
            n.key = e.key;
            n.velocity = e.velocity;
            n.channel = e.channel;
            n.beat = beat;
            n.duration = 1. / 4096;
            open_[e.token] = clip_.notes.size();
            clip_.notes.push_back(n);
        } else if (auto it = open_.find(e.token); it != open_.end()) {
            auto& n = clip_.notes[it->second];
            n.duration = std::max(1. / 4096, beat - n.beat);
            open_.erase(it);
        }
        clip_.length = std::max(clip_.length, beat + .001);
    }
    void updatePosition(std::int64_t frame) {
        if (frame < start_)
            return;
        double beat = tempo_.beatAt(frame - start_);
        for (auto [token, index] : open_) {
            (void)token;
            auto& note = clip_.notes[index];
            note.duration = std::max(1. / 4096, beat - note.beat);
        }
        clip_.length = std::max(.25, beat);
    }
    Clip finish(std::int64_t frame) {
        double beat = std::max(0., tempo_.beatAt(frame - start_));
        for (auto [token, index] : open_) {
            (void)token;
            auto& n = clip_.notes[index];
            n.duration = std::max(1. / 4096, beat - n.beat);
        }
        for (int ch = 0; ch < 16; ++ch)
            clip_.controllers.push_back({beat, ch, 64, 0});
        clip_.length = std::max(.25, beat);
        open_.clear();
        return clip_;
    }
    const Clip& preview() const {
        return clip_;
    }
};
} // namespace reverie
