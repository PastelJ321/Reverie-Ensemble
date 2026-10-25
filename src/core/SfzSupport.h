// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <filesystem>
#include <vector>
namespace reverie {
// Bounded offline estimate for literal envelopes/includes; macros and CC modulation may differ.
float sfzReleaseTail(const std::filesystem::path& file);
// Dependency scan for plain SFZ maps. Macro-generated paths are rejected explicitly.
std::vector<std::filesystem::path> sfzDependencies(const std::filesystem::path& file,
                                                   const std::filesystem::path& root);
} // namespace reverie
