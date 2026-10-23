// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Model.h"
#include <atomic>
#include <stdexcept>
#include <unordered_set>
namespace reverie {
static std::atomic<Id> counter{1};
Id nextId() {
    return counter.fetch_add(1, std::memory_order_relaxed);
}
void reserveId(Id id) {
    auto old = counter.load();
    while (old <= id && !counter.compare_exchange_weak(old, id + 1)) {
    }
}
std::vector<Preset> factoryPresets() {
    std::vector<Preset> p(9);
    p[0] = {"Simple Piano-like",
            InstrumentKind::Synth,
            Waveform::Triangle,
            .003f,
            .65f,
            .15f,
            .3f,
            5500,
            .38f,
            .35f,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[1] = {"Soft Pad",
            InstrumentKind::Synth,
            Waveform::Triangle,
            .45f,
            .4f,
            .7f,
            1.1f,
            1800,
            .28f,
            .15f,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[2] = {"Synth Lead",
            InstrumentKind::Synth,
            Waveform::Saw,
            .01f,
            .15f,
            .65f,
            .18f,
            4200,
            .23f,
            .1f,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[3] = {"Synth Bass",
            InstrumentKind::Synth,
            Waveform::Square,
            .004f,
            .18f,
            .45f,
            .12f,
            950,
            .27f,
            0,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[4] = {"Pluck",
            InstrumentKind::Synth,
            Waveform::Saw,
            .002f,
            .18f,
            .05f,
            .15f,
            3300,
            .32f,
            0,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[5] = {"Organ-like",
            InstrumentKind::Synth,
            Waveform::Sine,
            .006f,
            .1f,
            .85f,
            .1f,
            9000,
            .24f,
            .6f,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[6] = {"Bell",
            InstrumentKind::Synth,
            Waveform::Sine,
            .001f,
            1.1f,
            .05f,
            .7f,
            10000,
            .33f,
            .85f,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[7] = {"Basic Synth",
            InstrumentKind::Synth,
            Waveform::Sine,
            .008f,
            .2f,
            .65f,
            .25f,
            8000,
            .3f,
            0,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    p[8] = {"Studio Drums (synthetic)",
            InstrumentKind::Drums,
            Waveform::Sine,
            .001f,
            .14f,
            0,
            .08f,
            12000,
            .55f,
            0,
            60,
            "",
            "",
            "",
            "",
            "",
            "",
            {}};
    return p;
}
double Project::endBeat() const {
    double end = 4.;
    for (const auto& t : tracks)
        for (const auto& c : t.clips)
            end = std::max(end, c.beat + c.length);
    return end;
}
double Project::tailSeconds() const {
    double tail = 2.;
    for (const auto& t : tracks)
        if (t.kind == TrackKind::Midi)
            tail = std::max(tail, double(t.preset.release) + .05);
    for (const auto& t : tracks)
        if (t.effects.enabled) {
            if (t.effects.reverbMix > 0)
                tail = std::max(tail, 6.);
            if (t.effects.delayMix > 0) {
                double repeats =
                    t.effects.feedback > 0
                        ? std::ceil(std::log(.0001) / std::log(double(t.effects.feedback)))
                        : 1.;
                tail = std::max(tail, double(t.effects.delaySeconds) * (repeats + 1));
            }
        }
    return tail;
}
Track makeTrack(TrackKind kind, int index) {
    Track t;
    t.kind = kind;
    t.name = (kind == TrackKind::Midi ? "Instrument " : "Audio ") + std::to_string(index + 1);
    static constexpr std::uint32_t colors[] = {0xffb18c70, 0xff7ea89b, 0xffbbad7b, 0xff849da5,
                                               0xffb78f85};
    t.color = colors[index % 5];
    t.preset = factoryPresets()[0];
    return t;
}
static bool in(double x, double lo, double hi) {
    return std::isfinite(x) && x >= lo && x <= hi;
}
void validate(const Project& p) {
    if ((p.formatVersion != 1 && p.formatVersion != 2) || !in(p.bpm, 20, 300) || p.numerator < 1 ||
        p.numerator > 16 ||
        (p.denominator != 2 && p.denominator != 4 && p.denominator != 8 && p.denominator != 16) ||
        !in(p.master, 0, 2) || p.tracks.size() > maxTracks)
        throw std::runtime_error("Unsupported project version or invalid transport/track count");
    if (!in(p.loopStart, 0, 100000) || !in(p.loopEnd, p.loopStart + 1. / 128, 200000) ||
        !in(p.clickLevel, 0, .5))
        throw std::runtime_error("Invalid loop/metronome range");
    std::unordered_set<Id> ids;
    auto unique = [&](Id id) {
        if (!id || id > 1000000000000ULL || !ids.insert(id).second)
            throw std::runtime_error("Duplicate/invalid object identifier");
    };
    std::size_t totalNotes = 0, totalClips = 0;
    for (const auto& t : p.tracks) {
        unique(t.id);
        const auto& s = t.preset;
        if (!in(t.volume, 0, 2) || !in(t.pan, -1, 1) || !in(s.attack, .00009, 10) ||
            !in(s.decay, .00009, 10) || !in(s.sustain, 0, 1) || !in(s.release, .0009, 10) ||
            !in(s.gain, 0, 2) || !in(s.cutoff, 20, 24000) || !in(s.color, 0, 1) || s.rootNote < 0 ||
            s.rootNote > 127 || int(s.kind) < 0 || int(s.kind) > 2 || int(s.wave) < 0 ||
            int(s.wave) > 3)
            throw std::runtime_error("Invalid mixer or instrument settings");
        const auto& fx = t.effects;
        if (!in(fx.lowDb, -18, 18) || !in(fx.highDb, -18, 18) || !in(fx.delayMix, 0, 1) ||
            !in(fx.delaySeconds, .0099, 2) || !in(fx.feedback, 0, .8501) || !in(fx.reverbMix, 0, 1))
            throw std::runtime_error("Invalid effect parameters");
        if (s.zones.size() > 4096)
            throw std::runtime_error("Too many sampler zones");
        for (const auto& z : s.zones)
            if (z.lowKey < 0 || z.highKey > 127 || z.lowKey > z.highKey || z.rootKey < 0 ||
                z.rootKey > 127 || z.lowVelocity < 0 || z.highVelocity > 127 ||
                z.lowVelocity > z.highVelocity || !in(z.gain, 0, 8) ||
                (z.end && z.end <= z.start) || (z.loop && z.loopEnd <= z.loopStart) ||
                (z.loop && z.oneShot))
                throw std::runtime_error("Invalid sample mapping");
        totalClips += t.clips.size();
        for (const auto& c : t.clips) {
            unique(c.id);
            if (!in(c.beat, 0, 100000) || !in(c.length, 1. / 1024, 100000) ||
                !in(c.sourceOffset, 0, 100000))
                throw std::runtime_error("Invalid clip range");
            totalNotes += c.notes.size();
            for (const auto& n : c.notes) {
                unique(n.id);
                if (!in(n.beat, 0, 100000) || !in(n.duration, 1. / 4096, 100000) || n.key < 0 ||
                    n.key > 127 || !in(n.velocity, 0, 1))
                    throw std::runtime_error("Invalid MIDI note");
            }
        }
    }
    if (totalNotes > 200000 || totalClips > 20000)
        throw std::runtime_error("Project exceeds 0.1 editing limits");
}
Project demoProject() {
    Project p;
    p.name = "First light";
    auto lead = makeTrack(TrackKind::Midi, 0);
    lead.name = "01  Glass keys";
    Clip c;
    c.name = "First light";
    c.length = 16.;
    int melody[] = {60, 64, 67, 71, 69, 67, 64, 62, 60, 64, 67, 72, 71, 67, 64, 62};
    for (int i = 0; i < 16; ++i) {
        Note n;
        n.beat = i;
        n.duration = .75;
        n.key = melody[i];
        c.notes.push_back(n);
    }
    lead.clips.push_back(c);
    p.tracks.push_back(lead);
    auto pad = makeTrack(TrackKind::Midi, 1);
    pad.name = "02  Slow bloom";
    pad.preset = factoryPresets()[1];
    pad.volume = .55;
    Clip chords;
    chords.name = "Slow bloom";
    chords.length = 16;
    for (int bar = 0; bar < 4; ++bar)
        for (int key : {48, 55, 64}) {
            Note n;
            n.beat = bar * 4;
            n.duration = 3.5;
            n.key = key + (bar == 2 ? 5 : 0);
            n.velocity = .55;
            chords.notes.push_back(n);
        }
    pad.clips.push_back(chords);
    p.tracks.push_back(pad);
    return p;
}
Clip* findClip(Project& p, Id id) {
    for (auto& t : p.tracks)
        for (auto& c : t.clips)
            if (c.id == id)
                return &c;
    return nullptr;
}
Track* findTrack(Project& p, Id id) {
    for (auto& t : p.tracks)
        if (t.id == id)
            return &t;
    return nullptr;
}
} // namespace reverie
