// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Engine.h"
#include "core/Library.h"
#include "json.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <set>
using namespace reverie;
struct Level {
    double peak = 0, rms = 0;
};
static Level measure(const Preset& preset, int key, float velocity, bool chord = false) {
    Project p;
    p.master = 1;
    Track t;
    t.volume = 1;
    t.preset = preset;
    Clip c;
    c.length = 4;
    for (int k : chord ? std::vector<int>{key, key + 4, key + 7} : std::vector<int>{key}) {
        Note n;
        n.key = std::min(127, k);
        n.duration = 2;
        n.velocity = velocity;
        c.notes.push_back(n);
    }
    t.clips.push_back(c);
    p.tracks.push_back(t);
    AudioEngine e(48000, false, false);
    e.publish(p);
    e.setPlaying(true);
    std::array<float, 512> b{};
    long double sum = 0;
    Level l;
    for (int f = 0; f < 72000; f += 256) {
        auto count = std::min(256, 72000 - f);
        e.process(b.data(), count);
        for (int i = 0; i < count * 2; ++i) {
            l.peak = std::max(l.peak, double(std::abs(b[i])));
            sum += double(b[i]) * b[i];
        }
    }
    l.rms = std::sqrt(double(sum / 144000));
    return l;
}
int main(int argc, char** argv) try {
    if (argc < 3)
        throw std::runtime_error("Usage: reverie-calibrate assets/sounds report.csv [--apply]");
    std::ofstream report(argv[2]);
    report << "instrument,key,velocity,before_peak_dbfs,before_rms_dbfs,calibration_db,after_peak_"
              "dbfs,after_rms_dbfs,chord_peak_dbfs\n";
    auto db = [](double x) { return 20 * std::log10(std::max(1e-9, x)); };
    auto process = [&](Preset p, std::string name, std::string family, nlohmann::json* meta) {
        std::vector<int> keys;
        if (!p.zones.empty()) {
            for (auto& z : p.zones)
                keys.push_back(z.rootKey);
            std::sort(keys.begin(), keys.end());
            keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
            keys = {keys.front(), keys[keys.size() / 2], keys.back()};
        } else
            keys = p.kind == InstrumentKind::Drums ? std::vector<int>{36, 38, 42}
                                                   : std::vector<int>{48, 60, 72};
        std::vector<double> middle;
        double maxPeak = 0;
        for (int k : keys) {
            middle.push_back(measure(p, k, .7f).rms);
            maxPeak = std::max(maxPeak, measure(p, k, 1).peak);
        }
        std::sort(middle.begin(), middle.end());
        double target = (family == "strings" || family == "wind" || family == "synth") ? -25 : -30;
        double gainDb = std::clamp(target - db(middle[1]), -18., 30.);
        gainDb = std::min(gainDb, -7 - db(maxPeak));
        gainDb = std::max(-18., gainDb);
        for (int k : keys)
            for (float v : {.3f, .7f, 1.f}) {
                auto a = measure(p, k, v);
                auto q = p;
                q.calibrationGain = float(std::pow(10., gainDb / 20));
                auto b = measure(q, k, v);
                auto chord = measure(q, k, v, true);
                report << name << ',' << k << ',' << v << ',' << db(a.peak) << ',' << db(a.rms)
                       << ',' << gainDb << ',' << db(b.peak) << ',' << db(b.rms) << ','
                       << db(chord.peak) << '\n';
            }
        if (meta) {
            (*meta)["calibrationDb"] = gainDb;
            (*meta)["calibrationVersion"] = 1;
            (*meta)["auditionDb"] = std::clamp(-16 - db(maxPeak) - gainDb, -12., 8.);
        }
        std::cout << name << ": " << gainDb << " dB; medium RMS " << db(middle[1]) << " -> "
                  << db(middle[1]) + gainDb << "\n";
    };
    std::ofstream sources(std::string(argv[2]) + ".sources.csv");
    sources << "file,root_key,raw_peak_dbfs,raw_rms_dbfs\n";
    std::set<std::string> measured;
    for (auto& folder : std::filesystem::directory_iterator(argv[1]))
        if (std::filesystem::exists(folder.path() / "pack.json")) {
            auto pack = readSoundPack(folder.path());
            nlohmann::json j;
            {
                std::ifstream f(folder.path() / "pack.json");
                f >> j;
            }
            SampleCache cache;
            for (std::size_t i = 0; i < pack.instruments.size(); ++i) {
                auto p = loadSfz(pack.instruments[i].file, cache);
                for (auto& z : p.zones)
                    if (measured.insert(z.path).second) {
                        double peak = 0, sum = 0;
                        for (float f : z.sample->stereo) {
                            peak = std::max(peak, double(std::abs(f)));
                            sum += double(f) * f;
                        }
                        sources << std::filesystem::path(z.path).filename().string() << ','
                                << z.rootKey << ',' << db(peak) << ','
                                << db(std::sqrt(sum / z.sample->stereo.size())) << '\n';
                    }
                process(p,
                        pack.instruments[i].name + (pack.instruments[i].patchName == "Natural"
                                                        ? ""
                                                        : " / " + pack.instruments[i].patchName),
                        j["instruments"][i].value("family", "strings"), &j["instruments"][i]);
            }
            if (argc > 3 && std::string(argv[3]) == "--apply") {
                std::ofstream f(folder.path() / "pack.json");
                f << j.dump(2) << '\n';
            }
        }
    nlohmann::json synth = nlohmann::json::array();
    int index = 0;
    for (auto p : factoryPresets()) {
        nlohmann::json m;
        process(p, p.name, p.kind == InstrumentKind::Drums ? "drums" : "synth", &m);
        m["index"] = index++;
        synth.push_back(m);
    }
    if (argc > 3 && std::string(argv[3]) == "--apply") {
        std::ofstream f(std::filesystem::path(argv[1]) / "synth-calibration.json");
        f << synth.dump(2) << '\n';
    }
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
