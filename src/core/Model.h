// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace reverie {
using Id = std::uint64_t;
Id nextId();
void reserveId(Id id);
constexpr int maxTracks = 64;
constexpr double defaultRate = 48000.0;
enum class Waveform { Sine, Saw, Square, Triangle };
enum class InstrumentKind { Synth, Drums, Sampler };
struct Preset {
    std::string name = "Basic Synth";
    InstrumentKind kind = InstrumentKind::Synth;
    Waveform wave = Waveform::Sine;
    float attack = .008f, decay = .2f, sustain = .65f, release = .25f;
    float cutoff = 8000.f, gain = .3f, color = 0.f;
    int rootNote = 60;
    std::string samplePath, source, license;
};
std::vector<Preset> factoryPresets();
struct Sample {
    std::string path;
    std::uint32_t sampleRate = 48000;
    std::vector<float> stereo;
    // Min/max pairs, one pair per 256 source frames. Built once on load.
    std::vector<std::pair<float, float>> peaks;
    std::size_t frames() const {
        return stereo.size() / 2;
    }
};
struct Note {
    Id id = nextId();
    double beat = 0., duration = 1.;
    int key = 60;
    float velocity = .8f;
};
enum class TrackKind { Midi, Audio };
struct Clip {
    Id id = nextId();
    std::string name = "MIDI clip";
    double beat = 0., length = 4.,
           sourceOffset = 0.; // Offset is seconds, destructive edits never touch source.
    std::vector<Note> notes;
    std::shared_ptr<const Sample> sample;
    std::string mediaPath;
};
struct Track {
    Id id = nextId();
    std::string name = "Instrument";
    TrackKind kind = TrackKind::Midi;
    std::uint32_t color = 0xff8d91ed;
    bool mute = false, solo = false;
    float volume = .8f, pan = 0.;
    Preset preset;
    std::shared_ptr<const Sample> instrumentSample;
    std::vector<Clip> clips;
};
struct Project {
    int formatVersion = 1;
    std::string name = "Untitled";
    double bpm = 120.;
    int numerator = 4, denominator = 4;
    float master = .8f;
    std::vector<Track> tracks;
    double endBeat() const;
    double tailSeconds() const;
};
struct TempoMap {
    double bpm = 120., sampleRate = defaultRate;
    std::int64_t sampleAt(double beat) const {
        return std::llround(beat * 60. * sampleRate / bpm);
    }
    double beatAt(std::int64_t sample) const {
        return double(sample) * bpm / (60. * sampleRate);
    }
};
void validate(const Project&);
Project demoProject();
Track makeTrack(TrackKind kind, int index);
Clip* findClip(Project&, Id);
Track* findTrack(Project&, Id);
} // namespace reverie
