// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Export.h"
#include "core/Engine.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
namespace reverie {
static void little(std::ostream& f, std::uint32_t n, int bytes) {
    for (int i = 0; i < bytes; ++i)
        f.put(char((n >> (i * 8)) & 255));
}
ExportResult renderAudio(const Project& original, const std::filesystem::path& path,
                         const ExportOptions& o, const std::function<bool(double)>& progress) {
    if ((o.sampleRate != 44100 && o.sampleRate != 48000 && o.sampleRate != 96000) ||
        (o.channels != 1 && o.channels != 2) ||
        (o.depth != WavDepth::Pcm16 && o.depth != WavDepth::Pcm24 &&
         o.depth != WavDepth::Float32) ||
        !std::isfinite(o.startBeat) || !std::isfinite(o.endBeat) || o.startBeat < 0)
        throw std::runtime_error("Invalid export options");
    Project p = original;
    validate(p);
    double end = o.endBeat < 0 ? p.endBeat() : o.endBeat;
    if (end <= o.startBeat || end > 1000000)
        throw std::runtime_error("Export range is empty or too long");
    // Stop sources at the range boundary; preserve their releases/effect tails.
    for (auto& t : p.tracks) {
        std::erase_if(t.clips, [&](const Clip& c) { return c.beat >= end; });
        for (auto& c : t.clips) {
            c.length = std::min(c.length, end - c.beat);
            std::erase_if(c.notes, [&](const Note& n) { return n.beat >= c.length; });
            for (auto& n : c.notes)
                n.duration = std::min(n.duration, c.length - n.beat);
        }
    }
    TempoMap tempo{p.bpm, double(o.sampleRate)};
    auto start = tempo.sampleAt(o.startBeat);
    auto total = tempo.sampleAt(end) - start +
                 std::int64_t(o.includeTail ? o.sampleRate * p.tailSeconds() : 0);
    int bytes = int(o.depth) / 8;
    const int headerBytes = o.depth == WavDepth::Float32 ? 58 : 44;
    if (total <= 0 ||
        std::uint64_t(total) > (UINT32_MAX - std::uint64_t(headerBytes - 8)) / (bytes * o.channels))
        throw std::runtime_error("Export exceeds RIFF WAV 4 GiB limit");
    auto temp = path;
    temp += ".render-" + std::to_string(nextId()) + ".tmp";
    auto raw = temp;
    raw += ".float";
    auto report = [&](double value) {
        if (progress && !progress(value))
            throw std::runtime_error("Export cancelled");
    };
    try {
        report(0);
        ExportResult result;
        result.frames = total;
        AudioEngine engine(o.sampleRate, false, false);
        engine.publish(p);
        engine.seek(start);
        engine.setPlaying(true);
        std::ofstream scratch(raw, std::ios::binary | std::ios::trunc);
        if (!scratch)
            throw std::runtime_error("Cannot create export staging file");
        std::array<float, 1024> block{};
        long double square = 0;
        for (std::int64_t pos = 0; pos < total; pos += 512) {
            auto n = std::uint32_t(std::min<std::int64_t>(512, total - pos));
            engine.process(block.data(), n);
            for (std::size_t i = 0; i < n; ++i)
                for (int c = 0; c < o.channels; ++c) {
                    float v = o.channels == 1 ? (block[i * 2] + block[i * 2 + 1]) * .5f
                                              : block[i * 2 + c];
                    if (!std::isfinite(v))
                        v = 0;
                    block[i * o.channels + c] = v;
                    result.peak = std::max(result.peak, double(std::abs(v)));
                    square += double(v) * v;
                }
            scratch.write(reinterpret_cast<const char*>(block.data()),
                          n * o.channels * sizeof(float));
            if (!scratch)
                throw std::runtime_error("Export staging write failed (check free space)");
            report(.5 * double(pos + n) / total);
        }
        scratch.close();
        if (!scratch)
            throw std::runtime_error("Export staging close failed");
        result.rms = std::sqrt(double(square / (total * o.channels)));
        result.gain =
            o.normalize && result.peak > 0 ? .891250938 / result.peak : 1.; // explicit -1 dBFS peak
        std::ifstream input(raw, std::ios::binary);
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!input || !output)
            throw std::runtime_error("Cannot open export staging file");
        output.write("RIFF", 4);
        little(output, std::uint32_t(total * o.channels * bytes + headerBytes - 8), 4);
        output.write("WAVEfmt ", 8);
        little(output, o.depth == WavDepth::Float32 ? 18 : 16, 4);
        little(output, o.depth == WavDepth::Float32 ? 3 : 1, 2);
        little(output, o.channels, 2);
        little(output, o.sampleRate, 4);
        little(output, o.sampleRate * o.channels * bytes, 4);
        little(output, o.channels * bytes, 2);
        little(output, int(o.depth), 2);
        if (o.depth == WavDepth::Float32) {
            little(output, 0, 2); // WAVEFORMATEX cbSize
            output.write("fact", 4);
            little(output, 4, 4);
            little(output, std::uint32_t(total), 4);
        }
        output.write("data", 4);
        little(output, std::uint32_t(total * o.channels * bytes), 4);
        std::uint32_t rng = 0x71ab391u;
        auto random = [&]() {
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            return double(rng) / UINT32_MAX;
        };
        for (std::int64_t pos = 0; pos < total; pos += 512) {
            auto n = std::min<std::int64_t>(512, total - pos);
            input.read(reinterpret_cast<char*>(block.data()), n * o.channels * sizeof(float));
            if (!input)
                throw std::runtime_error("Export staging read failed");
            for (int i = 0; i < n * o.channels; ++i) {
                double v = block[i] * result.gain;
                if (std::abs(v) > 1)
                    ++result.clippedSamples;
                if (o.depth == WavDepth::Float32)
                    little(output, std::bit_cast<std::uint32_t>(float(v)), 4);
                else {
                    double scale = o.depth == WavDepth::Pcm16 ? 32768. : 8388608.;
                    double noise = o.dither ? random() - random() : 0;
                    auto q = std::int32_t(
                        std::llround(std::clamp(v * scale + noise, -scale, scale - 1)));
                    little(output, std::uint32_t(q), bytes);
                }
            }
            if (!output)
                throw std::runtime_error("Export write failed (check free space)");
            report(.5 + .5 * double(pos + n) / total);
        }
        output.close();
        if (!output)
            throw std::runtime_error("Export close failed");
        input.close();
        report(1);
        std::filesystem::rename(temp, path);
        std::filesystem::remove(raw);
        return result;
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        std::filesystem::remove(raw, ec);
        throw;
    }
}
} // namespace reverie
