// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Media.h"
#include "ui/Editor.h"
#include "ui/PianoRoll.h"
#include "ui/Timeline.h"
#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>
using namespace reverie;
static void require(bool value, const char* msg) {
    if (!value)
        throw std::runtime_error(msg);
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        Editor e(false);
        e.show();
        QTest::qWait(100);
        require(e.project.tracks.size() == 2, "demo tracks");
        e.addTrack(TrackKind::Midi);
        require(e.project.tracks.size() == 3, "add MIDI track");
        const int index = e.selectedTrack;
        QPoint clipPoint(Timeline::header + 32, Timeline::ruler + index * Timeline::row + 25);
        QTest::mouseDClick(e.timeline(), Qt::LeftButton, Qt::NoModifier, clipPoint);
        require(e.project.tracks[index].clips.size() == 1, "create clip via timeline");
        auto id = e.selectedClip;
        auto* c = findClip(e.project, id);
        require(c && c->notes.empty(), "new clip empty");
        e.openPiano(id);
        QPoint notePoint(PianoRoll::keys + 16,
                         PianoRoll::ruler + (PianoRoll::topKey - 60) * PianoRoll::keyHeight + 6);
        QTest::mouseClick(e.piano(), Qt::LeftButton, Qt::NoModifier, notePoint);
        require(findClip(e.project, id)->notes.size() == 1, "draw MIDI note via piano roll");
        e.undo();
        require(findClip(e.project, id)->notes.empty(), "undo note");
        e.redo();
        require(findClip(e.project, id)->notes.size() == 1, "redo note");
        e.piano()->setVelocity(.4);
        require(findClip(e.project, id)->notes[0].velocity == .4f, "velocity editing");
        auto before = findClip(e.project, id)->notes[0];
        auto rect = e.piano()->noteRect(before);
        QPoint start = rect.center().toPoint(), end = start + QPoint(64, -15);
        QTest::mousePress(e.piano(), Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(e.piano(), end, 10);
        QTest::mouseRelease(e.piano(), Qt::LeftButton, Qt::NoModifier, end);
        require(findClip(e.project, id)->notes[0].key == 61, "drag note pitch");
        auto moved = findClip(e.project, id)->notes[0];
        auto noteRect = e.piano()->noteRect(moved);
        start = QPoint(int(noteRect.right()) - 2, int(noteRect.center().y()));
        end = start + QPoint(32, 0);
        QTest::mousePress(e.piano(), Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(e.piano(), end, 10);
        QTest::mouseRelease(e.piano(), Qt::LeftButton, Qt::NoModifier, end);
        require(findClip(e.project, id)->notes[0].duration > moved.duration, "resize note");
        auto oldClip = *findClip(e.project, id);
        auto clipRect = e.timeline()->clipRect(index, oldClip);
        start = clipRect.center().toPoint();
        end = start + QPoint(64, 0);
        QTest::mousePress(e.timeline(), Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(e.timeline(), end, 10);
        QTest::mouseRelease(e.timeline(), Qt::LeftButton, Qt::NoModifier, end);
        require(findClip(e.project, id)->beat == oldClip.beat + 1, "move clip");
        clipRect = e.timeline()->clipRect(index, *findClip(e.project, id));
        start = QPoint(int(clipRect.right()) - 3, int(clipRect.center().y()));
        end = start + QPoint(64, 0);
        QTest::mousePress(e.timeline(), Qt::LeftButton, Qt::NoModifier, start);
        QTest::mouseMove(e.timeline(), end, 10);
        QTest::mouseRelease(e.timeline(), Qt::LeftButton, Qt::NoModifier, end);
        require(findClip(e.project, id)->length == oldClip.length + 1, "resize clip");
        e.findChild<QDoubleSpinBox*>("trackVolume")->setValue(.4);
        require(e.project.tracks[index].volume == .4f, "mixer model binding");
        e.applyInstrument(factoryPresets()[2]);
        require(e.project.tracks[index].preset.name == "Synth Lead", "instrument binding");
        e.duplicateClip();
        require(e.project.tracks[index].clips.size() == 2, "duplicate clip");
        e.deleteSelection();
        require(e.project.tracks[index].clips.size() == 1, "delete clip");
        e.undo();
        require(e.project.tracks[index].clips.size() == 2, "undo delete clip");
        e.setTempo(98);
        require(e.project.bpm == 98, "BPM binding");
        QTemporaryDir temp;
        auto file = temp.path() + "/ui.reverie";
        require(e.saveProject(file), "save from editor");
        auto wav = temp.path() + "/ui.wav";
        require(e.exportProject(wav), "export from editor");
        require(loadWav(wav.toStdString())->frames() > 48000, "rendered WAV length");
        const auto trackCount = e.project.tracks.size();
        e.addTrack(TrackKind::Audio);
        require(e.openProject(file), "open from editor");
        require(e.project.tracks.size() == trackCount && e.project.bpm == 98,
                "restore editor state");
        e.importWav(wav, 0);
        require(e.project.tracks.back().kind == TrackKind::Audio &&
                    e.project.tracks.back().clips[0].sample,
                "audio import from editor");
        require(e.saveProject(file), "save imported media");
        require(e.project.tracks.back().clips[0].mediaPath.find(".media") != std::string::npos,
                "rebind saved media reference");
        std::filesystem::rename(wav.toStdString(),
                                (temp.path() + "/original-moved.wav").toStdString());
        e.undo();
        e.redo();
        require(e.saveProject(file), "resave after source moved and history restored");
        require(e.openProject(file), "reopen collected media after source moved");
        e.grab().save(temp.path() + "/editor.png");
        AudioEngine engine;
        AudioDeviceManager device(engine);
        require(!device.startOutput(true) && !device.error().empty(), "unavailable audio handling");
        std::cout << "PASS: native editor note/clip gestures, undo/redo, mixer/preset/tempo "
                     "binding, save/open, import/export, unavailable audio\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return 1;
    }
}
