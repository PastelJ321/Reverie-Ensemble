// SPDX-License-Identifier: Apache-2.0
#include "core/Engine.h"
#include "core/Export.h"
#include "core/Plugin.h"
#include "core/Serializer.h"
#include <array>
#include <iostream>
using namespace reverie;
void check(bool b, const char* s) {
    if (!b)
        throw std::runtime_error(s);
}
int main(int argc, char** argv) try {
    if (argc < 2) {
        std::cerr << "Pass actual CLAP bundle path\n";
        return 2;
    }
    auto infos = scanClap(std::filesystem::absolute(argv[1]));
    check(!infos.empty(), "real discovery");
    Preset p;
    p.kind = InstrumentKind::Clap;
    p.backendPath = infos[0].path;
    p.pluginId = infos[0].id;
    p.name = infos[0].name;
    p.gain = .3;
    p.renderTailSeconds = 4;
    {
        PluginInstance editor(p, 48000);
        auto params = editor.parameters();
        check(params.size() > 0, "real parameters");
        for (auto& v : params)
            if (!v.readOnly && v.maximum > v.minimum) {
                editor.setParameter(v.id, v.minimum + (v.maximum - v.minimum) * .4);
                break;
            }
        p.pluginState = editor.saveState();
        check(!p.pluginState.empty(), "real state");
    }
    PluginInstance instance(p, 48000);
    instance.offline(true);
    instance.midi(0x90, 60, 100);
    std::array<float, 512> out{};
    double peak = 0;
    for (int b = 0; b < 100; ++b) {
        instance.process(out.data(), 256);
        for (auto x : out) {
            check(std::isfinite(x), "finite CLAP output");
            peak = std::max(peak, double(std::abs(x)));
        }
    }
    check(peak > 0, "actual plugin audio");
    instance.midi(0x80, 60, 0);
    instance.reset();
    Project project;
    auto t = makeTrack(TrackKind::Midi, 0);
    t.preset = p;
    Clip c;
    c.notes.push_back(Note{});
    t.clips.push_back(c);
    project.tracks.push_back(t);
    auto path = std::filesystem::path(REVERIE_TEST_ROOT) / "out/plugin05";
    std::filesystem::create_directories(path);
    ProjectSerializer::save(project, path / "Surge Study.reverie");
    auto loaded = ProjectSerializer::load(path / "Surge Study.reverie");
    check(loaded.project.tracks[0].preset.pluginState == p.pluginState, "project state roundtrip");
    auto result = renderAudio(loaded.project, path / "Surge Study.wav", {});
    check(result.peak > 0, "real plugin offline output");
    auto fxInfo =
        scanClap(std::filesystem::path(p.backendPath).parent_path() / "Surge XT Effects.clap");
    check(!fxInfo.empty() && !fxInfo[0].instrument, "real effect discovery");
    Preset fx;
    fx.kind = InstrumentKind::Clap;
    fx.backendPath = fxInfo[0].path;
    fx.pluginId = fxInfo[0].id;
    fx.name = fxInfo[0].name;
    fx.renderTailSeconds = 5;
    {
        PluginInstance editor(fx, 48000);
        auto params = editor.parameters();
        check(!params.empty(), "effect parameters");
        for (auto& v : params)
            if (!v.readOnly && v.maximum > v.minimum) {
                editor.setParameter(v.id, v.minimum + (v.maximum - v.minimum) * .6);
                break;
            }
        fx.pluginState = editor.saveState();
    }
    loaded.project.tracks[0].inserts.push_back({fx, false, .75f});
    ProjectSerializer::save(loaded.project, path / "Surge FX Study.reverie");
    auto withFx = ProjectSerializer::load(path / "Surge FX Study.reverie");
    check(withFx.project.tracks[0].inserts[0].plugin.pluginState == fx.pluginState,
          "effect state restore");
    auto effected = renderAudio(withFx.project, path / "Surge FX Study.wav", {});
    check(effected.peak > 0, "actual effect insert offline process");
    Preset missing = p;
    missing.backendPath = "/nonexistent/missing.clap";
    check(prepareBackend(missing, 48000, true) != nullptr, "missing plugin tolerant");
    shutdownPluginModules();
    std::cout << "PASS real " << p.name
              << " discovery/parameters/state/audio/project/reopen/export; peak=" << peak
              << " stateBytes=" << p.pluginState.size() << " exportPeak=" << result.peak << '\n';
} catch (const std::exception& e) {
    std::cerr << "FAIL " << e.what() << '\n';
    return 1;
}
