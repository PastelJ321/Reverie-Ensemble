// SPDX-License-Identifier: Apache-2.0
#include "core/Plugin.h"
#include "clap/clap.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/latency.h"
#include "clap/ext/note-ports.h"
#include "clap/ext/params.h"
#include "clap/ext/render.h"
#include "clap/ext/state.h"
#include "clap/ext/tail.h"
#include "clap/ext/thread-check.h"
#include <array>
#include <atomic>
#include <cstring>
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
namespace reverie {
namespace {
std::function<void(std::function<void()>)> mainDispatch;
thread_local bool audioThread = false;
void onMain(std::function<void()> f) {
    if (mainDispatch)
        mainDispatch(std::move(f));
    else
        f();
}
std::string binaryPath(const std::filesystem::path& path) {
    if (std::filesystem::is_directory(path)) {
        auto dir = path / "Contents/MacOS";
        for (auto& p : std::filesystem::directory_iterator(dir))
            if (p.is_regular_file())
                return p.path().string();
        throw std::runtime_error("CLAP bundle has no executable");
    }
    return path.string();
}
struct Module {
    void* handle = nullptr;
    const clap_plugin_entry_t* entry = nullptr;
    explicit Module(const std::string& path) {
        handle = dlopen(binaryPath(path).c_str(), RTLD_LOCAL | RTLD_NOW);
        if (!handle)
            throw std::runtime_error(std::string("Cannot open CLAP: ") + dlerror());
        entry = (const clap_plugin_entry_t*)dlsym(handle, "clap_entry");
        if (!entry || !clap_version_is_compatible(entry->clap_version) ||
            !entry->init(path.c_str())) {
            dlclose(handle);
            handle = nullptr;
            throw std::runtime_error("Invalid CLAP entry/version");
        }
    }
    ~Module() {
        if (handle) {
            entry->deinit();
            dlclose(handle);
        }
    }
    const clap_plugin_factory_t* factory() const {
        return (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
    }
};
std::mutex moduleMutex;
std::map<std::string, std::shared_ptr<Module>> modules;
std::shared_ptr<Module> loadModule(const std::string& path) {
    std::lock_guard lock(moduleMutex);
    auto key = std::filesystem::weakly_canonical(path).string();
    if (auto it = modules.find(key); it != modules.end())
        return it->second;
    if (modules.size() >= 128)
        throw std::runtime_error("Restart Ensemble before loading more plugin modules");
    auto module = std::make_shared<Module>(key);
    modules.emplace(key, module);
    return module;
}
} // namespace
void shutdownPluginModules() {
    onMain([] {
        std::lock_guard lock(moduleMutex);
        modules.clear();
    });
}
void setPluginMainDispatcher(std::function<void(std::function<void()>)> f) {
    mainDispatch = std::move(f);
}
std::vector<PluginInfo> scanClap(const std::filesystem::path& path) {
    auto m = loadModule(path.string());
    auto* f = m->factory();
    if (!f)
        throw std::runtime_error("No CLAP plugin factory");
    std::vector<PluginInfo> out;
    for (unsigned i = 0; i < std::min(4096u, f->get_plugin_count(f)); ++i) {
        auto* d = f->get_plugin_descriptor(f, i);
        if (!d || !d->id || !d->name)
            continue;
        bool instrument = false;
        if (d->features)
            for (auto v = d->features; *v; ++v)
                if (!std::strcmp(*v, CLAP_PLUGIN_FEATURE_INSTRUMENT))
                    instrument = true;
        out.push_back({path.string(), d->id, d->name, d->vendor ? d->vendor : "",
                       d->version ? d->version : "", instrument});
    }
    return out;
}
struct PluginInstance::Impl {
    std::shared_ptr<Module> module;
    const clap_plugin_t* plugin = nullptr;
    clap_host_t host{};
    std::thread::id mainId;
    std::atomic<bool> callback{false}, restart{false};
    bool processing = false, activated = false, overflow = false;
    const clap_plugin_params_t* params = nullptr;
    const clap_plugin_state_t* state = nullptr;
    clap_event_transport_t transport{};
    std::array<clap_event_midi_t, 4096> events{};
    unsigned count = 0;
    std::int64_t time = 0;
    std::array<float, 256> l{}, r{}, il{}, ir{}, zeroL{}, zeroR{};
    std::array<std::array<float, 256>, 16> aux{};
    unsigned outputs = 1;
    unsigned inputs = 0;
    bool instrument = false;
    double rate_ = 48000;
    clap_input_events_t in{};
    clap_output_events_t out{};
    static Impl& self(const clap_host_t* h) {
        return *(Impl*)h->host_data;
    }
    static const void* ext(const clap_host_t*, const char* id) {
        static const clap_host_thread_check_t check = {
            [](const clap_host_t* h) { return std::this_thread::get_id() == self(h).mainId; },
            [](const clap_host_t*) { return audioThread; }};
        return !std::strcmp(id, CLAP_EXT_THREAD_CHECK) ? &check : nullptr;
    }
    void service() {
        if (callback.exchange(false))
            plugin->on_main_thread(plugin);
    }
    explicit Impl(const Preset& p, double rate) {
        std::exception_ptr error;
        onMain([&] {
            try {
                rate_ = rate;
                mainId = std::this_thread::get_id();
                instrument = p.kind == InstrumentKind::Clap;
                host = {CLAP_VERSION,
                        this,
                        "Reverie Ensemble",
                        "QuadraJ",
                        "https://github.com/PastelJ321/Reverie-Ensemble",
                        "0.5.0",
                        ext,
                        [](const clap_host_t* h) { self(h).restart = true; },
                        [](const clap_host_t*) {},
                        [](const clap_host_t* h) { self(h).callback = true; }};
                module = loadModule(p.backendPath);
                auto* f = module->factory();
                if (!f)
                    throw std::runtime_error("CLAP factory missing");
                plugin = f->create_plugin(f, &host, p.pluginId.c_str());
                if (!plugin)
                    throw std::runtime_error("Plugin ID missing: " + p.pluginId);
                if (!plugin->init(plugin))
                    throw std::runtime_error("CLAP init failed");
                instrument = false;
                if (plugin->desc->features)
                    for (auto f = plugin->desc->features; *f; ++f)
                        if (!std::strcmp(*f, CLAP_PLUGIN_FEATURE_INSTRUMENT))
                            instrument = true;
                if (instrument) {
                    auto* ports = (const clap_plugin_note_ports_t*)plugin->get_extension(
                        plugin, CLAP_EXT_NOTE_PORTS);
                    clap_note_port_info_t info{};
                    if (!ports || ports->count(plugin, true) < 1 ||
                        !ports->get(plugin, 0, true, &info) ||
                        !(info.supported_dialects & CLAP_NOTE_DIALECT_MIDI))
                        throw std::runtime_error("CLAP instrument requires MIDI input dialect");
                }
                params =
                    (const clap_plugin_params_t*)plugin->get_extension(plugin, CLAP_EXT_PARAMS);
                state = (const clap_plugin_state_t*)plugin->get_extension(plugin, CLAP_EXT_STATE);
                if (!p.pluginState.empty()) {
                    if (!state)
                        throw std::runtime_error("CLAP cannot restore state");
                    struct Reader {
                        const std::vector<std::uint8_t>& data;
                        size_t pos = 0;
                    } reader{p.pluginState};
                    clap_istream_t stream{
                        &reader, [](const clap_istream_t* s, void* b, uint64_t n) -> int64_t {
                            auto& r = *(Reader*)s->ctx;
                            n = std::min(n, uint64_t(r.data.size() - r.pos));
                            std::memcpy(b, r.data.data() + r.pos, n);
                            r.pos += n;
                            return n;
                        }};
                    if (!state->load(plugin, &stream))
                        throw std::runtime_error("CLAP state restore failed");
                }
                auto* ports = (const clap_plugin_audio_ports_t*)plugin->get_extension(
                    plugin, CLAP_EXT_AUDIO_PORTS);
                clap_audio_port_info_t port{};
                if (!ports ||
                    (ports->count(plugin, false) < 1 || ports->count(plugin, false) > 8) ||
                    !ports->get(plugin, 0, false, &port) || port.channel_count != 2)
                    throw std::runtime_error("CLAP requires a stereo main output");
                outputs = ports->count(plugin, false);
                for (unsigned n = 0; n < outputs; ++n)
                    if (!ports->get(plugin, n, false, &port) || port.channel_count != 2)
                        throw std::runtime_error("CLAP output must be stereo");
                inputs = ports->count(plugin, true);
                if (inputs > 8)
                    throw std::runtime_error("CLAP has too many audio inputs");
                for (unsigned i = 0; i < inputs; ++i)
                    if (!ports->get(plugin, i, true, &port) || port.channel_count != 2)
                        throw std::runtime_error("CLAP input must be stereo");
                if (!plugin->activate(plugin, rate, 1, 256))
                    throw std::runtime_error("CLAP activation failed");
                activated = true;
                restart = false;
                service();
            } catch (...) {
                error = std::current_exception();
            }
        });
        if (error) {
            onMain([&] {
                if (plugin) {
                    if (activated)
                        plugin->deactivate(plugin);
                    plugin->destroy(plugin);
                    plugin = nullptr;
                }
                module.reset();
            });
            std::rethrow_exception(error);
        }
        in = {this, [](const clap_input_events_t* s) { return ((Impl*)s->ctx)->count; },
              [](const clap_input_events_t* s, uint32_t i) -> const clap_event_header_t* {
                  auto& v = *(Impl*)s->ctx;
                  return i < v.count ? &v.events[i].header : nullptr;
              }};
        out = {this, [](const clap_output_events_t*, const clap_event_header_t*) { return true; }};
    }
    ~Impl() {
        if (plugin) {
            if (processing) {
                audioThread = true;
                plugin->stop_processing(plugin);
                audioThread = false;
            }
            onMain([&] {
                if (activated)
                    plugin->deactivate(plugin);
                plugin->destroy(plugin);
                module.reset();
            });
        }
    }
};
PluginInstance::PluginInstance(const Preset& p, double rate)
    : p_(std::make_unique<Impl>(p, rate)) {}
PluginInstance::~PluginInstance() = default;
std::vector<PluginParameter> PluginInstance::parameters() {
    std::vector<PluginParameter> v;
    onMain([&] {
        auto& p = *p_;
        p.service();
        if (!p.params)
            return;
        for (unsigned i = 0; i < std::min(8192u, p.params->count(p.plugin)); ++i) {
            clap_param_info_t info{};
            double value;
            if (p.params->get_info(p.plugin, i, &info) &&
                p.params->get_value(p.plugin, info.id, &value))
                v.push_back({info.id, info.name, info.min_value, info.max_value, value,
                             bool(info.flags & CLAP_PARAM_IS_STEPPED),
                             bool(info.flags & CLAP_PARAM_IS_READONLY)});
        }
    });
    return v;
}
void PluginInstance::setParameter(uint32_t id, double value) {
    onMain([&] {
        auto& p = *p_;
        if (p.processing)
            throw std::runtime_error("Stop processing before editing plugin state");
        if (!p.params)
            return;
        clap_event_param_value_t event{};
        event.header = {sizeof(event), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_PARAM_VALUE, 0};
        event.param_id = id;
        event.note_id = -1;
        event.port_index = event.channel = event.key = -1;
        event.value = value;
        clap_input_events_t in{
            &event, [](const clap_input_events_t*) { return 1u; },
            [](const clap_input_events_t* s, uint32_t) -> const clap_event_header_t* {
                return &((clap_event_param_value_t*)s->ctx)->header;
            }};
        audioThread = true;
        p.params->flush(p.plugin, &in, &p.out);
        audioThread = false;
        p.service();
    });
}
std::vector<uint8_t> PluginInstance::saveState() {
    std::vector<uint8_t> bytes;
    onMain([&] {
        auto& p = *p_;
        p.service();
        if (!p.state)
            throw std::runtime_error("Plugin has no state extension");
        clap_ostream_t stream{&bytes,
                              [](const clap_ostream_t* s, const void* b, uint64_t n) -> int64_t {
                                  auto& v = *(std::vector<uint8_t>*)s->ctx;
                                  if (n > 8 * 1024 * 1024 - v.size())
                                      return -1;
                                  auto* p = (const uint8_t*)b;
                                  v.insert(v.end(), p, p + n);
                                  return n;
                              }};
        if (!p.state->save(p.plugin, &stream))
            throw std::runtime_error("Plugin state save failed");
    });
    return bytes;
}
void PluginInstance::midi(int status, int a, int b, int offset) noexcept {
    auto& p = *p_;
    if (p.count == p.events.size()) {
        p.overflow = true;
        return;
    }
    auto& e = p.events[p.count++];
    e = {};
    e.header = {sizeof(e), uint32_t(std::clamp(offset, 0, 255)), CLAP_CORE_EVENT_SPACE_ID,
                CLAP_EVENT_MIDI, 0};
    e.port_index = 0;
    e.data[0] = status;
    e.data[1] = a;
    e.data[2] = b;
}
void PluginInstance::process(float* stereo, unsigned n) noexcept {
    auto& p = *p_;
    audioThread = true;
    if (!p.activated) {
        std::fill_n(stereo, n * 2, 0.f);
        audioThread = false;
        return;
    }
    if (!p.processing)
        p.processing = p.plugin->start_processing(p.plugin);
    if (!p.processing || p.restart) {
        std::fill_n(stereo, n * 2, 0.f);
        audioThread = false;
        return;
    }
    if (p.overflow) {
        p.plugin->reset(p.plugin);
        p.count = 0;
        p.overflow = false;
    }
    for (unsigned i = 0; i < n; ++i) {
        p.il[i] = stereo[i * 2];
        p.ir[i] = stereo[i * 2 + 1];
        p.l[i] = p.r[i] = 0;
    }
    std::array<std::array<float*, 2>, 8> ptrs{};
    std::array<clap_audio_buffer_t, 8> obs{};
    for (unsigned k = 0; k < p.outputs; ++k) {
        ptrs[k] = {k ? p.aux[k * 2].data() : p.l.data(), k ? p.aux[k * 2 + 1].data() : p.r.data()};
        obs[k] = {ptrs[k].data(), nullptr, 2, 0, 0};
    }
    p.zeroL.fill(0);
    p.zeroR.fill(0);
    std::array<std::array<float*, 2>, 8> inPtrs{};
    std::array<clap_audio_buffer_t, 8> ibs{};
    for (unsigned k = 0; k < p.inputs; ++k) {
        inPtrs[k] = {k ? p.zeroL.data() : p.il.data(), k ? p.zeroR.data() : p.ir.data()};
        ibs[k] = {inPtrs[k].data(), nullptr, 2, 0, k ? 3u : 0u};
    }

    clap_process_t process{};
    process.steady_time = p.time;
    process.frames_count = n;
    process.audio_outputs = obs.data();
    process.audio_outputs_count = p.outputs;
    process.audio_inputs = p.inputs ? ibs.data() : nullptr;
    process.audio_inputs_count = p.inputs;
    process.transport = &p.transport;
    process.in_events = &p.in;
    process.out_events = &p.out;
    auto status = p.plugin->process(p.plugin, &process);
    for (unsigned i = 0; i < n; ++i) {
        stereo[i * 2] = status == CLAP_PROCESS_ERROR || !std::isfinite(p.l[i]) ? 0 : p.l[i];
        stereo[i * 2 + 1] = status == CLAP_PROCESS_ERROR || !std::isfinite(p.r[i]) ? 0 : p.r[i];
    }
    p.time += n;
    p.count = 0;
    audioThread = false;
}
void PluginInstance::transport(std::int64_t frame, double rate, double bpm, int numerator,
                               int denominator, bool playing) noexcept {
    auto& t = p_->transport;
    t = {};
    t.header = {sizeof(t), 0, CLAP_CORE_EVENT_SPACE_ID, CLAP_EVENT_TRANSPORT, 0};
    t.flags = CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE |
              CLAP_TRANSPORT_HAS_SECONDS_TIMELINE | CLAP_TRANSPORT_HAS_TIME_SIGNATURE |
              (playing ? CLAP_TRANSPORT_IS_PLAYING : 0);
    t.tempo = bpm;
    t.song_pos_seconds = std::llround(frame / rate * CLAP_SECTIME_FACTOR);
    t.song_pos_beats = std::llround(frame / rate * bpm / 60 * CLAP_BEATTIME_FACTOR);
    t.tsig_num = numerator;
    t.tsig_denom = denominator;
}
void PluginInstance::reset() noexcept {
    auto& p = *p_;
    p.count = 0;
    p.overflow = false;
    audioThread = true;
    p.plugin->reset(p.plugin);
    audioThread = false;
}
void PluginInstance::offline(bool offline) noexcept {
    onMain([&] {
        auto* p =
            (const clap_plugin_render_t*)p_->plugin->get_extension(p_->plugin, CLAP_EXT_RENDER);
        if (p && !p_->processing) {
            p_->plugin->deactivate(p_->plugin);
            p->set(p_->plugin, offline ? CLAP_RENDER_OFFLINE : CLAP_RENDER_REALTIME);
            p_->activated = p_->plugin->activate(p_->plugin, p_->rate_, 1, 256);
        }
    });
}
unsigned PluginInstance::latency() const {
    auto* p = (const clap_plugin_latency_t*)p_->plugin->get_extension(p_->plugin, CLAP_EXT_LATENCY);
    return p ? p->get(p_->plugin) : 0;
}
unsigned PluginInstance::tail() const {
    auto* p = (const clap_plugin_tail_t*)p_->plugin->get_extension(p_->plugin, CLAP_EXT_TAIL);
    return p ? std::min(p->get(p_->plugin), 48000u * 120) : 0;
}
} // namespace reverie
