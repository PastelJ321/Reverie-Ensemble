// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/InstrumentEditor.h"
#include "ui/Editor.h"
#include "ui/InstrumentArt.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
namespace reverie {
InstrumentEditor::InstrumentEditor(Editor& e) : e_(e) {
    setObjectName("smartControls");
    auto* form = new QFormLayout(this);
    form->setContentsMargins(0, 4, 0, 4);
    tone_ = new QDoubleSpinBox;
    tone_->setObjectName("smartTone");
    tone_->setRange(20, 24000);
    tone_->setDecimals(0);
    tone_->setSingleStep(250);
    tone_->setSuffix(" Hz");
    form->addRow("Tone / low-pass", tone_);
    time_ = new QDoubleSpinBox;
    time_->setObjectName("smartTime");
    time_->setRange(.001, 10);
    time_->setDecimals(3);
    time_->setSuffix(" s");
    timeLabel_ = new QLabel("Release");
    form->addRow(timeLabel_, time_);
    space_ = new QDoubleSpinBox;
    space_->setObjectName("smartRoom");
    space_->setRange(0, 1);
    space_->setSingleStep(.05);
    form->addRow("Room / reverb", space_);
    auto change = [this] {
        if (updating_ || e_.selectedTrack < 0)
            return;
        e_.edit([&] {
            auto& t = e_.project.tracks[e_.selectedTrack];
            t.preset.cutoff = float(tone_->value());
            auto family = instrumentFamily(t.preset);
            if (family == "strings" || family == "cello")
                t.preset.attack = float(time_->value());
            else
                t.preset.release = float(time_->value());
            t.effects.reverbMix = float(space_->value());
            if (t.effects.reverbMix > 0)
                t.effects.enabled = true;
        });
    };
    for (auto* c : {tone_, time_, space_})
        connect(c, &QDoubleSpinBox::valueChanged, this, change);
}
void InstrumentEditor::refresh() {
    updating_ = true;
    bool valid = e_.selectedTrack >= 0 && e_.selectedTrack < int(e_.project.tracks.size()) &&
                 e_.project.tracks[e_.selectedTrack].kind == TrackKind::Midi;
    setVisible(valid);
    if (valid) {
        const auto& t = e_.project.tracks[e_.selectedTrack];
        auto family = instrumentFamily(t.preset);
        bool strings = family == "strings" || family == "cello";
        timeLabel_->setText(strings ? "Bow attack" : "Release");
        time_->setValue(strings ? t.preset.attack : t.preset.release);
        time_->setVisible(family != "drums");
        timeLabel_->setVisible(family != "drums");
        tone_->setValue(t.preset.cutoff);
        space_->setValue(t.effects.reverbMix);
    }
    updating_ = false;
}
} // namespace reverie
