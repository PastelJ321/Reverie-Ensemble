// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Effects.h"
#include "core/Engine.h"
#include "core/Hash.h"
#include "core/Library.h"
#include "core/Media.h"
#include "core/MidiFile.h"
#include "core/Serializer.h"
#include <cstdlib>
#include <new>
static thread_local bool realtime = false;
static thread_local int rtAlloc = 0, rtFree = 0;
void* operator new(std::size_t n) {
    if (realtime)
        ++rtAlloc;
    if (auto* p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete(void* p) noexcept {
    if (realtime && p)
        ++rtFree;
    std::free(p);
}
void operator delete[](void* p) noexcept {
    ::operator delete(p);
}
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
using namespace reverie;
void require(bool pass, const char* msg) {
    if (!pass)
        throw std::runtime_error(msg);
}
float energy(const float* p, std::size_t n) {
    float sum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        require(std::isfinite(p[i]), "Non-finite output");
        sum += p[i] * p[i];
    }
    return sum;
}
int main() {
    try {
        auto root = std::filesystem::path(REVERIE_TEST_ROOT);
        auto temp = std::filesystem::temp_directory_path() /
                    ("reverie02-" +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(temp);
        std::ofstream(temp / "abc") << "abc";
        require(sha256File(temp / "abc") ==
                    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                "SHA-256 known vector");
        std::ofstream(temp / "empty");
        require(sha256File(temp / "empty") ==
                    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                "SHA-256 empty vector");
        auto pack = readSoundPack(root / "assets/sounds/factory");
        require(pack.instruments.size() >= 9, "Factory recording inventory missing");
        SampleCache cache;
        require(loadSfz(root / "assets/sounds/factory/flute.sfz", cache).zones[0].rootKey == 60,
                "Official flute root mapping octave regressed");
        for (const auto& item : pack.instruments) {
            auto preset = loadSfz(item.file, cache);
            require(preset.zones.size() >= 3, "Instrument is not multisampled");
            Project p;
            auto t = makeTrack(TrackKind::Midi, 0);
            t.preset = preset;
            t.preset.instrumentId = item.id;
            t.preset.source = pack.source;
            t.preset.license = pack.license;
            Clip c;
            Note n;
            n.key = item.category == "Drums & Percussion" ? 38 : 60;
            n.duration = .6;
            c.notes.push_back(n);
            t.clips.push_back(c);
            p.tracks.push_back(t);
            AudioEngine e;
            e.publish(p);
            e.setPlaying(true);
            std::array<float, 512> b{};
            float sum = 0;
            for (int i = 0; i < 150; ++i) {
                realtime = true;
                e.process(b.data(), 256);
                realtime = false;
                sum += energy(b.data(), b.size());
            }
            require(sum > .001, "Recorded instrument silent");
            auto file = temp / (item.id + ".reverie");
            auto saved = ProjectSerializer::save(p, file);
            auto loaded = ProjectSerializer::load(file);
            require(loaded.warnings.empty() &&
                        loaded.project.tracks[0].preset.zones.size() == preset.zones.size(),
                    "Multisample project round trip");
            require(loaded.project.tracks[0].preset.instrumentId == item.id, "Instrument ID lost");
            exportWav(loaded.project, temp / (item.id + ".wav"));
            require(loadWav(temp / (item.id + ".wav"))->frames() >= 48000,
                    "Recorded instrument export failed");
        }
        {
            auto patch = loadSfz(root / "assets/sounds/factory/flute.sfz", cache);
            Project p;
            auto t = makeTrack(TrackKind::Midi, 0);
            t.preset = patch;
            Clip c;
            for (int key : {60, 64, 67}) {
                Note n;
                n.key = key;
                n.duration = 1;
                c.notes.push_back(n);
            }
            t.clips.push_back(c);
            p.tracks = {t};
            AudioEngine engine(44100);
            engine.publish(p);
            engine.setPlaying(true);
            std::array<float, 512> out{};
            float sum = 0;
            for (int i = 0; i < 100; ++i) {
                realtime = true;
                engine.process(out.data(), 256);
                realtime = false;
                sum += energy(out.data(), out.size());
            }
            require(sum > 0, "Recorded sampler chords at 44.1 kHz failed");
        }
        Project click;
        click.metronome = true;
        click.numerator = 6;
        click.denominator = 8;
        AudioEngine met;
        met.publish(click);
        met.setPlaying(true);
        std::vector<float> clicks(26000 * 2);
        met.process(clicks.data(), 26000);
        require(energy(clicks.data(), 2000) > 0, "Metronome first beat missing");
        require(energy(clicks.data() + 12000 * 2, 2000) > 0, "Metronome denominator timing wrong");
        require(energy(clicks.data() + 6000 * 2, 2000) == 0, "Metronome clicked between beats");
        exportWav(click, temp / "click-disabled-export.wav");
        auto dry = loadWav(temp / "click-disabled-export.wav");
        require(energy(dry->stereo.data(), dry->stereo.size()) == 0,
                "Monitor metronome leaked into export");
        Project loop = demoProject();
        loop.loopEnabled = true;
        loop.loopStart = .25;
        loop.loopEnd = .75;
        AudioEngine looping;
        looping.publish(loop);
        looping.setPlaying(true);
        std::array<float, 1024> b{};
        for (int i = 0; i < 100; ++i) {
            realtime = true;
            looping.process(b.data(), 512);
            realtime = false;
        }
        require(looping.position() == 6000 + (51200 % 12000),
                "Loop wrap sample position inaccurate");
        require(looping.playing(), "Loop transport stopped");
        AudioEngine audition;
        audition.publish(loop);
        audition.audition(loop.tracks[0], 60, .7, .15);
        realtime = true;
        audition.process(b.data(), 512);
        realtime = false;
        require(energy(b.data(), b.size()) > 0 && audition.position() == 0,
                "Audition changed timeline or stayed silent");
        for (int i = 0; i < 30; ++i) {
            audition.audition(loop.tracks[0], 50 + i, .7, .03);
            audition.process(b.data(), 512);
        }
        for (int i = 0; i < 500; ++i)
            audition.process(b.data(), 512);
        require(energy(b.data(), b.size()) == 0, "Audition voices never stopped");
        Effects delay;
        delay.enabled = true;
        delay.delayMix = .5;
        delay.delaySeconds = .01;
        delay.feedback = 0;
        EffectChain fx(delay, 48000);
        std::pair<float, float> echo;
        for (int i = 0; i <= 480; ++i)
            echo = fx.next(i == 0 ? .2f : 0, i == 0 ? .2f : 0);
        require(std::abs(echo.first - .1) < .0001, "Delay timing/amplitude wrong");
        fx.reset();
        require(fx.next(0, 0).first == 0, "Effect reset failed");
        Effects reverb;
        reverb.enabled = true;
        reverb.reverbMix = .4;
        EffectChain room(reverb, 44100);
        float tail = 0;
        for (int i = 0; i < 44100; ++i) {
            auto v = room.next(i == 0 ? .2 : 0, 0);
            if (i > 1000)
                tail += std::abs(v.first);
        }
        require(tail > 0, "Reverb has no tail");
        Effects eq;
        eq.enabled = true;
        eq.lowDb = 6;
        EffectChain tone(eq, 48000);
        float dc = 0;
        for (int i = 0; i < 5000; ++i)
            dc = tone.next(.1, .1).first;
        require(dc > .19 && dc < .21, "EQ low band wrong");
        auto old = ProjectSerializer::load(root / "examples/First Light.reverie");
        require(old.project.tracks.size() == 2 && old.project.tracks[0].clips[0].notes.size() == 16,
                "0.1 migration lost notes");
        auto upgraded = old.project;
        upgraded.loopEnabled = true;
        upgraded.metronome = true;
        upgraded.tracks[0].effects = delay;
        ProjectSerializer::save(upgraded, temp / "upgrade.reverie");
        auto result = ProjectSerializer::load(temp / "upgrade.reverie");
        require(result.project.loopEnabled && result.project.metronome &&
                    result.project.tracks[0].effects.delayMix == .5f,
                "0.2 fields lost");
        std::ofstream(temp / "invalid.sfz") << "<region> sample=absent.wav key=60";
        bool rejected = false;
        try {
            loadSfz(temp / "invalid.sfz", cache);
        } catch (...) {
            rejected = true;
        }
        require(rejected, "Missing sample was not handled");
        std::filesystem::create_directory(temp / "badpack");
        std::ofstream(temp / "badpack/pack.json")
            << R"({"id":"evil.pack","version":"1","name":"Bad","author":"Test","source":"local","license":"CC0-1.0","installSize":3,"files":[{"path":"../../escape.wav","size":3,"sha256":"bad"}],"instruments":[]})";
        rejected = false;
        try {
            installSoundPack(temp / "badpack", temp / "installed");
        } catch (...) {
            rejected = true;
        }
        require(rejected && !std::filesystem::exists(temp / "installed/evil.pack"),
                "Pack traversal was not rejected");

        // Key up can arrive before the first callback: it must not leave a ten-second voice.
        audition.audition(loop.tracks[0], 60, .7, 10);
        audition.releaseAudition();
        for (int i = 0; i < 300; ++i) {
            realtime = true;
            audition.process(b.data(), 512);
            realtime = false;
        }
        require(energy(b.data(), b.size()) == 0, "Early key release was lost");
        require(rtAlloc == 0 && rtFree == 0, "0.2 callback allocated/freed C++ memory");
        // Mapping pitch/velocity zones and a sustained loop use deterministic PCM fixtures.
        auto low = std::make_shared<Sample>();
        low->stereo.assign(256, .1f);
        auto high = std::make_shared<Sample>();
        high->stereo.assign(256, .8f);
        Preset zones;
        zones.kind = InstrumentKind::Sampler;
        zones.attack = .0001;
        zones.decay = .01;
        zones.sustain = 1;
        zones.cutoff = 20000;
        SampleZone zl;
        zl.sample = low;
        zl.highVelocity = 63;
        zl.loop = true;
        zl.loopStart = 0;
        zl.loopEnd = 128;
        SampleZone zh = zl;
        zh.sample = high;
        zh.lowVelocity = 64;
        zh.highVelocity = 127;
        zones.zones = {zl, zh};
        Synthesizer quiet(zones, 48000), loud(zones, 48000);
        quiet.noteOn(1, 60, .4);
        loud.noteOn(2, 60, .9);
        float a = 0, v = 0;
        for (int i = 0; i < 1000; ++i) {
            a = quiet.next().first;
            v = loud.next().first;
        }
        require(v > a * 10 && a > 0, "Velocity mapping or sample loop failed");
        quiet.noteOff(1);
        for (int i = 0; i < 20000; ++i)
            a = quiet.next().first;
        require(a == 0, "Sampler release voice stuck");
        // Self-contained valid pack installation and SHA-256 tamper rejection.
        auto local = temp / "local-pack";
        std::filesystem::create_directory(local);
        std::array<float, 512> pcm{};
        pcm.fill(.1);
        writeWav16(local / "note.wav", pcm.data(), 256, 48000);
        std::ofstream(local / "note.sfz") << "<region> sample=note.wav key=60";
        auto manifest = [&] {
            auto text =
                std::string("{\"id\":\"test.pack\",\"version\":\"1\",\"name\":\"Test\","
                            "\"author\":\"QuadraJ\",\"source\":\"local test "
                            "fixture\",\"license\":\"CC0-1.0\",\"installSize\":4096,\"files\":[{"
                            "\"path\":\"note.wav\",\"size\":") +
                std::to_string(std::filesystem::file_size(local / "note.wav")) + ",\"sha256\":\"" +
                sha256File(local / "note.wav") + "\"},{\"path\":\"note.sfz\",\"size\":" +
                std::to_string(std::filesystem::file_size(local / "note.sfz")) + ",\"sha256\":\"" +
                sha256File(local / "note.sfz") +
                "\"}],\"instruments\":[{\"id\":\"test.note\",\"name\":\"Note\",\"category\":"
                "\"User Instruments\",\"format\":\"sfz\",\"file\":\"note.sfz\"}]}";
            text.replace(text.find("4096"), 4,
                         std::to_string(std::filesystem::file_size(local / "note.wav") +
                                        std::filesystem::file_size(local / "note.sfz")));
            return text;
        };
        std::ofstream(local / "pack.json") << manifest();
        auto installed = installSoundPack(local, temp / "installed");
        require(installed.id == "test.pack", "Local pack install failed");
        {
            std::fstream damage(local / "note.wav",
                                std::ios::in | std::ios::out | std::ios::binary);
            damage.seekp(50);
            damage.put('x');
        }
        rejected = false;
        try {
            readSoundPack(local);
        } catch (...) {
            rejected = true;
        }
        require(rejected, "Tampered sample checksum not rejected");
        const unsigned char midi[] = {'M', 'T',  'h',  'd',  0,    0,   0, 6, 0,    0,    0, 1,
                                      1,   0xe0, 'M',  'T',  'r',  'k', 0, 0, 0,    13,   0, 0x90,
                                      60,  100,  0x83, 0x60, 0x80, 60,  0, 0, 0xff, 0x2f, 0};
        {
            std::ofstream mf(temp / "test.mid", std::ios::binary);
            mf.write(reinterpret_cast<const char*>(midi), sizeof midi);
        }
        auto imported = importMidiFile(temp / "test.mid");
        require(imported.tracks.size() == 1 && imported.tracks[0].clips[0].notes[0].duration == 1,
                "SMF PPQ note timing wrong");
        std::filesystem::remove_all(temp);
        std::cout << "PASS: 9 recorded instruments -> MIDI -> collected project -> WAV, SHA-256, "
                     "metronome, loop, audition lifecycle, effects, v1 migration, missing samples, "
                     "traversal\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
