// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
// Explicit network integration executable; intentionally not part of offline CTest.
#include "core/InstrumentBackend.h"
#include "core/Library.h"
#include "core/SfzSupport.h"
#include "json.hpp"
#include "ui/Editor.h"
#include "ui/SoundBrowser.h"
#include "ui/SoundLibraryManager.h"
#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <fstream>
#include <iostream>
#include <set>
using namespace reverie;
using json = nlohmann::json;
static void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-Network04");
    app.setApplicationName("Ensemble");
    QSettings().clear();
    try {
        auto root = std::filesystem::path(REVERIE_TEST_ROOT);
        auto temp = root / "out/network04";
        std::filesystem::remove_all(temp);
        std::filesystem::create_directories(temp / "catalog");
        std::ifstream source(root / "out/community04/vcsl/pack.json");
        json j;
        source >> j;
        json selected;
        for (auto& i : j["instruments"])
            if (i["file"] == "Idiophones/Struck Idiophones/Claps.sfz")
                selected = i;
        check(!selected.empty(), "Claps original map missing");
        auto dependencies =
            sfzDependencies(root / "out/community04/vcsl" / selected["file"].get<std::string>(),
                            root / "out/community04/vcsl");
        std::set<std::string> paths;
        for (auto& d : dependencies)
            paths.insert(d.lexically_relative(root / "out/community04/vcsl").generic_string());
        paths.insert("README.md");
        auto all = j["files"];
        j["files"] = json::array();
        std::uint64_t size = 0;
        for (auto& f : all)
            if (paths.contains(f["path"].get<std::string>())) {
                j["files"].push_back(f);
                size += f["size"].get<std::uint64_t>();
            }
        j["id"] = "org.versilian.network-claps-test";
        j["name"] = "VCSL Claps · network integration test";
        j["instruments"] = json::array({selected});
        j["installSize"] = size;
        auto catalog = temp / "catalog/test.json";
        auto write = [&] { std::ofstream(catalog) << j.dump(2); };
        write();
        qputenv("REVERIE_CATALOG_ROOT", QByteArray::fromStdString((temp / "catalog").string()));
        qputenv("REVERIE_LIBRARY_ROOT", QByteArray::fromStdString((temp / "installed").string()));
        Editor editor(false);
        SoundLibraryManager manager(*editor.findChild<SoundBrowser*>("soundBrowser"));
        manager.show();
        QTimer confirm;
        QObject::connect(&confirm, &QTimer::timeout, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                if (auto* yes = box->button(QMessageBox::Yes))
                    yes->click();
        });
        confirm.start(10);
        auto* download = manager.findChild<QPushButton*>("downloadSoundPack");
        check(download, "Download button missing");
        download->click();
        auto installed = temp / "installed/org.versilian.network-claps-test";
        if (!std::filesystem::exists(installed / "pack.json")) {
            manager.grab().save(QString::fromStdString((temp / "failed.png").string()));
            std::cerr << manager.findChild<QLabel*>("libraryInstallStatus")->text().toStdString()
                      << "\n";
        }
        check(std::filesystem::exists(installed / "pack.json"), "Native HTTPS installation failed");
        auto pack = readSoundPack(installed, true);
        SampleCache cache;
        auto preset = loadLibraryPreset(pack.instruments[0], pack, cache);
        auto backend = prepareBackend(preset, 48000);
        backend->setOffline(true);
        backend->noteOn(1, 60, .8, 0);
        std::array<float, 512> audio{};
        float peak = 0;
        for (int b = 0; b < 20; ++b) {
            backend->render(audio.data(), 256);
            for (float v : audio)
                peak = std::max(peak, std::abs(v));
        }
        check(peak > 0, "Downloaded instrument produced no audio");
        auto* browser = editor.findChild<SoundBrowser*>("soundBrowser");
        editor.findChild<QLineEdit*>("soundSearch")->setText("Claps");
        browser->preview();
        float auditionPeak = 0;
        for (int b = 0; b < 30; ++b) {
            editor.engine.process(audio.data(), 256);
            for (float sample : audio)
                auditionPeak = std::max(auditionPeak, std::abs(sample));
        }
        check(auditionPeak > 0, "Installed non-GM percussion browser audition is audible");
        manager.grab().save(QString::fromStdString((temp / "installed.png").string()));
        // Update uses verified staging then atomic activation.
        j["version"] = "network-update";
        write();
        download->click();
        check(readSoundPack(installed, false).version == "network-update",
              "Update did not activate");
        // A bad hash must retain the prior working installation and remove staging.
        auto good = j;
        j["files"][0]["sha256"] = std::string(64, '0');
        write();
        download->click();
        check(readSoundPack(installed, false).version == "network-update",
              "Bad hash replaced installed pack");
        // Explicit cancel during an actual network request leaves no partial pack.
        j["files"] = all;
        j["installSize"] = 0;
        for (auto& f : all)
            j["installSize"] =
                j["installSize"].get<std::uint64_t>() + f["size"].get<std::uint64_t>();
        write();
        QTimer::singleShot(250, [&] {
            for (auto* button : manager.findChildren<QPushButton*>())
                if (button->text() == "Cancel download")
                    button->click();
        });
        download->click();
        for (auto& entry : std::filesystem::directory_iterator(temp / "installed"))
            check(!entry.path().filename().string().starts_with(".download"),
                  "Cancelled staging leaked");
        check(readSoundPack(installed, false).version == "network-update",
              "Cancel changed prior pack");
        auto* installedList = manager.findChild<QListWidget*>("installedSoundPacks");
        for (int i = 0; i < installedList->count(); ++i)
            if (!installedList->item(i)->data(Qt::UserRole).toString().isEmpty())
                installedList->setCurrentRow(i);
        for (auto* button : manager.findChildren<QPushButton*>())
            if (button->text() == "Remove optional pack…")
                button->click();
        check(!std::filesystem::exists(installed), "Optional removal failed");
        j = good;
        write();
        download->click();
        check(std::filesystem::exists(installed / "pack.json"), "Reinstall after removal failed");
        std::cout << "PASS: native HTTPS download/install, SHA-256, real downloaded sfizz audio, "
                     "update, corrupt hash rollback, cancel cleanup; bytes="
                  << size << " peak=" << peak << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
