// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/SfzSupport.h"
#include <algorithm>
#include <fstream>
#include <regex>
#include <set>
#include <stdexcept>
namespace reverie {
float sfzReleaseTail(const std::filesystem::path& file) {
    float seconds = 2.f;
    std::set<std::filesystem::path> visited;
    auto scan = [&](auto&& self, const std::filesystem::path& path) -> void {
        std::error_code ec;
        auto canonical = std::filesystem::weakly_canonical(path, ec);
        if (ec || visited.size() >= 128 || !visited.insert(canonical).second ||
            std::filesystem::file_size(canonical, ec) > 8 * 1024 * 1024 || ec)
            return;
        std::ifstream in(canonical);
        std::string text((std::istreambuf_iterator<char>(in)), {});
        text = std::regex_replace(text, std::regex(R"(//[^\r\n]*)"), "");
        std::regex release(R"(\bampeg_release\s*=\s*([0-9]+(?:\.[0-9]*)?))");
        for (std::sregex_iterator it(text.begin(), text.end(), release), end; it != end; ++it) {
            try {
                seconds = std::max(seconds, std::min(120.f, std::stof((*it)[1]) + .1f));
            } catch (const std::exception&) {
                // Malformed/huge literals remain the backend loader's responsibility.
            }
        }
        // One-shot samples can outlive note-off independently of the envelope.
        if (std::regex_search(text, std::regex(R"(\bloop_mode\s*=\s*one_shot\b)")))
            seconds = std::max(seconds, 30.f);
        std::regex includes("#include\\s*\"([^\"]+)\"");
        for (std::sregex_iterator it(text.begin(), text.end(), includes), end; it != end; ++it)
            self(self, canonical.parent_path() / (*it)[1].str());
    };
    scan(scan, file);
    return seconds;
}
std::vector<std::filesystem::path> sfzDependencies(const std::filesystem::path& file,
                                                   const std::filesystem::path& root) {
    auto base = std::filesystem::weakly_canonical(root);
    std::set<std::filesystem::path> files, visited;
    auto resolve = [&](std::filesystem::path path) {
        auto full = std::filesystem::weakly_canonical(path);
        auto rel = full.lexically_relative(base);
        if (rel.empty() || *rel.begin() == ".." || !std::filesystem::is_regular_file(full))
            throw std::runtime_error("SFZ missing or unsafe dependency: " + full.string());
        files.insert(full);
        return full;
    };
    auto trim = [](std::string value) {
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
            value.pop_back();
        std::replace(value.begin(), value.end(), '\\', '/');
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        return value;
    };
    auto scan = [&](auto&& self, std::filesystem::path path) -> void {
        path = resolve(path);
        if (!visited.insert(path).second)
            return;
        if (visited.size() > 128 || std::filesystem::file_size(path) > 8 * 1024 * 1024)
            throw std::runtime_error("SFZ dependency limit");
        std::ifstream in(path);
        std::string text((std::istreambuf_iterator<char>(in)), {});
        text = std::regex_replace(text, std::regex(R"(//[^\r\n]*)"), "");
        if (text.find("#define") != std::string::npos || text.find('$') != std::string::npos)
            throw std::runtime_error("SFZ macro paths require an audited mapping");
        auto sampleBase = path.parent_path();
        std::regex refs(
            R"(\b(default_path|sample)\s*=\s*([^\r\n<]*?)(?=\s+[a-zA-Z_0-9]+\s*=|[\r\n<]|$))");
        for (std::sregex_iterator it(text.begin(), text.end(), refs), end; it != end; ++it) {
            auto value = trim((*it)[2]);
            if ((*it)[1] == "default_path") {
                sampleBase = path.parent_path() / value;
                continue;
            }
            if (value.starts_with('*'))
                continue;
            resolve(sampleBase / value);
        }
        std::regex includes("#include\\s*\"([^\"]+)\"");
        for (std::sregex_iterator it(text.begin(), text.end(), includes), end; it != end; ++it)
            self(self, path.parent_path() / trim((*it)[1]));
    };
    scan(scan, file);
    return {files.begin(), files.end()};
}
} // namespace reverie
