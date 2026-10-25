// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
// Native Cocoa + actual miniaudio device; explicit developer probe, not a listening test.
#include "core/Library.h"
#include "ui/Editor.h"
#include "ui/PerformanceKeyboard.h"
#include "ui/SoundBrowser.h"
#include "ui/Timeline.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QSettings>
#include <QTest>
#include <iostream>
using namespace reverie;
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-Native04");
    app.setApplicationName("Ensemble");
    QSettings().clear();
    try {
        auto root = std::filesystem::path(REVERIE_TEST_ROOT);
        auto output = root / "out/native-performance04";
        std::filesystem::create_directories(output);
        qputenv("REVERIE_LIBRARY_ROOT",
                QByteArray::fromStdString((root / "out/community04").string()));
        Editor e(true);
        e.resize(1440, 900);
        e.show();
        auto* browser = e.findChild<SoundBrowser*>("soundBrowser");
        auto* keys = e.findChild<PerformanceKeyboard*>("performanceKeyboard");
        SampleCache cache;
        auto pack = readSoundPack(root / "out/community04/vcsl", false);
        auto it = std::find_if(pack.instruments.begin(), pack.instruments.end(), [](auto& i) {
            return i.file.filename() == "Grand Piano, Kawai.sfz";
        });
        if (it == pack.instruments.end())
            throw std::runtime_error("Full original Kawai mapping missing");
        auto preset = loadLibraryPreset(*it, pack, cache);
        e.edit([&] {
            e.project.tracks.clear();
            e.selectedTrack = -1;
            e.selectedClip = 0;
            e.project.name = "Paper Study · performed on Musical Typing";
        });
        e.applyInstrument(preset, true);
        e.armTrack(0, true);
        e.showKeyboard();
        QTest::qWait(100);
        keys->setFocus();
        e.findChild<QComboBox*>("countIn")->setCurrentIndex(1);
        e.toggleRecord();
        QTest::qWait(2200);
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        QTest::keyPress(keys, Qt::Key_D);
        QTest::keyPress(keys, Qt::Key_G);
        QTest::qWait(400);
        float measuredPeak = e.engine.masterPeak();
        e.grab().save(QString::fromStdString((output / "native-midi-recording.png").string()));
        QTest::keyPress(keys, Qt::Key_Tab);
        QTest::keyRelease(keys, Qt::Key_A);
        QTest::keyRelease(keys, Qt::Key_D);
        QTest::keyRelease(keys, Qt::Key_G);
        QTest::qWait(150);
        for (auto key : {Qt::Key_S, Qt::Key_F, Qt::Key_H, Qt::Key_J}) {
            QTest::keyPress(keys, key);
            QTest::qWait(180);
            QTest::keyRelease(keys, key);
        }
        QTest::keyClick(keys, Qt::Key_X);
        QTest::keyClick(keys, Qt::Key_V);
        QTest::keyPress(keys, Qt::Key_A);
        QTest::keyPress(keys, Qt::Key_D);
        QTest::qWait(400);
        QTest::keyRelease(keys, Qt::Key_A);
        QTest::keyRelease(keys, Qt::Key_D);
        QTest::keyRelease(keys, Qt::Key_Tab);
        QTest::qWait(200);
        e.stop();
        if (e.project.tracks[0].clips.empty() ||
            e.project.tracks[0].clips.back().notes.size() != 9 || measuredPeak <= 0)
            throw std::runtime_error("Native performance capture/audio failed");
        if (!e.saveProject(QString::fromStdString((output / "Paper Study.reverie").string())))
            throw std::runtime_error("Save failed");
        if (!e.openProject(QString::fromStdString((output / "Paper Study.reverie").string())))
            throw std::runtime_error("Reopen failed");
        if (!e.exportProject(QString::fromStdString((output / "Paper Study.wav").string())))
            throw std::runtime_error("Export failed");
        e.openPiano(e.project.tracks[0].clips[0].id);
        browser->setExpanded(true);
        QTest::qWait(100);
        browser->parentWidget()->grab().save(
            QString::fromStdString((output / "native-expanded-browser.png").string()));
        browser->setExpanded(false);
        e.grab().save(QString::fromStdString((output / "native-keyboard-wide.png").string()));
        e.resize(1280, 800);
        QTest::qWait(100);
        e.grab().save(QString::fromStdString((output / "native-keyboard-1280x800.png").string()));
        std::cout << e.audioDeviceReport()
                  << "\nPASS: native Cocoa Musical Typing → real sfizz Grand audio → 9 MIDI notes "
                     "/ sustain / count-in → collect/save → reopen → WAV; peak="
                  << measuredPeak << '\n';
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
