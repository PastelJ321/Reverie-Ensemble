// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Export.h"
#include "core/InstrumentBackend.h"
#include "core/Library.h"
#include "core/SfzSupport.h"
#include "json.hpp"
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
using namespace reverie;
int main(int argc, char** argv) try {
    if (argc < 3) {
        std::cerr << "Usage: reverie-library-audit PACK_FOLDER REPORT.csv [--verify]\n";
        return 2;
    }
    auto pack = readSoundPack(argv[1], argc > 3);
    std::ofstream report(argv[2]);
    report << "instrument,patch,file,samples,peak,rms,unknown_opcodes,prepare_ms,preloaded_bytes\n";
    SampleCache cache;
    std::set<std::string> instruments;
    unsigned audible = 0;
    float worstPeak = 0;
    for (const auto& entry : pack.instruments) {
        try {
            auto dependencies = sfzDependencies(entry.file, pack.folder);
            std::set<int> roots;
            std::ifstream mapping(entry.file);
            std::string text((std::istreambuf_iterator<char>(mapping)), {});
            std::regex keys(R"(\b(pitch_keycenter|key)\s*=\s*(\d+))");
            for (std::sregex_iterator it(text.begin(), text.end(), keys), end; it != end; ++it) {
                int key = std::stoi((*it)[2]);
                if (key >= 21 && key <= 108)
                    roots.insert(key);
            }
            if (roots.empty())
                roots.insert(60);
            std::vector<int> notes(roots.begin(), roots.end());
            auto p = loadLibraryPreset(entry, pack, cache);
            auto begin = std::chrono::steady_clock::now();
            auto backend = prepareBackend(p, 48000);
            backend->setOffline(true);
            double ms =
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                    .count();
            std::array<float, 512> out{};
            double sum = 0;
            std::uint64_t count = 0;
            float peak = 0;
            for (int index : {0, int(notes.size() / 2), int(notes.size() - 1)})
                for (float velocity : {.3f, .7f, 1.f}) {
                    backend->reset();
                    backend->noteOn(1, notes[index], velocity, 0);
                    for (int b = 0; b < 45; ++b) {
                        backend->render(out.data(), 256);
                        for (float v : out) {
                            if (!std::isfinite(v))
                                throw std::runtime_error("Nonfinite instrument render");
                            peak = std::max(peak, std::abs(v));
                            sum += double(v) * v;
                            ++count;
                        }
                    }
                    backend->noteOff(1);
                    for (int b = 0; b < 10; ++b)
                        backend->render(out.data(), 256);
                }
            instruments.insert(entry.instrumentId);
            if (peak > 0)
                ++audible;
            worstPeak = std::max(worstPeak, peak);
            auto quote = [](std::string s) {
                std::string escaped;
                for (char c : s) {
                    if (c == '"')
                        escaped += '"';
                    escaped += c;
                }
                return '"' + escaped + '"';
            };
            report << quote(entry.name) << ',' << quote(entry.patchName) << ','
                   << quote(entry.file.lexically_relative(pack.folder).string()) << ','
                   << dependencies.size() - 1 << ',' << peak << ','
                   << std::sqrt(sum / std::max(std::uint64_t(1), count)) << ','
                   << quote(backend->diagnostics()) << ',' << ms << ',' << backend->memoryBytes()
                   << '\n';
            report.flush();
            std::cout << entry.name << " / " << entry.patchName << " peak=" << peak
                      << " unsupported=" << backend->diagnostics() << '\n';
        } catch (const std::exception& e) {
            std::cout << "FAILED " << entry.file << ": " << e.what() << std::endl;
        }
    }
    std::cout << "SUMMARY instruments=" << instruments.size()
              << " patches=" << pack.instruments.size() << " audible=" << audible
              << " maxPeak=" << worstPeak << " bytes=" << pack.installSize << '\n';
    return audible == pack.instruments.size() ? 0 : 1;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
