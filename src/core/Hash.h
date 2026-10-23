// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <filesystem>
#include <string>
namespace reverie {
std::string sha256File(const std::filesystem::path&);
}
