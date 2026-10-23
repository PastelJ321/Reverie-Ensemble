// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Media.h"
#include "core/Serializer.h"
#include <iostream>
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--write-demo") {
            reverie::ProjectSerializer::save(reverie::demoProject(), argv[2]);
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--demo") {
            reverie::exportWav(reverie::demoProject(), argv[2]);
            return 0;
        }
        if (argc != 3) {
            std::cout << "Reverie Ensemble 0.1.0 offline renderer\nUsage: reverie-render "
                         "project.reverie output.wav\n       reverie-render --demo output.wav\n";
            return 2;
        }
        auto result = reverie::ProjectSerializer::load(argv[1]);
        for (const auto& w : result.warnings)
            std::cerr << w << '\n';
        reverie::exportWav(result.project, argv[2]);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
