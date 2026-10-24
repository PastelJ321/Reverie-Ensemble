// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <unordered_map>
namespace reverie {
class SampleCache {
  public:
    std::shared_ptr<const Sample> load(const std::filesystem::path&);

  private:
    std::unordered_map<std::string, std::weak_ptr<const Sample>> cache_;
};
struct LibraryInstrument {
    std::string id, name, category, tags, format;
    std::string instrumentId, patchId, patchName, family, articulation, source;
    float calibrationDb = 0, auditionDb = 0;
    int calibrationVersion = 0;
    std::filesystem::path file;
};
struct SoundPack {
    std::string id, version, name, author, source, license;
    std::uint64_t installSize = 0;
    std::filesystem::path folder;
    std::vector<LibraryInstrument> instruments;
};
SoundPack readSoundPack(const std::filesystem::path&, bool verify = true);
SoundPack installSoundPack(const std::filesystem::path& folder,
                           const std::filesystem::path& destination);
std::vector<Preset> calibratedFactoryPresets(const std::filesystem::path&);
Preset loadLibraryPreset(const LibraryInstrument&, const SoundPack&, SampleCache&);
Preset loadSfz(const std::filesystem::path&, SampleCache&);
} // namespace reverie
