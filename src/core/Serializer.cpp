// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Serializer.h"
#include "core/Hash.h"
#include "core/InstrumentBackend.h"
#include "core/Media.h"
#include "core/SfzSupport.h"
#include "json.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
namespace reverie {
using json = nlohmann::json;
static json presetJson(const Preset& s) {
    json j = {{"name", s.name},
              {"kind", int(s.kind)},
              {"wave", int(s.wave)},
              {"attack", s.attack},
              {"decay", s.decay},
              {"sustain", s.sustain},
              {"release", s.release},
              {"cutoff", s.cutoff},
              {"gain", s.gain},
              {"color", s.color},
              {"rootNote", s.rootNote},
              {"sample", s.samplePath},
              {"source", s.source},
              {"license", s.license},
              {"instrumentId", s.instrumentId},
              {"category", s.category},
              {"tags", s.tags},
              {"family", s.family},
              {"patchId", s.patchId},
              {"articulation", s.articulation},
              {"calibrationVersion", s.calibrationVersion},
              {"calibrationGain", s.calibrationGain},
              {"auditionGain", s.auditionGain},
              {"cubicInterpolation", s.cubicInterpolation},
              {"zones", json::array()}};
    j["backendPath"] = s.backendPath;
    j["renderTailSeconds"] = s.renderTailSeconds;
    j["soundFontBank"] = s.soundFontBank;
    j["soundFontProgram"] = s.soundFontProgram;
    j["artworkId"] = s.artworkId;
    j["artworkUri"] = s.artworkUri;
    j["thumbnailAsset"] = s.thumbnailAsset;
    j["artworkLicense"] = s.artworkLicense;
    j["artworkAuthor"] = s.artworkAuthor;
    j["imageAspectRatio"] = s.imageAspectRatio;
    for (const auto& z : s.zones)
        j["zones"].push_back({{"path", z.path},
                              {"lowKey", z.lowKey},
                              {"highKey", z.highKey},
                              {"lowVelocity", z.lowVelocity},
                              {"highVelocity", z.highVelocity},
                              {"rootKey", z.rootKey},
                              {"start", z.start},
                              {"end", z.end},
                              {"loopStart", z.loopStart},
                              {"loopEnd", z.loopEnd},
                              {"loop", z.loop},
                              {"oneShot", z.oneShot},
                              {"loopContinuous", z.loopContinuous},
                              {"gain", z.gain},
                              {"sequenceLength", z.sequenceLength},
                              {"sequencePosition", z.sequencePosition}});
    return j;
}
static Preset readPreset(const json& j) {
    Preset s;
    s.backendPath = j.value("backendPath", "");
    s.renderTailSeconds = j.value("renderTailSeconds", 0.f);
    s.soundFontBank = j.value("soundFontBank", 0);
    s.soundFontProgram = j.value("soundFontProgram", 0);
    s.artworkId = j.value("artworkId", "");
    s.artworkUri = j.value("artworkUri", "");
    s.thumbnailAsset = j.value("thumbnailAsset", "");
    s.artworkLicense = j.value("artworkLicense", "");
    s.artworkAuthor = j.value("artworkAuthor", "");
    s.imageAspectRatio = j.value("imageAspectRatio", 1.f);
    s.name = j.at("name");
    s.kind = InstrumentKind(j.at("kind").get<int>());
    s.wave = Waveform(j.at("wave").get<int>());
    s.attack = j.at("attack");
    s.decay = j.at("decay");
    s.sustain = j.at("sustain");
    s.release = j.at("release");
    s.cutoff = j.at("cutoff");
    s.gain = j.at("gain");
    s.color = j.at("color");
    s.rootNote = j.value("rootNote", 60);
    s.samplePath = j.value("sample", "");
    s.source = j.value("source", "");
    s.license = j.value("license", "");
    s.instrumentId = j.value("instrumentId", "");
    s.category = j.value("category", "");
    s.tags = j.value("tags", "");
    s.family = j.value("family", "");
    s.patchId = j.value("patchId", "");
    s.articulation = j.value("articulation", "");
    s.calibrationVersion = j.value("calibrationVersion", 0);
    s.calibrationGain = j.value("calibrationGain", 1.f);
    s.auditionGain = j.value("auditionGain", 1.f);
    s.cubicInterpolation = j.value("cubicInterpolation", false);
    for (const auto& v : j.value("zones", json::array())) {
        SampleZone z;
        z.path = v.at("path");
        z.lowKey = v.value("lowKey", 0);
        z.highKey = v.value("highKey", 127);
        z.lowVelocity = v.value("lowVelocity", 0);
        z.highVelocity = v.value("highVelocity", 127);
        z.rootKey = v.value("rootKey", 60);
        z.start = v.value("start", std::uint64_t(0));
        z.end = v.value("end", std::uint64_t(0));
        z.loopStart = v.value("loopStart", std::uint64_t(0));
        z.loopEnd = v.value("loopEnd", std::uint64_t(0));
        z.loop = v.value("loop", false);
        z.oneShot = v.value("oneShot", false);
        z.loopContinuous = v.value("loopContinuous", false);
        z.gain = v.value("gain", 1.f);
        z.sequenceLength = v.value("sequenceLength", 1);
        z.sequencePosition = v.value("sequencePosition", 1);
        s.zones.push_back(z);
    }
    Project test;
    Track t;
    t.preset = s;
    test.tracks.push_back(t);
    validate(test);
    return s;
}
static void atomicJson(const json& j, const std::filesystem::path& path) {
    auto temp = path;
    temp += ".save-" + std::to_string(nextId()) + ".tmp";
    try {
        std::ofstream f(temp, std::ios::binary | std::ios::trunc);
        f.exceptions(std::ios::badbit | std::ios::failbit);
        f << j.dump(2) << '\n';
        f.flush();
        f.close();
        std::filesystem::rename(temp, path);
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        throw;
    }
}
Project ProjectSerializer::save(const Project& p, const std::filesystem::path& path) {
    validate(p);
    Project collected = p;
    const auto folder = std::filesystem::absolute(path).parent_path();
    std::filesystem::create_directories(folder);
    auto media = folder / (path.stem().string() + ".media");
    std::unordered_map<std::string, std::string> copied;
    auto stash = [&](const std::string& source) -> std::string {
        if (source.empty())
            return "";
        if (copied.contains(source))
            return copied.at(source);
        auto src = std::filesystem::absolute(source);
        if (!std::filesystem::exists(src))
            return source; // preserve missing references
        std::filesystem::create_directories(media);
        // Content hash prevents overwriting a media file still referenced by an earlier saved
        // project.
        std::ifstream input(src, std::ios::binary);
        if (!input)
            throw std::runtime_error("Cannot read media for collection: " + src.string());
        input.exceptions(std::ios::badbit);
        std::uint64_t hash = 14695981039346656037ULL;
        char block[8192];
        while (input) {
            input.read(block, sizeof block);
            for (std::streamsize i = 0; i < input.gcount(); ++i) {
                hash ^= std::uint8_t(block[i]);
                hash *= 1099511628211ULL;
            }
        }
        std::ostringstream name;
        name << std::hex << hash << (src.extension().empty() ? ".wav" : src.extension().string());
        auto dest = media / name.str();
        if (!std::filesystem::exists(dest)) {
            auto tmp = dest;
            tmp += ".tmp";
            std::filesystem::copy_file(src, tmp, std::filesystem::copy_options::overwrite_existing);
            std::filesystem::rename(tmp, dest);
        }
        const auto rel = std::filesystem::relative(dest, folder).generic_string();
        copied[source] = rel;
        return rel;
    };
    json j = {{"format", "Reverie Ensemble"},
              {"version", 4},
              {"loopEnabled", p.loopEnabled},
              {"loopStart", p.loopStart},
              {"loopEnd", p.loopEnd},
              {"metronome", p.metronome},
              {"clickLevel", p.clickLevel},
              {"name", p.name},
              {"bpm", p.bpm},
              {"numerator", p.numerator},
              {"denominator", p.denominator},
              {"master", p.master},
              {"tracks", json::array()}};
    auto absoluteRef = [&](const std::string& ref) {
        if (ref.empty())
            return std::string{};
        auto file = std::filesystem::path(ref);
        return (file.is_absolute() ? file : folder / file).lexically_normal().string();
    };
    for (auto& t : collected.tracks) {
        auto preset = presetJson(t.preset);
        preset["sample"] = stash(!t.preset.samplePath.empty()
                                     ? t.preset.samplePath
                                     : (t.instrumentSample ? t.instrumentSample->path : ""));
        t.preset.samplePath = absoluteRef(preset["sample"].get<std::string>());
        for (std::size_t i = 0; i < t.preset.zones.size(); ++i) {
            auto& z = t.preset.zones[i];
            auto relative = stash(z.path.empty() && z.sample ? z.sample->path : z.path);
            preset["zones"][i]["path"] = relative;
            z.path = absoluteRef(relative);
        }
        if (t.preset.kind == InstrumentKind::SoundFont &&
            std::filesystem::exists(t.preset.backendPath)) {
            auto relative = stash(t.preset.backendPath);
            preset["backendPath"] = relative;
            t.preset.backendPath = absoluteRef(relative);
        } else if (t.preset.kind == InstrumentKind::Sfizz &&
                   std::filesystem::exists(t.preset.backendPath)) {
            auto mapping = std::filesystem::path(t.preset.backendPath);
            auto root = mapping.parent_path();
            for (auto parent = root; !parent.empty() && parent != parent.root_path();
                 parent = parent.parent_path())
                if (std::filesystem::exists(parent / "pack.json")) {
                    root = parent;
                    break;
                }
            try {
                auto files = sfzDependencies(mapping, root);
                std::uint64_t fingerprint = 14695981039346656037ULL;
                for (auto& file : files)
                    for (unsigned char c :
                         sha256File(file) + file.lexically_relative(root).generic_string()) {
                        fingerprint ^= c;
                        fingerprint *= 1099511628211ULL;
                    }
                std::ostringstream label;
                label << std::hex << fingerprint;
                auto target = media / ("sfz-" + label.str());
                for (auto& file : files) {
                    auto relative = file.lexically_relative(root);
                    auto dst = target / relative;
                    std::filesystem::create_directories(dst.parent_path());
                    if (!std::filesystem::exists(dst) ||
                        std::filesystem::file_size(dst) != std::filesystem::file_size(file) ||
                        sha256File(dst) != sha256File(file)) {
                        auto temporary = dst;
                        temporary += ".collect-" + std::to_string(nextId()) + ".tmp";
                        try {
                            std::filesystem::copy_file(file, temporary);
                            std::filesystem::rename(temporary, dst);
                        } catch (...) {
                            std::error_code error;
                            std::filesystem::remove(temporary, error);
                            throw;
                        }
                    }
                }
                auto relocated = target / mapping.lexically_relative(root);
                preset["backendPath"] =
                    std::filesystem::relative(relocated, folder).generic_string();
                t.preset.backendPath = relocated.string();
            } catch (const std::runtime_error&) {
                preset["backendPath"] = t.preset.backendPath;
                preset["externalMediaReferenced"] = true;
            }
        }
        json jt = {
            {"id", t.id},       {"name", t.name},        {"kind", int(t.kind)}, {"color", t.color},
            {"mute", t.mute},   {"solo", t.solo},        {"volume", t.volume},  {"pan", t.pan},
            {"preset", preset}, {"clips", json::array()}};
        jt["armed"] = t.armed;
        jt["midiChannel"] = t.midiChannel;
        jt["midiInput"] = t.midiInput;
        const auto& fx = t.effects;
        jt["effects"] = {{"enabled", fx.enabled},
                         {"lowDb", fx.lowDb},
                         {"highDb", fx.highDb},
                         {"delayMix", fx.delayMix},
                         {"delaySeconds", fx.delaySeconds},
                         {"feedback", fx.feedback},
                         {"reverbMix", fx.reverbMix}};
        for (auto& c : t.clips) {
            json jc = {{"id", c.id},
                       {"name", c.name},
                       {"beat", c.beat},
                       {"length", c.length},
                       {"sourceOffset", c.sourceOffset},
                       {"media", stash(!c.mediaPath.empty() ? c.mediaPath
                                                            : (c.sample ? c.sample->path : ""))},
                       {"notes", json::array()}};
            jc["mute"] = c.mute;
            jc["controllers"] = json::array();
            for (const auto& cc : c.controllers)
                jc["controllers"].push_back({{"beat", cc.beat},
                                             {"channel", cc.channel},
                                             {"controller", cc.controller},
                                             {"value", cc.value}});
            c.mediaPath = absoluteRef(jc["media"].get<std::string>());
            for (const auto& n : c.notes)
                jc["notes"].push_back({{"id", n.id},
                                       {"beat", n.beat},
                                       {"duration", n.duration},
                                       {"key", n.key},
                                       {"velocity", n.velocity},
                                       {"channel", n.channel}});
            jt["clips"].push_back(jc);
        }
        j["tracks"].push_back(jt);
    }
    atomicJson(j, path);
    return collected;
}
LoadResult ProjectSerializer::load(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 64 * 1024 * 1024)
        throw std::runtime_error("Project exceeds 64 MiB limit");
    std::ifstream f(path);
    json j;
    f >> j;
    if (j.at("format") != "Reverie Ensemble" || (j.at("version") < 1 || j.at("version") > 4))
        throw std::runtime_error("Unsupported project format/version");
    LoadResult result;
    auto& p = result.project;
    p.name = j.at("name");
    p.bpm = j.at("bpm");
    p.numerator = j.at("numerator");
    p.denominator = j.at("denominator");
    p.master = j.at("master");
    p.formatVersion = 4;
    p.loopEnabled = j.value("loopEnabled", false);
    p.loopStart = j.value("loopStart", 0.);
    p.loopEnd = j.value("loopEnd", 16.);
    p.metronome = j.value("metronome", false);
    p.clickLevel = j.value("clickLevel", .12f);
    std::unordered_map<std::string, std::shared_ptr<const Sample>> cache;
    std::size_t sampleBytes = 0;
    auto resolve = [&](std::string& ref) -> std::shared_ptr<const Sample> {
        if (ref.empty())
            return {};
        auto file = std::filesystem::path(ref);
        if (file.is_relative())
            file = std::filesystem::absolute(path).parent_path() / file;
        ref = file.lexically_normal().string();
        if (cache.contains(ref))
            return cache[ref];
        try {
            auto s = loadWav(file);
            if (sampleBytes + s->stereo.size() * sizeof(float) > 512ULL * 1024 * 1024)
                throw std::runtime_error("Project media exceeds 512 MiB decoded cache budget");
            sampleBytes += s->stereo.size() * sizeof(float);
            cache[ref] = s;
            return s;
        } catch (const std::exception& e) {
            result.warnings.push_back(e.what());
            cache[ref] = {};
            return {};
        }
    };
    if (j.at("tracks").size() > maxTracks)
        throw std::runtime_error("Too many tracks");
    for (const auto& jt : j.at("tracks")) {
        Track t;
        t.id = jt.at("id");
        t.name = jt.at("name");
        t.armed = jt.value("armed", false);
        t.midiChannel = jt.value("midiChannel", -1);
        t.midiInput = jt.value("midiInput", "all");
        auto kind = jt.at("kind").get<int>();
        if (kind < 0 || kind > 1)
            throw std::runtime_error("Invalid track kind");
        t.kind = TrackKind(kind);
        t.color = jt.at("color");
        t.mute = jt.at("mute");
        t.solo = jt.at("solo");
        t.volume = jt.at("volume");
        t.pan = jt.at("pan");
        t.preset = readPreset(jt.at("preset"));
        if (!t.preset.backendPath.empty() &&
            std::filesystem::path(t.preset.backendPath).is_relative())
            t.preset.backendPath =
                (std::filesystem::absolute(path).parent_path() / t.preset.backendPath)
                    .lexically_normal()
                    .string();
        if (!t.preset.backendPath.empty() && !std::filesystem::exists(t.preset.backendPath)) {
            result.warnings.push_back("Missing external instrument: " + t.preset.backendPath);
        }
        if ((t.preset.kind == InstrumentKind::Sfizz && !sfizzAvailable()) ||
            (t.preset.kind == InstrumentKind::SoundFont && !fluidSynthAvailable()))
            result.warnings.push_back("Instrument backend unavailable; track is silent: " +
                                      t.preset.name);
        t.instrumentSample = resolve(t.preset.samplePath);
        for (auto& z : t.preset.zones)
            z.sample = resolve(z.path);
        if (jt.contains("effects")) {
            const auto& fx = jt["effects"];
            t.effects.enabled = fx.value("enabled", false);
            t.effects.lowDb = fx.value("lowDb", 0.f);
            t.effects.highDb = fx.value("highDb", 0.f);
            t.effects.delayMix = fx.value("delayMix", 0.f);
            t.effects.delaySeconds = fx.value("delaySeconds", .3f);
            t.effects.feedback = fx.value("feedback", .25f);
            t.effects.reverbMix = fx.value("reverbMix", 0.f);
        }
        for (const auto& jc : jt.at("clips")) {
            Clip c;
            c.id = jc.at("id");
            c.name = jc.at("name");
            c.mute = jc.value("mute", false);
            for (const auto& cc : jc.value("controllers", json::array()))
                c.controllers.push_back(
                    {cc.at("beat"), cc.value("channel", 0), cc.at("controller"), cc.at("value")});
            c.beat = jc.at("beat");
            c.length = jc.at("length");
            c.sourceOffset = jc.at("sourceOffset");
            c.mediaPath = jc.at("media");
            c.sample = resolve(c.mediaPath);
            for (const auto& jn : jc.at("notes")) {
                Note n;
                n.id = jn.at("id");
                n.beat = jn.at("beat");
                n.duration = jn.at("duration");
                n.key = jn.at("key");
                n.velocity = jn.at("velocity");
                n.channel = jn.value("channel", 0);
                c.notes.push_back(n);
            }
            t.clips.push_back(std::move(c));
        }
        p.tracks.push_back(std::move(t));
    }
    validate(p);
    for (const auto& t : p.tracks) {
        reserveId(t.id);
        for (const auto& c : t.clips) {
            reserveId(c.id);
            for (const auto& n : c.notes)
                reserveId(n.id);
        }
    }
    return result;
}
void ProjectSerializer::savePreset(const Preset& s, const std::filesystem::path& p) {
    atomicJson({{"format", "Reverie Instrument"}, {"version", 1}, {"preset", presetJson(s)}}, p);
}
Preset ProjectSerializer::loadPreset(const std::filesystem::path& p) {
    if (std::filesystem::file_size(p) > 1024 * 1024)
        throw std::runtime_error("Preset too large");
    std::ifstream f(p);
    json j;
    f >> j;
    if (j.at("format") != "Reverie Instrument" || j.at("version") != 1)
        throw std::runtime_error("Unknown preset format");
    auto s = readPreset(j.at("preset"));
    if (!s.samplePath.empty() && std::filesystem::path(s.samplePath).is_relative())
        s.samplePath = (std::filesystem::absolute(p).parent_path() / s.samplePath).string();
    for (auto& z : s.zones) {
        if (std::filesystem::path(z.path).is_relative())
            z.path = (std::filesystem::absolute(p).parent_path() / z.path).string();
        z.sample = loadWav(z.path);
    }
    return s;
}
} // namespace reverie
