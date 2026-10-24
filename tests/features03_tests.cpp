// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Dsp.h"
#include "core/Export.h"
#include "core/Library.h"
#include "core/Media.h"
#include "core/Serializer.h"
#include <chrono>
#include <fstream>
#include <iostream>
using namespace reverie;
static void check(bool value, const char* what) {
    if (!value)
        throw std::runtime_error(what);
}
static std::uint32_t le(const std::vector<unsigned char>& data, int offset, int bytes) {
    std::uint32_t n = 0;
    for (int i = 0; i < bytes; ++i)
        n |= std::uint32_t(data.at(offset + i)) << (8 * i);
    return n;
}
int main() try {
    auto root = std::filesystem::path(REVERIE_TEST_ROOT);
    auto temp = std::filesystem::temp_directory_path() /
                ("reverie03-" +
                 std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);
    Project p;
    Track t = makeTrack(TrackKind::Midi, 0);
    t.preset = factoryPresets()[7];
    t.preset.attack = .001f;
    t.volume = .6f;
    Clip c;
    c.length = 2;
    Note n;
    n.beat = .25;
    n.duration = .5;
    c.notes = {n};
    t.clips = {c};
    p.tracks = {t};
    for (int rate : {44100, 48000, 96000})
        for (auto depth : {WavDepth::Pcm16, WavDepth::Pcm24, WavDepth::Float32})
            for (int channels : {1, 2}) {
                ExportOptions o;
                o.sampleRate = rate;
                o.depth = depth;
                o.channels = channels;
                o.endBeat = 1;
                o.includeTail = false;
                auto file = temp / "matrix.wav";
                auto r = renderAudio(p, file, o);
                std::ifstream in(file, std::ios::binary);
                std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), {});
                check(le(bytes, 24, 4) == unsigned(rate) &&
                          le(bytes, 22, 2) == unsigned(channels) &&
                          le(bytes, 34, 2) == unsigned(int(depth)),
                      "WAV requested format differs from header");
                check(le(bytes, 20, 2) == (depth == WavDepth::Float32 ? 3u : 1u), "WAV format tag");
                check(r.frames == unsigned(rate / 2), "Sample accurate export range");
                check(bytes.size() == (depth == WavDepth::Float32 ? 58 : 44) +
                                          r.frames * channels * (int(depth) / 8),
                      "WAV size");
                auto decoded = loadWav(file);
                check(decoded->sampleRate == unsigned(rate) && decoded->frames() == r.frames,
                      "Decoded matrix frames");
                check(r.peak > 0 && r.rms > 0, "Rendered matrix silent");
            }
    ExportOptions o;
    o.endBeat = 1;
    o.includeTail = false;
    o.normalize = true;
    o.depth = WavDepth::Pcm24;
    auto r = renderAudio(p, temp / "normal.wav", o);
    auto normal = loadWav(temp / "normal.wav");
    float peak = 0;
    for (float f : normal->stereo)
        peak = std::max(peak, std::abs(f));
    check(std::abs(peak - .89125094) < .00001, "Explicit normalization target");
    ExportOptions tailOptions;
    tailOptions.endBeat = 1;
    tailOptions.includeTail = true;
    auto tail = renderAudio(p, temp / "tail.wav", tailOptions);
    check(tail.frames == 24000 + std::uint64_t(48000 * p.tailSeconds()),
          "Effect/release tail frame count");
    auto tailAudio = loadWav(temp / "tail.wav");
    bool releaseSound = false;
    for (std::size_t i = 48000; i < std::min(tailAudio->stereo.size(), std::size_t(60000)); ++i)
        releaseSound |= std::abs(tailAudio->stereo[i]) > 1e-6;
    check(releaseSound, "Release tail was truncated at range end");
    Project hot = p;
    hot.master = 2;
    hot.tracks[0].volume = 2;
    hot.tracks[0].preset.gain = 2;
    for (int i = 0; i < 8; ++i) {
        Note loud = hot.tracks[0].clips[0].notes[0];
        loud.id = nextId();
        hot.tracks[0].clips[0].notes.push_back(loud);
    }
    ExportOptions hotOptions;
    hotOptions.endBeat = 1;
    hotOptions.includeTail = false;
    hotOptions.depth = WavDepth::Float32;
    auto floatResult = renderAudio(hot, temp / "headroom.wav", hotOptions);
    check(floatResult.peak > 1 && floatResult.clippedSamples > 0, "Actual over-range detection");
    auto floats = loadWav(temp / "headroom.wav");
    bool above = false;
    for (float v : floats->stereo)
        above |= std::abs(v) > 1;
    check(above, "Float WAV lost headroom");
    hotOptions.depth = WavDepth::Pcm16;
    renderAudio(hot, temp / "safe-pcm.wav", hotOptions);
    auto integers = loadWav(temp / "safe-pcm.wav");
    for (float v : integers->stereo)
        check(std::isfinite(v) && std::abs(v) <= 1, "Integer PCM overflow");
    p.tracks[0].clips[0].notes.clear();
    o.normalize = false;
    o.dither = true;
    o.depth = WavDepth::Pcm16;
    renderAudio(p, temp / "dither.wav", o);
    auto noise = loadWav(temp / "dither.wav");
    bool nonzero = false;
    for (float f : noise->stereo) {
        nonzero |= f != 0;
        check(std::abs(f) < .0001, "Dither amplitude");
    }
    check(nonzero, "TPDF dither not applied");
    auto existing = temp / "cancel.wav";
    std::ofstream(existing) << "preserve";
    bool cancelled = false;
    try {
        renderAudio(p, existing, o, [](double progress) { return progress < .15; });
    } catch (...) {
        cancelled = true;
    }
    check(cancelled, "Export did not cancel");
    std::ifstream preserved(existing);
    std::string contents;
    preserved >> contents;
    check(contents == "preserve", "Cancelled export replaced destination");
    for (auto& f : std::filesystem::directory_iterator(temp))
        check(f.path().extension() != ".tmp" && f.path().extension() != ".float",
              "Cancelled temporary file leaked");
    bool failed = false;
    try {
        renderAudio(p, temp / "missing" / "failure.wav", o);
    } catch (...) {
        failed = true;
    }
    check(failed, "Bad destination not rejected");
    auto old = ProjectSerializer::load(root / "examples/First Light.reverie");
    check(old.warnings.empty() && old.project.tracks[0].preset.calibrationVersion == 0 &&
              old.project.tracks[0].preset.calibrationGain == 1,
          "Legacy mix was silently calibrated");
    auto legacy2 = ProjectSerializer::load(root / "tests/fixtures/legacy-v2.reverie");
    check(legacy2.warnings.empty() && legacy2.project.tracks[0].preset.calibrationVersion == 0 &&
              !legacy2.project.tracks[0].preset.cubicInterpolation,
          "v2 sampler mix was silently changed");
    SampleCache cache;
    int count = 0;
    for (auto folder : {"factory", "essentials03"}) {
        auto pack = readSoundPack(root / "assets/sounds" / folder);
        for (auto& i : pack.instruments) {
            auto preset = loadLibraryPreset(i, pack, cache);
            check(preset.calibrationVersion == 1 && preset.calibrationGain > 0,
                  "Factory calibration metadata");
            p = Project{};
            t = makeTrack(TrackKind::Midi, 0);
            t.preset = preset;
            c = Clip{};
            n = Note{};
            n.key = preset.zones.front().rootKey;
            n.duration = .5;
            n.velocity = .7;
            c.notes = {n};
            c.length = 2;
            t.clips = {c};
            p.tracks = {t};
            auto saved = ProjectSerializer::save(p, temp / "sample.reverie");
            auto back = ProjectSerializer::load(temp / "sample.reverie");
            check(back.warnings.empty() &&
                      back.project.tracks[0].preset.calibrationGain == preset.calibrationGain,
                  "Calibrated preset round trip");
            o = ExportOptions{};
            o.endBeat = 1;
            o.includeTail = false;
            auto audio = renderAudio(back.project, temp / "sample.wav", o);
            check(audio.peak > 0, "New recorded instrument is silent");
            ++count;
        }
    }
    check(count == 14, "Installed recorded patch count");
    auto a = std::make_shared<Sample>(), b = std::make_shared<Sample>();
    a->stereo.assign(4096, .2);
    b->stereo.assign(4096, .6);
    Preset rr;
    rr.kind = InstrumentKind::Sampler;
    rr.attack = .0001;
    rr.cutoff = 20000;
    SampleZone za;
    za.sample = a;
    za.sequenceLength = 2;
    za.sequencePosition = 1;
    SampleZone zb = za;
    zb.sample = b;
    zb.sequencePosition = 2;
    rr.zones = {za, zb};
    Synthesizer synth(rr, 48000);
    synth.noteOn(1, 60, 1);
    float first = 0;
    for (int i = 0; i < 100; ++i)
        first = synth.next().first;
    synth.releaseAll();
    for (int i = 0; i < 20000; ++i)
        synth.next();
    synth.noteOn(2, 60, 1);
    float second = 0;
    for (int i = 0; i < 100; ++i)
        second = synth.next().first;
    check(second > first * 2, "Round-robin sample did not advance");
    std::filesystem::remove_all(temp);
    std::cout << "PASS: 18 WAV formats, range/tail/normalization/dither/cancel, 14 calibrated "
                 "recorded patches / 13 instruments, v1 mix preservation, round-robin\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
}
