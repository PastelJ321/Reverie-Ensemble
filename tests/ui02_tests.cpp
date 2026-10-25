// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Media.h"
#include "ui/Editor.h"
#include "ui/PianoRoll.h"
#include "ui/SoundBrowser.h"
#include "ui/Timeline.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <array>
#include <iostream>
using namespace reverie;
void check(bool pass, const char* message) {
    if (!pass)
        throw std::runtime_error(message);
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("QuadraJ");
    QApplication::setApplicationName("Reverie Ensemble Tests");
    try {
        Editor e(false);
        e.resize(1440, 900);
        e.show();
        QTest::qWait(80);
        check(QApplication::font().family() == "IBM Plex Sans KR",
              "Plex typography was not loaded");
        auto* browser = e.findChild<SoundBrowser*>("soundBrowser");
        check(browser, "Browser missing");
        auto* search = e.findChild<QLineEdit*>("soundSearch");
        search->setText("concert flute");
        auto* list = e.findChild<QListWidget*>("soundList");
        check(list->count() == 1, "Sound name/tag filtering failed");
        browser->applyCurrent();
        check(e.project.tracks[0].preset.instrumentId == "quadraj.cc0.flute",
              "Recorded instrument not applied");
        browser->preview();
        std::array<float, 512> audio{};
        float sum = 0;
        for (int i = 0; i < 20; ++i) {
            e.engine.process(audio.data(), 256);
            for (auto f : audio)
                sum += f * f;
        }
        check(sum > 0 && e.engine.position() == 0, "Browser preview failed or moved transport");
        // Click piano keys and release before a callback: no stuck preview and no project mutation.
        auto before = e.project.tracks[0].clips[0].notes.size();
        QPoint key(25, PianoRoll::ruler + (PianoRoll::topKey - 60) * PianoRoll::keyHeight + 6);
        QTest::mousePress(e.piano(), Qt::LeftButton, Qt::NoModifier, key);
        e.engine.process(audio.data(), 256);
        QTest::mouseRelease(e.piano(), Qt::LeftButton, Qt::NoModifier, key);
        check(e.project.tracks[0].clips[0].notes.size() == before,
              "Keyboard audition edited MIDI data");
        auto* loop = e.findChild<QPushButton*>("loop");
        auto* metro = e.findChild<QPushButton*>("metronome");
        loop->click();
        metro->click();
        check(e.project.loopEnabled && e.project.metronome, "Transport tools not bound");
        // Recorded patch survives save/restore and drives the actual offline renderer.
        QTemporaryDir tmp;
        check(e.saveProject(tmp.path() + "/sample.reverie"), "Recorded save failed");
        check(e.openProject(tmp.path() + "/sample.reverie"), "Recorded restore failed");
        check(!e.project.tracks[0].preset.zones.empty(), "Mapping missing after restore");
        check(e.exportProject(tmp.path() + "/sample.wav"), "Recorded export failed");
        auto pcm = loadWav((tmp.path() + "/sample.wav").toStdString());
        float wav = 0;
        for (float v : pcm->stereo)
            wav += v * v;
        check(wav > 0, "Recorded offline export silent");
        search->clear();
        e.select(0, e.project.tracks[0].clips[0].id);
        e.openPiano(e.selectedClip);
        auto* sourceScroll =
            qobject_cast<QScrollArea*>(e.timeline()->parentWidget()->parentWidget());
        check(sourceScroll, "Arrangement scroll area missing");
        sourceScroll->horizontalScrollBar()->setValue(400);
        QTest::qWait(30);
        auto frozen =
            e.timeline()->grab(QRect(400, Timeline::ruler, Timeline::header, 70)).toImage();
        check(!frozen.isNull(), "Frozen track header draw failed");
        bool oldMute = e.project.tracks[0].mute;
        QTest::mouseClick(
            e.timeline(), Qt::LeftButton, Qt::NoModifier,
            QPoint(400 + Timeline::header - 60, Timeline::ruler + Timeline::row - 25));
        check(e.project.tracks[0].mute != oldMute,
              "Frozen header mute hit target moved with content");
        e.undo();
        e.select(0, e.project.tracks[0].clips[0].id);
        e.openPiano(e.selectedClip);
        sourceScroll->horizontalScrollBar()->setValue(0);
        auto* clip = findClip(e.project, e.selectedClip);
        auto originalCount = clip->notes.size();
        e.piano()->selection = {clip->notes[0].id, clip->notes[1].id};
        e.piano()->duplicate();
        check(findClip(e.project, e.selectedClip)->notes.size() == originalCount + 2,
              "Note duplicate command failed");
        e.undo();
        e.piano()->selection.clear();
        e.refresh();
        auto shots = QDir(QString::fromUtf8(REVERIE_TEST_ROOT) + "/out/0.2");
        QDir().mkpath(shots.path());
        for (const auto& size : {QSize(1440, 900), QSize(1280, 800)}) {
            e.resize(size);
            QTest::qWait(40);
            check(e.size() == size, "Panel minimums prevent requested window size");
            e.grab().save(shots.filePath(
                QString("arrangement-%1x%2.png").arg(size.width()).arg(size.height())));
        }
        e.piano()->parentWidget()->grab().save(shots.filePath("piano-roll.png"));
        e.resize(1440, 900);
        auto* categories = e.findChild<QListWidget*>("soundCategory");
        categories->setCurrentItem(categories->findItems("Strings", Qt::MatchExactly).front());
        QTest::qWait(20);
        check(list->count() >= 2, "Recorded strings category absent");
        e.grab().save(shots.filePath("sound-browser.png"));
        auto* tabs = e.findChild<QTabWidget*>("inspectorTabs");
        tabs->setCurrentIndex(1);
        e.findChild<QWidget*>("instrumentControls")->show();
        QTest::qWait(30);
        e.grab().save(shots.filePath("instrument-inspector.png"));
        e.showMixer();
        QTest::qWait(40);
        e.grab().save(shots.filePath("mixer.png"));
        std::cout << "PASS: Paper/Plex responsive UI, recorded browser preview/apply/save/export, "
                     "keyboard audition, loop/metronome binding, frozen header, screenshots\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
