// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "ui/Editor.h"
#include "core/Media.h"
#include "core/MidiFile.h"
#include "core/Serializer.h"
#include "ui/AudioEditor.h"
#include "ui/ExportDialog.h"
#include "ui/InspectorPanel.h"
#include "ui/InstrumentEditor.h"
#include "ui/MixerView.h"
#include "ui/PerformanceKeyboard.h"
#include "ui/PianoRoll.h"
#include "ui/SettingsDialog.h"
#include "ui/SoundBrowser.h"
#include "ui/Theme.h"
#include "ui/Timeline.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <chrono>
#include <fstream>
#include <future>
#include <unordered_map>
namespace reverie {
Editor::Editor(bool audio) : project(demoProject()), device_(engine), audioEnabled_(audio) {
    setObjectName("reverieEditor");
    initializeTheme();
    resize(1440, 900);
    setMinimumSize(1000, 650);
    QSettings prefs;
    trackHeight = std::clamp(prefs.value("interface/trackHeight", 82).toInt(), 62, 130);
    followPlayhead = prefs.value("interface/follow", false).toBool();
    buildUi();
    buildMenus();
    try {
        auto first = browser_->presetAt(0);
        auto second = browser_->presetAt(2);
        if (first.instrumentId != "quadraj.cc0.upright" ||
            second.instrumentId != "quadraj.cc0.strings")
            throw std::runtime_error("Community Essentials is not installed");
        project.tracks[0].preset = first;
        project.tracks[0].name = "Upright Piano";
        project.tracks[1].preset = second;
        project.tracks[1].name = "Violin Ensemble";
        for (auto& clip : project.tracks[1].clips)
            for (auto& note : clip.notes)
                note.key += 12;
    } catch (const std::exception& ex) {
        message(QString("Recorded starter unavailable; using factory synths: ") + ex.what());
    }
    if (audioEnabled_) {
        int rate = prefs.value("audio/rate", 48000).toInt();
        if (rate != 44100 && rate != 48000 && rate != 96000)
            rate = 48000;
        engine.setSampleRateStopped(rate);
        auto names = device_.outputDevices();
        int output = -1;
        for (int i = 0; i < int(names.size()); ++i)
            if (QString::fromStdString(names[i]) == prefs.value("audio/outputName").toString())
                output = i;
        unsigned frames = prefs.value("audio/buffer", 256).toUInt();
        if (frames != 128 && frames != 256 && frames != 512 && frames != 1024)
            frames = 256;
        device_.configure(output, frames);
        inputs_->clear();
        inputs_->addItem("System default");
        for (auto& name : device_.inputDevices())
            inputs_->addItem(QString::fromStdString(name));
        inputs_->setCurrentText(prefs.value("audio/inputName", "System default").toString());
    }
    publishProject();
    if (audioEnabled_ && !device_.startOutput())
        message(QString::fromStdString(device_.error()));
    connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
    timer_.start(33);
    connect(&midiTimer_, &QTimer::timeout, this, [this] { drainMidi(); });
    midiTimer_.setTimerType(Qt::PreciseTimer);
    midiTimer_.start(4);
    select(0, project.tracks[0].clips[0].id);
    openPiano(selectedClip);
    refresh();
}
Editor::~Editor() {
    releasePerformance();
    timer_.stop();
    midiTimer_.stop();
    device_.stopCapture();
    recorder_.stop();
    device_.stopOutput();
}
void Editor::buildMenus() {
    auto* file = menuBar()->addMenu("&File");
    auto action = [this](QMenu* menu, const QString& text, const QKeySequence& key,
                         std::function<void()> fn) {
        auto* a = menu->addAction(text);
        if (!key.isEmpty())
            a->setShortcut(key);
        connect(a, &QAction::triggered, this, std::move(fn));
        return a;
    };
    action(file, "New", QKeySequence::New, [this] { newProject(); });
    action(file, "Open…", QKeySequence::Open, [this] {
        if (!confirmUnsaved())
            return;
        auto name =
            QFileDialog::getOpenFileName(this, "Open project", {}, "Reverie project (*.reverie)");
        if (!name.isEmpty())
            openProject(name);
    });
    action(file, "Save", QKeySequence::Save, [this] { saveProject(); });
    action(file, "Save As…", QKeySequence::SaveAs, [this] {
        auto name = QFileDialog::getSaveFileName(this, "Save project as", {},
                                                 "Reverie project (*.reverie)");
        if (!name.isEmpty())
            saveProject(name);
    });
    action(file, "Project name…", {}, [this] {
        if (recording_)
            return;
        bool ok = false;
        auto name = QInputDialog::getText(this, "Project name", "Name", QLineEdit::Normal,
                                          QString::fromStdString(project.name), &ok);
        if (ok && !name.trimmed().isEmpty())
            edit([&] { project.name = name.trimmed().toStdString(); });
    });
    file->addSeparator();
    action(file, "Import WAV…", QKeySequence("Ctrl+I"), [this] { importWav(); });
    action(file, "Export Audio…", QKeySequence("Ctrl+Shift+E"), [this] { showExport(); });
    auto* settingsAction =
        action(file, "Settings…", QKeySequence::Preferences, [this] { showSettings(); });
    settingsAction->setMenuRole(QAction::PreferencesRole);
    action(file, "Quit", QKeySequence::Quit, [this] { close(); });
    action(file, "Import MIDI file…", {}, [this] {
        auto path = QFileDialog::getOpenFileName(this, "Import Standard MIDI File", {},
                                                 "MIDI (*.mid *.midi)");
        if (path.isEmpty())
            return;
        try {
            auto imported = importMidiFile(path.toStdString());
            edit([&] {
                if (project.tracks.size() + imported.tracks.size() > maxTracks)
                    throw std::runtime_error("Track limit exceeded");
                if (project.tracks.empty()) {
                    project.bpm = imported.bpm;
                    project.numerator = imported.numerator;
                    project.denominator = imported.denominator;
                }
                for (auto& t : imported.tracks)
                    project.tracks.push_back(std::move(t));
            });
        } catch (const std::exception& ex) {
            message(ex.what());
        }
    });
    auto* editMenu = menuBar()->addMenu("&Edit");
    action(editMenu, "Undo", QKeySequence::Undo, [this] { undo(); });
    action(editMenu, "Redo", QKeySequence::Redo, [this] { redo(); });
    action(editMenu, "Duplicate clip", QKeySequence("Ctrl+D"), [this] {
        if (piano_->hasFocus() && !piano_->selection.empty())
            piano_->duplicate();
        else
            duplicateClip();
    });
    action(editMenu, "Quantize notes", {}, [this] { piano_->quantize(); });
    auto* track = menuBar()->addMenu("&Track");
    action(track, "Add MIDI track", QKeySequence("Ctrl+Shift+M"),
           [this] { addTrack(TrackKind::Midi); });
    action(track, "Add audio track", QKeySequence("Ctrl+Shift+A"),
           [this] { addTrack(TrackKind::Audio); });
    action(track, "Delete selected track", {}, [this] {
        selectedClip = 0;
        deleteSelection();
    });
    action(track, "Move up", {}, [this] { moveTrack(-1); });
    action(track, "Move down", {}, [this] { moveTrack(1); });
    auto* transport = menuBar()->addMenu("&Transport");
    action(transport, "Play / pause", QKeySequence(Qt::Key_Space), [this] { play(); });
    action(transport, "Stop / return to start", QKeySequence(Qt::Key_Home), [this] { stop(); });
    action(track, "Arm selected track", QKeySequence("Ctrl+Shift+R"), [this] {
        if (selectedTrack >= 0)
            armTrack(selectedTrack, !project.tracks[selectedTrack].armed);
    });
    action(transport, "Record", QKeySequence(Qt::Key_R), [this] { toggleRecord(); });
    auto* view = menuBar()->addMenu("&View");
    action(view, "Virtual Keyboard / Musical Typing", QKeySequence("Ctrl+K"),
           [this] { showKeyboard(); });
    action(view, "Expand Instrument Browser", QKeySequence("Ctrl+Shift+B"),
           [this] { showBrowser(); });
    auto* help = menuBar()->addMenu("&Help");
    action(help, "Editing guide", {}, [this] {
        QMessageBox::information(
            this, "Making music in Reverie Ensemble",
            "1. Browse/search instruments and patches; Expand opens the category/instrument/detail "
            "columns.\n"
            "2. Keyboard (Cmd+K) opens the real selected-track piano. Enable Musical Typing: A W S "
            "E D F T G Y H U J K; Z/X octave, C/V velocity, Tab sustain.\n"
            "3. Arm a MIDI track, choose count-in, press Record and play. Stop creates an editable "
            "MIDI clip. Arm an Audio track for microphone capture.\n"
            "4. Double-click a clip to edit notes. Drag to move/resize; Shift-drag selects. "
            "Right-click opens commands; Delete removes selection.\n"
            "5. Use Mixer, Save with the media folder, and File → Export Audio.\n"
            "6. Sound Library Manager installs optional packs with license consent and SHA-256 "
            "verification. Import SFZ/SF2/SF3 through browser tools.\n\n"
            "Recording is linear, one armed track at a time. Audio input monitoring and latency "
            "compensation are not provided.");
    });
    action(help, "About Reverie Ensemble", {}, [this] {
        QMessageBox::about(
            this, "Reverie Ensemble 0.4.0",
            "<h2>Reverie Ensemble</h2><p>0.4.0 · QuadraJ</p><p>Copyright 2026 QuadraJ and "
            "contributors.</p><p>A free, offline, open-source digital audio "
            "workstation.</p><p>Reverie code: Apache License 2.0.<br>Qt Core / Gui / Widgets / "
            "Network / SVG: "
            "dynamically linked under LGPLv3.<br>miniaudio 0.11.23: MIT-0.<br>nlohmann/json "
            "3.11.3: MIT.<br>sfizz: BSD-2-Clause.<br>FluidSynth: "
            "LGPL-2.1-or-later.</p><p>Third-party components retain their own licenses. Source, "
            "relinking and distribution instructions are in THIRD_PARTY_LICENSES.md.</p>");
    });
}
void Editor::message(const QString& msg) {
    statusBar()->showMessage(msg, 12000);
}
void Editor::beginEdit() {
    if (recording_) {
        return;
    }
    if (transaction_)
        return;
    undo_.push_back(project);
    if (undo_.size() > 80)
        undo_.erase(undo_.begin());
    redo_.clear();
    transaction_ = true;
}
void Editor::endEdit() {
    if (!transaction_)
        return;
    transaction_ = false;
    try {
        validate(project);
        publishProject();
        dirty_ = true;
        refresh();
    } catch (const std::exception& ex) {
        if (!undo_.empty()) {
            project = undo_.back();
            undo_.pop_back();
        }
        refresh();
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::edit(const std::function<void()>& fn) {
    if (recording_) {
        message("Finish recording before editing the project.");
        return;
    }
    beginEdit();
    try {
        fn();
        endEdit();
    } catch (const std::exception& ex) {
        if (transaction_ && !undo_.empty()) {
            project = std::move(undo_.back());
            undo_.pop_back();
            transaction_ = false;
        }
        refresh();
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::refresh() {
    refreshing_ = true;
    if (auto* loop = findChild<QPushButton*>("loop"))
        loop->setChecked(project.loopEnabled);
    if (auto* click = findChild<QPushButton*>("metronome"))
        click->setChecked(project.metronome);
    if (auto* start = findChild<QDoubleSpinBox*>("loopStart"))
        start->setValue(project.loopStart);
    if (auto* end = findChild<QDoubleSpinBox*>("loopEnd"))
        end->setValue(project.loopEnd);
    if (auto* level = findChild<QDoubleSpinBox*>("clickLevel"))
        level->setValue(project.clickLevel);
    projectLabel_->setText(QString::fromStdString(project.name));
    projectLabel_->setToolTip(QString::fromStdString(project.name));
    selectedTrack =
        project.tracks.empty() ? -1 : std::clamp(selectedTrack, 0, int(project.tracks.size()) - 1);
    if (!findClip(project, selectedClip))
        selectedClip = 0;
    bool valid = selectedTrack >= 0;
    bool midi = valid && project.tracks[selectedTrack].kind == TrackKind::Midi;
    for (QWidget* w : std::initializer_list<QWidget*>{volume_, pan_, mute_, solo_})
        w->setEnabled(valid && !recording_);
    for (QWidget* w :
         std::initializer_list<QWidget*>{wave_, attack_, decay_, sustain_, release_, cutoff_})
        w->setEnabled(midi && !recording_);
    if (bottomStack_)
        bottomStack_->setCurrentIndex(valid && !midi ? 1 : 0);
    if (audioEditor_)
        audioEditor_->update();
    if (browser_)
        browser_->setEnabled(!recording_);
    piano_->setEnabled(midi && !recording_);
    timeline_->setEnabled(!recording_);
    bpm_->setEnabled(!recording_);
    bpm_->setValue(project.bpm);
    master_->setValue(project.master);
    auto* sig = findChild<QComboBox*>("timeSignature");
    QString text = QString("%1/%2").arg(project.numerator).arg(project.denominator);
    if (sig->findText(text) < 0)
        sig->addItem(text);
    sig->setCurrentText(text);
    sig->setEnabled(!recording_);
    if (valid) {
        const auto& t = project.tracks[selectedTrack];
        if (auto* channel = findChild<QComboBox*>("midiChannel")) {
            QSignalBlocker block(channel);
            channel->setCurrentIndex(channel->findData(t.midiChannel));
            channel->setEnabled(!recording_);
        }
        if (auto* input = findChild<QComboBox*>("midiInputDevice")) {
            QSignalBlocker block(input);
            int device = t.midiInput == "all" ? -1 : QString::fromStdString(t.midiInput).toInt();
            input->setCurrentIndex(std::max(0, input->findData(device)));
            input->setEnabled(!recording_);
        }
        trackTitle_->setText(QString::fromStdString(t.name));
        volume_->setValue(t.volume);
        pan_->setValue(t.pan);
        mute_->setChecked(t.mute);
        solo_->setChecked(t.solo);
        auto& s = t.preset;
        bool external = s.kind == InstrumentKind::Sfizz || s.kind == InstrumentKind::SoundFont;
        if (auto* controls = findChild<QWidget*>("instrumentControls"))
            controls->setEnabled(!external);

        wave_->setCurrentIndex(int(s.wave));
        if (auto* form = qobject_cast<QFormLayout*>(wave_->parentWidget()->layout()))
            form->setRowVisible(wave_, s.kind == InstrumentKind::Synth);
        attack_->setValue(s.attack);
        decay_->setValue(s.decay);
        sustain_->setValue(s.sustain);
        release_->setValue(s.release);
        cutoff_->setValue(s.cutoff);
        instrumentLabel_->setText(
            midi ? QString::fromStdString(s.name) +
                       (s.kind == InstrumentKind::Sampler
                            ? (s.zones.empty()
                                   ? QString("\nRoot: C4 (MIDI 60) · ")
                                   : QString("\n%1 key/velocity zones · ").arg(s.zones.size())) +
                                  QString::fromStdString(s.license)
                            : "")
                 : "Audio track · non-destructive WAV playback");
    } else {
        trackTitle_->setText("No track selected");
        instrumentLabel_->clear();
    }
    if (auto* arm = findChild<QCheckBox*>("trackArm"))
        arm->setChecked(valid && project.tracks[selectedTrack].armed);
    if (auto* controls = findChild<QWidget*>("smartControls"))
        static_cast<InstrumentEditor*>(controls)->refresh();
    timeline_->updateExtent();
    piano_->updateExtent();
    if (selectionInspector_)
        selectionInspector_->refresh();
    setWindowTitle(QString::fromStdString(project.name) + (dirty_ ? " *" : "") +
                   " — Reverie Ensemble 0.4");
    refreshing_ = false;
    refreshPerformance();
    if (keyboard_)
        keyboard_->refresh();
}
void Editor::select(int track, Id clip) {
    if (recording_ && track != selectedTrack) {
        message("Stop the take before changing tracks.");
        return;
    }
    if (track != selectedTrack)
        releasePerformance();
    if (selectedClip != clip)
        piano_->selection.clear();
    if (auto* tabs = findChild<QTabWidget*>("inspectorTabs"))
        tabs->setCurrentIndex(clip ? 0 : 1);
    selectedTrack = track;
    if (track >= 0 && track < int(project.tracks.size())) {
        midiInput_.select(-1);
    }
    selectedClip = clip;
    refresh();
}
void Editor::addTrack(TrackKind kind) {
    if (project.tracks.size() >= maxTracks) {
        message("0.1 supports up to 64 tracks.");
        return;
    }
    edit([&] {
        project.tracks.push_back(makeTrack(kind, int(project.tracks.size())));
        selectedTrack = int(project.tracks.size()) - 1;
        selectedClip = 0;
    });
}
void Editor::deleteSelection() {
    if (selectedTrack < 0)
        return;
    edit([&] {
        if (selectedClip) {
            auto& clips = project.tracks[selectedTrack].clips;
            std::erase_if(clips, [&](const Clip& c) { return c.id == selectedClip; });
            selectedClip = 0;
        } else
            project.tracks.erase(project.tracks.begin() + selectedTrack);
    });
}
void Editor::duplicateClip() {
    auto* source = findClip(project, selectedClip);
    if (!source || selectedTrack < 0)
        return;
    Clip copy = *source;
    copy.id = nextId();
    copy.beat += copy.length;
    for (auto& n : copy.notes)
        n.id = nextId();
    edit([&] {
        selectedClip = copy.id;
        project.tracks[selectedTrack].clips.push_back(copy);
    });
}
void Editor::moveTrack(int delta) {
    int dest = selectedTrack + delta;
    if (selectedTrack < 0 || dest < 0 || dest >= int(project.tracks.size()))
        return;
    edit([&] {
        std::swap(project.tracks[selectedTrack], project.tracks[dest]);
        selectedTrack = dest;
    });
}
void Editor::undo() {
    if (recording_ || transaction_ || undo_.empty())
        return;
    redo_.push_back(project);
    project = std::move(undo_.back());
    undo_.pop_back();
    dirty_ = true;
    publishProject();
    refresh();
}
void Editor::redo() {
    if (recording_ || transaction_ || redo_.empty())
        return;
    undo_.push_back(project);
    project = std::move(redo_.back());
    redo_.pop_back();
    dirty_ = true;
    publishProject();
    refresh();
}
void Editor::setTempo(double bpm) {
    const double beat = TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.displayPosition());
    edit([&] { project.bpm = bpm; });
    engine.seek(TempoMap{project.bpm, engine.sampleRate()}.sampleAt(beat));
}
void Editor::play() {
    if (recording_) {
        finishRecord();
        return;
    }
    if (audioEnabled_ && !device_.outputRunning()) {
        message("No output device. Use Reconnect audio; offline WAV export is available.");
        return;
    }
    engine.setPlaying(!engine.playing());
}
void Editor::stop() {
    engine.releaseAudition();
    releasePerformance();
    if (recording_)
        finishRecord();
    engine.setPlaying(false);
    engine.seek(0);
}
void Editor::openPiano(Id clip) {
    selectedClip = clip;
    refresh();
    if (auto* c = findClip(project, clip)) {
        int center = 60;
        if (!c->notes.empty())
            center = c->notes[0].key;
        pianoScroll_->verticalScrollBar()->setValue(
            PianoRoll::ruler + (PianoRoll::topKey - center) * PianoRoll::keyHeight - 110);
    }
    piano_->setFocus();
}
void Editor::tick() {
    engine.collect();
    if (midiRecording()) {
        auto frame = engine.displayPosition();
        midiRecorder_.updatePosition(frame);
        auto start = TempoMap{project.bpm, engine.sampleRate()}.sampleAt(recordingBeat_);
        recordButton_->setText(frame < start
                                   ? QString("Count-in %1")
                                         .arg(int(std::ceil((start - frame) / engine.sampleRate() *
                                                            project.bpm / 60.)))
                                   : "Finish MIDI");
    }
    auto beat = TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.displayPosition());
    double bar = project.numerator * 4. / project.denominator;
    if (!position_->hasFocus())
        position_->setText(QString("%1 : %2 : %3")
                               .arg(int(beat / bar) + 1, 3, 10, QChar('0'))
                               .arg(int(std::fmod(beat, bar) / (4. / project.denominator)) + 1)
                               .arg(int(std::fmod(beat / (4. / project.denominator), 1.) * 960), 3,
                                    10, QChar('0')));
    if (!position_->hasFocus() && timeDisplaySeconds) {
        double seconds = double(engine.displayPosition()) / engine.sampleRate();
        position_->setText(QString("%1:%2s")
                               .arg(int(seconds / 60), 2, 10, QChar('0'))
                               .arg(std::fmod(seconds, 60), 6, 'f', 3, QChar('0')));
    }
    playButton_->setText(engine.playing() ? "Pause" : "Play");
    if (auto* label = findChild<QLabel*>("transportDeviceStatus"))
        label->setText(device_.outputRunning()
                           ? QString::number(int(engine.sampleRate() / 1000.)) + " kHz · active"
                           : "Offline audio");
    output_->setText(QString(device_.outputRunning() ? "Audio active · reported format in Settings"
                                                     : "Offline audio · export available") +
                     (engine.clipped() ? "\nCLIPPING — lower track/master levels" : ""));
    timeline_->update();
    playButton_->setIcon(toolIcon(engine.playing() ? "pause" : "play"));
    if (followPlayhead && engine.playing() && !timeline_->interacting() &&
        QApplication::mouseButtons() == Qt::NoButton) {
        int x = Timeline::header + int(beat * pixelsPerBeat);
        auto* bar = timelineScroll_->horizontalScrollBar();
        if (x > bar->value() + timelineScroll_->viewport()->width() - 50 ||
            x < bar->value() + Timeline::header)
            bar->setValue(std::max(0, x - Timeline::header - 80));
    }
    if (recording_ && !midiRecording_ &&
        (recorder_.failed() || recorder_.overflowed() || !engine.playing())) {
        bool failed = recorder_.failed(), overflow = recorder_.overflowed();
        finishRecord();
        if (failed || overflow)
            message(failed
                        ? "Recording disk write failed; partial WAV retained."
                        : "Recording buffer overflow; partial WAV retained. Reduce system load.");
    }
}
// Long decoding, copying and rendering run off the UI/audio threads. The modal progress dialog
// keeps transport meters alive while preventing project mutations during an operation.
template <class F> static auto background(QWidget* owner, const QString& label, F fn) {
    QProgressDialog progress(label, QString(), 0, 0, owner);
    progress.setCancelButton(nullptr);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.show();
    auto future = std::async(std::launch::async, std::move(fn));
    while (future.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready)
        QApplication::processEvents();
    return future.get();
}
void Editor::publishProject() {
    bool external = std::any_of(project.tracks.begin(), project.tracks.end(), [](auto& t) {
        return t.preset.kind == InstrumentKind::Sfizz || t.preset.kind == InstrumentKind::SoundFont;
    });
    if (!external) {
        engine.publish(project);
        return;
    }
    auto snapshot = project;
    double rate = engine.sampleRate();
    releasePerformance();
    preparingInstrument_ = true;
    try {
        auto graph =
            background(this, "Preparing instrument audio…", [snapshot = std::move(snapshot), rate] {
                return std::make_unique<AudioProcessingGraph>(snapshot, rate);
            });
        engine.publishPrepared(std::move(graph));
        preparingInstrument_ = false;
    } catch (...) {
        preparingInstrument_ = false;
        throw;
    }
}
void Editor::importWav(const QString& supplied, double beat) {
    if (recording_)
        return;
    auto path = supplied;
    if (path.isEmpty())
        path = QFileDialog::getOpenFileName(this, "Import WAV", {}, "WAV (*.wav)");
    if (path.isEmpty())
        return;
    try {
        auto sample = background(this, "Decoding WAV and caching waveform…",
                                 [path] { return loadWav(path.toStdString()); });
        edit([&] {
            if (selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Audio) {
                if (project.tracks.size() >= maxTracks)
                    throw std::runtime_error("Track limit reached");
                project.tracks.push_back(makeTrack(TrackKind::Audio, int(project.tracks.size())));
                selectedTrack = int(project.tracks.size()) - 1;
            }
            Clip c;
            c.name = std::filesystem::path(path.toStdString()).stem().string();
            c.sample = sample;
            c.mediaPath = sample->path;
            c.beat = beat >= 0
                         ? beat
                         : std::max(0., snapped(TempoMap{project.bpm, engine.sampleRate()}.beatAt(
                                            engine.position())));
            c.length = double(sample->frames()) / sample->sampleRate * project.bpm / 60.;
            selectedClip = c.id;
            project.tracks[selectedTrack].clips.push_back(c);
        });
        message("WAV imported. Drag the clip to position; its right edge trims non-destructively.");
    } catch (const std::exception& ex) {
        message(QString::fromUtf8(ex.what()));
    }
}
bool Editor::saveProject(const QString& supplied) {
    if (recording_)
        finishRecord();
    QString path = supplied.isEmpty() ? currentPath_ : supplied;
    if (path.isEmpty())
        path =
            QFileDialog::getSaveFileName(this, "Save project", {}, "Reverie project (*.reverie)");
    if (path.isEmpty())
        return false;
    if (!path.endsWith(".reverie", Qt::CaseInsensitive))
        path += ".reverie";
    try {
        auto snapshot = project;
        auto collected = background(this, "Saving project and collecting media…", [snapshot, path] {
            return ProjectSerializer::save(snapshot, path.toStdString());
        });
        // Saving establishes durable references without copying or replacing shared PCM.
        std::unordered_map<std::string, std::string> refs;
        for (std::size_t i = 0; i < snapshot.tracks.size(); ++i) {
            const auto& before = snapshot.tracks[i];
            const auto& after = collected.tracks[i];
            if (!before.preset.backendPath.empty())
                refs[before.preset.backendPath] = after.preset.backendPath;
            auto sampleSource =
                !before.preset.samplePath.empty()
                    ? before.preset.samplePath
                    : (before.instrumentSample ? before.instrumentSample->path : "");
            if (!sampleSource.empty())
                refs[sampleSource] = after.preset.samplePath;
            for (std::size_t z = 0; z < before.preset.zones.size(); ++z)
                refs[before.preset.zones[z].path] = after.preset.zones[z].path;
            for (std::size_t j = 0; j < before.clips.size(); ++j) {
                const auto& c = before.clips[j];
                auto source = !c.mediaPath.empty() ? c.mediaPath : (c.sample ? c.sample->path : "");
                if (!source.empty())
                    refs[source] = after.clips[j].mediaPath;
            }
        }
        auto rebase = [&](Project& state) {
            for (auto& t : state.tracks) {
                if (refs.contains(t.preset.backendPath))
                    t.preset.backendPath = refs.at(t.preset.backendPath);
                auto source = !t.preset.samplePath.empty()
                                  ? t.preset.samplePath
                                  : (t.instrumentSample ? t.instrumentSample->path : "");
                if (refs.contains(source))
                    t.preset.samplePath = refs.at(source);
                for (auto& z : t.preset.zones)
                    if (refs.contains(z.path))
                        z.path = refs.at(z.path);
                for (auto& c : t.clips) {
                    source = !c.mediaPath.empty() ? c.mediaPath : (c.sample ? c.sample->path : "");
                    if (refs.contains(source))
                        c.mediaPath = refs.at(source);
                }
            }
        };
        for (auto& state : undo_)
            rebase(state);
        for (auto& state : redo_)
            rebase(state);
        project = std::move(collected);
        publishProject();
        performanceSignature_.clear();
        currentPath_ = path;
        dirty_ = false;
        refresh();
        bool referenced = false;
        for (auto& t : project.tracks)
            if (!t.preset.backendPath.empty() &&
                t.preset.backendPath.find(
                    (std::filesystem::path(path.toStdString()).stem().string() + ".media")) ==
                    std::string::npos)
                referenced = true;
        message(referenced ? "Project saved. Keep external SFZ libraries available; some "
                             "dependencies remain external references."
                           : "Project saved with a portable media folder.");
        return true;
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Save failed", QString::fromUtf8(ex.what()));
        return false;
    }
}
bool Editor::openProject(const QString& path) {
    if (recording_)
        finishRecord();
    try {
        auto loaded = background(this, "Opening project and media…",
                                 [path] { return ProjectSerializer::load(path.toStdString()); });
        engine.setPlaying(false);
        engine.seek(0);
        project = std::move(loaded.project);
        currentPath_ = path;
        dirty_ = false;
        undo_.clear();
        redo_.clear();
        selectedTrack = 0;
        selectedClip = 0;
        publishProject();
        refresh();
        if (!loaded.warnings.empty()) {
            QString text = "Some media is unavailable. References are preserved and those clips "
                           "play silently.\n\n";
            for (const auto& w : loaded.warnings)
                text += QString::fromStdString(w) + "\n";
            QMessageBox::warning(this, "Missing media", text);
        }
        return true;
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Open failed", QString::fromUtf8(ex.what()));
        return false;
    }
}
void Editor::showSettings() {
    if (recording_) {
        message("Stop recording before opening device settings.");
        return;
    }
    SettingsDialog dialog(*this);
    dialog.exec();
}
void Editor::showExport() {
    if (recording_)
        finishRecord();
    auto* c = findClip(project, selectedClip);
    ExportDialog dialog(project, c ? c->beat : 0, c ? c->beat + c->length : 0, this);
    dialog.exec();
}
bool Editor::exportProject(const QString& supplied) {
    if (recording_)
        finishRecord();
    try {
        QString path = supplied;
        if (!path.endsWith(".wav", Qt::CaseInsensitive))
            path += ".wav";
        auto snapshot = project;
        background(this, "Rendering stereo WAV at 48 kHz / 16 bit…",
                   [snapshot, path] { exportWav(snapshot, path.toStdString()); });
        message("WAV exported with at least two seconds of release tail.");
        return true;
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, "Export failed", QString::fromUtf8(ex.what()));
        return false;
    }
}
void Editor::loadSampler() {
    if (recording_ || selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Midi) {
        message("Select a MIDI track for the sampler.");
        return;
    }
    auto path =
        QFileDialog::getOpenFileName(this, "Select your licensed sampler WAV", {}, "WAV (*.wav)");
    if (path.isEmpty())
        return;
    try {
        auto sample = background(this, "Loading pitched sampler…",
                                 [path] { return loadWav(path.toStdString()); });
        edit([&] {
            auto& t = project.tracks[selectedTrack];
            t.preset = factoryPresets()[7];
            t.preset.kind = InstrumentKind::Sampler;
            t.preset.name = "Sampler: " + std::filesystem::path(path.toStdString()).stem().string();
            t.preset.samplePath = sample->path;
            t.preset.source = "User supplied WAV";
            t.preset.license = "User-managed; verify before redistribution";
            t.preset.rootNote = 60;
            t.instrumentSample = sample;
        });
    } catch (const std::exception& ex) {
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::loadPreset() {
    if (recording_ || selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Midi)
        return;
    auto path = QFileDialog::getOpenFileName(this, "Load instrument preset", {},
                                             "Reverie instrument (*.json)");
    if (path.isEmpty())
        return;
    try {
        auto preset = ProjectSerializer::loadPreset(path.toStdString());
        std::shared_ptr<const Sample> sample;
        if (preset.kind == InstrumentKind::Sampler && !preset.samplePath.empty())
            sample = background(this, "Loading preset sample…",
                                [preset] { return loadWav(preset.samplePath); });
        edit([&] {
            auto& t = project.tracks[selectedTrack];
            t.preset = preset;
            t.instrumentSample = sample;
        });
    } catch (const std::exception& ex) {
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::savePreset() {
    if (selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Midi)
        return;
    auto path = QFileDialog::getSaveFileName(this, "Save instrument preset", {},
                                             "Reverie instrument (*.json)");
    if (path.isEmpty())
        return;
    if (!path.endsWith(".json"))
        path += ".json";
    try {
        ProjectSerializer::savePreset(project.tracks[selectedTrack].preset, path.toStdString());
        message("Preset saved. Sampler WAV references remain separate from preset data.");
    } catch (const std::exception& ex) {
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::toggleRecord() {
    if (recording_) {
        finishRecord();
        return;
    }
    if (audioEnabled_ && !device_.outputRunning()) {
        message("Connect an output device before recording.");
        return;
    }
    int armed = -1;
    for (int i = 0; i < int(project.tracks.size()); ++i)
        if (project.tracks[i].armed) {
            if (armed >= 0) {
                message("Arm one recording track at a time.");
                return;
            }
            armed = i;
        }
    if (armed < 0) {
        message("Arm a MIDI or Audio track before recording.");
        return;
    }
    select(armed);
    releasePerformance();
    if (project.tracks[armed].kind == TrackKind::Midi) {
        auto tempo = TempoMap{project.bpm, engine.sampleRate()};
        recordingTrack_ = project.tracks[armed].id;
        double start = tempo.beatAt(engine.displayPosition());
        int bars = QSettings().value("record/countInBars", 1).toInt();
        double bar = project.numerator * 4. / project.denominator;
        double capture = bars > 0 ? std::ceil(start / bar) * bar + bars * bar : start;
        recordingBeat_ = capture;
        midiRecorder_.start(tempo, tempo.sampleAt(capture));
        midiRecording_ = true;
        recording_ = true;
        engine.setCountInEnd(bars > 0 ? tempo.sampleAt(capture) : -1);
        engine.setFreeRunning(true);
        engine.setPlaying(true);
        recordButton_->setText("Finish MIDI");
        refresh();
        message("MIDI take armed · Count-in then play · Stop closes every held note");
        return;
    }
    if (currentPath_.isEmpty() && !saveProject())
        return;
    if (!requestMicrophonePermission()) {
        QMessageBox::warning(
            this, "Microphone access denied",
            "Enable Reverie Ensemble in System Settings → Privacy & Security → Microphone. You can "
            "continue composing and exporting without input access.");
        return;
    }

    if (selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Audio)
        return;
    try {
        auto base = std::filesystem::path(currentPath_.toStdString());
        auto media = base.parent_path() / (base.stem().string() + ".media");
        std::filesystem::create_directories(media);
        auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
        recordingPath_ = media / ("take-" + std::to_string(stamp) + ".wav");
        recordingTrack_ = project.tracks[selectedTrack].id;
        recordingBeat_ =
            TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.displayPosition());
        recorder_.start(recordingPath_, 48000);
        if (!device_.startCapture(inputs_->currentIndex() - 1, recorder_)) {
            recorder_.stop();
            message(QString::fromStdString(device_.error()));
            return;
        }
        engine.releaseAudition();
        recording_ = true;
        engine.setFreeRunning(true);
        engine.setPlaying(true);
        recordButton_->setText("Finish take");
        refresh();
        message("Recording to project media. Press Stop or Finish take to place the clip.");
    } catch (const std::exception& ex) {
        device_.stopCapture();
        recorder_.stop();
        message(QString::fromUtf8(ex.what()));
    }
}
void Editor::finishRecord() {
    if (!recording_)
        return;
    if (midiRecording_) {
        auto frame = engine.displayPosition();
        releasePerformance();
        Clip take = midiRecorder_.finish(frame);
        recording_ = midiRecording_ = false;
        engine.setPlaying(false);
        engine.setFreeRunning(false);
        engine.setCountInEnd(-1);
        recordButton_->setText("Record");
        if (!take.notes.empty())
            edit([&] {
                if (auto* t = findTrack(project, recordingTrack_)) {
                    selectedClip = take.id;
                    t->clips.push_back(take);
                }
            });
        refresh();
        if (selectedClip)
            openPiano(selectedClip);
        message("MIDI take captured · edit notes or change the instrument");
        return;
    }
    device_.stopCapture();
    recorder_.stop();
    engine.setPlaying(false);
    engine.setFreeRunning(false);
    recording_ = false;
    recordButton_->setText("Record");
    try {
        if (recorder_.framesWritten() > 0) {
            auto sample = background(this, "Finalizing take and waveform…",
                                     [path = recordingPath_] { return loadWav(path); });
            edit([&] {
                auto* t = findTrack(project, recordingTrack_);
                if (!t)
                    return;
                Clip c;
                c.name = "Recorded take";
                c.sample = sample;
                c.mediaPath = sample->path;
                c.beat = recordingBeat_;
                c.length = double(sample->frames()) / sample->sampleRate * project.bpm / 60.;
                selectedClip = c.id;
                t->clips.push_back(c);
            });
            message("Take placed on the audio track. Save the project to keep its arrangement.");
        }
    } catch (const std::exception& ex) {
        message("Take retained at " + QString::fromStdString(recordingPath_.string()) + ": " +
                QString::fromUtf8(ex.what()));
    }
    refresh();
}
bool Editor::confirmUnsaved() {
    if (recording_)
        finishRecord();
    if (!dirty_)
        return true;
    auto answer = QMessageBox::question(
        this, "Unsaved project", "Save your project before continuing?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel)
        return false;
    if (answer == QMessageBox::Save)
        return saveProject();
    return true;
}
void Editor::newProject() {
    if (!confirmUnsaved())
        return;
    engine.setPlaying(false);
    engine.seek(0);
    project = Project{};
    undo_.clear();
    redo_.clear();
    currentPath_.clear();
    dirty_ = false;
    selectedTrack = -1;
    selectedClip = 0;
    publishProject();
    refresh();
}
void Editor::closeEvent(QCloseEvent* e) {
    if (confirmUnsaved())
        e->accept();
    else
        e->ignore();
}
void Editor::auditionNote(int key, float velocity, double duration) {
    if (selectedTrack < 0 || selectedTrack >= int(project.tracks.size()) || recording_)
        return;
    releaseAuditionNote();
    auditionToken_ = nextId();
    auditionKey_ = key;
    auto token = auditionToken_;
    engine.performanceEvent({0, token, key, velocity, true});
    QTimer::singleShot(int(duration * 1000), this, [this, token] {
        if (auditionToken_ == token)
            releaseAuditionNote();
    });
}
void Editor::releaseAuditionNote() {
    if (auditionToken_) {
        engine.performanceEvent({0, auditionToken_, auditionKey_, 0, false});
        auditionToken_ = 0;
    }
}
void Editor::applyInstrument(const Preset& preset, bool newTrack) {
    releasePerformance();
    performanceSignature_.clear();
    edit([&] {
        if (newTrack || selectedTrack < 0 ||
            project.tracks[selectedTrack].kind != TrackKind::Midi) {
            if (project.tracks.size() >= maxTracks)
                throw std::runtime_error("Track limit");
            project.tracks.push_back(makeTrack(TrackKind::Midi, int(project.tracks.size())));
            selectedTrack = int(project.tracks.size()) - 1;
            selectedClip = 0;
        }
        auto& t = project.tracks[selectedTrack];
        t.preset = preset;
        t.instrumentSample.reset();
        t.name = preset.name;
    });
}
void Editor::showMixer() {
    mixer_->refresh();
    workspace_->setCurrentIndex(1);
}
} // namespace reverie
