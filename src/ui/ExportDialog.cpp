// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/ExportDialog.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTimer>
#include <QVBoxLayout>
namespace reverie {
ExportDialog::ExportDialog(const Project& p, double start, double end, QWidget* parent)
    : QDialog(parent), project_(p), selectionStart_(start), selectionEnd_(end) {
    setWindowTitle("Export Audio");
    setObjectName("exportDialog");
    resize(590, 470);
    auto* box = new QVBoxLayout(this);
    auto* form = new QFormLayout;
    box->addLayout(form);
    path_ = new QLineEdit;
    path_->setObjectName("exportPath");
    auto* row = new QHBoxLayout;
    row->addWidget(path_);
    auto* browse = new QPushButton("Choose…");
    row->addWidget(browse);
    form->addRow("File / destination", row);
    connect(browse, &QPushButton::clicked, this, [this] {
        auto p = QFileDialog::getSaveFileName(this, "Export WAV", path_->text(), "WAV (*.wav)");
        if (!p.isEmpty())
            path_->setText(p);
    });
    form->addRow("Format", new QLabel("WAV — RIFF (maximum 4 GiB)"));
    rate_ = new QComboBox;
    rate_->setObjectName("exportRate");
    for (int r : {44100, 48000, 96000})
        rate_->addItem(QString::number(r) + " Hz", r);
    rate_->setCurrentIndex(1);
    form->addRow("Sample rate", rate_);
    depth_ = new QComboBox;
    depth_->setObjectName("exportDepth");
    depth_->addItem("16-bit PCM", 16);
    depth_->addItem("24-bit PCM", 24);
    depth_->addItem("32-bit float", 32);
    depth_->setCurrentIndex(1);
    form->addRow("Bit depth", depth_);
    channels_ = new QComboBox;
    channels_->setObjectName("exportChannels");
    channels_->addItem("Stereo", 2);
    channels_->addItem("Mono (L+R / 2)", 1);
    form->addRow("Channels", channels_);
    range_ = new QComboBox;
    range_->setObjectName("exportRange");
    range_->addItems({"Entire project", "Loop / cycle range", "Selected clip range"});
    if (end <= start)
        static_cast<QStandardItemModel*>(range_->model())->item(2)->setEnabled(false);
    form->addRow("Range", range_);
    tail_ = new QCheckBox("Include instrument release and effect tail");
    tail_->setChecked(true);
    tail_->setObjectName("exportTail");
    box->addWidget(tail_);
    normalize_ = new QCheckBox("Normalize peak to −1 dBFS (explicit gain, no compression)");
    normalize_->setObjectName("exportNormalize");
    box->addWidget(normalize_);
    dither_ = new QCheckBox("TPDF dither for integer PCM");
    dither_->setObjectName("exportDither");
    box->addWidget(dither_);
    status_ = new QLabel;
    status_->setWordWrap(true);
    status_->setObjectName("exportStatus");
    box->addWidget(status_);
    progress_ = new QProgressBar;
    box->addWidget(progress_);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    auto* cancel = new QPushButton("Cancel");
    buttons->addWidget(cancel);
    render_ = new QPushButton("Export");
    render_->setObjectName("exportStart");
    render_->setDefault(true);
    buttons->addWidget(render_);
    box->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &ExportDialog::reject);
    connect(render_, &QPushButton::clicked, this, [this] {
        if (done_)
            accept();
        else
            startRender();
    });
    for (auto* c : {rate_, depth_, channels_, range_})
        connect(c, &QComboBox::currentIndexChanged, this, [this] {
            dither_->setEnabled(depth_->currentData().toInt() != 32 && !running_);
            updateEstimate();
        });
    connect(tail_, &QCheckBox::toggled, this, [this] { updateEstimate(); });
    updateEstimate();
    auto* poll = new QTimer(this);
    poll->start(30);
    connect(poll, &QTimer::timeout, this, [this] {
        if (!running_)
            return;
        progress_->setValue(int(percent_.load() * 100));
        if (job_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;
        running_ = false;
        try {
            auto r = job_.get();
            done_ = true;
            status_->setText(
                QString("Finished · %1 frames · peak %2 dBFS · RMS %3 dBFS\nApplied gain %4 dB · "
                        "%5 over-range samples%6")
                    .arg(r.frames)
                    .arg(20 * std::log10(std::max(1e-9, r.peak)), 0, 'f', 1)
                    .arg(20 * std::log10(std::max(1e-9, r.rms)), 0, 'f', 1)
                    .arg(20 * std::log10(r.gain), 0, 'f', 1)
                    .arg(r.clippedSamples)
                    .arg(r.clippedSamples ? " — lower levels or enable Normalize" : ""));
            render_->setText("Close");
            render_->setEnabled(true);
        } catch (const std::exception& e) {
            status_->setText(e.what());
            render_->setEnabled(true);
            for (auto* w : findChildren<QWidget*>())
                if (w != progress_)
                    w->setEnabled(true);
            dither_->setEnabled(depth_->currentData().toInt() != 32);
        }
    });
}
ExportDialog::~ExportDialog() {
    cancel_.store(true);
    if (job_.valid())
        job_.wait(); // Join before the worker's project/progress atomics are destroyed.
}
ExportOptions ExportDialog::options() const {
    ExportOptions o;
    o.sampleRate = rate_->currentData().toInt();
    o.depth = WavDepth(depth_->currentData().toInt());
    o.channels = channels_->currentData().toInt();
    o.includeTail = tail_->isChecked();
    o.normalize = normalize_->isChecked();
    o.dither = dither_->isEnabled() && dither_->isChecked();
    if (range_->currentIndex() == 1) {
        o.startBeat = project_.loopStart;
        o.endBeat = project_.loopEnd;
    }
    if (range_->currentIndex() == 2) {
        o.startBeat = selectionStart_;
        o.endBeat = selectionEnd_;
    }
    return o;
}
void ExportDialog::updateEstimate() {
    if (running_ || done_)
        return;
    auto o = options();
    double beats = (o.endBeat < 0 ? project_.endBeat() : o.endBeat) - o.startBeat;
    double sec = beats * 60 / project_.bpm + (o.includeTail ? project_.tailSeconds() : 0);
    status_->setText(
        QString("Approx. %1 seconds · %2 MiB\nFloat output preserves headroom above 0 dBFS. "
                "Integer PCM saturates safely.")
            .arg(sec, 0, 'f', 2)
            .arg(sec * o.sampleRate * o.channels * int(o.depth) / 8 / 1048576., 0, 'f', 1));
}
void ExportDialog::startRender() {
    if (running_)
        return;
    QString path = path_->text().trimmed();
    if (path.isEmpty()) {
        status_->setText("Choose a destination WAV file.");
        return;
    }
    if (!path.endsWith(".wav", Qt::CaseInsensitive))
        path += ".wav";
    path_->setText(path);
    if (std::filesystem::exists(path.toStdString()) &&
        QMessageBox::question(this, "Replace WAV?",
                              "Replace the existing file only after a successful export?") !=
            QMessageBox::Yes)
        return;
    auto o = options();
    cancel_.store(false);
    percent_.store(0);
    running_ = true;
    for (auto* w : findChildren<QWidget*>())
        if (w != progress_ && !qobject_cast<QLabel*>(w))
            w->setEnabled(false);
    // Keep the actual Cancel button responsive while all editable options are frozen.
    for (auto* b : findChildren<QPushButton*>())
        if (b->text() == "Cancel")
            b->setEnabled(true);
    status_->setText("Rendering… Cancel preserves any existing destination.");
    job_ = std::async(std::launch::async, [this, o, path] {
        return renderAudio(project_, path.toStdString(), o, [this](double p) {
            percent_.store(p);
            return !cancel_.load();
        });
    });
}
void ExportDialog::reject() {
    if (running_) {
        cancel_.store(true);
        status_->setText("Cancelling…");
        return;
    }
    if (done_)
        accept();
    else
        QDialog::reject();
}
} // namespace reverie
