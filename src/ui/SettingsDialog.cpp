// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/SettingsDialog.h"
#include "ui/Editor.h"
#include "ui/SoundBrowser.h"
#include "ui/Theme.h"
#include "ui/Timeline.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>
namespace reverie {
SettingsDialog::SettingsDialog(Editor& e) : QDialog(&e) {
    setWindowTitle("Ensemble Settings");
    setObjectName("settingsDialog");
    resize(660, 520);
    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);
    auto* audio = new QWidget;
    auto* form = new QFormLayout(audio);
    tabs->addTab(audio, "Audio");
    auto* output = new QComboBox;
    output->setObjectName("settingsOutput");
    auto* input = new QComboBox;
    input->setObjectName("settingsInput");
    QSettings settings;
    auto fill = [output, input, &e] {
        QSettings settings;
        output->clear();
        input->clear();
        output->addItem("System default");
        input->addItem("System default");
        for (auto& n : e.device_.outputDevices())
            output->addItem(QString::fromStdString(n));
        for (auto& n : e.device_.inputDevices())
            input->addItem(QString::fromStdString(n));
        output->setCurrentText(settings.value("audio/outputName", "System default").toString());
        input->setCurrentText(settings.value("audio/inputName", "System default").toString());
    };
    fill();
    form->addRow("Output device", output);
    form->addRow("Input device", input);
    auto* rate = new QComboBox;
    rate->setObjectName("settingsRate");
    for (int r : {44100, 48000, 96000})
        rate->addItem(QString::number(r), r);
    rate->setCurrentText(QString::number(int(e.engine.sampleRate())));
    form->addRow("Requested sample rate", rate);
    auto* buffer = new QComboBox;
    buffer->setObjectName("settingsBuffer");
    for (int b : {128, 256, 512, 1024})
        buffer->addItem(QString::number(b), b);
    buffer->setCurrentText(settings.value("audio/buffer", 256).toString());
    form->addRow("Requested period / frames", buffer);
    auto* report = new QLabel(QString::fromStdString(e.device_.deviceReport()));
    report->setObjectName("deviceReport");
    report->setWordWrap(true);
    report->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(report);
    auto* refresh = new QPushButton("Refresh devices");
    form->addRow(refresh);
    connect(refresh, &QPushButton::clicked, this, [fill, report, &e] {
        fill();
        report->setText(QString::fromStdString(e.device_.deviceReport()));
    });
    auto* apply = new QPushButton("Apply audio configuration");
    apply->setObjectName("settingsApplyAudio");
    form->addRow(apply);
    connect(apply, &QPushButton::clicked, this, [=, &e] {
        if (e.recording_) {
            report->setText("Stop recording before changing devices.");
            return;
        }
        double beat = TempoMap{e.project.bpm, e.engine.sampleRate()}.beatAt(e.engine.position());
        e.device_.stopCapture();
        e.device_.stopOutput();
        e.engine.setSampleRateStopped(rate->currentData().toDouble());
        e.engine.publish(e.project);
        e.engine.seek(TempoMap{e.project.bpm, e.engine.sampleRate()}.sampleAt(beat));
        e.device_.configure(output->currentIndex() - 1, buffer->currentData().toUInt());
        e.inputs_->clear();
        e.inputs_->addItem("System default");
        for (int i = 1; i < input->count(); ++i)
            e.inputs_->addItem(input->itemText(i));
        e.inputs_->setCurrentIndex(input->currentIndex());
        if (e.audioEnabled_ && !e.device_.startOutput())
            e.message(QString::fromStdString(e.device_.error()));
        QSettings s;
        s.setValue("audio/rate", rate->currentData());
        s.setValue("audio/buffer", buffer->currentData());
        s.setValue("audio/outputName", output->currentText());
        s.setValue("audio/inputName", input->currentText());
        report->setText(QString::fromStdString(e.device_.deviceReport()));
    });
    auto* test = new QPushButton("Test output (quiet A4 tone)");
    form->addRow(test);
    connect(test, &QPushButton::clicked, this, [&e] {
        auto t = makeTrack(TrackKind::Midi, 0);
        t.volume = .25f;
        e.engine.audition(t, 69, .5f, .35);
    });
    auto* library = new QWidget;
    auto* lf = new QFormLayout(library);
    tabs->addTab(library, "Library");
    lf->addRow("Bundled libraries",
               new QLabel("Community Essentials + Ensemble Essentials\nOffline-ready; per-file CC0 "
                          "provenance in pack.json"));
    auto* loc = new QLabel(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
                           "/SoundPacks");
    loc->setWordWrap(true);
    loc->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lf->addRow("User library location", loc);
    auto* rescan = new QPushButton("Rescan installed sound packs");
    lf->addRow(rescan);
    connect(rescan, &QPushButton::clicked, this, [&e] { e.browser_->reload(); });
    lf->addRow(new QLabel("Install licensed unpacked packs through Sound Library.\nMissing/damaged "
                          "libraries are reported there; no automatic downloads."));
    auto* ui = new QWidget;
    auto* uf = new QFormLayout(ui);
    tabs->addTab(ui, "Interface");
    uf->addRow("Theme", new QLabel("Paper & Graphite · IBM Plex Sans KR / Mono"));
    auto* height = new QSpinBox;
    height->setRange(62, 130);
    height->setValue(e.trackHeight);
    height->setObjectName("settingsTrackHeight");
    uf->addRow("Default track height", height);
    auto* follow = new QCheckBox("Follow playback outside visible range");
    follow->setChecked(e.followPlayhead);
    uf->addRow(follow);
    connect(height, &QSpinBox::valueChanged, this, [&e](int v) {
        e.trackHeight = v;
        e.timeline_->updateExtent();
        QSettings().setValue("interface/trackHeight", v);
    });
    connect(follow, &QCheckBox::toggled, this, [&e](bool v) {
        e.followPlayhead = v;
        QSettings().setValue("interface/follow", v);
    });
    auto* general = new QWidget;
    auto* gf = new QFormLayout(general);
    tabs->addTab(general, "General");
    gf->addRow("Version", new QLabel("Reverie Ensemble 0.3.0 · QuadraJ"));
    gf->addRow("Project policy", new QLabel("Explicit Save; unsaved-change prompt on close.\nNo "
                                            "automatic save/backup or online update checks."));
    gf->addRow("Licenses",
               new QLabel("Original code: Apache-2.0\nQt: LGPLv3; IBM Plex: SIL OFL; recorded "
                          "audio: CC0\nFull terms: About and THIRD_PARTY_LICENSES.md"));
    gf->addRow(
        "MIDI",
        new QLabel(
            "SMF file import supported.\nExternal MIDI input is not implemented in this build."));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
} // namespace reverie
