// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Engine.h"
#include "core/Export.h"
#include "core/Library.h"
#include "core/Media.h"
#include "core/MidiRecording.h"
#include "core/Serializer.h"
#include <chrono>
#include <fstream>
#include <iostream>
using namespace reverie;
static void check(bool x, const char* msg) {
    if (!x)
        throw std::runtime_error(msg);
}
static float peak(const float* f, std::size_t n) {
    float p = 0;
    for (std::size_t i = 0; i < n; ++i) {
        check(std::isfinite(f[i]), "nonfinite audio");
        p = std::max(p, std::abs(f[i]));
    }
    return p;
}
static int crossings(InstrumentBackend& backend) {
    std::array<float, 512> audio{};
    int count = 0;
    float last = 0;
    for (int b = 0; b < 64; ++b) {
        backend.render(audio.data(), 256);
        for (int i = 0; i < 256; ++i) {
            float sample = audio[i * 2];
            if (last < 0 && sample >= 0)
                ++count;
            last = sample;
        }
    }
    return count;
}
static void checkBend(InstrumentBackend& backend) {
    // A faster-than-real-time measurement must use the backend offline streaming mode.
    backend.setOffline(true);
    backend.reset();
    backend.noteOn(9001, 60, .8f, 0);
    int natural = crossings(backend);
    backend.pitchBend(0, 16383);
    int bent = crossings(backend);
    check(natural > 30 && bent > natural * 1.05 && bent < natural * 1.3,
          "Real pitch bend changes rendered frequency");
    backend.controller(0, 1, 90);
    backend.reset();
}
int main() try {
    auto root = std::filesystem::path(REVERIE_TEST_ROOT);
    auto temp = root / "out/tests04" /
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    std::filesystem::create_directories(temp);
    check(sfizzAvailable(), "sfizz dependency missing in integration build");
    check(fluidSynthAvailable(), "FluidSynth dependency missing in integration build");
    for (auto ext : {"sf2", "sf3"}) {
        auto file = root / "tests/fixtures" / (std::string("tone.") + ext);
        auto programs = soundFontPrograms(file);
        check(programs.size() == 2, "SoundFont bank inventory");
        auto p = externalPreset(file, InstrumentKind::SoundFont, 5, 7);
        auto backend = prepareBackend(p, 48000);
        std::array<float, 512> audio{};
        backend->noteOn(1, 60, .8, 0);
        backend->render(audio.data(), 256);
        check(peak(audio.data(), 512) > 0, "SoundFont actual rendering");
        backend->controller(0, 64, 127);
        backend->noteOff(1);
        for (int i = 0; i < 30; ++i)
            backend->render(audio.data(), 256);
        check(peak(audio.data(), 512) > 0, "SoundFont sustain");
        backend->controller(0, 64, 0);
        for (int i = 0; i < 400; ++i)
            backend->render(audio.data(), 256);
        check(peak(audio.data(), 512) < 1e-5, "SoundFont note release");
        backend->reset();
        backend->render(audio.data(), 256);
        check(peak(audio.data(), 512) < 1e-5, "SoundFont panic");
        checkBend(*backend);
        Project project;
        auto t = makeTrack(TrackKind::Midi, 0);
        t.preset = p;
        Clip c;
        c.length = 2;
        c.notes.push_back(Note{});
        c.controllers.push_back({.5, 0, 1, 80});
        t.clips.push_back(c);
        project.tracks.push_back(t);
        ProjectSerializer::save(project, temp / (std::string(ext) + ".reverie"));
        auto loaded = ProjectSerializer::load(temp / (std::string(ext) + ".reverie"));
        check(loaded.project.tracks[0].preset.soundFontBank == 5, "Bank restore");
        check(loaded.project.tracks[0].preset.soundFontProgram == 7, "Program restore");
        ExportOptions o;
        o.endBeat = 2;
        auto result = renderAudio(loaded.project, temp / (std::string(ext) + ".wav"), o);
        check(result.peak > 0, "SoundFont offline render");
    }
    // Multi-layer SFZ uses opcodes beyond the legacy sampler: tune, per-region envelope, release
    // trigger.
    std::vector<float> pcm(48000 * 2);
    for (int i = 0; i < 48000; ++i)
        pcm[i * 2] = pcm[i * 2 + 1] = .25f * std::sin(i * .035);
    writeWav16(temp / "sample.wav", pcm.data(), 48000, 48000);
    std::ofstream(temp / "test.sfz")
        << "<group> ampeg_release=0.05\n<region> sample=sample.wav lokey=21 hikey=108 "
           "pitch_keycenter=60 tune=3 lovel=0 hivel=80\n<region> sample=sample.wav lokey=21 "
           "hikey=108 pitch_keycenter=60 lovel=81 hivel=127 ampeg_attack=0.02\n<group> "
           "trigger=release\n<region> sample=sample.wav key=60 ampeg_release=0.02\n";
    auto sfz = externalPreset(temp / "test.sfz", InstrumentKind::Sfizz);
    std::ofstream(temp / "long-release.inc") << "<group> ampeg_release=4\n";
    std::ofstream(temp / "long-release.sfz")
        << "#include \"long-release.inc\"\n<region> sample=sample.wav key=60 "
           "loop_mode=loop_continuous loop_start=0 loop_end=47999\n";
    Project ringing;
    auto ringingTrack = makeTrack(TrackKind::Midi, 0);
    ringingTrack.preset = externalPreset(temp / "long-release.sfz", InstrumentKind::Sfizz);
    Clip ringingClip;
    ringingClip.length = 2;
    ringingClip.notes.push_back(Note{});
    ringingTrack.clips.push_back(ringingClip);
    ringing.tracks.push_back(ringingTrack);
    check(ringing.tailSeconds() > 4, "Included SFZ release expands export tail");
    ProjectSerializer::save(ringing, temp / "ringing.reverie");
    ringing = ProjectSerializer::load(temp / "ringing.reverie").project;
    check(ringing.tailSeconds() > 4, "External release hint survives collect and reopen");
    ExportOptions ringingOptions;
    ringingOptions.endBeat = 2;
    renderAudio(ringing, temp / "ringing.wav", ringingOptions);
    auto ringingAudio = loadWav(temp / "ringing.wav");
    check(ringingAudio->frames() > 48000 * 5 &&
              peak(ringingAudio->stereo.data() + 48000 * 3 * 2, 512) > 1e-5,
          "Long-release SFZ exports audible sound beyond former two-second tail");
    auto backend = prepareBackend(sfz, 48000);
    std::array<float, 512> audio{};
    checkBend(*backend);
    for (int k = 48; k < 80; ++k)
        backend->noteOn(k, k, .7f, 0);
    backend->render(audio.data(), 256);
    check(peak(audio.data(), 512) > 0, "sfizz polyphony");
    backend->controller(0, 64, 127);
    for (int k = 48; k < 80; ++k)
        backend->noteOff(k);
    backend->render(audio.data(), 256);
    backend->controller(0, 64, 0);
    backend->reset();
    backend->render(audio.data(), 256);
    check(peak(audio.data(), 512) < 1e-5, "sfizz reset");
    MidiRecording capture;
    capture.start({120, 48000}, 48000);
    Event e{48000, 100, 60, .75f, true};
    e.channel = 2;
    capture.capture(e);
    e = {72000, 100, 60, 0, false};
    e.channel = 2;
    capture.capture(e);
    Event pedal{60000, 0, 0, 0, false};
    pedal.controller = 64;
    pedal.value = 127;
    capture.capture(pedal);
    auto clip = capture.finish(96000);
    check(clip.notes.size() == 1 && clip.notes[0].duration == 1 && clip.notes[0].channel == 2,
          "MIDI frame-to-beat/channel");
    check(clip.controllers.size() == 17, "CC64 and stop cleanup");
    capture.start({120, 48000}, 48000);
    capture.capture({24000, 101, 61, .8, true});
    capture.capture({48000, 102, 64, .8, true});
    capture.updatePosition(60000);
    check(capture.preview().notes[0].duration == .5, "Held note grows in live recording preview");
    clip = capture.finish(60000);
    check(clip.notes.size() == 1 && clip.notes[0].duration == .5,
          "Count-in filter and held note close");
    AudioEngine engine;
    auto t = makeTrack(TrackKind::Midi, 0);
    t.preset = factoryPresets()[7];
    t.preset.release = .01;
    Project p;
    p.tracks.push_back(t);
    engine.publish(p);
    engine.preparePerformance(t);
    engine.performanceEvent({0, 1001, 60, .8, true});
    engine.performanceEvent({0, 1002, 64, .8, true});
    engine.process(audio.data(), 256);
    check(peak(audio.data(), 512) > 0, "Live polyphony while stopped");
    engine.panic();
    engine.process(audio.data(), 256);
    check(peak(audio.data(), 512) == 0, "Live panic clears notes");
    engine.performanceEvent({0, 1003, 67, .8, true});
    engine.process(audio.data(), 256);
    check(peak(audio.data(), 512) > 0, "New notes after panic");
    for (int i = 0; i < 5000; ++i)
        engine.performanceEvent({0, Id(2000 + i), 60, .8, true});
    engine.process(audio.data(), 256);
    check(peak(audio.data(), 512) == 0, "Queue overflow fails silent");
    Project sustained;
    auto heldTrack = makeTrack(TrackKind::Midi, 0);
    heldTrack.preset = factoryPresets()[7];
    heldTrack.preset.sustain = 1;
    heldTrack.preset.release = .01;
    Clip pedalClip;
    pedalClip.length = 3;
    Note shortNote;
    shortNote.duration = .25;
    pedalClip.notes = {shortNote};
    pedalClip.controllers = {{0, 0, 64, 127}, {2, 0, 64, 0}};
    heldTrack.clips = {pedalClip};
    sustained.tracks = {heldTrack};
    AudioEngine seekPedal;
    seekPedal.publish(sustained);
    seekPedal.seek(24000);
    seekPedal.setPlaying(true);
    seekPedal.process(audio.data(), 256);
    check(peak(audio.data(), 512) > 0, "Seek restores pedal-held note");
    for (int i = 0; i < 220; ++i)
        seekPedal.process(audio.data(), 256);
    check(peak(audio.data(), 512) < 1e-5, "Pedal seek release is finite");
    auto unavailable = sfz;
    unavailable.backendPath = (temp / "missing.sfz").string();
    auto silent = prepareBackend(unavailable, 48000, true);
    silent->noteOn(1, 60, .8, 0);
    silent->render(audio.data(), 256);
    check(peak(audio.data(), 512) == 0,
          "Missing external instrument remains silent and recoverable");
    // v1-v3 continue loading; no schema conversion of their instrument backend.
    auto legacy = ProjectSerializer::load(root / "examples/First Light.reverie");
    (void)legacy;
    std::cout << "PASS: real sfizz/SF2/SF3 render, banks/programs, sustain, MIDI "
                 "timestamps/count-in/stop, live queue/polyphony/panic/overflow, project "
                 "restore/export\n";
} catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
}
