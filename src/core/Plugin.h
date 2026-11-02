// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "core/Model.h"
#include <functional>
namespace reverie {
struct PluginInfo {
    std::string path, id, name, vendor, version;
    bool instrument = false;
};
struct PluginParameter {
    std::uint32_t id;
    std::string name;
    double minimum, maximum, value;
    bool stepped = false, readOnly = false;
};
// UI installs a synchronous main-thread dispatcher. CLI/scanner runs directly.
void shutdownPluginModules();
void setPluginMainDispatcher(std::function<void(std::function<void()>)>);
std::vector<PluginInfo> scanClap(const std::filesystem::path&);
class PluginInstance {
  public:
    PluginInstance(const Preset&, double rate);
    ~PluginInstance();
    PluginInstance(const PluginInstance&) = delete;
    std::vector<PluginParameter> parameters();
    void setParameter(std::uint32_t, double); // inactive editor instance, main thread
    std::vector<std::uint8_t> saveState();    // inactive editor instance, main thread
    void midi(int status, int a, int b, int offset = 0) noexcept;
    void process(float* stereo, unsigned frames) noexcept;
    void transport(std::int64_t frame, double rate, double bpm, int numerator = 4,
                   int denominator = 4, bool playing = true) noexcept;
    void reset() noexcept;
    void offline(bool) noexcept;
    unsigned latency() const;
    unsigned tail() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
} // namespace reverie
