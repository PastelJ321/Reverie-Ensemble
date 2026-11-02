// SPDX-License-Identifier: Apache-2.0
#include "core/Effects.h"
#include "core/Engine.h"
#include "core/Serializer.h"
#include <iostream>
using namespace reverie;
void check(bool b, const char* m) {
    if (!b)
        throw std::runtime_error(m);
}
int main() try {
    Effects s;
    s.enabled = true;
    s.compressor = true;
    s.thresholdDb = -24;
    s.ratio = 8;
    s.attackMs = 1;
    EffectChain compressor(s, 48000);
    float result = 0;
    for (int i = 0; i < 48000; ++i)
        result = compressor.next(.8, .8).first;
    check(result < .15, "compressor gain reduction");
    s.compressor = false;
    s.limiter = true;
    EffectChain limiter(s, 48000);
    check(limiter.next(3, -3).first <= .892, "limiter ceiling");
    auto old = factoryPresets()[7];
    auto modern = old;
    modern.modern = true;
    modern.fm = 3;
    modern.mono = true;
    modern.legato = true;
    Synthesizer a(old, 48000), b(modern, 48000);
    a.noteOn(1, 60, .8);
    b.noteOn(1, 60, .8);
    double difference = 0;
    for (int i = 0; i < 16000; ++i) {
        auto x = a.next().first, y = b.next().first;
        check(std::isfinite(y), "finite modern synth");
        difference += std::abs(x - y);
    }
    check(difference > 10, "extended oscillator changes actual audio");
    b.noteOn(2, 67, .8);
    b.noteOff(2);
    b.noteOff(1);
    for (int i = 0; i < 48000; ++i)
        b.next();
    check(std::abs(b.next().first) < 1e-6, "mono priority/release has no stuck note");
    auto priority = modern;
    priority.glide = 0;
    priority.vibrato = 0;
    priority.fm = 0;
    priority.oscMix = 0;
    priority.sustain = 1;
    priority.attack = .001f;
    priority.wave = Waveform::Sine;
    Synthesizer mono(priority, 48000);
    mono.noteOn(1, 60, .8);
    mono.noteOn(2, 64, .8);
    mono.noteOff(1);
    mono.noteOn(3, 67, .8);
    mono.noteOn(4, 72, .8);
    mono.noteOff(4);
    int crosses = 0;
    float previous = 0;
    for (int i = 0; i < 4800; ++i) {
        float next = mono.next().first;
        if (previous < 0 && next >= 0)
            ++crosses;
        previous = next;
    }
    check(crosses >= 38 && crosses <= 40,
          "Mono last-note priority remains G4 after non-last key releases");
    Project p;
    auto t = makeTrack(TrackKind::Midi, 0);
    t.preset = modern;
    t.effects = s;
    Preset missing;
    missing.kind = InstrumentKind::Clap;
    missing.pluginId = "org.example.missing";
    missing.backendPath = "/missing/instrument.clap";
    t.inserts.push_back({missing, true, .5});
    p.tracks.push_back(t);
    auto path = std::filesystem::path(REVERIE_TEST_ROOT) / "out/production05.reverie";
    ProjectSerializer::save(p, path);
    auto loaded = ProjectSerializer::load(path).project;
    check(loaded.tracks[0].preset.fm == 3 && loaded.tracks[0].effects.limiter &&
              loaded.tracks[0].inserts.size() == 1 && loaded.tracks[0].inserts[0].bypass,
          "production state roundtrip");
    std::cout << "PASS modern synth, mono note priority, compressor, limiter, insert/parameter "
                 "persistence\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
