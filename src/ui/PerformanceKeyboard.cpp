// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/PerformanceKeyboard.h"
#include "ui/Editor.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTextEdit>
#include <QVBoxLayout>
namespace reverie {
static bool textInput(QObject* object) {
    return qobject_cast<QLineEdit*>(object) || qobject_cast<QTextEdit*>(object) ||
           qobject_cast<QPlainTextEdit*>(object) || qobject_cast<QAbstractSpinBox*>(object) ||
           qobject_cast<QComboBox*>(object);
}
static bool black(int key) {
    int p = key % 12;
    return p == 1 || p == 3 || p == 6 || p == 8 || p == 10;
}
PerformanceKeyboard::PerformanceKeyboard(Editor& e) : editor_(e) {
    setObjectName("performanceKeyboard");
    setMinimumHeight(180);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(5);
    auto* controls = new QHBoxLayout;
    target_ = new QLabel;
    controls->addWidget(target_, 1);
    typing_ = new QCheckBox("Musical Typing");
    typing_->setObjectName("musicalTyping");
    controls->addWidget(typing_);
    controls->addWidget(new QLabel("Octave"));
    octave_ = new QSpinBox;
    octave_->setRange(-1, 7);
    octave_->setValue(4);
    controls->addWidget(octave_);
    controls->addWidget(new QLabel("Velocity"));
    velocity_ = new QSpinBox;
    velocity_->setRange(1, 127);
    velocity_->setValue(100);
    controls->addWidget(velocity_);
    sustain_ = new QCheckBox("Sustain · Tab");
    sustain_->setObjectName("keyboardSustain");
    controls->addWidget(sustain_);
    layout->addLayout(controls);
    keys_ = new QWidget;
    keys_->setObjectName("virtualPianoKeys");
    keys_->setMinimumHeight(110);
    keys_->setMouseTracking(true);
    keys_->setFocusPolicy(Qt::StrongFocus);
    keys_->setAccessibleName("Virtual piano · MIDI 21 to 108");
    keys_->installEventFilter(this);
    layout->addWidget(keys_, 1);
    auto* hint = new QLabel("A W S E D F T G Y H U J K → C D♭ D E♭ E F G♭ G A♭ A B♭ B C  ·  Z/X "
                            "octave  ·  C/V velocity  ·  MIDI 21–108");
    hint->setStyleSheet("color:#BEB2A4;font-family:IBM Plex Mono;font-size:10px");
    layout->addWidget(hint);
    connect(typing_, &QCheckBox::toggled, this, [this](bool) { releaseAll(); });
    connect(octave_, &QSpinBox::valueChanged, this, [this] { releaseAll(); });
    connect(sustain_, &QCheckBox::toggled, this,
            [this](bool on) { editor_.inputController(64, on ? 127 : 0); });
    qApp->installEventFilter(this);
    refresh();
}
PerformanceKeyboard::~PerformanceKeyboard() {
    qApp->removeEventFilter(this);
}
QRectF PerformanceKeyboard::keyRect(int key) const {
    int whites = 0;
    for (int n = 21; n < key; ++n)
        if (!black(n))
            ++whites;
    double w = keys_->width() / 52.;
    return black(key) ? QRectF(whites * w - w * .32, 0, w * .64, keys_->height() * .62)
                      : QRectF(whites * w, 0, w, keys_->height());
}
int PerformanceKeyboard::keyAt(QPointF p) const {
    for (int n = 21; n <= 108; ++n)
        if (black(n) && keyRect(n).contains(p))
            return n;
    for (int n = 21; n <= 108; ++n)
        if (!black(n) && keyRect(n).contains(p))
            return n;
    return -1;
}
void PerformanceKeyboard::showActivity(int n, bool on) {
    if (on)
        pressed_.insert(n);
    else
        pressed_.remove(n);
    keys_->update();
}
void PerformanceKeyboard::releaseAll() {
    QSignalBlocker blocker(sustain_);
    sustain_->setChecked(false);
    typingNotes_.clear();
    pressed_.clear();
    mouseKey_ = -1;
    editor_.releasePerformance();
    keys_->update();
}
void PerformanceKeyboard::setTyping(bool on) {
    typing_->setChecked(on);
}
void PerformanceKeyboard::refresh() {
    QString name = "Select a MIDI track";
    if (editor_.selectedTrack >= 0 && editor_.selectedTrack < int(editor_.project.tracks.size())) {
        auto& t = editor_.project.tracks[editor_.selectedTrack];
        name = QString::fromStdString(t.preset.name) + (t.armed ? " · ARMED" : "");
        auto identity = std::to_string(t.id) + t.preset.instrumentId + t.preset.patchId;
        auto instrumentName = QString::fromStdString(t.preset.name).toLower();
        bool drums = t.preset.kind == InstrumentKind::Drums || instrumentName.contains("kit") ||
                     instrumentName.contains("gm percussion");
        if (drums)
            name += " · GM 36 Kick / 38 Snare / 42 Hat / 46 Open hat";
        if (identity != targetIdentity_) {
            targetIdentity_ = identity;
            if (drums)
                octave_->setValue(2);
            else if (t.preset.kind == InstrumentKind::Sfizz)
                octave_->setValue(std::clamp(t.preset.rootNote / 12 - 1, -1, 7));
        }
    }
    target_->setText(name);
    keys_->update();
}
void PerformanceKeyboard::mouseNote(int n) {
    if (n == mouseKey_)
        return;
    if (mouseKey_ >= 0)
        editor_.inputNote(mouseKey_, false, 0, 0, 1);
    mouseKey_ = n;
    if (n >= 0)
        editor_.inputNote(n, true, velocity_->value() / 127.f, 0, 1);
}
void PerformanceKeyboard::hideEvent(QHideEvent*) {
    releaseAll();
}
bool PerformanceKeyboard::eventFilter(QObject* obj, QEvent* ev) {
    if (obj == keys_) {
        if (ev->type() == QEvent::Paint) {
            QPainter p(keys_);
            p.fillRect(keys_->rect(), QColor("#33312D"));
            for (bool dark : {false, true})
                for (int n = 21; n <= 108; ++n)
                    if (black(n) == dark) {
                        auto r = keyRect(n);
                        p.setPen(QColor("#302D28"));
                        p.setBrush(pressed_.contains(n)
                                       ? QColor("#7EA89B")
                                       : (dark ? QColor("#302D28") : QColor("#E8E1D5")));
                        p.drawRect(r.adjusted(0, 0, -1, -1));
                        if (!dark && n % 12 == 0) {
                            p.setPen(QColor("#302D28"));
                            p.drawText(r.adjusted(0, 0, 0, -4), Qt::AlignBottom | Qt::AlignHCenter,
                                       QString("C%1\n%2").arg(n / 12 - 1).arg(n));
                        }
                    }
            return true;
        }
        if (ev->type() == QEvent::MouseButtonPress) {
            auto* m = static_cast<QMouseEvent*>(ev);
            if (m->button() == Qt::LeftButton) {
                keys_->grabMouse();
                mouseNote(keyAt(m->position()));
                return true;
            }
        }
        if (ev->type() == QEvent::MouseMove) {
            auto* m = static_cast<QMouseEvent*>(ev);
            keys_->setToolTip(QString("MIDI %1").arg(keyAt(m->position())));
            if (m->buttons() & Qt::LeftButton)
                mouseNote(keyAt(m->position()));
        }
        if (ev->type() == QEvent::MouseButtonRelease) {
            mouseNote(-1);
            keys_->releaseMouse();
            return true;
        }
    }
    if (ev->type() == QEvent::ApplicationDeactivate ||
        (obj == &editor_ && ev->type() == QEvent::WindowDeactivate)) {
        releaseAll();
        return false;
    }
    if (ev->type() == QEvent::FocusOut && (!typingNotes_.isEmpty() || mouseKey_ >= 0)) {
        releaseAll();
    }
    if (ev->type() == QEvent::FocusIn && textInput(obj) &&
        (!typingNotes_.isEmpty() || mouseKey_ >= 0))
        releaseAll();
    if (!isVisible() || !typing_->isChecked())
        return false;
    if (ev->type() != QEvent::KeyPress && ev->type() != QEvent::KeyRelease)
        return false;
    auto* k = static_cast<QKeyEvent*>(ev);
    bool on = ev->type() == QEvent::KeyPress;
    // Always release a tracked key even when focus moved into a text field.
    if (!on && typingNotes_.contains(k->key())) {
        if (!k->isAutoRepeat()) {
            editor_.inputNote(typingNotes_.take(k->key()), false, 0, 0, 2);
        }
        return true;
    }
    if (QApplication::activePopupWidget() || QApplication::activeModalWidget())
        return false;
    auto* focus = qApp->focusWidget();
    if (textInput(focus) || textInput(obj)) {
        if (on && (!typingNotes_.isEmpty() || mouseKey_ >= 0))
            releaseAll();
        return false;
    }
    if (k->modifiers() & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier))
        return false;
    if (k->key() == Qt::Key_Tab) {
        if (!k->isAutoRepeat())
            sustain_->setChecked(on);
        return true;
    }
    if (!on)
        return false;
    if (k->isAutoRepeat())
        return true;
    if (k->key() == Qt::Key_Z || k->key() == Qt::Key_X) {
        octave_->setValue(octave_->value() + (k->key() == Qt::Key_X ? 1 : -1));
        return true;
    }
    if (k->key() == Qt::Key_C || k->key() == Qt::Key_V) {
        velocity_->setValue(velocity_->value() + (k->key() == Qt::Key_V ? 8 : -8));
        return true;
    }
    const QString map = "AWSEDFTGYHUJK";
    int index = map.indexOf(QChar(k->key()));
    if (index >= 0) {
        int n = std::clamp((octave_->value() + 1) * 12 + index, 0, 127);
        if (!typingNotes_.contains(k->key())) {
            typingNotes_[k->key()] = n;
            editor_.inputNote(n, true, velocity_->value() / 127.f, 0, 2);
        }
        return true;
    }
    return false;
}
} // namespace reverie
