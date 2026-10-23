// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/InspectorPanel.h"
#include "ui/Editor.h"
#include "ui/PianoRoll.h"
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
namespace reverie {
InspectorPanel::InspectorPanel(Editor& e) : e_(e) {
    setObjectName("selectionInspector");
    auto* l = new QVBoxLayout(this);
    title_ = new QLabel("Selection");
    title_->setStyleSheet("font-size:14px;font-weight:600");
    l->addWidget(title_);
    form_ = new QFormLayout;
    auto* form = form_;
    name_ = new QLineEdit;
    form->addRow("Name", name_);
    auto number = [&](QString label, double lo, double hi) {
        auto* spin = new QDoubleSpinBox;
        spin->setRange(lo, hi);
        spin->setDecimals(3);
        spin->setSingleStep(.25);
        spin->setKeyboardTracking(false);
        form->addRow(label, spin);
        return spin;
    };
    beat_ = number("Start / beats", 0, 100000);
    length_ = number("Length / beats", .001, 100000);
    velocity_ = number("Velocity", .01, 1);
    offset_ = number("Source offset / s", 0, 100000);
    pitch_ = new QSpinBox;
    pitch_->setRange(0, 127);
    form->addRow("MIDI key", pitch_);
    l->addLayout(form);
    l->addStretch();
    connect(name_, &QLineEdit::editingFinished, this, [this] {
        if (updating_)
            return;
        if (auto* c = findClip(e_.project, e_.selectedClip))
            e_.edit([&] { c->name = name_->text().toStdString(); });
    });
    auto commit = [this](int field, double value) {
        if (updating_)
            return;
        auto* c = findClip(e_.project, e_.selectedClip);
        if (!c)
            return;
        e_.edit([&] {
            const Note* reference = nullptr;
            for (const auto& n : c->notes)
                if (e_.piano()->selection.contains(n.id)) {
                    reference = &n;
                    break;
                }
            double deltaBeat = reference ? value - reference->beat : 0;
            int deltaKey = reference ? int(value) - reference->key : 0;
            if (reference) {
                for (auto& n : c->notes)
                    if (e_.piano()->selection.contains(n.id)) {
                        if (field == 0)
                            n.beat = std::max(0., n.beat + deltaBeat);
                        else if (field == 1)
                            n.duration = value;
                        else if (field == 2)
                            n.velocity = value;
                        else if (field == 4)
                            n.key = std::clamp(n.key + deltaKey, 0, 127);
                    }
            } else {
                if (field == 0)
                    c->beat = value;
                else if (field == 1)
                    c->length = value;
                else if (field == 3)
                    c->sourceOffset = value;
            }
        });
    };
    connect(beat_, &QDoubleSpinBox::valueChanged, this, [commit](double v) { commit(0, v); });
    connect(length_, &QDoubleSpinBox::valueChanged, this, [commit](double v) { commit(1, v); });
    connect(velocity_, &QDoubleSpinBox::valueChanged, this, [commit](double v) { commit(2, v); });
    connect(offset_, &QDoubleSpinBox::valueChanged, this, [commit](double v) { commit(3, v); });
    connect(pitch_, &QSpinBox::valueChanged, this, [commit](int v) { commit(4, v); });
    refresh();
}
void InspectorPanel::refresh() {
    updating_ = true;
    auto* c = findClip(e_.project, e_.selectedClip);
    const Note* note = nullptr;
    if (c)
        for (const auto& n : c->notes)
            if (e_.piano()->selection.contains(n.id)) {
                note = &n;
                break;
            }
    bool audio =
        e_.selectedTrack >= 0 && e_.project.tracks[e_.selectedTrack].kind == TrackKind::Audio;
    title_->setText(note ? "Note properties"
                    : c  ? (audio ? "Audio clip properties" : "MIDI clip properties")
                         : "Select a clip or note");
    form_->setRowVisible(name_, c && !note);
    form_->setRowVisible(beat_, c != nullptr);
    form_->setRowVisible(length_, c != nullptr);
    form_->setRowVisible(velocity_, note != nullptr);
    form_->setRowVisible(pitch_, note != nullptr);
    form_->setRowVisible(offset_, c && audio);
    name_->setEnabled(c && !note);
    beat_->setEnabled(c);
    length_->setEnabled(c);
    velocity_->setEnabled(note);
    pitch_->setEnabled(note);
    offset_->setEnabled(c && audio);
    if (c) {
        name_->setText(QString::fromStdString(c->name));
        beat_->setValue(note ? note->beat : c->beat);
        length_->setValue(note ? note->duration : c->length);
        offset_->setValue(c->sourceOffset);
    }
    if (note) {
        pitch_->setValue(note->key);
        velocity_->setValue(note->velocity);
    }
    updating_ = false;
}
} // namespace reverie
