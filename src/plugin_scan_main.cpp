// SPDX-License-Identifier: Apache-2.0
#include "core/Plugin.h"
#include "json.hpp"
#include <iostream>
int main(int argc, char** argv) try {
    if (argc != 2)
        return 2;
    nlohmann::json j = nlohmann::json::array();
    for (auto& i : reverie::scanClap(argv[1]))
        j.push_back({{"path", i.path},
                     {"id", i.id},
                     {"name", i.name},
                     {"vendor", i.vendor},
                     {"version", i.version},
                     {"instrument", i.instrument}});
    std::cout << j.dump() << '\n';
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
