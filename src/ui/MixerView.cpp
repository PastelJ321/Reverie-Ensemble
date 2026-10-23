// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/MixerView.h"
#include "ui/DesignTokens.h"
#include "ui/Editor.h"
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
namespace reverie {
class StereoMeter : public QWidget {
    Editor& e_;
    int i_;

  public:
    StereoMeter(Editor& e, int i) : e_(e), i_(i) {
        setFixedWidth(24);
        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, qOverload<>(&QWidget::update));
        timer->start(33);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), design::shell);
        for (int c = 0; c < 2; ++c) {
            float value = i_ < 0 ? e_.engine.masterPeak()
                                 : (c ? e_.engine.trackPeakR(i_) : e_.engine.trackPeakL(i_));
            double db = value > 0 ? 20 * std::log10(value) : -60;
            int h = int(std::clamp((db + 60) / 60., 0., 1.) * height());
            p.fillRect(QRect(c * 12 + 2, height() - h, 8, h),
                       value > 1 ? design::error : design::signal);
        }
    }
};
MixerView::MixerView(Editor& e) : e_(e) {
    setObjectName("mixerView");
    auto* l = new QVBoxLayout(this);
    auto* heading = new QLabel("MIXER    /    dBFS · stereo output");
    heading->setStyleSheet("font-size:14px;font-weight:600");
    l->addWidget(heading);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    channels_ = new QWidget;
    scroll->setWidget(channels_);
    l->addWidget(scroll);
}
void MixerView::refresh() {
    if (auto* old = channels_->layout()) {
        while (auto* item = old->takeAt(0)) {
            delete item->widget();
            delete item;
        }
        delete old;
    }
    auto* layout = new QHBoxLayout(channels_);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(1);
    for (int index = 0; index <= int(e_.project.tracks.size()); ++index) {
        bool master = index == int(e_.project.tracks.size());
        auto* strip = new QWidget;
        strip->setFixedWidth(130);
        strip->setStyleSheet(
            QString("background:%1;border-right:1px solid #777065;")
                .arg(!master && e_.selectedTrack == index ? "#4A453E" : "#33312D"));
        auto* column = new QVBoxLayout(strip);
        auto* name = new QPushButton(
            master ? "MASTER" : QString::fromStdString(e_.project.tracks[index].name));
        name->setToolTip(name->text());
        column->addWidget(name);
        if (!master)
            connect(name, &QPushButton::clicked, this, [this, index] {
                e_.select(index);
                refresh();
            });
        auto* value = new QDoubleSpinBox;
        value->setRange(-60, 6);
        value->setSuffix(" dB");
        value->setDecimals(1);
        float gain = master ? e_.project.master : e_.project.tracks[index].volume;
        value->setValue(gain > 0 ? 20 * std::log10(gain) : -60);
        column->addWidget(value);
        auto* meters = new QHBoxLayout;
        auto* slider = new QSlider(Qt::Vertical);
        slider->setRange(-600, 60);
        slider->setValue(int(value->value() * 10));
        slider->setFixedHeight(240);
        meters->addWidget(slider);
        auto* meter = new StereoMeter(e_, master ? -1 : index);
        meter->setFixedHeight(240);
        meters->addWidget(meter);
        column->addLayout(meters);
        connect(slider, &QSlider::valueChanged, value,
                [value](int v) { value->setValue(v / 10.); });
        connect(value, &QDoubleSpinBox::valueChanged, this,
                [this, index, master, slider](double db) {
                    QSignalBlocker guard(slider);
                    slider->setValue(int(db * 10));
                    float gain = db <= -60 ? 0 : std::pow(10.f, float(db) / 20);
                    e_.edit([&] {
                        if (master)
                            e_.project.master = gain;
                        else
                            e_.project.tracks[index].volume = gain;
                    });
                });
        if (!master) {
            auto* pan = new QDoubleSpinBox;
            pan->setRange(-1, 1);
            pan->setSingleStep(.1);
            pan->setPrefix("Pan ");
            pan->setValue(e_.project.tracks[index].pan);
            column->addWidget(pan);
            connect(pan, &QDoubleSpinBox::valueChanged, this, [this, index](double v) {
                e_.edit([&] { e_.project.tracks[index].pan = float(v); });
            });
            auto* switches = new QHBoxLayout;
            for (bool solo : {false, true}) {
                auto* box = new QCheckBox(solo ? "S" : "M");
                box->setChecked(solo ? e_.project.tracks[index].solo
                                     : e_.project.tracks[index].mute);
                switches->addWidget(box);
                connect(box, &QCheckBox::toggled, this, [this, index, solo](bool on) {
                    e_.edit([&] {
                        if (solo)
                            e_.project.tracks[index].solo = on;
                        else
                            e_.project.tracks[index].mute = on;
                    });
                });
            }
            column->addLayout(switches);
            auto* enabled = new QCheckBox("EQ / Space");
            enabled->setChecked(e_.project.tracks[index].effects.enabled);
            column->addWidget(enabled);
            connect(enabled, &QCheckBox::toggled, this, [this, index](bool on) {
                e_.edit([&] { e_.project.tracks[index].effects.enabled = on; });
            });
            for (int param = 0; param < 4; ++param) {
                auto* fx = new QDoubleSpinBox;
                fx->setRange(param < 2 ? -18 : 0, param < 2 ? 18 : 1);
                fx->setDecimals(param < 2 ? 1 : 2);
                fx->setSingleStep(param < 2 ? 1 : .05);
                fx->setPrefix(QStringList{"Low ", "High ", "Delay ", "Reverb "}[param]);
                const auto& s = e_.project.tracks[index].effects;
                fx->setValue(param == 0   ? s.lowDb
                             : param == 1 ? s.highDb
                             : param == 2 ? s.delayMix
                                          : s.reverbMix);
                column->addWidget(fx);
                connect(fx, &QDoubleSpinBox::valueChanged, this, [this, index, param](double v) {
                    e_.edit([&] {
                        auto& s = e_.project.tracks[index].effects;
                        if (param == 0)
                            s.lowDb = v;
                        else if (param == 1)
                            s.highDb = v;
                        else if (param == 2)
                            s.delayMix = v;
                        else
                            s.reverbMix = v;
                    });
                });
            }
        }
        column->addStretch();
        layout->addWidget(strip);
    }
    layout->addStretch();
}
} // namespace reverie
