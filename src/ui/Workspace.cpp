// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "core/Media.h"
#include "core/Serializer.h"
#include "ui/AudioEditor.h"
#include "ui/DesignTokens.h"
#include "ui/Editor.h"
#include "ui/InspectorPanel.h"
#include "ui/MixerView.h"
#include "ui/PianoRoll.h"
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
        p.fillRect(rect(), design::shell);
        float value = master_ ? e_.engine.masterPeak() : e_.engine.trackPeak(e_.selectedTrack);
        double db = value > 0 ? 20 * std::log10(value) : -60;
        int w = int(std::clamp((db + 60) / 60, 0., 1.) * width());
        p.fillRect(QRect(0, 2, w, height() - 4), value >= 1 ? design::error : design::signal);
        p.setPen(design::text);
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
void Editor::buildUi() {
    setStyleSheet(themeStyle());
    auto* transport = addToolBar("Transport");
    transport->setMovable(false);
    transport->setFixedHeight(design::transportHeight);
    auto* brand = new QLabel("ensemble.", this);
    brand->setStyleSheet("font-size:19px;font-weight:600;color:#EDE5D8;padding-right:8px");
    transport->addWidget(brand);
    projectLabel_ = new QLabel;
    projectLabel_->setMaximumWidth(140);
    projectLabel_->setStyleSheet("color:#BEB2A4;padding-right:8px");
    transport->addWidget(projectLabel_);
    playButton_ = new QPushButton("Play", this);
    playButton_->setObjectName("play");
    playButton_->setIcon(toolIcon("play"));
    transport->addWidget(playButton_);
    connect(playButton_, &QPushButton::clicked, this, [this] { play(); });
    auto* stopButton = new QPushButton("Stop", this);
    stopButton->setObjectName("stop");
    stopButton->setIcon(toolIcon("stop"));
    transport->addWidget(stopButton);
    connect(stopButton, &QPushButton::clicked, this, [this] { stop(); });
    recordButton_ = new QPushButton("Record", this);
    recordButton_->setObjectName("record");
    recordButton_->setIcon(toolIcon("record"));
    recordButton_->setStyleSheet("color:#BC655C");
    transport->addWidget(recordButton_);
    connect(recordButton_, &QPushButton::clicked, this, [this] { toggleRecord(); });
    auto* loop = new QPushButton;
    loop->setObjectName("loop");
    loop->setIcon(toolIcon("loop"));
    loop->setCheckable(true);
    loop->setToolTip("Loop playback");
    loop->setAccessibleName("Loop playback");
    transport->addWidget(loop);
    connect(loop, &QPushButton::toggled, this, [this](bool on) {
        if (!refreshing_)
            edit([&] { project.loopEnabled = on; });
    });
    auto* click = new QPushButton;
    click->setObjectName("metronome");
    click->setIcon(toolIcon("metronome"));
    click->setCheckable(true);
    click->setToolTip("Metronome (monitor only)");
    click->setAccessibleName("Metronome");
    transport->addWidget(click);
    connect(click, &QPushButton::toggled, this, [this](bool on) {
        if (!refreshing_)
            edit([&] { project.metronome = on; });
    });
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
    position_->setMinimumWidth(125);
    position_->setStyleSheet("font-family:IBM Plex Mono;font-size:15px;color:#EDE5D8;padding:8px");
    transport->addWidget(position_);
    auto* masterMeter = new Meter(*this, true);
    masterMeter->setMinimumWidth(120);
    transport->addWidget(masterMeter);
    connect(&timer_, &QTimer::timeout, masterMeter, qOverload<>(&QWidget::update));
    auto* deviceStatus = new QLabel("AUDIO");
    deviceStatus->setObjectName("transportDeviceStatus");
    deviceStatus->setStyleSheet("font-size:11px;color:#BEB2A4");
    deviceStatus->setMaximumWidth(120);
    transport->addWidget(deviceStatus);
    auto* left = new QDockWidget("SOUND LIBRARY", this);
    left->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    browser_ = new SoundBrowser(*this);
    left->setWidget(browser_);
    left->setMinimumWidth(220);
    left->setMaximumWidth(280);
    addDockWidget(Qt::LeftDockWidgetArea, left);
    auto* central = new QSplitter(Qt::Vertical, this);
    timeline_ = new Timeline(*this);
    timelineScroll_ = new QScrollArea;
    timelineScroll_->setWidget(timeline_);
    timelineScroll_->setWidgetResizable(true);
    central->addWidget(timelineScroll_);
    connect(timelineScroll_->horizontalScrollBar(), &QScrollBar::valueChanged, timeline_,
            qOverload<>(&QWidget::update));
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
    bottomStack_ = new QStackedWidget;
    bottomStack_->addWidget(pianoScroll_);
    audioEditor_ = new AudioEditor(*this);
    bottomStack_->addWidget(audioEditor_);
    pianoLayout->addWidget(bottomStack_);
    central->addWidget(pianoPanel);
    central->setSizes({460, 300});
    workspace_ = new QStackedWidget;
    workspace_->addWidget(central);
    mixer_ = new MixerView(*this);
    workspace_->addWidget(mixer_);
    setCentralWidget(workspace_);
    addToolBarBreak();
    auto* modes = addToolBar("Workspace");
    modes->setFixedHeight(34);
    modes->setMovable(false);
    auto* arrange = modes->addAction("Arrange");
    auto* mix = modes->addAction("Mixer");
    connect(arrange, &QAction::triggered, this, [this] { workspace_->setCurrentIndex(0); });
    connect(mix, &QAction::triggered, this, [this] {
        mixer_->refresh();
        workspace_->setCurrentIndex(1);
    });
    auto* lower = modes->addAction("Piano editor");
    lower->setCheckable(true);
    lower->setChecked(true);
    connect(lower, &QAction::toggled, pianoPanel, &QWidget::setVisible);
    auto* follow = new QCheckBox("Follow");
    modes->addWidget(follow);
    connect(follow, &QCheckBox::toggled, this, [this](bool value) { followPlayhead = value; });
    modes->addWidget(new QLabel("Track height"));
    auto* heights = number(this, 62, 130, 4, 0);
    heights->setValue(trackHeight);
    modes->addWidget(heights);
    connect(heights, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        trackHeight = int(v);
        timeline_->updateExtent();
    });
    modes->addWidget(new QLabel("Loop beats"));
    for (bool end : {false, true}) {
        auto* point = number(this, end ? .125 : 0, 100000, .25, 2);
        point->setObjectName(end ? "loopEnd" : "loopStart");
        point->setValue(end ? project.loopEnd : project.loopStart);
        point->setMaximumWidth(85);
        modes->addWidget(point);
        connect(point, &QDoubleSpinBox::valueChanged, this, [this, end](double v) {
            if (!refreshing_)
                edit([&] {
                    if (end)
                        project.loopEnd = std::max(v, project.loopStart + .125);
                    else
                        project.loopStart = std::min(v, project.loopEnd - .125);
                });
        });
    }
    auto* level = number(this, 0, .5, .02, 2);
    level->setObjectName("clickLevel");
    level->setValue(project.clickLevel);
    level->setMaximumWidth(70);
    modes->addWidget(new QLabel("Click level"));
    modes->addWidget(level);
    connect(level, &QDoubleSpinBox::valueChanged, this, [this](double v) {
        if (!refreshing_)
            edit([&] { project.clickLevel = float(v); });
    });
    auto* right = new QDockWidget("INSPECTOR", this);
    auto* inspector = new QWidget;
    auto* layout = new QVBoxLayout(inspector);
    trackTitle_ = new QLabel;
    trackTitle_->setStyleSheet("font-weight:600;font-size:14px;color:#EDE5D8");
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
    instrumentLabel_->setStyleSheet("color:#BEB2A4;font-size:11px");
    layout->addWidget(instrumentLabel_);
    auto* instrumentToggle = new QPushButton("Instrument parameters");
    instrumentToggle->setCheckable(true);
    layout->addWidget(instrumentToggle);
    auto* instrumentControls = new QWidget;
    instrumentControls->setObjectName("instrumentControls");
    instrumentControls->hide();
    auto* synthForm = new QFormLayout(instrumentControls);
    connect(instrumentToggle, &QPushButton::toggled, instrumentControls, &QWidget::setVisible);
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
    layout->addWidget(instrumentControls);
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
    output_->setStyleSheet("color:#BEB2A4;font-size:11px");
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
    auto* inspectorTabs = new QTabWidget;
    inspectorTabs->setObjectName("inspectorTabs");
    selectionInspector_ = new InspectorPanel(*this);
    inspectorTabs->addTab(selectionInspector_, "Selection");
    inspectorTabs->addTab(inspectorScroll, "Track / Instrument");
    right->setWidget(inspectorTabs);
    right->setMinimumWidth(260);
    right->setMaximumWidth(320);
    addDockWidget(Qt::RightDockWidgetArea, right);
    resizeDocks({left, right}, {250, 280}, Qt::Horizontal);
    statusBar()->showMessage("Double-click a lane to create a clip · Click to draw notes · "
                             "Shift-drag to select · Space to play");
}
} // namespace reverie
