// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Engine.h"
#include "core/Media.h"
#include "core/Recorder.h"
#include "core/Serializer.h"
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <thread>
static thread_local bool checkingRealtime = false;
static thread_local int realtimeAllocations = 0, realtimeDeletions = 0;
void* operator new(std::size_t size) {
    if (checkingRealtime)
        ++realtimeAllocations;
    if (auto* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    return ::operator new(size);
}
void operator delete(void* p) noexcept {
    if (checkingRealtime && p)
        ++realtimeDeletions;
    std::free(p);
}
void operator delete[](void* p) noexcept {
    ::operator delete(p);
}

using namespace reverie;
static int assertions = 0;
static void check(bool pass, const char* message) {
    ++assertions;
    if (!pass)
        throw std::runtime_error(message);
}
static float energy(const std::vector<float>& b) {
    float v = 0;
    for (float x : b) {
        check(std::isfinite(x), "non-finite DSP output");
        v += x * x;
    }
    return v;
}
static std::vector<float> render(Project p, int count = 48000, double rate = 48000) {
    AudioEngine e(rate);
    e.publish(p);
    e.setPlaying(true);
    std::vector<float> b(count * 2);
    for (int off = 0; off < count; off += 128)
        e.process(b.data() + off * 2, std::min(128, count - off));
    return b;
}
int main() {
    try {
        auto dir = std::filesystem::temp_directory_path() /
                   ("reverie-tests-" +
                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                    "-" + std::to_string(nextId()));
        std::filesystem::create_directories(dir);
        Project p;
        p.name = "Round trip";
        auto t = makeTrack(TrackKind::Midi, 0);
        t.preset = factoryPresets()[7];
        Clip c;
        c.length = 4;
        Note n;
        n.beat = .25;
        n.duration = .25;
        n.key = 69;
        c.notes.push_back(n);
        t.clips.push_back(c);
        p.tracks.push_back(t);
        TempoMap tempo{123.5, 44100};
        check(std::abs(tempo.beatAt(tempo.sampleAt(913.125)) - 913.125) < .0001,
              "tempo round trip");
        auto data = render(p);
        check(energy(data) > 1, "synth silent");
        for (int i = 0; i < 6000 * 2; ++i)
            check(data[i] == 0, "MIDI started before exact sample");
        check(std::abs(data[12002]) > 0, "MIDI did not start at correct sample");
        float tail = 0;
        for (int i = 30000 * 2; i < 48000 * 2; ++i)
            tail += std::abs(data[i]);
        check(tail == 0, "note off release failed");
        auto muted = p;
        muted.tracks[0].mute = true;
        check(energy(render(muted)) == 0, "mute failed");
        auto pan = p;
        pan.tracks[0].pan = 1;
        auto stereo = render(pan);
        float left = 0, right = 0;
        for (int i = 0; i < 48000; ++i) {
            left += std::abs(stereo[i * 2]);
            right += std::abs(stereo[i * 2 + 1]);
        }
        check(left == 0 && right > 0, "pan failed");
        auto two = p;
        auto second = t;
        second.id = nextId();
        for (auto& clip : second.clips) {
            clip.id = nextId();
            for (auto& note : clip.notes)
                note.id = nextId();
        }
        two.tracks.push_back(second);
        check(energy(render(two)) > energy(data) * 3.9f, "multi-track mixing failed");
        two.tracks[0].solo = true;
        auto solo = render(two);
        check(energy(solo) == energy(data), "solo failed");
        auto quiet = p;
        quiet.tracks[0].volume *= .5;
        check(std::abs(energy(render(quiet)) / energy(data) - .25) < .001, "volume failed");
        quiet = p;
        quiet.master = 0;
        check(energy(render(quiet)) == 0, "master failed");
        // Seek into an existing sustained note, then change mixer state without dropping it.
        AudioEngine seeking;
        seeking.publish(p);
        seeking.seek(8000);
        seeking.setPlaying(true);
        std::array<float, 512> rt{};
        checkingRealtime = true;
        seeking.process(rt.data(), 256);
        checkingRealtime = false;
        float seekEnergy = 0;
        for (float v : rt)
            seekEnergy += v * v;
        check(seekEnergy > 0, "seek lost held notes");
        auto live = p;
        live.tracks[0].volume = .5;
        seeking.publish(live);
        checkingRealtime = true;
        seeking.process(rt.data(), 256);
        seeking.seek(6100);
        seeking.process(rt.data(), 256);
        checkingRealtime = false;
        check(realtimeAllocations == 0 && realtimeDeletions == 0,
              "audio callback allocated or destroyed memory");
        // Publishing an unchanged schedule must preserve oscillator/envelope phase exactly.
        AudioEngine baseline, updated;
        baseline.publish(p);
        updated.publish(p);
        baseline.setPlaying(true);
        updated.setPlaying(true);
        std::array<float, 512> aBlock{}, bBlock{};
        for (int i = 0; i < 30; ++i) {
            baseline.process(aBlock.data(), 256);
            updated.process(bBlock.data(), 256);
        }
        updated.publish(p);
        baseline.process(aBlock.data(), 256);
        updated.process(bBlock.data(), 256);
        check(aBlock == bBlock, "snapshot replacement reset DSP phase");
        ProjectSerializer::save(p, dir / "test.reverie");
        auto loaded = ProjectSerializer::load(dir / "test.reverie");
        check(loaded.project.tracks[0].clips[0].notes[0].key == 69, "note round trip");
        check(loaded.project.name == p.name, "name round trip");
        check(render(loaded.project) == data, "project render round trip");
        exportWav(p, dir / "mix.wav");
        auto sample = loadWav(dir / "mix.wav");
        check(sample->sampleRate == 48000 && sample->frames() == 192000,
              "WAV export duration/header");
        check(!sample->peaks.empty(), "wave cache missing");
        auto audio = makeTrack(TrackKind::Audio, 1);
        Clip a;
        a.name = "Imported";
        a.sample = sample;
        a.mediaPath = sample->path;
        a.length = 8;
        audio.clips.push_back(a);
        Project ap;
        ap.tracks.push_back(audio);
        check(energy(render(ap)) > 1, "WAV playback failed");
        ProjectSerializer::save(ap, dir / "audio.reverie");
        auto al = ProjectSerializer::load(dir / "audio.reverie");
        check(al.warnings.empty() && al.project.tracks[0].clips[0].sample, "media round trip");
        auto rebound = ProjectSerializer::save(ap, dir / "durable.reverie");
        auto original = dir / "mix.wav";
        auto movedOriginal = dir / "moved-original.wav";
        std::filesystem::rename(original, movedOriginal);
        ProjectSerializer::save(rebound, dir / "durable.reverie");
        check(ProjectSerializer::load(dir / "durable.reverie").warnings.empty(),
              "resave lost collected media after original moved");
        std::filesystem::rename(movedOriginal, original);
        const auto mediaPath = al.project.tracks[0].clips[0].sample->path;
        std::filesystem::remove(mediaPath);
        auto missing = ProjectSerializer::load(dir / "audio.reverie");
        check(!missing.warnings.empty() && !missing.project.tracks[0].clips[0].sample,
              "missing media not handled");
        check(energy(render(missing.project)) == 0, "missing audio should be silent");
        for (const auto& preset : factoryPresets()) {
            auto synth = p;
            synth.tracks[0].preset = preset;
            check(energy(render(synth, 48000, 44100)) > 0, "preset silent at 44.1 kHz");
        }
        auto poly = p;
        poly.tracks[0].clips[0].notes.clear();
        for (int i = 0; i < 20; ++i) {
            Note v;
            v.key = 48 + i;
            v.duration = .4;
            poly.tracks[0].clips[0].notes.push_back(v);
        }
        check(energy(render(poly)) > 1, "polyphony failed");
        for (int key : {36, 38, 42, 46}) {
            auto drum = p;
            drum.tracks[0].preset = factoryPresets()[8];
            drum.tracks[0].clips[0].notes[0].key = key;
            check(energy(render(drum)) > 0, "drum key silent");
        }
        auto oneFrame = std::make_shared<Sample>();
        oneFrame->stereo = {.5f, .25f};
        Project edge;
        auto edgeTrack = makeTrack(TrackKind::Audio, 0);
        Clip edgeClip;
        edgeClip.sample = oneFrame;
        edgeTrack.clips.push_back(edgeClip);
        edge.tracks.push_back(edgeTrack);
        auto edgeAudio = render(edge, 128);
        check(edgeAudio[0] > 0 && edgeAudio[1] > 0, "final sample frame lost");
        auto sampler = p;
        sampler.tracks[0].preset.kind = InstrumentKind::Sampler;
        sampler.tracks[0].instrumentSample = sample;
        check(energy(render(sampler)) > 0, "pitched sampler failed");
        auto preset = factoryPresets()[1];
        ProjectSerializer::savePreset(preset, dir / "pad.json");
        check(ProjectSerializer::loadPreset(dir / "pad.json").attack == preset.attack,
              "preset round trip");
        AudioRecorder recorder;
        recorder.start(dir / "capture.wav", 48000);
        std::array<float, 512> captured{};
        captured.fill(.15f);
        for (int i = 0; i < 20; ++i)
            recorder.capture(captured.data(), 256);
        recorder.stop();
        check(!recorder.failed() && !recorder.overflowed() && recorder.framesWritten() == 5120,
              "record worker lost frames");
        check(loadWav(dir / "capture.wav")->frames() == 5120, "capture WAV header");
        // One UI producer publishes/collects while one audio consumer renders.
        AudioEngine concurrent;
        std::atomic<bool> done = false;
        std::thread audioThread([&] {
            std::array<float, 256> b{};
            while (!done)
                concurrent.process(b.data(), 128);
        });
        for (int i = 0; i < 600; ++i) {
            concurrent.publish(i % 3 ? two : p);
            concurrent.seek(i % 100 * 128);
            concurrent.setPlaying(i % 5 != 0);
            concurrent.collect();
        }
        done = true;
        audioThread.join();
        concurrent.collect();
        bool rejected = false;
        auto invalid = p;
        invalid.bpm = 0;
        try {
            validate(invalid);
        } catch (...) {
            rejected = true;
        }
        check(rejected, "invalid tempo accepted");
        std::ofstream(dir / "bad.reverie") << "{broken";
        rejected = false;
        try {
            ProjectSerializer::load(dir / "bad.reverie");
        } catch (...) {
            rejected = true;
        }
        check(rejected, "corrupt project accepted");
        // Failed save must preserve the existing document.
        ProjectSerializer::save(p, dir / "safe.reverie");
        try {
            ProjectSerializer::save(invalid, dir / "safe.reverie");
        } catch (...) {
        }
        check(ProjectSerializer::load(dir / "safe.reverie").project.bpm == 120,
              "failed save damaged old project");
        AudioRecorder overflowRecorder;
        overflowRecorder.start(dir / "overflow.wav", 48000);
        overflowRecorder.capture(captured.data(), 262145);
        overflowRecorder.stop();
        check(overflowRecorder.overflowed(), "record overflow undetected");
        auto sustain = poly;
        sustain.tracks[0].preset.release = 3.5;
        check(sustain.tailSeconds() >= 3.5, "export release tail truncated");
        rejected = false;
        try {
            AudioEngine invalidRate(0);
        } catch (...) {
            rejected = true;
        }
        check(rejected, "invalid sample rate accepted");
        std::filesystem::remove_all(dir);
        std::cout << "PASS: " << assertions
                  << " assertions; timing, DSP, mixer, serialization, media, recorder, concurrent "
                     "edits\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
