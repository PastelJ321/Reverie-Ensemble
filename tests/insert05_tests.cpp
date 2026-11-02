// SPDX-License-Identifier: Apache-2.0
#include "core/Engine.h"
#include "core/Plugin.h"
#include <iostream>
using namespace reverie;
int main(int argc, char** argv) try {
    if (argc != 2)
        return 2;
    auto infos = scanClap(argv[1]);
    Preset p;
    p.kind = InstrumentKind::Clap;
    p.pluginId = infos[0].id;
    p.backendPath = infos[0].path;
    auto setting = [&](double gain, double bias) {
        PluginInstance e(p, 48000);
        e.setParameter(0, gain);
        e.setParameter(1, bias);
        auto out = p;
        out.pluginState = e.saveState();
        return out;
    };
    auto a = setting(.5, .1), b = setting(.2, 0);
    Project project;
    auto t = makeTrack(TrackKind::Audio, 0);
    t.volume = 1;
    t.inserts = {{a, false, 1}, {b, false, 1}};
    project.master = 1;
    project.tracks.push_back(t);
    AudioProcessingGraph chain(project, 48000, true);
    float out[512]{};
    chain.render(out, 256, 0);
    if (std::abs(out[400] - .02f) > 1e-5)
        throw std::runtime_error("ordered insert chain wrong");
    std::swap(project.tracks[0].inserts[0], project.tracks[0].inserts[1]);
    AudioProcessingGraph reversed(project, 48000, true);
    reversed.render(out, 256, 0);
    if (std::abs(out[400] - .1f) > 1e-5)
        throw std::runtime_error("insert reorder not applied");
    project.tracks[0].inserts = {{a, true, 1}};
    AudioProcessingGraph bypass(project, 48000, true);
    bypass.render(out, 256, 0);
    if (out[400] != 0)
        throw std::runtime_error("bypass not dry");
    project.tracks[0].inserts = {{a, false, .5}};
    AudioProcessingGraph wet(project, 48000, true);
    wet.render(out, 256, 0);
    if (std::abs(out[400] - .05f) > 1e-5)
        throw std::runtime_error("wet/dry incorrect");
    shutdownPluginModules();
    std::cout << "PASS CLAP main/audio thread assertions, independent effect parameters/state, "
                 "ordered chain, reorder, bypass and wet/dry\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
