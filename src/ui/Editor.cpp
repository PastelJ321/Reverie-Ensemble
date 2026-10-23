// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "ui/Editor.h"
#include "core/Media.h"
#include "core/Serializer.h"
#include "ui/PianoRoll.h"
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
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <chrono>
#include <fstream>
#include <future>
#include <unordered_map>
namespace reverie {
class Meter final : public QWidget {
    Editor& e_;
    bool master_;

  public:
    Meter(Editor& e, bool master) : e_(e), master_(master) {
        setMinimumHeight(18);
        setMaximumHeight(18);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor("#111621"));
        float value = master_ ? e_.engine.masterPeak() : e_.engine.trackPeak(e_.selectedTrack);
        double db = value > 0 ? 20 * std::log10(value) : -60;
        int w = int(std::clamp((db + 60) / 60, 0., 1.) * width());
        p.fillRect(QRect(0, 2, w, height() - 4),
                   value >= 1 ? QColor("#eb7d84") : QColor("#71c9b3"));
        p.setPen(QColor("#b4bdd2"));
        p.drawText(rect(), Qt::AlignCenter,
                   value > 0 ? QString::number(db, 'f', 1) + " dBFS" : "−∞ dBFS");
    }
};
static QDoubleSpinBox* number(QWidget* parent, double lo, double hi, double step,
                              int decimals = 2) {
    auto* w = new QDoubleSpinBox(parent);
    w->setRange(lo, hi);
    w->setSingleStep(step);
    w->setDecimals(decimals);
    w->setKeyboardTracking(false);
    return w;
}
Editor::Editor(bool audio) : project(demoProject()), device_(engine), audioEnabled_(audio) {
    setObjectName("reverieEditor");
    resize(1440, 930);
    setMinimumSize(1040, 680);
    buildUi();
    buildMenus();
    engine.publish(project);
    if (audioEnabled_ && !device_.startOutput())
        message(QString::fromStdString(device_.error()));
    connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
    timer_.start(33);
    select(0, project.tracks[0].clips[0].id);
    openPiano(selectedClip);
    refresh();
}
Editor::~Editor() {
    timer_.stop();
    device_.stopCapture();
    recorder_.stop();
    device_.stopOutput();
}
void Editor::buildUi() {
    setStyleSheet(R"(
        QMainWindow,QWidget { background:#1b1f2b; color:#dce1f0; font-family:"Avenir Next",sans-serif; font-size:12px; }
        QToolBar { background:#141824; border:0; spacing:9px; padding:8px; }
        QPushButton,QToolButton { background:#30364b; border:1px solid #454c66; border-radius:5px; padding:6px 10px; }
        QPushButton:hover,QToolButton:hover { background:#444a66; }
        QPushButton:checked { background:#645d92; }
        QSpinBox,QDoubleSpinBox,QComboBox { background:#111622; border:1px solid #3e465e; border-radius:4px; padding:5px; min-height:19px; }
        QDockWidget { font-weight:600; }
        QDockWidget::title { background:#141824; padding:9px; }
        QListWidget { background:#202534; border:0; outline:0; padding:5px; }
        QListWidget::item { padding:10px 5px; border-bottom:1px solid #2b3143; }
        QListWidget::item:selected { background:#484666; border-radius:4px; }
        QMenuBar,QMenu,QStatusBar { background:#151925; }
        QScrollArea { border:0; }
        QScrollBar { background:#171c29; width:11px; height:11px; }
        QScrollBar::handle { background:#46506a; border-radius:4px; min-width:22px; min-height:22px; }
        QScrollBar::add-line,QScrollBar::sub-line { width:0; height:0; }
        QSplitter::handle { background:#40475f; height:4px; }
        QCheckBox { spacing:7px; }
    )");
    auto* transport = addToolBar("Transport");
    transport->setMovable(false);
    auto* brand = new QLabel("◇  REVERIE", this);
    brand->setStyleSheet("font-size:17px;font-weight:700;color:#c3b9f1;padding-right:20px");
    transport->addWidget(brand);
    projectLabel_ = new QLabel;
    projectLabel_->setMaximumWidth(140);
    projectLabel_->setStyleSheet("color:#abb2cd;padding-right:8px");
    transport->addWidget(projectLabel_);
    playButton_ = new QPushButton("▶  Play", this);
    playButton_->setObjectName("play");
    transport->addWidget(playButton_);
    connect(playButton_, &QPushButton::clicked, this, [this] { play(); });
    auto* stopButton = new QPushButton("■  Stop", this);
    stopButton->setObjectName("stop");
    transport->addWidget(stopButton);
    connect(stopButton, &QPushButton::clicked, this, [this] { stop(); });
    recordButton_ = new QPushButton("●  Record", this);
    recordButton_->setObjectName("record");
    recordButton_->setStyleSheet("color:#ef9ba7");
    transport->addWidget(recordButton_);
    connect(recordButton_, &QPushButton::clicked, this, [this] { toggleRecord(); });
    transport->addSeparator();
    transport->addWidget(new QLabel("BPM", this));
    bpm_ = number(this, 20, 300, 1, 1);
    bpm_->setObjectName("bpm");
    bpm_->setValue(project.bpm);
    bpm_->setMaximumWidth(85);
    transport->addWidget(bpm_);
    connect(bpm_, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        if (!refreshing_)
            setTempo(v);
    });
    auto* meter = new QComboBox(this);
    meter->setObjectName("timeSignature");
    meter->addItems({"4/4", "3/4", "6/8", "7/8"});
    transport->addWidget(meter);
    connect(meter, &QComboBox::currentIndexChanged, this, [this, meter] {
        if (refreshing_)
            return;
        auto parts = meter->currentText().split('/');
        edit([&] {
            project.numerator = parts[0].toInt();
            project.denominator = parts[1].toInt();
        });
    });
    position_ = new QLabel("1 : 1 : 000", this);
    position_->setMinimumWidth(150);
    position_->setStyleSheet("font-family:monospace;font-size:15px;color:#e7cd98;padding:8px");
    transport->addWidget(position_);
    auto* masterMeter = new Meter(*this, true);
    masterMeter->setMinimumWidth(120);
    transport->addWidget(masterMeter);
    connect(&timer_, &QTimer::timeout, masterMeter, qOverload<>(&QWidget::update));
    auto* left = new QDockWidget("SOUND LIBRARY", this);
    left->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* library = new QWidget;
    auto* libLayout = new QVBoxLayout(library);
    libLayout->setContentsMargins(10, 10, 10, 10);
    auto* heading = new QLabel("Factory instruments");
    heading->setStyleSheet("color:#a8acc4;font-weight:600");
    libLayout->addWidget(heading);
    auto* presets = new QListWidget;
    presets->setObjectName("factoryPresets");
    for (const auto& p : factoryPresets())
        presets->addItem(QString::fromStdString(p.name));
    libLayout->addWidget(presets);
    connect(presets, &QListWidget::itemDoubleClicked, this, [this, presets] {
        int index = presets->currentRow();
        if (selectedTrack < 0 || selectedTrack >= int(project.tracks.size()) ||
            project.tracks[selectedTrack].kind != TrackKind::Midi)
            return;
        edit([&] {
            auto& t = project.tracks[selectedTrack];
            t.preset = factoryPresets()[index];
            t.instrumentSample.reset();
        });
    });
    auto* note =
        new QLabel("Double-click an instrument to apply it to the selected MIDI track.\n\nThese "
                   "are original synth presets, not sampled acoustic instruments.");
    note->setWordWrap(true);
    note->setStyleSheet("color:#8f9bb5;font-size:11px");
    libLayout->addWidget(note);
    for (auto [text, fn] : std::vector<std::pair<QString, std::function<void()>>>{
             {"+ MIDI track", [this] { addTrack(TrackKind::Midi); }},
             {"+ Audio track", [this] { addTrack(TrackKind::Audio); }},
             {"Import WAV…", [this] { importWav(); }},
             {"Sampler WAV…", [this] { loadSampler(); }}}) {
        auto* b = new QPushButton(text);
        libLayout->addWidget(b);
        connect(b, &QPushButton::clicked, this, fn);
    }
    left->setWidget(library);
    addDockWidget(Qt::LeftDockWidgetArea, left);
    left->setMinimumWidth(190);
    auto* central = new QSplitter(Qt::Vertical, this);
    timeline_ = new Timeline(*this);
    timelineScroll_ = new QScrollArea;
    timelineScroll_->setWidget(timeline_);
    timelineScroll_->setWidgetResizable(false);
    central->addWidget(timelineScroll_);
    auto* pianoPanel = new QWidget;
    auto* pianoLayout = new QVBoxLayout(pianoPanel);
    pianoLayout->setContentsMargins(0, 0, 0, 0);
    pianoLayout->setSpacing(0);
    auto* pianoBar = new QToolBar;
    pianoBar->addWidget(new QLabel("PIANO ROLL  "));
    piano_ = new PianoRoll(*this);
    auto* quant = pianoBar->addAction("Quantize");
    connect(quant, &QAction::triggered, this, [this] { piano_->quantize(); });
    pianoBar->addWidget(new QLabel("Velocity "));
    auto* velocity = number(this, .01, 1, .05);
    velocity->setObjectName("velocity");
    velocity->setValue(.8);
    velocity->setMaximumWidth(80);
    pianoBar->addWidget(velocity);
    connect(velocity, &QDoubleSpinBox::valueChanged, this,
            [this](double v) { piano_->setVelocity(v); });
    auto* snapBox = new QCheckBox("Snap");
    snapBox->setChecked(true);
    pianoBar->addWidget(snapBox);
    connect(snapBox, &QCheckBox::toggled, this, [this](bool v) { snap = v; });
    auto* gridBox = new QComboBox;
    gridBox->addItems({"1/4", "1/8", "1/16", "1/32"});
    gridBox->setCurrentIndex(2);
    pianoBar->addWidget(gridBox);
    connect(gridBox, &QComboBox::currentIndexChanged, this, [this](int i) {
        grid = 1. / std::pow(2., i);
        piano_->update();
        timeline_->update();
    });
    pianoBar->addWidget(new QLabel(" Zoom "));
    auto* zoom = number(this, 24, 160, 8, 0);
    zoom->setValue(64);
    zoom->setMaximumWidth(75);
    pianoBar->addWidget(zoom);
    connect(zoom, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        pixelsPerBeat = v;
        timeline_->updateExtent();
        piano_->updateExtent();
    });
    pianoLayout->addWidget(pianoBar);
    pianoScroll_ = new QScrollArea;
    pianoScroll_->setWidget(piano_);
    pianoScroll_->setWidgetResizable(false);
    pianoLayout->addWidget(pianoScroll_);
    central->addWidget(pianoPanel);
    central->setSizes({460, 300});
    setCentralWidget(central);
    auto* right = new QDockWidget("INSPECTOR / MIXER", this);
    auto* inspector = new QWidget;
    auto* layout = new QVBoxLayout(inspector);
    trackTitle_ = new QLabel;
    trackTitle_->setStyleSheet("font-weight:600;font-size:16px;color:#cec9f2");
    layout->addWidget(trackTitle_);
    auto* form = new QFormLayout;
    volume_ = number(this, 0, 2, .05);
    volume_->setObjectName("trackVolume");
    pan_ = number(this, -1, 1, .05);
    pan_->setObjectName("trackPan");
    form->addRow("Volume", volume_);
    form->addRow("Pan", pan_);
    layout->addLayout(form);
    auto* toggles = new QHBoxLayout;
    mute_ = new QCheckBox("Mute");
    solo_ = new QCheckBox("Solo");
    toggles->addWidget(mute_);
    toggles->addWidget(solo_);
    layout->addLayout(toggles);
    auto* selectedMeter = new Meter(*this, false);
    layout->addWidget(selectedMeter);
    connect(&timer_, &QTimer::timeout, selectedMeter, qOverload<>(&QWidget::update));
    auto mixerChange = [this] {
        if (refreshing_ || selectedTrack < 0 || selectedTrack >= int(project.tracks.size()))
            return;
        edit([&] {
            auto& t = project.tracks[selectedTrack];
            t.volume = float(volume_->value());
            t.pan = float(pan_->value());
            t.mute = mute_->isChecked();
            t.solo = solo_->isChecked();
        });
    };
    connect(volume_, &QDoubleSpinBox::valueChanged, this, mixerChange);
    connect(pan_, &QDoubleSpinBox::valueChanged, this, mixerChange);
    connect(mute_, &QCheckBox::toggled, this, mixerChange);
    connect(solo_, &QCheckBox::toggled, this, mixerChange);
    preset_ = new QComboBox;
    preset_->setObjectName("preset");
    for (const auto& p : factoryPresets())
        preset_->addItem(QString::fromStdString(p.name));
    layout->addWidget(preset_);
    connect(preset_, &QComboBox::activated, this, [this](int i) {
        if (selectedTrack < 0 || selectedTrack >= int(project.tracks.size()) ||
            project.tracks[selectedTrack].kind != TrackKind::Midi)
            return;
        edit([&] {
            project.tracks[selectedTrack].preset = factoryPresets()[i];
            project.tracks[selectedTrack].instrumentSample.reset();
        });
    });
    instrumentLabel_ = new QLabel;
    instrumentLabel_->setWordWrap(true);
    instrumentLabel_->setStyleSheet("color:#96a3bb;font-size:11px");
    layout->addWidget(instrumentLabel_);
    auto* synthForm = new QFormLayout;
    wave_ = new QComboBox;
    wave_->addItems({"Sine", "Saw", "Square", "Triangle"});
    synthForm->addRow("Oscillator", wave_);
    attack_ = number(this, .0001, 10, .01, 4);
    decay_ = number(this, .0001, 10, .05, 4);
    sustain_ = number(this, 0, 1, .05);
    release_ = number(this, .001, 10, .05, 3);
    cutoff_ = number(this, 20, 24000, 100, 0);
    synthForm->addRow("Attack (s)", attack_);
    synthForm->addRow("Decay (s)", decay_);
    synthForm->addRow("Sustain", sustain_);
    synthForm->addRow("Release (s)", release_);
    synthForm->addRow("Low-pass Hz", cutoff_);
    layout->addLayout(synthForm);
    auto synthChange = [this] {
        if (refreshing_ || selectedTrack < 0 || selectedTrack >= int(project.tracks.size()) ||
            project.tracks[selectedTrack].kind != TrackKind::Midi)
            return;
        edit([&] {
            auto& s = project.tracks[selectedTrack].preset;
            s.wave = Waveform(wave_->currentIndex());
            s.attack = float(attack_->value());
            s.decay = float(decay_->value());
            s.sustain = float(sustain_->value());
            s.release = float(release_->value());
            s.cutoff = float(cutoff_->value());
        });
    };
    for (auto* spin : {attack_, decay_, sustain_, release_, cutoff_})
        connect(spin, &QDoubleSpinBox::valueChanged, this, synthChange);
    connect(wave_, &QComboBox::activated, this, synthChange);
    auto* presetButtons = new QHBoxLayout;
    auto* load = new QPushButton("Load preset");
    auto* save = new QPushButton("Save preset");
    presetButtons->addWidget(load);
    presetButtons->addWidget(save);
    layout->addLayout(presetButtons);
    connect(load, &QPushButton::clicked, this, [this] { loadPreset(); });
    connect(save, &QPushButton::clicked, this, [this] { savePreset(); });
    layout->addStretch();
    auto* masterForm = new QFormLayout;
    master_ = number(this, 0, 2, .05);
    master_->setValue(project.master);
    masterForm->addRow("Master", master_);
    layout->addLayout(masterForm);
    connect(master_, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        if (!refreshing_)
            edit([&] { project.master = float(v); });
    });
    inputs_ = new QComboBox;
    inputs_->addItem("Default audio input");
    layout->addWidget(inputs_);
    auto* scan = new QPushButton("Choose / refresh inputs");
    layout->addWidget(scan);
    connect(scan, &QPushButton::clicked, this, [this] {
        inputs_->clear();
        inputs_->addItem("Default audio input");
        for (const auto& name : device_.inputDevices())
            inputs_->addItem(QString::fromStdString(name));
        if (inputs_->count() == 1)
            message("No input devices found. Recording will try the system default.");
    });
    output_ = new QLabel;
    output_->setWordWrap(true);
    output_->setStyleSheet("color:#99a9c5;font-size:11px");
    layout->addWidget(output_);
    auto* retry = new QPushButton("Reconnect audio / clear clip");
    layout->addWidget(retry);
    connect(retry, &QPushButton::clicked, this, [this] {
        if (recording_)
            return;
        engine.clearClip();
        if (audioEnabled_ && !device_.outputRunning() && !device_.startOutput())
            message(QString::fromStdString(device_.error()));
    });
    auto* inspectorScroll = new QScrollArea;
    inspectorScroll->setWidget(inspector);
    inspectorScroll->setWidgetResizable(true);
    right->setWidget(inspectorScroll);
    right->setMinimumWidth(255);
    addDockWidget(Qt::RightDockWidgetArea, right);
    statusBar()->showMessage("Double-click a lane to create a clip · Click to draw notes · "
                             "Shift-drag to select · Space to play");
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
    action(file, "Export WAV…", QKeySequence("Ctrl+Shift+E"), [this] {
        auto name = QFileDialog::getSaveFileName(this, "Export 48 kHz · 16-bit stereo WAV", {},
                                                 "WAV (*.wav)");
        if (!name.isEmpty())
            exportProject(name);
    });
    action(file, "Quit", QKeySequence::Quit, [this] { close(); });
    auto* editMenu = menuBar()->addMenu("&Edit");
    action(editMenu, "Undo", QKeySequence::Undo, [this] { undo(); });
    action(editMenu, "Redo", QKeySequence::Redo, [this] { redo(); });
    action(editMenu, "Duplicate clip", QKeySequence("Ctrl+D"), [this] { duplicateClip(); });
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
    auto* help = menuBar()->addMenu("&Help");
    action(help, "Editing guide", {}, [this] {
        QMessageBox::information(
            this, "Making music in Reverie Ensemble",
            "1. Add a MIDI track and double-click its lane to create a clip.\n2. Double-click the "
            "clip, then click the piano grid to add notes.\n3. Drag notes or their right edges to "
            "move / resize. Ctrl/Cmd-click toggles selection; Shift-drag selects an area. "
            "Right-click deletes a note.\n4. Select a factory preset and press Play. Drums use "
            "MIDI 36 (kick), 38 (snare), 42 (hat), 46 (open hat).\n5. Select each track to mix "
            "volume / pan / mute / solo.\n6. File → Save and File → Export WAV.\n\nClip edges "
            "resize non-destructively; Ctrl/Cmd+D duplicates. Click the arrangement ruler to "
            "seek.\n\nRecording: choose an audio track, input device, then Record. Press Stop to "
            "finish and place the clip. Monitoring and latency compensation are not provided in "
            "0.1.");
    });
    action(help, "About Reverie Ensemble", {}, [this] {
        QMessageBox::about(
            this, "Reverie Ensemble 0.1.0",
            "<h2>◇ Reverie Ensemble</h2><p>0.1.0 · QuadraJ</p><p>Copyright 2026 QuadraJ and "
            "contributors.</p><p>A free, offline, open-source digital audio "
            "workstation.</p><p>Reverie code: Apache License 2.0.<br>Qt Core / Gui / Widgets: "
            "dynamically linked under LGPLv3.<br>miniaudio 0.11.23: MIT-0.<br>nlohmann/json "
            "3.11.3: MIT.</p><p>Third-party components retain their own licenses. Source, "
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
        engine.publish(project);
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
    for (QWidget* w : std::initializer_list<QWidget*>{preset_, wave_, attack_, decay_, sustain_,
                                                      release_, cutoff_})
        w->setEnabled(midi && !recording_);
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
        trackTitle_->setText(QString::fromStdString(t.name));
        volume_->setValue(t.volume);
        pan_->setValue(t.pan);
        mute_->setChecked(t.mute);
        solo_->setChecked(t.solo);
        auto& s = t.preset;
        int idx = preset_->findText(QString::fromStdString(s.name));
        preset_->setCurrentIndex(idx);
        wave_->setCurrentIndex(int(s.wave));
        attack_->setValue(s.attack);
        decay_->setValue(s.decay);
        sustain_->setValue(s.sustain);
        release_->setValue(s.release);
        cutoff_->setValue(s.cutoff);
        instrumentLabel_->setText(
            midi ? QString::fromStdString(s.name) +
                       (s.kind == InstrumentKind::Sampler
                            ? "\nRoot: C4 (MIDI 60) · " + QString::fromStdString(s.license)
                            : "")
                 : "Audio track · non-destructive WAV playback");
    } else {
        trackTitle_->setText("No track selected");
        instrumentLabel_->clear();
    }
    timeline_->updateExtent();
    piano_->updateExtent();
    setWindowTitle(QString::fromStdString(project.name) + (dirty_ ? " *" : "") +
                   " — Reverie Ensemble 0.1");
    refreshing_ = false;
}
void Editor::select(int track, Id clip) {
    selectedTrack = track;
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
    engine.publish(project);
    refresh();
}
void Editor::redo() {
    if (recording_ || transaction_ || redo_.empty())
        return;
    undo_.push_back(project);
    project = std::move(redo_.back());
    redo_.pop_back();
    dirty_ = true;
    engine.publish(project);
    refresh();
}
void Editor::setTempo(double bpm) {
    const double beat = TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.position());
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
    auto beat = TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.position());
    double bar = project.numerator * 4. / project.denominator;
    position_->setText(QString("%1 : %2 : %3")
                           .arg(int(beat / bar) + 1, 3, 10, QChar('0'))
                           .arg(int(std::fmod(beat, bar) / (4. / project.denominator)) + 1)
                           .arg(int((beat - std::floor(beat)) * 960), 3, 10, QChar('0')));
    playButton_->setText(engine.playing() ? "Ⅱ  Pause" : "▶  Play");
    output_->setText(QString(device_.outputRunning() ? "48 kHz · 256 frames · float32"
                                                     : "Output unavailable · offline editor") +
                     (engine.clipped() ? "\nCLIPPING — lower track/master levels" : ""));
    timeline_->update();
    if (recording_ && (recorder_.failed() || recorder_.overflowed() || !engine.playing())) {
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
            auto sampleSource =
                !before.preset.samplePath.empty()
                    ? before.preset.samplePath
                    : (before.instrumentSample ? before.instrumentSample->path : "");
            if (!sampleSource.empty())
                refs[sampleSource] = after.preset.samplePath;
            for (std::size_t j = 0; j < before.clips.size(); ++j) {
                const auto& c = before.clips[j];
                auto source = !c.mediaPath.empty() ? c.mediaPath : (c.sample ? c.sample->path : "");
                if (!source.empty())
                    refs[source] = after.clips[j].mediaPath;
            }
        }
        auto rebase = [&](Project& state) {
            for (auto& t : state.tracks) {
                auto source = !t.preset.samplePath.empty()
                                  ? t.preset.samplePath
                                  : (t.instrumentSample ? t.instrumentSample->path : "");
                if (refs.contains(source))
                    t.preset.samplePath = refs.at(source);
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
        currentPath_ = path;
        dirty_ = false;
        refresh();
        message("Project saved with a portable media folder.");
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
        engine.publish(project);
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
    if (!device_.outputRunning()) {
        message("Connect an output device before recording.");
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
        addTrack(TrackKind::Audio);
    if (selectedTrack < 0 || project.tracks[selectedTrack].kind != TrackKind::Audio)
        return;
    try {
        auto base = std::filesystem::path(currentPath_.toStdString());
        auto media = base.parent_path() / (base.stem().string() + ".media");
        std::filesystem::create_directories(media);
        auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
        recordingPath_ = media / ("take-" + std::to_string(stamp) + ".wav");
        recordingTrack_ = project.tracks[selectedTrack].id;
        recordingBeat_ = TempoMap{project.bpm, engine.sampleRate()}.beatAt(engine.position());
        recorder_.start(recordingPath_, 48000);
        if (!device_.startCapture(inputs_->currentIndex() - 1, recorder_)) {
            recorder_.stop();
            message(QString::fromStdString(device_.error()));
            return;
        }
        recording_ = true;
        engine.setFreeRunning(true);
        engine.setPlaying(true);
        recordButton_->setText("■  Finish take");
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
    device_.stopCapture();
    recorder_.stop();
    engine.setPlaying(false);
    engine.setFreeRunning(false);
    recording_ = false;
    recordButton_->setText("●  Record");
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
    engine.publish(project);
    refresh();
}
void Editor::closeEvent(QCloseEvent* e) {
    if (confirmUnsaved())
        e->accept();
    else
        e->ignore();
}
} // namespace reverie
