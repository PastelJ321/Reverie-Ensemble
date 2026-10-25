// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "core/Media.h"
#include "ui/Editor.h"
#include "ui/PerformanceKeyboard.h"
#include "ui/PianoRoll.h"
#include "ui/SoundBrowser.h"
#include "ui/SoundLibraryManager.h"
#include "ui/Theme.h"
#include "ui/Timeline.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDir>
#include <QDockWidget>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QSettings>
#include <QTest>
#include <QTimer>
#include <iostream>
using namespace reverie;
static void check(bool x, const char* msg) {
    if (!x)
        throw std::runtime_error(msg);
}
static float pump(Editor& e, int blocks) {
    std::array<float, 512> out{};
    float peak = 0;
    for (int i = 0; i < blocks; ++i) {
        e.engine.process(out.data(), 256);
        for (float f : out)
            peak = std::max(peak, std::abs(f));
    }
    return peak;
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("QuadraJ-Tests04");
    app.setApplicationName("Ensemble");
    QSettings().clear();
    try {
        qputenv("REVERIE_LIBRARY_ROOT", QByteArray(REVERIE_TEST_ROOT) + "/out/testpacks04");
        Editor e(false);
        e.resize(1280, 800);
        e.show();
        QTest::qWait(250);
        auto* browser = e.findChild<SoundBrowser*>("soundBrowser");
        auto* keys = e.findChild<PerformanceKeyboard*>("performanceKeyboard");
        QString shots = QString::fromUtf8(REVERIE_TEST_ROOT) + "/out/0.4";
        QDir().mkpath(shots);
        e.grab().save(shots + "/compact-1280x800.png");
        e.resize(1440, 900);
        QTest::qWait(40);
        e.grab().save(shots + "/workspace-1440x900.png");
        e.resize(1280, 800);
        browser->setExpanded(true);
        QTest::qWait(40);
        browser->parentWidget()->grab().save(shots + "/expanded-browser.png");
        auto* search = e.findChild<QLineEdit*>("soundSearch");
        search->setText("Concert Grand");
        auto* list = e.findChild<QListWidget*>("soundList");
        check(list->count() == 1, "Grand search");
        browser->applyCurrent(true);
        check(e.project.tracks[e.selectedTrack].preset.family == "grand", "Grand actual patch");
        QTest::qWait(40);
        browser->parentWidget()->grab().save(shots + "/instrument-detail.png");
        browser->setExpanded(false);
        QTest::qWait(40);
        check(browser->parentWidget()->width() <= 300, "Compact browser releases expanded width");
        auto& t = e.project.tracks[e.selectedTrack];
        t.clips.clear();
        e.engine.publish(e.project);
        e.showKeyboard();
        e.armTrack(e.selectedTrack, true);
        QTest::qWait(50);
        pump(e, 1);
        auto* countIn = e.findChild<QComboBox*>("countIn");
        countIn->setCurrentIndex(1);
        e.engine.seek(0);
        pump(e, 1);
        e.toggleRecord();
        check(e.midiRecording(), "Arm then Record creates MIDI capture");
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        pump(e, 20);
        QTest::keyRelease(keys, Qt::Key_A); // count-in notes should be excluded
        pump(e, 360);
        QTest::keyPress(keys, Qt::Key_A);
        QTest::keyPress(keys, Qt::Key_D);
        QTest::keyPress(keys, Qt::Key_G);
        check(pump(e, 40) > 0, "Typing chord audio");
        e.grab().save(shots + "/midi-recording.png");
        QTest::keyPress(keys, Qt::Key_Tab);
        QTest::keyRelease(keys, Qt::Key_A);
        QTest::keyRelease(keys, Qt::Key_D);
        QTest::keyRelease(keys, Qt::Key_G);
        pump(e, 30);
        QTest::keyRelease(keys, Qt::Key_Tab);
        // Live external MIDI follows the same capture path, using the mock device queue.
        e.injectMidi({0, 0x92, 67, 100, 17});
        QTest::qWait(40);
        pump(e, 25);
        e.injectMidi({0, 0x82, 67, 0, 17});
        QTest::qWait(40);
        pump(e, 10);
        e.stop();
        QTest::qWait(40);
        check(!e.midiRecording(), "Stop ends capture");
        auto* clip = findClip(e.project, e.selectedClip);
        check(clip && clip->notes.size() == 4, "Chord + mock MIDI captured, count-in excluded");
        check(clip->notes[3].channel == 2, "MIDI channel capture");
        for (auto& n : clip->notes)
            check(n.duration > 0, "Positive note duration");
        check(!clip->controllers.empty(), "Sustain captured");
        e.grab().save(shots + "/musical-typing.png");
        keys->grab().save(shots + "/virtual-piano.png");
        // Text fields must not play or intercept typing. Focus loss must clear held notes.
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        pump(e, 2);
        search->setFocus();
        QTest::keyClicks(search, "asdf");
        check(search->text().contains("asdf"), "Text input is not intercepted by Musical Typing");
        QTest::keyRelease(keys, Qt::Key_A);
        check(pump(e, 2) == 0, "Focus loss automatically releases notes");
        search->clear();
        auto saved = QString::fromUtf8(REVERIE_TEST_ROOT) + "/out/0.4/Performed Grand.reverie";
        check(e.saveProject(saved), "Save recorded project");
        check(
            e.exportProject(QString::fromUtf8(REVERIE_TEST_ROOT) + "/out/0.4/Performed Grand.wav"),
            "Export recorded project");
        check(e.openProject(saved), "Reopen recording");
        check(e.project.tracks.back().clips.back().notes.size() == 4, "MIDI persists");
        // Octave and velocity controls, independent of editing shortcuts.
        keys->setFocus();
        QTest::keyClick(keys, Qt::Key_X);
        QTest::keyClick(keys, Qt::Key_V);
        QTest::keyPress(keys, Qt::Key_A);
        check(pump(e, 8) > 0, "New octave audio");
        QTest::keyRelease(keys, Qt::Key_A);
        e.releasePerformance();
        pump(e, 2);
        auto* channel = e.findChild<QComboBox*>("midiChannel");
        channel->setCurrentIndex(3);
        keys->setFocus();
        QTest::keyPress(keys, Qt::Key_A);
        check(pump(e, 8) > 0, "Virtual input routes to selected MIDI channel");
        QTest::keyRelease(keys, Qt::Key_A);
        e.releasePerformance();
        pump(e, 1);
        channel->setCurrentIndex(0);
        SoundLibraryManager manager(*browser);
        manager.show();
        QTest::qWait(40);
        manager.grab().save(shots + "/sound-library-manager.png");
        manager.close();
        if (fluidSynthAvailable()) {
            QTimer::singleShot(40, [&] {
                if (auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget())) {
                    dialog->grab().save(shots + "/soundfont-selection.png");
                    dialog->accept();
                }
            });
            browser->importInstrument(QString::fromUtf8(REVERIE_TEST_ROOT) +
                                      "/tests/fixtures/tone.sf2");
            check(e.project.tracks[e.selectedTrack].preset.kind == InstrumentKind::SoundFont,
                  "SoundFont selection applies real backend");
            check(QDir(soundPackPath() + "/.user-instruments")
                          .entryList({"*.json"}, QDir::Files)
                          .size() >= 2,
                  "User SoundFont programs persist");
            {
                Editor reopened(false);
                reopened.findChild<QLineEdit*>("soundSearch")->setText("tone");
                check(reopened.findChild<QListWidget*>("soundList")->count() == 1,
                      "Persistent user bank restored");
            }
        }
        // Restore recorded grand before exercising clip menus.
        check(e.openProject(saved), "Restore grand for context test");
        // Capture actual native Qt context menus and invoke Duplicate through the menu.
        e.select(int(e.project.tracks.size()) - 1, e.project.tracks.back().clips.back().id);
        auto* c = findClip(e.project, e.selectedClip);
        auto rect = e.timeline()->clipRect(e.selectedTrack, *c);
        QTimer::singleShot(30, [&] {
            if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
                menu->grab().save(shots + "/arrangement-context-menu.png");
                for (auto* a : menu->actions())
                    if (a->text() == "Duplicate") {
                        a->trigger();
                        break;
                    }
                menu->close();
            }
        });
        QContextMenuEvent context(QContextMenuEvent::Mouse, rect.center().toPoint(),
                                  e.timeline()->mapToGlobal(rect.center().toPoint()));
        QApplication::sendEvent(e.timeline(), &context);
        check(e.project.tracks.back().clips.size() == 2, "Context Duplicate works");
        e.openPiano(e.selectedClip);
        c = findClip(e.project, e.selectedClip);
        auto note = e.piano()->noteRect(c->notes[0]).center().toPoint();
        QTimer::singleShot(30, [&] {
            if (auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget())) {
                menu->grab().save(shots + "/piano-context-menu.png");
                menu->close();
            }
        });
        QTest::mouseClick(e.piano(), Qt::RightButton, Qt::NoModifier, note);
        check(c->notes.size() == 4, "Right click does not delete");
        std::cout
            << "PASS: browser apply, musical typing chord, count-in, live notes, mock external "
               "MIDI, MIDI capture/save/reopen/export, focus safety, actual context menus\n";
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
