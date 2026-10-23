// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include <functional>
namespace reverie {
std::shared_ptr<const Sample> loadWav(const std::filesystem::path&);
void exportWav(const Project&, const std::filesystem::path&, double rate = defaultRate,
               const std::function<void(double)>& progress = {});
void writeWav16(const std::filesystem::path&, const float* stereo, std::size_t frames,
                std::uint32_t rate);
} // namespace reverie
