// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/Editor.h"
#include "ui/PerformanceKeyboard.h"
#include "ui/SoundBrowser.h"
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QProgressDialog>
#include <QSettings>
#include <QSignalBlocker>
#include <future>
#include <sstream>
namespace reverie {
void Editor::refreshPerformance() {
    if (selectedTrack < 0 || selectedTrack >= int(project.tracks.size()) ||
        project.tracks[selectedTrack].kind != TrackKind::Midi) {
        if (!performanceSignature_.empty()) {
            releasePerformance();
            performanceSignature_.clear();
        }
        return;
    }
    auto& t = project.tracks[selectedTrack];
    auto& p = t.preset;
    std::ostringstream key;
    key << t.id << p.name << int(p.kind) << p.backendPath << p.soundFontBank << p.soundFontProgram
        << p.patchId << p.attack << p.decay << p.sustain << p.release << p.cutoff << p.gain
        << p.calibrationGain << t.volume << t.pan << t.midiChannel << t.effects.enabled
        << t.effects.reverbMix << t.effects.delayMix << t.effects.delaySeconds << t.effects.feedback
        << t.effects.lowDb << t.effects.highDb;
    for (auto& z : p.zones)
        key << z.sample.get();
    if (key.str() == performanceSignature_)
        return;
    releasePerformance();
    try {
        if (p.kind == InstrumentKind::Sfizz || p.kind == InstrumentKind::SoundFont) {
            preparingInstrument_ = true;
            auto copy = t;
            copy.clips.clear();
            copy.mute = copy.solo = false;
            Project monitor;
            monitor.master = 1;
            monitor.tracks.push_back(copy);
            double rate = engine.sampleRate();
            // Preparation is kept out of the GUI and audio threads. The UI dispatcher is
            // temporarily gated while transport/output continues.
            auto job = std::async(std::launch::async, [monitor = std::move(monitor), rate] {
                return std::make_unique<AudioProcessingGraph>(monitor, rate);
            });
            while (job.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready)
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            engine.publishPerformance(job.get());
            preparingInstrument_ = false;
        } else
            engine.preparePerformance(t);
        performanceSignature_ = key.str();
    } catch (const std::exception& ex) {
        preparingInstrument_ = false;
        message(ex.what());
    }
}
void Editor::inputNote(int key, bool on, float velocity, int channel, int source,
                       std::int64_t frame) {
    if (preparingInstrument_)
        return;
    if (key < 0 || key > 127 || channel < 0 || channel > 15 || selectedTrack < 0 ||
        project.tracks[selectedTrack].kind != TrackKind::Midi)
        return;
    auto& t = project.tracks[selectedTrack];
    if (source < 3 && t.midiChannel >= 0)
        channel = t.midiChannel;
    else if (t.midiChannel >= 0 && t.midiChannel != channel)
        return;
    int slot = (source * 16 + channel) * 128 + key;
    Event e{frame < 0 ? engine.displayPosition() : frame, 0, key, velocity, on};
    e.channel = channel;
    if (on) {
        if (heldInput_.contains(slot))
            return;
        e.token = nextId();
        heldInput_[slot] = e.token;
    } else {
        auto it = heldInput_.find(slot);
        if (it == heldInput_.end())
            return;
        e.token = it->second;
        heldInput_.erase(it);
    }
    engine.performanceEvent(e);
    if (midiRecording())
        midiRecorder_.capture(e);
    if (keyboard_)
        keyboard_->showActivity(key, on);
    if (auto* label = findChild<QLabel*>("midiActivity"))
        label->setText(QString("MIDI %1 · Ch %2 · %3")
                           .arg(key)
                           .arg(channel + 1)
                           .arg(on ? "Note On" : "Note Off"));
}
void Editor::inputController(int cc, int value, int channel, std::int64_t frame, bool external) {
    if (preparingInstrument_)
        return;
    if (channel < 0 || channel > 15 || cc < 0 || cc > 128 || value < 0 ||
        value > (cc == 128 ? 16383 : 127))
        return;
    if (selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Midi)
        return;
    auto& t = project.tracks[selectedTrack];
    if (!external && t.midiChannel >= 0)
        channel = t.midiChannel;
    else if (t.midiChannel >= 0 && channel != t.midiChannel)
        return;
    Event e{frame < 0 ? engine.displayPosition() : frame, 0, 0, 0, false};
    e.channel = channel;
    e.controller = cc;
    e.value = value;
    engine.performanceEvent(e);
    if (midiRecording())
        midiRecorder_.capture(e);
}
void Editor::releasePerformance() {
    auditionToken_ = 0;
    auto frame = engine.displayPosition();
    if (midiRecording())
        for (auto [slot, token] : heldInput_) {
            Event e{frame, token, slot % 128, 0, false};
            e.channel = (slot / 128) % 16;
            midiRecorder_.capture(e);
        }
    if (keyboard_)
        for (auto [slot, token] : heldInput_) {
            (void)token;
            keyboard_->showActivity(slot % 128, false);
        }
    heldInput_.clear();
    if (midiRecording())
        for (int c = 0; c < 16; ++c) {
            Event e{frame, 0, 0, 0, false};
            e.channel = c;
            e.controller = 64;
            midiRecorder_.capture(e);
        }
    engine.panic();
}
void Editor::armTrack(int i, bool armed) {
    if (i < 0 || i >= int(project.tracks.size()))
        return;
    edit([&] {
        for (auto& t : project.tracks)
            t.armed = false;
        project.tracks[i].armed = armed;
    });
}
void Editor::showKeyboard() {
    auto* dock = findChild<QDockWidget*>("keyboardDock");
    if (!dock)
        return;
    dock->show();
    dock->raise();
    keyboard_->setTyping(true);
    keyboard_->setFocus();
}
void Editor::showBrowser() {
    if (browser_) {
        browser_->setExpanded(!browser_->expanded());
    }
}
void Editor::drainMidi() {
    if (midiInput_.takeDevicesChanged()) {
        releasePerformance();
        message("MIDI devices changed · notes released");
        if (auto* combo = findChild<QComboBox*>("midiInputDevice")) {
            auto selected = combo->currentData();
            QSignalBlocker block(combo);
            combo->clear();
            combo->addItem("All MIDI inputs", -1);
            combo->addItem("MIDI disabled", 0);
            for (auto& d : midiInput_.devices())
                combo->addItem(QString::fromStdString(d.name), d.id);
            int i = combo->findData(selected);
            combo->setCurrentIndex(i < 0 ? 0 : i);
        }
    }
    if (midiInput_.takeOverflow()) {
        releasePerformance();
        message("MIDI input overflow · notes released");
    }
    MidiMessage m;
    while (midiInput_.pop(m)) {
        if (selectedTrack < 0)
            continue;
        auto& t = project.tracks[selectedTrack];
        if (t.midiInput != "all" && t.midiInput != std::to_string(m.device))
            continue;
        int type = m.status & 0xf0, ch = m.status & 15;
        auto frame =
            std::max(std::int64_t(0),
                     engine.displayPosition() -
                         std::int64_t(midiInput_.secondsSince(m.hostTime) * engine.sampleRate()));
        auto source =
            midiSources_.try_emplace(m.device, int(midiSources_.size()) + 3).first->second;
        if (type == 0x90 || type == 0x80)
            inputNote(m.data1, type == 0x90 && m.data2 > 0, m.data2 / 127.f, ch, source, frame);
        if (type == 0xb0)
            inputController(m.data1, m.data2, ch, frame, true);
        if (type == 0xe0)
            inputController(128, m.data1 + (m.data2 << 7), ch, frame, true);
    }
}
} // namespace reverie
