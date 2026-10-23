// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/MidiFile.h"
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace reverie {
Project importMidiFile(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 16 * 1024 * 1024)
        throw std::runtime_error("MIDI exceeds 16 MiB");
    std::ifstream file(path, std::ios::binary);
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), {});
    std::size_t cursor = 0, limit = data.size();
    auto byte = [&]() -> unsigned {
        if (cursor >= limit)
            throw std::runtime_error("Truncated MIDI data");
        return data[cursor++];
    };
    auto word = [&] {
        unsigned n = byte();
        return (n << 8) | byte();
    };
    auto dword = [&] {
        unsigned n = word();
        return (n << 16) | word();
    };
    auto tag = [&](const char* s) {
        for (int i = 0; i < 4; ++i)
            if (byte() != unsigned(s[i]))
                throw std::runtime_error("Invalid MIDI chunk");
    };
    auto vlq = [&] {
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            auto b = byte();
            value = (value << 7) | (b & 127);
            if (!(b & 128))
                return value;
        }
        throw std::runtime_error("Invalid MIDI VLQ");
    };
    tag("MThd");
    auto header = dword();
    if (header < 6 || header > data.size() - cursor)
        throw std::runtime_error("Invalid MIDI header");
    unsigned format = word(), tracks = word(), ppq = word();
    if (format > 1 || !tracks || tracks > 64 || !ppq || (ppq & 0x8000))
        throw std::runtime_error("Only SMF type 0/1 with PPQ timing is supported");
    cursor += header - 6;
    Project project;
    project.name = path.stem().string();
    bool tempoSet = false;
    for (unsigned index = 0; index < tracks; ++index) {
        limit = data.size();
        tag("MTrk");
        auto length = dword();
        if (length > data.size() - cursor)
            throw std::runtime_error("Truncated MIDI track");
        limit = cursor + length;
        unsigned status = 0;
        std::uint64_t ticks = 0;
        std::string name = "MIDI " + std::to_string(index + 1);
        std::array<std::vector<Note>, 16> notes;
        std::array<std::array<std::vector<Note>, 128>, 16> held;
        while (cursor < limit) {
            ticks += vlq();
            if (ticks > 100000000)
                throw std::runtime_error("MIDI timeline too long");
            unsigned first = byte();
            if (first & 128) {
                status = first;
                first = 0x100;
            } else if (status < 0x80 || status >= 0xf0)
                throw std::runtime_error("Invalid running status");
            if (status == 0xff) {
                auto type = byte(), size = vlq();
                if (size > limit - cursor)
                    throw std::runtime_error("Truncated MIDI meta event");
                if (type == 3)
                    name = std::string(reinterpret_cast<const char*>(data.data() + cursor),
                                       std::min(size, 128u));
                if (type == 0x51 && size == 3) {
                    unsigned micros =
                        (data[cursor] << 16) | (data[cursor + 1] << 8) | data[cursor + 2];
                    if (!micros)
                        throw std::runtime_error("Invalid MIDI tempo");
                    double bpm = 60000000. / micros;
                    if (ticks > 0 && (!tempoSet || std::abs(project.bpm - bpm) > .001))
                        throw std::runtime_error("MIDI tempo automation is not supported");
                    project.bpm = bpm;
                    tempoSet = true;
                }
                if (type == 0x58 && size >= 2) {
                    project.numerator = data[cursor];
                    if (data[cursor + 1] > 4)
                        throw std::runtime_error("Unsupported time signature");
                    project.denominator = 1 << data[cursor + 1];
                }
                cursor += size;
                status = 0;
                continue;
            }
            if (status == 0xf0 || status == 0xf7) {
                auto size = vlq();
                if (size > limit - cursor)
                    throw std::runtime_error("Truncated SysEx");
                cursor += size;
                status = 0;
                continue;
            }
            if (status >= 0xf0)
                throw std::runtime_error("Unsupported MIDI system event");
            unsigned a = first == 0x100 ? byte() : first, kind = status & 0xf0, ch = status & 15,
                     b = 0;
            if (kind != 0xc0 && kind != 0xd0)
                b = byte();
            if (a > 127 || b > 127)
                throw std::runtime_error("Invalid MIDI channel data");
            if (kind == 0x90 && b) {
                Note n;
                n.key = a;
                n.beat = double(ticks) / ppq;
                n.velocity = b / 127.f;
                held[ch][a].push_back(n);
            } else if (kind == 0x80 || (kind == 0x90 && !b)) {
                auto& queue = held[ch][a];
                if (!queue.empty()) {
                    auto n = queue.front();
                    queue.erase(queue.begin());
                    n.duration = std::max(1. / double(ppq), double(ticks) / ppq - n.beat);
                    notes[ch].push_back(n);
                }
            }
        }
        for (int ch = 0; ch < 16; ++ch) {
            for (auto& key : held[ch])
                for (auto n : key) {
                    n.duration = std::max(.25, double(ticks) / ppq - n.beat);
                    notes[ch].push_back(n);
                }
            if (notes[ch].empty())
                continue;
            if (project.tracks.size() >= maxTracks)
                throw std::runtime_error("MIDI exceeds track limit");
            auto track = makeTrack(TrackKind::Midi, int(project.tracks.size()));
            track.name = name + " / " + std::to_string(ch + 1);
            if (ch == 9)
                track.preset = factoryPresets()[8];
            Clip clip;
            clip.name = name;
            clip.length = std::max(4., std::ceil(double(ticks) / ppq));
            clip.notes = std::move(notes[ch]);
            track.clips.push_back(std::move(clip));
            project.tracks.push_back(std::move(track));
        }
    }
    validate(project);
    return project;
}
} // namespace reverie
