// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "core/Model.h"
#include <vector>
namespace reverie {
class Editor;
std::vector<Preset> registeredPluginInstruments();
void showPlugins(Editor&, bool effects = false);
void showInserts(Editor&, int track);
void editPluginParameters(Editor&, Preset&, bool& accepted);
} // namespace reverie
