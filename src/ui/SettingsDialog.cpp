#include <QCoreApplication>
// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/Editor.h"
#include "ui/SettingsDialog.h"
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
    setWindowTitle(QCoreApplication::translate("Ensemble", "Ensemble Settings"));
    setObjectName("settingsDialog");
    resize(660, 520);
    auto* layout = new QVBoxLayout(this);
    auto* tabs = new QTabWidget;
    layout->addWidget(tabs);
    auto* localePage = new QWidget;
    auto* generalForm = new QFormLayout(localePage);
    auto* language = new QComboBox;
    language->setObjectName("languageChoice");
    language->addItem(QCoreApplication::translate("Ensemble", "Follow System Language"), "system");
    language->addItem("English", "en");
    language->addItem(QString::fromUtf8("한국어"), "ko_KR");
    language->setCurrentIndex(
        std::max(0, language->findData(QSettings().value("interface/language", "system"))));
    generalForm->addRow(QCoreApplication::translate("Ensemble", "Language"), language);
    auto* restart = new QLabel(QCoreApplication::translate(
        "Ensemble", "Language changes apply after restarting Ensemble. Your project stays open."));
    restart->setWordWrap(true);
    generalForm->addRow(restart);
    connect(language, &QComboBox::currentIndexChanged, this,
            [language] { QSettings().setValue("interface/language", language->currentData()); });

    auto* audio = new QWidget;
    auto* form = new QFormLayout(audio);
    tabs->addTab(audio, QCoreApplication::translate("Ensemble", "Audio"));
    auto* output = new QComboBox;
    output->setObjectName("settingsOutput");
    auto* input = new QComboBox;
    input->setObjectName("settingsInput");
    QSettings settings;
    auto fill = [output, input, &e] {
        QSettings settings;
        output->clear();
        input->clear();
        output->addItem(QCoreApplication::translate("Ensemble", "System default"),
                        "System default");
        input->addItem(QCoreApplication::translate("Ensemble", "System default"), "System default");
        for (auto& n : e.device_.outputDevices())
            output->addItem(QString::fromStdString(n), QString::fromStdString(n));
        for (auto& n : e.device_.inputDevices())
            input->addItem(QString::fromStdString(n), QString::fromStdString(n));
        output->setCurrentIndex(
            std::max(0, output->findData(settings.value("audio/outputName", "System default"))));
        input->setCurrentIndex(
            std::max(0, input->findData(settings.value("audio/inputName", "System default"))));
    };
    fill();
    form->addRow(QCoreApplication::translate("Ensemble", "Output device"), output);
    form->addRow(QCoreApplication::translate("Ensemble", "Input device"), input);
    auto* rate = new QComboBox;
    rate->setObjectName("settingsRate");
    for (int r : {44100, 48000, 96000})
        rate->addItem(QString::number(r), r);
    rate->setCurrentText(QString::number(int(e.engine.sampleRate())));
    form->addRow(QCoreApplication::translate("Ensemble", "Requested sample rate"), rate);
    auto* buffer = new QComboBox;
    buffer->setObjectName("settingsBuffer");
    for (int b : {128, 256, 512, 1024})
        buffer->addItem(QString::number(b), b);
    buffer->setCurrentText(settings.value("audio/buffer", 256).toString());
    form->addRow(QCoreApplication::translate("Ensemble", "Requested period / frames"), buffer);
    auto* report = new QLabel(QString::fromStdString(e.device_.deviceReport()));
    report->setObjectName("deviceReport");
    report->setWordWrap(true);
    report->setTextInteractionFlags(Qt::TextSelectableByMouse);
    form->addRow(report);
    auto* refresh = new QPushButton(QCoreApplication::translate("Ensemble", "Refresh devices"));
    form->addRow(refresh);
    connect(refresh, &QPushButton::clicked, this, [fill, report, &e] {
        fill();
        report->setText(QString::fromStdString(e.device_.deviceReport()));
    });
    auto* apply =
        new QPushButton(QCoreApplication::translate("Ensemble", "Apply audio configuration"));
    apply->setObjectName("settingsApplyAudio");
    form->addRow(apply);
    connect(apply, &QPushButton::clicked, this, [=, &e] {
        if (e.recording_) {
            report->setText(
                QCoreApplication::translate("Ensemble", "Stop recording before changing devices."));
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
        e.inputs_->addItem(QCoreApplication::translate("Ensemble", "System default"),
                           "System default");
        for (int i = 1; i < input->count(); ++i)
            e.inputs_->addItem(input->itemText(i), input->itemData(i));
        e.inputs_->setCurrentIndex(input->currentIndex());
        if (e.audioEnabled_ && !e.device_.startOutput())
            e.message(QString::fromStdString(e.device_.error()));
        QSettings s;
        s.setValue("audio/rate", rate->currentData());
        s.setValue("audio/buffer", buffer->currentData());
        s.setValue("audio/outputName", output->currentData());
        s.setValue("audio/inputName", input->currentData());
        report->setText(QString::fromStdString(e.device_.deviceReport()));
    });
    auto* test =
        new QPushButton(QCoreApplication::translate("Ensemble", "Test output (quiet A4 tone)"));
    form->addRow(test);
    connect(test, &QPushButton::clicked, this, [&e] {
        auto t = makeTrack(TrackKind::Midi, 0);
        t.volume = .25f;
        e.engine.audition(t, 69, .5f, .35);
    });
    auto* library = new QWidget;
    auto* lf = new QFormLayout(library);
    tabs->addTab(library, QCoreApplication::translate("Ensemble", "Library"));
    lf->addRow(QCoreApplication::translate("Ensemble", "Bundled libraries"),
               new QLabel(QCoreApplication::translate(
                   "Ensemble", "Factory Library + Community Essentials\nOffline-ready when "
                               "included; CC0 sources in pack.json")));
    auto* loc = new QLabel(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
                           "/SoundPacks");
    loc->setWordWrap(true);
    loc->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lf->addRow(QCoreApplication::translate("Ensemble", "User library location"), loc);
    auto* rescan =
        new QPushButton(QCoreApplication::translate("Ensemble", "Rescan installed sound packs"));
    lf->addRow(rescan);
    connect(rescan, &QPushButton::clicked, this, [&e] { e.browser_->reload(); });
    lf->addRow(new QLabel(QCoreApplication::translate(
        "Ensemble", "Install licensed unpacked packs through Sound Library.\nMissing/damaged "
                    "libraries are reported there; no automatic downloads.")));
    auto* ui = new QWidget;
    auto* uf = new QFormLayout(ui);
    tabs->addTab(ui, QCoreApplication::translate("Ensemble", "Interface"));
    uf->addRow(QCoreApplication::translate("Ensemble", "Theme"),
               new QLabel(QCoreApplication::translate(
                   "Ensemble", "Paper & Graphite · IBM Plex Sans KR / Mono")));
    auto* height = new QSpinBox;
    height->setRange(62, 130);
    height->setValue(e.trackHeight);
    height->setObjectName("settingsTrackHeight");
    uf->addRow(QCoreApplication::translate("Ensemble", "Default track height"), height);
    auto* follow = new QCheckBox(
        QCoreApplication::translate("Ensemble", "Follow playback outside visible range"));
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
    gf->addRow(localePage);
    tabs->addTab(general, QCoreApplication::translate("Ensemble", "General"));
    gf->addRow(
        QCoreApplication::translate("Ensemble", "Version"),
        new QLabel(QCoreApplication::translate("Ensemble", "Reverie Ensemble 0.5.0 · QuadraJ")));
    gf->addRow(QCoreApplication::translate("Ensemble", "Project policy"),
               new QLabel(QCoreApplication::translate(
                   "Ensemble", "Explicit Save; unsaved-change prompt on close.\nNo "
                               "automatic save/backup or online update checks.")));
    gf->addRow(QCoreApplication::translate("Ensemble", "Licenses"),
               new QLabel(QCoreApplication::translate(
                   "Ensemble", "Original code: Apache-2.0\nQt: LGPLv3; IBM Plex: SIL OFL; recorded "
                               "audio: CC0\nFull terms: About and THIRD_PARTY_LICENSES.md")));
    gf->addRow("MIDI", new QLabel(QCoreApplication::translate(
                           "Ensemble", "SMF import and CoreMIDI input supported.")));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
} // namespace reverie
