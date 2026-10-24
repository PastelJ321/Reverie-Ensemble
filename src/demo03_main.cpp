// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Export.h"
#include "core/Library.h"
#include "core/Media.h"
#include "core/Serializer.h"
#include <fstream>
#include <iostream>
using namespace reverie;
int main(int argc, char** argv) try {
    if (argc == 4 && std::string(argv[1]) == "--reopen") {
        auto loaded = ProjectSerializer::load(argv[2]);
        if (!loaded.warnings.empty())
            throw std::runtime_error("Missing demo media");
        ExportOptions o;
        o.sampleRate = 44100;
        o.depth = WavDepth::Pcm24;
        auto r = renderAudio(loaded.project, argv[3], o);
        std::cout << r.peak << " " << r.rms << " " << r.clippedSamples << '\n';
        return 0;
    }
    if (argc != 3)
        throw std::runtime_error("Usage: reverie-demo03 assets/sounds output-folder; or --reopen "
                                 "project.reverie output.wav");
    auto folder = std::filesystem::absolute(argv[2]);
    std::filesystem::create_directories(folder);
    SampleCache cache;
    auto pack = readSoundPack(std::filesystem::path(argv[1]) / "essentials03");
    Project p;
    p.name = "Ensemble 0.3 — Demo Composition";
    p.bpm = 108;
    p.master = .65f;
    p.loopStart = 0;
    p.loopEnd = 16;
    auto add = [&](int preset, std::string name, float volume) {
        auto t = makeTrack(TrackKind::Midi, int(p.tracks.size()));
        t.preset = loadLibraryPreset(pack.instruments.at(preset), pack, cache);
        t.name = name;
        t.volume = volume;
        t.clips.clear();
        p.tracks.push_back(t);
    };
    add(2, "Concert Grand Piano", .5f);
    add(0, "Electric Bass", .4f);
    add(1, "Hollowbody Guitar", .3f);
    add(3, "Acoustic Drum Kit", .55f);
    auto pad = makeTrack(TrackKind::Midi, 4);
    pad.preset =
        calibratedFactoryPresets(std::filesystem::path(argv[1]) / "synth-calibration.json")[1];
    pad.name = "Soft Pad";
    pad.volume = .2f;
    p.tracks.push_back(pad);
    auto note = [](Clip& c, double beat, int key, double length, float velocity) {
        Note n;
        n.beat = beat;
        n.key = key;
        n.duration = length;
        n.velocity = velocity;
        c.notes.push_back(n);
    };
    for (int track = 0; track < 5; ++track) {
        Clip c;
        c.length = 16;
        c.name = p.tracks[track].name;
        for (int bar = 0; bar < 4; ++bar) {
            int root = std::vector<int>{60, 57, 65, 67}[bar];
            if (track == 0)
                for (int step = 0; step < 8; ++step)
                    note(c, bar * 4 + step * .5,
                         root + std::vector<int>{0, 4, 7, 12, 7, 4, 7, 4}[step], .4,
                         .58f + (step % 2) * .12f);
            if (track == 1)
                for (int step = 0; step < 4; ++step)
                    note(c, bar * 4 + step, root - 24, .75, .7);
            if (track == 2)
                for (int beat : {0, 2})
                    for (int interval : {0, 4, 7})
                        note(c, bar * 4 + beat + interval * .006, root - 12 + interval, 1.5, .65);
            if (track == 3) {
                for (int step = 0; step < 8; ++step)
                    note(c, bar * 4 + step * .5, 42, .12, step % 2 ? .5 : .65);
                for (int beat : {0, 2})
                    note(c, bar * 4 + beat, 36, .2, .8);
                for (int beat : {1, 3})
                    note(c, bar * 4 + beat, 38, .2, .75);
            }
            if (track == 4)
                for (int interval : {0, 4, 7})
                    note(c, bar * 4, root + interval, 3.8, .6);
        }
        p.tracks[track].clips = {c};
    }
    p.tracks[0].effects.enabled = true;
    p.tracks[0].effects.reverbMix = .12f;
    p.tracks[2].effects.enabled = true;
    p.tracks[2].effects.delayMix = .08f;
    p.tracks[2].pan = -.2f;
    p.tracks[4].pan = .25f;
    Project stem = p;
    stem.tracks = {p.tracks[2]};
    stem.tracks[0].effects.enabled = false;
    ExportOptions stemOptions;
    stemOptions.endBeat = 4;
    stemOptions.includeTail = false;
    auto audioPath = folder / "Guitar Bounce.wav";
    renderAudio(stem, audioPath, stemOptions);
    auto audio = makeTrack(TrackKind::Audio, 5);
    audio.name = "Audio · Guitar Bounce";
    audio.volume = .2f;
    Clip clip;
    clip.name = "Non-destructive WAV bounce";
    clip.beat = 8;
    clip.length = 4;
    clip.mediaPath = audioPath.string();
    clip.sample = loadWav(audioPath);
    audio.clips = {clip};
    p.tracks.push_back(audio);
    auto path = folder / "Demo Composition.reverie";
    ProjectSerializer::save(p, path);
    auto loaded = ProjectSerializer::load(path);
    if (!loaded.warnings.empty() || loaded.project.tracks.size() != 6)
        throw std::runtime_error("Demo restore failed");
    ExportOptions options;
    options.sampleRate = 44100;
    options.depth = WavDepth::Pcm24;
    auto result = renderAudio(loaded.project, folder / "Demo Composition.wav", options);
    options.sampleRate = 96000;
    options.depth = WavDepth::Float32;
    renderAudio(loaded.project, folder / "Demo Composition-96k-float.wav", options);
    std::ofstream report(folder / "demo-metrics.txt");
    report << "6 tracks; 108 BPM; 16 beats; 44100 Hz / 24-bit stereo\nFrames: " << result.frames
           << "\nPeak: " << 20 * std::log10(std::max(1e-9, result.peak))
           << " dBFS\nRMS: " << 20 * std::log10(std::max(1e-9, result.rms))
           << " dBFS\nOver-range samples: " << result.clippedSamples
           << "\nAudio track is a rendered guitar bounce, not a microphone recording.\n";
    report.close();
    std::ifstream reportRead(folder / "demo-metrics.txt");
    std::cout << reportRead.rdbuf();
    if (result.clippedSamples)
        throw std::runtime_error("Demo clips");
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
