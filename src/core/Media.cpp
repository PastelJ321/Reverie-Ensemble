// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Media.h"
#include "core/Engine.h"
#include "miniaudio.h"
#include <array>
#include <fstream>
#include <stdexcept>
namespace reverie {
std::shared_ptr<const Sample> loadWav(const std::filesystem::path& path) {
    std::ifstream probe(path, std::ios::binary);
    std::array<char, 12> header{};
    probe.read(header.data(), 12);
    if (!probe || std::string(header.data(), 4) != "RIFF" ||
        std::string(header.data() + 8, 4) != "WAVE")
        throw std::runtime_error("Only RIFF WAV files are supported: " + path.string());
    auto cfg = ma_decoder_config_init(ma_format_f32, 2, 0);
    ma_decoder decoder{};
    if (ma_decoder_init_file(path.string().c_str(), &cfg, &decoder) != MA_SUCCESS)
        throw std::runtime_error("Cannot decode WAV: " + path.string());
    struct Guard {
        ma_decoder* d;
        ~Guard() {
            ma_decoder_uninit(d);
        }
    } guard{&decoder};
    ma_uint64 count = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &count) != MA_SUCCESS || !count ||
        count > ma_uint64(decoder.outputSampleRate) * 60 * 20 ||
        count > 256ULL * 1024 * 1024 / (2 * sizeof(float)))
        throw std::runtime_error(
            "WAV is empty or exceeds the 20 minute / 256 MiB decoded sample limit");
    auto s = std::make_shared<Sample>();
    s->path = std::filesystem::absolute(path).string();
    s->sampleRate = decoder.outputSampleRate;
    s->stereo.resize(std::size_t(count) * 2);
    ma_uint64 read = 0;
    if (ma_decoder_read_pcm_frames(&decoder, s->stereo.data(), count, &read) != MA_SUCCESS &&
        read == 0)
        throw std::runtime_error("WAV read failed");
    s->stereo.resize(std::size_t(read) * 2);
    for (float& f : s->stereo)
        if (!std::isfinite(f))
            f = 0;
    for (std::size_t i = 0; i < s->frames(); i += 256) {
        float lo = 0, hi = 0;
        for (std::size_t j = i; j < std::min(i + 256, s->frames()); ++j) {
            float v = (s->stereo[j * 2] + s->stereo[j * 2 + 1]) * .5f;
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
        s->peaks.emplace_back(lo, hi);
    }
    return s;
}
void writeWav16(const std::filesystem::path& p, const float* stereo, std::size_t frames,
                std::uint32_t rate) {
    auto cfg = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, 2, rate);
    ma_encoder encoder{};
    if (ma_encoder_init_file(p.string().c_str(), &cfg, &encoder) != MA_SUCCESS)
        throw std::runtime_error("Cannot write WAV");
    std::array<std::int16_t, 1024> pcm{};
    bool failed = false;
    for (std::size_t offset = 0; offset < frames; offset += 512) {
        auto n = std::min(std::size_t(512), frames - offset);
        for (std::size_t i = 0; i < n * 2; ++i)
            pcm[i] =
                std::int16_t(std::lrint(std::clamp(stereo[offset * 2 + i], -1.f, 1.f) * 32767));
        ma_uint64 written = 0;
        if (ma_encoder_write_pcm_frames(&encoder, pcm.data(), n, &written) != MA_SUCCESS ||
            written != n) {
            failed = true;
            break;
        }
    }
    ma_encoder_uninit(&encoder);
    if (failed)
        throw std::runtime_error("WAV write failed");
}
void exportWav(const Project& p, const std::filesystem::path& path, double rate,
               const std::function<void(double)>& progress) {
    AudioEngine engine(rate, false);
    engine.publish(p);
    engine.setPlaying(true);
    const auto total =
        TempoMap{p.bpm, rate}.sampleAt(p.endBeat()) + std::int64_t(rate * p.tailSeconds());
    if (total * 4 > std::int64_t(UINT32_MAX) - 64)
        throw std::runtime_error("Export exceeds RIFF WAV 4 GiB limit");
    auto temp = path;
    temp += ".render-" + std::to_string(nextId()) + ".tmp";
    auto cfg = ma_encoder_config_init(ma_encoding_format_wav, ma_format_s16, 2, ma_uint32(rate));
    ma_encoder enc{};
    if (ma_encoder_init_file(temp.string().c_str(), &cfg, &enc) != MA_SUCCESS)
        throw std::runtime_error("Cannot create export file");
    bool open = true;
    try {
        std::array<float, 1024> buffer{};
        std::array<std::int16_t, 1024> pcm{};
        for (std::int64_t offset = 0; offset < total; offset += 512) {
            const auto n = std::uint32_t(std::min(std::int64_t(512), total - offset));
            engine.process(buffer.data(), n);
            for (std::size_t i = 0; i < n * 2; ++i)
                pcm[i] = std::int16_t(std::lrint(buffer[i] * 32767));
            ma_uint64 written = 0;
            if (ma_encoder_write_pcm_frames(&enc, pcm.data(), n, &written) != MA_SUCCESS ||
                written != n)
                throw std::runtime_error("Export disk write failed");
            if (progress && offset % 32768 == 0)
                progress(double(offset) / double(total));
        }
        ma_encoder_uninit(&enc);
        open = false;
        std::filesystem::rename(temp, path);
        if (progress)
            progress(1.);
    } catch (...) {
        if (open)
            ma_encoder_uninit(&enc);
        std::error_code ec;
        std::filesystem::remove(temp, ec);
        throw;
    }
}
} // namespace reverie
