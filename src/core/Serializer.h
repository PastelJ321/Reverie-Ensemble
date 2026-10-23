// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
namespace reverie {
struct LoadResult {
    Project project;
    std::vector<std::string> warnings;
};
class ProjectSerializer {
  public:
    // Returns the same project with references rebound to collected media. PCM stays shared.
    static Project save(const Project&, const std::filesystem::path&);
    static LoadResult load(const std::filesystem::path&);
    static void savePreset(const Preset&, const std::filesystem::path&);
    static Preset loadPreset(const std::filesystem::path&);
};
} // namespace reverie
