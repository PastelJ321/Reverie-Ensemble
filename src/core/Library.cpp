// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Library.h"
#include "core/Hash.h"
#include "core/Media.h"
#include "json.hpp"
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
namespace reverie {
std::shared_ptr<const Sample> SampleCache::load(const std::filesystem::path& path) {
    auto key = std::filesystem::weakly_canonical(path).string();
    if (auto p = cache_[key].lock())
        return p;
    auto s = loadWav(path);
    std::size_t bytes = s->stereo.size() * sizeof(float);
    for (const auto& entry : cache_)
        if (auto active = entry.second.lock())
            bytes += active->stereo.size() * sizeof(float);
    if (bytes > 512ULL * 1024 * 1024)
        throw std::runtime_error("Sample cache exceeds 512 MiB");
    cache_[key] = s;
    return s;
}
static int keyNumber(const std::string& s) {
    if (s.empty())
        throw std::runtime_error("Empty SFZ key");
    if (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '-')
        return std::stoi(s);
    static const std::string names = "c d ef g a b";
    auto pos = names.find(char(std::tolower(static_cast<unsigned char>(s[0]))));
    if (pos == std::string::npos)
        throw std::runtime_error("Invalid SFZ key");
    int index = 1, acc = 0;
    if (s.size() > 1 && (s[1] == '#' || s[1] == 'b')) {
        acc = s[1] == '#' ? 1 : -1;
        ++index;
    }
    return (std::stoi(s.substr(index)) + 1) * 12 + int(pos) + acc;
}
Preset loadSfz(const std::filesystem::path& path, SampleCache& cache) {
    if (std::filesystem::file_size(path) > 2 * 1024 * 1024)
        throw std::runtime_error("SFZ exceeds 2 MiB");
    std::ifstream f(path);
    std::string text, line;
    while (std::getline(f, line)) {
        auto i = line.find("//");
        if (i != std::string::npos)
            line.resize(i);
        text += line + '\n';
    }
    if (text.find('#') != std::string::npos && text.find("#include") != std::string::npos)
        throw std::runtime_error("SFZ includes are unsupported");
    std::regex token(
        R"(<(global|group|region)>|([A-Za-z_]+)\s*=\s*([^<]*?)(?=\s+[A-Za-z_]+\s*=|<|$))");
    using Params = std::map<std::string, std::string>;
    Params global, group, region;
    Params* current = &global;
    Preset p;
    p.kind = InstrumentKind::Sampler;
    p.name = path.stem().string();
    p.attack = .002;
    p.decay = .1;
    p.sustain = 1;
    p.release = .2;
    p.gain = .7;
    std::map<std::string,float> envelopeValues;
    bool inRegion = false;
    auto flush = [&] {
        if (!inRegion)
            return;
        SampleZone z;
        for (auto [op, value] : region) {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.pop_back();
            if(op.rfind("ampeg_",0)==0){float v=std::stof(value);auto [it,inserted]=envelopeValues.try_emplace(op,v);if(!inserted&&it->second!=v)throw std::runtime_error("Region-specific SFZ envelope is unsupported");}
            if (op == "sample") {
                std::replace(value.begin(), value.end(), '\\', '/');
                z.path = (std::filesystem::absolute(path).parent_path() / value)
                             .lexically_normal()
                             .string();
            } else if (op == "key")
                z.lowKey = z.highKey = z.rootKey = keyNumber(value);
            else if (op == "lokey")
                z.lowKey = keyNumber(value);
            else if (op == "hikey")
                z.highKey = keyNumber(value);
            else if (op == "pitch_keycenter")
                z.rootKey = keyNumber(value);
            else if (op == "lovel")
                z.lowVelocity = std::stoi(value);
            else if (op == "hivel")
                z.highVelocity = std::stoi(value);
            else if (op == "offset")
                z.start = std::stoull(value);
            else if (op == "end")
                z.end = std::stoull(value) + 1;
            else if (op == "loop_start")
                z.loopStart = std::stoull(value);
            else if (op == "loop_end")
                z.loopEnd = std::stoull(value) + 1;
            else if (op == "loop_mode") {
                z.loop = value == "loop_continuous" || value == "loop_sustain";z.loopContinuous=value=="loop_continuous";
                z.oneShot = value == "one_shot";
                if (!z.loop && !z.oneShot && value != "no_loop")
                    throw std::runtime_error("Unsupported SFZ loop mode");
            } else if (op == "volume")
                z.gain = std::pow(10.f, std::stof(value) / 20);
            else if (op == "ampeg_attack")
                p.attack = std::max(.0001f, std::stof(value));
            else if (op == "ampeg_decay")
                p.decay = std::max(.0001f, std::stof(value));
            else if (op == "ampeg_sustain")
                p.sustain = std::stof(value) / 100;
            else if (op == "ampeg_release")
                p.release = std::max(.001f, std::stof(value));
            else
                throw std::runtime_error("Unsupported SFZ opcode: " + op);
        }
        if (z.path.empty())
            throw std::runtime_error("SFZ region has no sample");
        z.sample = cache.load(z.path);
        if (z.start >= z.sample->frames() || (z.end && z.end > z.sample->frames()) ||
            (z.loop && (z.loopStart < z.start || z.loopEnd > (z.end ? z.end : z.sample->frames()))))
            throw std::runtime_error("SFZ sample bounds invalid");
        p.zones.push_back(std::move(z));
        if (p.zones.size() > 4096)
            throw std::runtime_error("SFZ region limit");
    };
    for (std::sregex_iterator it(text.begin(), text.end(), token), end; it != end; ++it) {
        auto m = *it;
        if (m[1].matched) {
            flush();
            auto tag = m[1].str();
            inRegion = tag == "region";
            if (tag == "global")
                current = &global;
            else if (tag == "group") {
                group = global;
                current = &group;
            } else {
                region = group.empty() ? global : group;
                current = &region;
            }
        } else {
            auto value = m[3].str();
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            (*current)[m[2].str()] = value;
        }
    }
    flush();
    if (p.zones.empty())
        throw std::runtime_error("No supported SFZ regions");
    Project test;
    Track t;
    t.preset = p;
    test.tracks.push_back(t);
    validate(test);
    return p;
}
static std::filesystem::path safePath(const std::filesystem::path& base,
                                      const std::string& relative) {
    std::filesystem::path p(relative);
    if (p.empty() || p.is_absolute())
        throw std::runtime_error("Pack path must be relative");
    for (const auto& part : p)
        if (part == "..")
            throw std::runtime_error("Pack path traversal rejected");
    auto root = std::filesystem::weakly_canonical(base);
    auto full = std::filesystem::weakly_canonical(root / p);
    auto rel = full.lexically_relative(root);
    if (rel.empty() || *rel.begin() == "..")
        throw std::runtime_error("Pack symlink escape rejected");
    return full;
}
SoundPack readSoundPack(const std::filesystem::path& folder, bool verify) {
    auto path = folder / "pack.json";
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Pack manifest too large");
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    SoundPack p;
    p.folder = std::filesystem::absolute(folder);
    p.id = j.at("id");
    p.version = j.at("version");
    p.name = j.at("name");
    p.author = j.at("author");
    p.source = j.at("source");
    p.license = j.at("license");
    p.installSize = j.at("installSize");
    if (!std::regex_match(p.id, std::regex("[a-z0-9][a-z0-9.-]{0,95}")) || p.license.empty() ||
        p.installSize > 1024ULL * 1024 * 1024)
        throw std::runtime_error("Invalid sound pack metadata");
    std::uint64_t totalSize = 0;
    for (const auto& file : j.at("files")) {
        auto bytes = file.at("size").get<std::uint64_t>();
        if (bytes > 1024ULL * 1024 * 1024 - totalSize)
            throw std::runtime_error("Pack payload exceeds 1 GiB");
        totalSize += bytes;
        auto local = safePath(folder, file.at("path"));
        if (!std::filesystem::is_regular_file(local) ||
            std::filesystem::file_size(local) != file.at("size").get<std::uint64_t>() ||
            (verify && sha256File(local) != file.at("sha256").get<std::string>()))
            throw std::runtime_error("Missing or damaged pack file: " + local.string());
    }
    if (totalSize != p.installSize)
        throw std::runtime_error("Pack install size does not match file inventory");
    std::set<std::filesystem::path> listed;
    for (const auto& file : j.at("files"))
        listed.insert(safePath(folder, file.at("path")));
    for (const auto& i : j.at("instruments")) {
        LibraryInstrument v;
        v.id = i.at("id");
        v.name = i.at("name");
        v.category = i.at("category");
        v.tags = i.value("tags", "");
        v.format = i.at("format");
        v.file = safePath(folder, i.at("file"));
        if (!listed.contains(v.file) || !std::filesystem::is_regular_file(v.file))
            throw std::runtime_error("Instrument mapping absent from pack inventory");
        if (v.format != "sfz")
            throw std::runtime_error("Unsupported pack format");
        p.instruments.push_back(v);
    }
    if (verify) {
        SampleCache cache;
        for (const auto& i : p.instruments) {
            auto instrument = loadSfz(i.file, cache);
            for (const auto& z : instrument.zones)
                if (!listed.contains(std::filesystem::weakly_canonical(z.path)))
                    throw std::runtime_error("SFZ refers outside pack inventory");
        }
    }
    return p;
}
SoundPack installSoundPack(const std::filesystem::path& folder,
                           const std::filesystem::path& destination) {
    auto p = readSoundPack(folder);
    auto final = destination / p.id;
    if (std::filesystem::exists(final))
        throw std::runtime_error("Pack already installed");
    std::filesystem::create_directories(destination);
    auto staging = destination / (p.id + ".install-" + std::to_string(nextId()));
    try {
        std::filesystem::create_directory(staging);
        std::ifstream f(folder / "pack.json");
        nlohmann::json j;
        f >> j;
        std::filesystem::copy_file(folder / "pack.json", staging / "pack.json");
        for (const auto& entry : j.at("files")) {
            auto relative = entry.at("path").get<std::string>();
            auto src = safePath(folder, relative);
            auto dst = staging / relative;
            std::filesystem::create_directories(dst.parent_path());
            std::filesystem::copy_file(src, dst);
        }
        readSoundPack(staging);
        std::filesystem::rename(staging, final);
        return readSoundPack(final);
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove_all(staging, ec);
        throw;
    }
}
} // namespace reverie
