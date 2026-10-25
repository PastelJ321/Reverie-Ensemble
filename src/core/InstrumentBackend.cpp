// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/InstrumentBackend.h"
#include "core/SfzSupport.h"
#include <stdexcept>
#ifdef REVERIE_HAS_SFIZZ
#include <sfizz.h>
#endif
#ifdef REVERIE_HAS_FLUIDSYNTH
#include <fluidsynth.h>
#endif
namespace reverie {
bool sfizzAvailable() {
#ifdef REVERIE_HAS_SFIZZ
    return true;
#else
    return false;
#endif
}
bool fluidSynthAvailable() {
#ifdef REVERIE_HAS_FLUIDSYNTH
    return true;
#else
    return false;
#endif
}
namespace {
class Unavailable final : public InstrumentBackend {
    std::string reason_;

  public:
    explicit Unavailable(std::string reason) : reason_(std::move(reason)) {}
    void noteOn(Id, int, float, int, int) noexcept override {}
    void noteOff(Id, int) noexcept override {}
    void controller(int, int, int, int) noexcept override {}
    void pitchBend(int, int, int) noexcept override {}
    void render(float* out, unsigned n) noexcept override {
        std::fill_n(out, n * 2, 0.f);
    }
    void reset() noexcept override {}
    std::string diagnostics() const override {
        return reason_;
    }
    BackendCapabilities capabilities() const noexcept override {
        return {"Unavailable instrument", false, false, false, false};
    }
};
struct Held {
    Id token = 0;
    int key = 0, channel = 0;
};
#ifdef REVERIE_HAS_SFIZZ
class Sfizz final : public InstrumentBackend {
    sfizz_synth_t* synth_ = nullptr;
    std::array<Held, 256> held_{};
    std::array<float, 256> left_{}, right_{};
    float gain_;
    std::string diagnostics_;

  public:
    Sfizz(const Preset& p, double rate) : gain_(p.calibrationGain * p.gain) {
        synth_ = sfizz_create_synth();
        if (!synth_)
            throw std::runtime_error("Cannot create sfizz");
        sfizz_set_sample_rate(synth_, float(rate));
        sfizz_set_samples_per_block(synth_, 256);
        sfizz_set_num_voices(synth_, 128);
        sfizz_set_sample_quality(synth_, SFIZZ_PROCESS_LIVE, 4);
        sfizz_set_sample_quality(synth_, SFIZZ_PROCESS_FREEWHEELING, 4);
        if (!sfizz_load_file(synth_, p.backendPath.c_str())) {
            sfizz_free(synth_);
            synth_ = nullptr;
            throw std::runtime_error("sfizz could not load: " + p.backendPath);
        }
        if (auto* unknown = sfizz_get_unknown_opcodes(synth_)) {
            diagnostics_ = unknown;
            sfizz_free_memory(unknown);
        }
    }
    void setOffline(bool value) noexcept override {
        if (value)
            sfizz_enable_freewheeling(synth_);
        else
            sfizz_disable_freewheeling(synth_);
    }
    std::uint64_t memoryBytes() const noexcept override {
        return sfizz_get_num_bytes(synth_);
    }
    std::string diagnostics() const override {
        return diagnostics_;
    }
    ~Sfizz() override {
        if (synth_)
            sfizz_free(synth_);
    }
    void noteOn(Id token, int key, float v, int ch, int offset) noexcept override {
        for (auto& h : held_)
            if (!h.token) {
                h = {token, key, ch};
                sfizz_send_hd_note_on(synth_, offset, key, v);
                return;
            }
    }
    void noteOff(Id token, int offset) noexcept override {
        for (auto& h : held_)
            if (h.token == token) {
                sfizz_send_note_off(synth_, offset, h.key, 0);
                h.token = 0;
                return;
            }
    }
    void controller(int, int cc, int value, int offset) noexcept override {
        sfizz_send_cc(synth_, offset, cc, value);
    }
    void pitchBend(int, int value, int offset) noexcept override {
        sfizz_send_pitch_wheel(synth_, offset, value - 8192);
    }
    void render(float* out, unsigned n) noexcept override {
        float* channels[] = {left_.data(), right_.data()};
        sfizz_render_block(synth_, channels, 2, int(n));
        for (unsigned i = 0; i < n; ++i) {
            out[i * 2] = left_[i] * gain_;
            out[i * 2 + 1] = right_[i] * gain_;
        }
    }
    void reset() noexcept override {
        sfizz_send_cc(synth_, 0, 120, 0);
        sfizz_send_cc(synth_, 0, 121, 0);
        sfizz_send_pitch_wheel(synth_, 0, 0);
        sfizz_all_sound_off(synth_);
        held_.fill({});
    }
    BackendCapabilities capabilities() const noexcept override {
        return {"sfizz 1.2.3", true, true, true, false};
    }
};
#endif
#ifdef REVERIE_HAS_FLUIDSYNTH
class Fluid final : public InstrumentBackend {
    fluid_settings_t* settings_ = nullptr;
    fluid_synth_t* synth_ = nullptr;
    std::array<Held, 256> held_{};
    struct Command {
        int type, channel, a, b, offset;
    };
    std::array<Command, 4096> commands_{};
    unsigned count_ = 0;
    bool overflow_ = false;
    int sf_ = -1, bank_, program_;
    void command(int type, int ch, int a, int b, int offset) noexcept {
        if (overflow_)
            return;
        if (count_ < commands_.size())
            commands_[count_++] = {type, ch, a, b, offset};
        else {
            overflow_ = true;
            count_ = 0;
            for (int c = 0; c < 16; ++c) {
                fluid_synth_all_sounds_off(synth_, c);
                fluid_synth_cc(synth_, c, 64, 0);
            }
            held_.fill({});
        }
    }
    void write(float* out, unsigned n) noexcept {
        if (n)
            fluid_synth_write_float(synth_, int(n), out, 0, 2, out, 1, 2);
    }

