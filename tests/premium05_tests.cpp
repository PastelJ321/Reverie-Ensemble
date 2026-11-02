// SPDX-License-Identifier: Apache-2.0
// Explicit network QA; not in offline CTest.
#include "core/InstrumentBackend.h"
#include "core/Library.h"
#include "ui/Editor.h"
#include "ui/SoundBrowser.h"
#include "ui/SoundLibraryManager.h"
#include <QtWidgets>
#include <iostream>
using namespace reverie;
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-PremiumQA05");
    app.setApplicationName("Ensemble");
    try {
        auto root = std::filesystem::path(REVERIE_TEST_ROOT);
        auto test = root / "out/premium05";
        std::filesystem::create_directories(test / "catalog");
        std::filesystem::copy_file(root / "assets/sounds/catalog/meta-piano.json",
                                   test / "catalog/meta-piano.json",
                                   std::filesystem::copy_options::overwrite_existing);
        qputenv("REVERIE_CATALOG_ROOT", QByteArray::fromStdString((test / "catalog").string()));
        qputenv("REVERIE_LIBRARY_ROOT", QByteArray::fromStdString((test / "installed").string()));
        Editor e(false);
        SoundLibraryManager manager(*e.findChild<SoundBrowser*>("soundBrowser"));
        manager.show();
        QTimer confirm;
        QObject::connect(&confirm, &QTimer::timeout, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                if (auto* yes = box->button(QMessageBox::Yes))
                    yes->click();
        });
        confirm.start(10);
        auto* b = manager.findChild<QPushButton*>("downloadSoundPack");
        if (!b)
            throw std::runtime_error("No installer control");
        b->click();
        auto pack = readSoundPack(test / "installed/meta-piano", true);
        SampleCache cache;
        auto preset = loadLibraryPreset(pack.instruments[0], pack, cache);
        auto synth = prepareBackend(preset, 48000);
        synth->setOffline(true);
        synth->noteOn(1, 60, .8, 0);
        std::array<float, 512> audio{};
        float peak = 0;
        for (int n = 0; n < 100; ++n) {
            synth->render(audio.data(), 256);
            for (auto x : audio)
                peak = std::max(peak, std::abs(x));
        }
        if (peak <= 0)
            throw std::runtime_error("Installed Opus piano silent");
        manager.grab().save(QString::fromStdString((test / "installed-meta-piano.png").string()));
        std::cout << "PASS actual HTTPS complete Meta Piano install/integrity/Opus render: "
                  << pack.installSize << " bytes peak=" << peak << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "FAIL " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
