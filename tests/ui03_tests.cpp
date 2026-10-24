// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/Editor.h"
#include "ui/ExportDialog.h"
#include "ui/InstrumentArt.h"
#include "ui/SettingsDialog.h"
#include "ui/Timeline.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QTest>
#include <iostream>
using namespace reverie;
#define CHECK(...)                                                                                 \
    do {                                                                                           \
        if (!(__VA_ARGS__))                                                                        \
            throw std::runtime_error(#__VA_ARGS__);                                                \
    } while (false)
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-Tests03");
    app.setApplicationName("Ensemble-Tests");
    QSettings settings;
    settings.clear();
    try {
        Editor e(false);
        e.resize(1440, 900);
        e.show();
        QTest::qWait(50);
        auto* search = e.findChild<QLineEdit*>("soundSearch");
        auto* list = e.findChild<QListWidget*>("soundList");
        CHECK(search && list);
        search->setText("Hollowbody");
        CHECK(list->count() == 1);
        QTest::mouseClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                          list->visualItemRect(list->item(0)).center());
        QTest::mouseDClick(list->viewport(), Qt::LeftButton, Qt::NoModifier,
                           list->visualItemRect(list->item(0)).center());
        QTest::qWait(100);
        CHECK(e.project.tracks[e.selectedTrack].preset.family == "guitar");
        CHECK(e.project.tracks[e.selectedTrack].preset.calibrationVersion == 1);
        CHECK(e.findChild<QListWidget*>("patchList")->count() == 2);
        CHECK(e.findChild<QWidget*>("instrumentArtwork")->isVisible());
        CHECK(!list->item(0)->icon().isNull());
        auto* patches = e.findChild<QListWidget*>("patchList");
        auto point = patches->visualItemRect(patches->item(1)).center();
        QTest::mouseClick(patches->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QTest::mouseDClick(patches->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QTest::qWait(30);
        CHECK(e.project.tracks[e.selectedTrack].preset.patchId == "electric-guitar.staccato");
        CHECK(e.project.tracks[e.selectedTrack].preset.zones.size() == 4);
        CHECK(instrumentDrawing("grand", QSize(200, 110)).toImage() !=
              instrumentDrawing("upright", QSize(200, 110)).toImage());
        CHECK(instrumentDrawing("guitar", QSize(200, 110)).toImage() !=
              instrumentDrawing("bass", QSize(200, 110)).toImage());

        auto* timeline = e.timeline();
        double ppb = e.pixelsPerBeat;
        QPoint a(Timeline::header + int(ppb), 25), b(Timeline::header + int(3 * ppb), 25);
        QTest::mousePress(timeline, Qt::LeftButton, Qt::NoModifier, a);
        QTest::mouseMove(timeline, b);
        QTest::mouseRelease(timeline, Qt::LeftButton, Qt::NoModifier, b);
        CHECK(std::abs(TempoMap{e.project.bpm, e.engine.sampleRate()}.beatAt(
                           e.engine.displayPosition()) -
                       3) < .001);
        QTest::mousePress(timeline, Qt::LeftButton, Qt::ShiftModifier,
                          QPoint(Timeline::header + int(ppb), 4));
        QTest::mouseMove(timeline, QPoint(Timeline::header + int(4 * ppb), 4));
        QTest::mouseRelease(timeline, Qt::LeftButton, Qt::ShiftModifier,
                            QPoint(Timeline::header + int(4 * ppb), 4));
        CHECK(e.project.loopEnabled && e.project.loopStart == 1 && e.project.loopEnd == 4);
        e.undo();
        auto* position = e.findChild<QLineEdit*>("transportPosition");
        CHECK(position);
        position->setText("2:1:0");
        QTest::keyClick(position, Qt::Key_Return);
        CHECK(e.engine.displayPosition() ==
              TempoMap{e.project.bpm, e.engine.sampleRate()}.sampleAt(4));
        position->setText("0:12.5s");
        QTest::keyClick(position, Qt::Key_Return);
        CHECK(e.engine.displayPosition() == std::llround(12.5 * e.engine.sampleRate()));
        auto shots = QString::fromUtf8(REVERIE_TEST_ROOT) + "/out/0.3";
        QDir().mkpath(shots);
        for (auto size : {QSize(1440, 900), QSize(1280, 800)}) {
            e.resize(size);
            QTest::qWait(40);
            e.grab().save(shots +
                          QString("/arrangement-%1x%2.png").arg(size.width()).arg(size.height()));
        }
        e.showMixer();
        QTest::qWait(30);
        e.grab().save(shots + "/mixer.png");
        ExportDialog exportDialog(e.project, 0, 2, &e);
        exportDialog.show();
        exportDialog.findChild<QComboBox*>("exportRate")->setCurrentIndex(2);
        exportDialog.findChild<QComboBox*>("exportDepth")->setCurrentIndex(1);
        auto options = exportDialog.options();
        CHECK(options.sampleRate == 96000 && options.depth == WavDepth::Pcm24);
        exportDialog.grab().save(shots + "/export.png");
        auto path = shots + "/ui-export.wav";
        QFile::remove(path);
        exportDialog.findChild<QLineEdit*>("exportPath")->setText(path);
        exportDialog.startRender();
        for (int i = 0; i < 1000 && !QFile::exists(path); ++i)
            QTest::qWait(10);
        CHECK(QFile::exists(path));
        QTest::qWait(50);
        exportDialog.close();
        SettingsDialog dialog(e);
        dialog.show();
        dialog.findChild<QComboBox*>("settingsRate")->setCurrentText("96000");
        dialog.findChild<QComboBox*>("settingsBuffer")->setCurrentText("512");
        QTest::mouseClick(dialog.findChild<QPushButton*>("settingsApplyAudio"), Qt::LeftButton);
        CHECK(e.engine.sampleRate() == 96000);
        CHECK(QSettings().value("audio/buffer").toInt() == 512);
        dialog.grab().save(shots + "/settings.png");
        dialog.close();
        settings.clear();
        std::cout << "PASS: artwork/browser, calibrated apply, playhead and loop dragging, "
                     "position input, export options/render, settings persistence, screenshots\n";
        return 0;
    } catch (const std::exception& x) {
        settings.clear();
        std::cerr << "FAIL: " << x.what() << '\n';
        return 1;
    }
}
