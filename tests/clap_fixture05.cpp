// SPDX-License-Identifier: Apache-2.0
// Original independent host-conformance fixture. Never an installed instrument.
#include "clap/clap.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/params.h"
#include "clap/ext/state.h"
#include "clap/ext/thread-check.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static const char* features[] = {CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, nullptr};
static const clap_plugin_descriptor_t descriptor = {
    CLAP_VERSION, "org.quadraj.test-gain",    "QA Gain", "QuadraJ", "", "", "",
    "1",          "Independent host fixture", features};
struct Instance {
    clap_plugin_t plugin;
    const clap_host_t* host;
    double gain = 1, bias = 0;
};
static Instance& data(const clap_plugin_t* p) {
    return *(Instance*)p->plugin_data;
}
static bool mainThread(const clap_plugin_t* p) {
    auto& d = data(p);
    auto* c = (const clap_host_thread_check_t*)d.host->get_extension(d.host, CLAP_EXT_THREAD_CHECK);
    return c && c->is_main_thread(d.host);
}
static bool audioThread(const clap_plugin_t* p) {
    auto& d = data(p);
    auto* c = (const clap_host_thread_check_t*)d.host->get_extension(d.host, CLAP_EXT_THREAD_CHECK);
    return c && c->is_audio_thread(d.host);
}
static const clap_plugin_audio_ports_t ports = {
    [](const clap_plugin_t*, bool) { return 1u; },
    [](const clap_plugin_t*, uint32_t i, bool, clap_audio_port_info_t* v) {
        if (i)
            return false;
        *v = {};
        v->id = 0;
        std::strcpy(v->name, "Stereo");
        v->flags = CLAP_AUDIO_PORT_IS_MAIN;
        v->channel_count = 2;
        v->port_type = CLAP_PORT_STEREO;
        v->in_place_pair = CLAP_INVALID_ID;
        return true;
    }};
static const clap_plugin_params_t params = {
    [](const clap_plugin_t*) { return 2u; },
    [](const clap_plugin_t*, uint32_t i, clap_param_info_t* v) {
        if (i > 1)
            return false;
        *v = {};
        v->id = i;
        std::strcpy(v->name, i ? "Bias" : "Gain");
        v->min_value = 0;
        v->max_value = 2;
        v->default_value = i ? 0 : 1;
        return true;
    },
    [](const clap_plugin_t* p, clap_id i, double* v) {
        if (i > 1)
            return false;
        *v = i ? data(p).bias : data(p).gain;
        return true;
    },
    [](const clap_plugin_t*, clap_id, double v, char* b, uint32_t n) {
        return std::snprintf(b, n, "%.3f", v) > 0;
    },
    [](const clap_plugin_t*, clap_id, const char* b, double* v) {
        *v = std::strtod(b, nullptr);
        return true;
    },
    [](const clap_plugin_t* p, const clap_input_events_t* in, const clap_output_events_t*) {
        assert(audioThread(p));
        for (uint32_t i = 0; i < in->size(in); ++i) {
            auto* e = (const clap_event_param_value_t*)in->get(in, i);
            if (e->header.type == CLAP_EVENT_PARAM_VALUE) {
                if (e->param_id)
                    data(p).bias = e->value;
                else
                    data(p).gain = e->value;
            }
        }
    }};
static const clap_plugin_state_t state = {[](const clap_plugin_t* p, const clap_ostream_t* s) {
                                              assert(mainThread(p));
                                              double values[] = {data(p).gain, data(p).bias};
                                              return s->write(s, values, sizeof(values)) ==
                                                     sizeof(values);
                                          },
                                          [](const clap_plugin_t* p, const clap_istream_t* s) {
                                              assert(mainThread(p));
                                              double v[2];
                                              if (s->read(s, v, sizeof(v)) != sizeof(v))
                                                  return false;
                                              data(p).gain = v[0];
                                              data(p).bias = v[1];
                                              return true;
                                          }};
static const clap_plugin_t* create(const clap_plugin_factory_t*, const clap_host_t* h,
                                   const char* id) {
    if (std::strcmp(id, descriptor.id))
        return nullptr;
    auto* d = new Instance{};
    d->host = h;
    d->plugin = {&descriptor,
                 d,
                 [](const clap_plugin_t* p) {
                     assert(mainThread(p));
                     return true;
                 },
                 [](const clap_plugin_t* p) {
                     assert(mainThread(p));
                     delete &data(p);
                 },
                 [](const clap_plugin_t* p, double, uint32_t, uint32_t) {
                     assert(mainThread(p));
                     return true;
                 },
                 [](const clap_plugin_t* p) { assert(mainThread(p)); },
                 [](const clap_plugin_t* p) {
                     assert(audioThread(p));
                     return true;
                 },
                 [](const clap_plugin_t* p) { assert(audioThread(p)); },
                 [](const clap_plugin_t* p) { assert(audioThread(p)); },
                 [](const clap_plugin_t* p, const clap_process_t* pr) -> clap_process_status {
                     assert(audioThread(p));
                     for (uint32_t i = 0; i < pr->frames_count; ++i)
                         for (int ch = 0; ch < 2; ++ch)
                             pr->audio_outputs[0].data32[ch][i] =
                                 pr->audio_inputs[0].data32[ch][i] * data(p).gain + data(p).bias;
                     return CLAP_PROCESS_CONTINUE;
                 },
                 [](const clap_plugin_t*, const char* id) -> const void* {
                     if (!std::strcmp(id, CLAP_EXT_AUDIO_PORTS))
                         return &ports;
                     if (!std::strcmp(id, CLAP_EXT_PARAMS))
                         return &params;
                     if (!std::strcmp(id, CLAP_EXT_STATE))
                         return &state;
                     return nullptr;
                 },
                 [](const clap_plugin_t*) {}};
    return &d->plugin;
}
static const clap_plugin_factory_t factory = {
    [](const clap_plugin_factory_t*) { return 1u; },
    [](const clap_plugin_factory_t*, uint32_t i) { return i ? nullptr : &descriptor; }, create};
extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
    CLAP_VERSION,
    [](const char*) {
#ifdef REVERIE_CRASH_FIXTURE
        std::abort();
#endif
        return true;
    },
    [] {},
    [](const char* id) -> const void* {
        return !std::strcmp(id, CLAP_PLUGIN_FACTORY_ID) ? &factory : nullptr;
    }};