  public:
    Fluid(const Preset& p, double rate) : bank_(p.soundFontBank), program_(p.soundFontProgram) {
        settings_ = new_fluid_settings();
        fluid_settings_setnum(settings_, "synth.sample-rate", rate);
        fluid_settings_setnum(settings_, "synth.gain", .2 * p.calibrationGain * p.gain);
        fluid_settings_setint(settings_, "synth.polyphony", 128);
        fluid_settings_setint(settings_, "synth.threadsafe-api", 0);
        fluid_settings_setint(settings_, "synth.cpu-cores", 1);
        fluid_settings_setint(settings_, "synth.reverb.active", 0);
        fluid_settings_setint(settings_, "synth.chorus.active", 0);
        synth_ = new_fluid_synth(settings_);
        sf_ = fluid_synth_sfload(synth_, p.backendPath.c_str(), 0);
        if (sf_ < 0) {
            delete_fluid_synth(synth_);
            delete_fluid_settings(settings_);
            throw std::runtime_error("Cannot load SoundFont: " + p.backendPath);
        }
        for (int c = 0; c < 16; ++c)
            fluid_synth_program_select(synth_, c, sf_, bank_, program_);
    }
    ~Fluid() override {
        delete_fluid_synth(synth_);
        delete_fluid_settings(settings_);
    }
    void noteOn(Id token, int key, float v, int ch, int offset) noexcept override {
        for (auto& h : held_)
            if (!h.token) {
                h = {token, key, ch};
                command(0, ch, key, std::clamp(int(v * 127), 1, 127), offset);
                return;
            }
    }
    void noteOff(Id token, int offset) noexcept override {
        for (auto& h : held_)
            if (h.token == token) {
                command(1, h.channel, h.key, 0, offset);
                h.token = 0;
                return;
            }
    }
    void controller(int ch, int cc, int v, int offset) noexcept override {
        command(2, ch, cc, v, offset);
    }
    void pitchBend(int ch, int v, int offset) noexcept override {
        command(3, ch, v, 0, offset);
    }
    void render(float* out, unsigned n) noexcept override {
        if (overflow_) {
            std::fill_n(out, n * 2, 0.f);
            overflow_ = false;
            return;
        }
        unsigned pos = 0;
        for (unsigned i = 0; i < count_; ++i) {
            auto c = commands_[i];
            auto end = std::clamp(unsigned(std::max(0, c.offset)), pos, n);
            write(out + pos * 2, end - pos);
            pos = end;
            if (c.type == 0)
                fluid_synth_noteon(synth_, c.channel, c.a, c.b);
            if (c.type == 1)
                fluid_synth_noteoff(synth_, c.channel, c.a);
            if (c.type == 2)
                fluid_synth_cc(synth_, c.channel, c.a, c.b);
            if (c.type == 3)
                fluid_synth_pitch_bend(synth_, c.channel, c.a);
        }
        write(out + pos * 2, n - pos);
        count_ = 0;
    }
    void reset() noexcept override {
        count_ = 0;
        overflow_ = false;
        held_.fill({});
        for (int c = 0; c < 16; ++c) {
            fluid_synth_all_sounds_off(synth_, c);
            fluid_synth_cc(synth_, c, 121, 0);
            fluid_synth_cc(synth_, c, 64, 0);
            fluid_synth_pitch_bend(synth_, c, 8192);
        }
    }
    BackendCapabilities capabilities() const noexcept override {
        return {"FluidSynth", true, true, false, true};
    }
};
#endif
} // namespace
std::unique_ptr<InstrumentBackend> prepareBackend(const Preset& p, double rate,
                                                  bool tolerateMissing) {
    if (tolerateMissing &&
        (p.kind == InstrumentKind::Sfizz || p.kind == InstrumentKind::SoundFont) &&
        (!std::filesystem::is_regular_file(p.backendPath) ||
         (p.kind == InstrumentKind::Sfizz && !sfizzAvailable()) ||
         (p.kind == InstrumentKind::SoundFont && !fluidSynthAvailable())))
        return std::make_unique<Unavailable>("Missing external instrument/backend: " +
                                             p.backendPath);
    if (p.kind == InstrumentKind::Sfizz) {
#ifdef REVERIE_HAS_SFIZZ
        return std::make_unique<Sfizz>(p, rate);
#else
        throw std::runtime_error("sfizz is unavailable in this build");
#endif
    }
    if (p.kind == InstrumentKind::SoundFont) {
#ifdef REVERIE_HAS_FLUIDSYNTH
        return std::make_unique<Fluid>(p, rate);
#else
        throw std::runtime_error("FluidSynth is unavailable in this build");
#endif
    }
    return {};
}
std::vector<SoundFontProgram> soundFontPrograms(const std::filesystem::path& file) {
#ifdef REVERIE_HAS_FLUIDSYNTH
    auto* settings = new_fluid_settings();
    auto* s = new_fluid_synth(settings);
    int id = fluid_synth_sfload(s, file.c_str(), 0);
    std::vector<SoundFontProgram> result;
    if (id >= 0) {
        auto* font = fluid_synth_get_sfont_by_id(s, id);
        fluid_sfont_iteration_start(font);
        while (auto* p = fluid_sfont_iteration_next(font))
            result.push_back(
                {fluid_preset_get_banknum(p), fluid_preset_get_num(p), fluid_preset_get_name(p)});
    }
    delete_fluid_synth(s);
    delete_fluid_settings(settings);
    if (id < 0)
        throw std::runtime_error("Invalid or unsupported SoundFont");
    return result;
#else
    (void)file;
    throw std::runtime_error("FluidSynth is unavailable in this build");
#endif
}
Preset externalPreset(const std::filesystem::path& path, InstrumentKind kind, int bank,
                      int program) {
    Preset p;
    p.kind = kind;
    p.name = path.stem().string();
    p.backendPath = std::filesystem::absolute(path).string();
    p.renderTailSeconds = kind == InstrumentKind::Sfizz ? sfzReleaseTail(path) : 10.f;
    p.soundFontBank = bank;
    p.soundFontProgram = program;
    p.gain = 1;
    p.category = "User Instruments";
    p.instrumentId = p.backendPath;
    p.patchId = std::to_string(bank) + ":" + std::to_string(program);
    p.source = "User supplied";
    p.license = "User-managed; verify asset license";
    return p;
}
} // namespace reverie
