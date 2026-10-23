// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "ui/PianoRoll.h"
#include "ui/Editor.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
namespace reverie {
PianoRoll::PianoRoll(Editor& e) : e_(e) {
    setFocusPolicy(Qt::StrongFocus);
    setObjectName("pianoRoll");
    updateExtent();
}
void PianoRoll::updateExtent() {
    auto* c = findClip(e_.project, e_.selectedClip);
    setMinimumSize(keys + int(std::max(16., c ? c->length : 16.) * e_.pixelsPerBeat) + 30,
                   ruler + (topKey - bottomKey + 1) * keyHeight);
    resize(minimumSize());
    update();
}
QRectF PianoRoll::noteRect(const Note& n) const {
    return {keys + n.beat * e_.pixelsPerBeat, double(ruler + (topKey - n.key) * keyHeight + 1),
            n.duration * e_.pixelsPerBeat, keyHeight - 2};
}
void PianoRoll::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#191d29"));
    auto* c = findClip(e_.project, e_.selectedClip);
    for (int key = bottomKey; key <= topKey; ++key) {
        int y = ruler + (topKey - key) * keyHeight;
        int pitch = key % 12;
        bool black = pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
        p.fillRect(QRect(keys, y, width() - keys, keyHeight),
                   QColor(black ? "#1b1e2b" : "#222735"));
        p.fillRect(QRect(0, y, keys - 2, keyHeight - 1), QColor(black ? "#33394c" : "#adb5c9"));
        if (pitch == 0) {
            p.setPen(QColor("#273044"));
            p.drawText(QRect(0, y, keys - 6, keyHeight), Qt::AlignRight | Qt::AlignVCenter,
                       QString("C%1").arg(key / 12 - 1));
        }
        p.setPen(QColor(pitch == 0 ? "#454d64" : "#2d3344"));
        p.drawLine(keys, y, width(), y);
    }
    const int first = std::max(0, int((event->rect().left() - keys) / e_.pixelsPerBeat / e_.grid));
    const int last = int((event->rect().right() - keys) / e_.pixelsPerBeat / e_.grid) + 1;
    for (int i = first; i <= last; ++i) {
        double beat = i * e_.grid;
        int x = keys + int(beat * e_.pixelsPerBeat);
        bool whole = std::abs(beat - std::round(beat)) < .001;
        p.setPen(QColor(whole ? "#49516a" : "#30374a"));
        p.drawLine(x, ruler, x, height());
    }
    if (c) {
        int end = keys + int(c->length * e_.pixelsPerBeat);
        p.fillRect(QRect(end, ruler, std::max(0, width() - end), height()),
                   QColor(12, 14, 22, 160));
        p.setRenderHint(QPainter::Antialiasing);
        for (const auto& n : c->notes) {
            auto r = noteRect(n);
            if (!r.intersects(event->rect()))
                continue;
            QColor color = selection.contains(n.id) ? QColor("#cec9ff") : QColor("#8e91e7");
            p.setBrush(color);
            p.setPen(color.lighter(125));
            p.drawRoundedRect(r.adjusted(0, 0, -1, 0), 3, 3);
            p.fillRect(QRectF(r.x() + 2, r.bottom() - 3, (r.width() - 4) * n.velocity, 2),
                       QColor("#3a3e71"));
        }
    }
    if (marquee_) {
        p.setPen(QColor("#c3bdff"));
        p.setBrush(QColor(140, 140, 230, 40));
        p.drawRect(selectionRect_.normalized());
    }
    p.fillRect(QRect(0, 0, width(), ruler), QColor("#141824"));
    p.setPen(QColor("#b1bad0"));
    p.drawText(8, 20, "KEYS");
    double bar = e_.project.numerator * 4. / e_.project.denominator;
    for (int i = 0; i < int(width() / e_.pixelsPerBeat / bar) + 1; ++i)
        p.drawText(keys + int(i * bar * e_.pixelsPerBeat) + 5, 20, QString::number(i + 1));
    if (!c) {
        p.fillRect(rect(), QColor(18, 22, 32, 175));
        p.setPen(QColor("#b2bad0"));
        p.drawText(QRect(90, ruler + 400, 650, 60),
                   "Double-click a MIDI clip in the arrangement to edit notes.");
    }
}
void PianoRoll::mousePressEvent(QMouseEvent* ev) {
    setFocus();
    if (ev->button() != Qt::LeftButton && ev->button() != Qt::RightButton)
        return;
    auto* c = findClip(e_.project, e_.selectedClip);
    if (!c)
        return;
    const auto pt = ev->position();
    Id hit = 0;
    for (auto it = c->notes.rbegin(); it != c->notes.rend(); ++it)
        if (noteRect(*it).contains(pt)) {
            hit = it->id;
            break;
        }
    if (ev->button() == Qt::RightButton) {
        if (hit) {
            selection = {hit};
            erase();
        }
        return;
    }
    if (hit) {
        if (ev->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) {
            if (selection.contains(hit))
                selection.remove(hit);
            else
                selection.insert(hit);
        } else if (!selection.contains(hit))
            selection = {hit};
        drag_ = hit;
        press_ = pt;
        original_ = c->notes;
        const auto it = std::find_if(c->notes.begin(), c->notes.end(),
                                     [&](const Note& n) { return n.id == hit; });
        resizing_ = pt.x() > noteRect(*it).right() - 8;
        changed_ = false;
        update();
        return;
    }
    if (pt.x() < keys || pt.y() < ruler)
        return;
    if (ev->modifiers() & Qt::ShiftModifier) {
        marquee_ = true;
        press_ = pt;
        selectionRect_ = QRectF(pt, pt);
        selection.clear();
        return;
    }
    double beat = std::max(0., e_.snapped((pt.x() - keys) / e_.pixelsPerBeat));
    if (beat >= c->length)
        return;
    int key = std::clamp(topKey - int((pt.y() - ruler) / keyHeight), bottomKey, topKey);
    e_.edit([&] {
        Note n;
        n.beat = beat;
        n.key = key;
        n.duration = std::min(e_.grid * 2, c->length - beat);
        c->notes.push_back(n);
        selection = {n.id};
    });
}
void PianoRoll::mouseMoveEvent(QMouseEvent* ev) {
    auto* c = findClip(e_.project, e_.selectedClip);
    if (!c)
        return;
    auto pt = ev->position();
    if (marquee_) {
        selectionRect_ = QRectF(press_, pt).normalized();
        selection.clear();
        for (const auto& n : c->notes)
            if (noteRect(n).intersects(selectionRect_))
                selection.insert(n.id);
        update();
        return;
    }
    if (!drag_)
        return;
    if (!changed_ && (pt - press_).manhattanLength() < 3)
        return;
    if (!changed_) {
        e_.beginEdit();
        changed_ = true;
    }
    double delta = (pt.x() - press_.x()) / e_.pixelsPerBeat;
    int deltaKey = -int(std::round((pt.y() - press_.y()) / keyHeight));
    double minBeat = 100000., maxBeat = 0;
    int minKey = 127, maxKey = 0;
    for (const auto& n : original_)
        if (selection.contains(n.id)) {
            minBeat = std::min(minBeat, n.beat);
            maxBeat = std::max(maxBeat, n.beat);
            minKey = std::min(minKey, n.key);
            maxKey = std::max(maxKey, n.key);
        }
    double snapped = e_.snap ? std::round(delta / e_.grid) * e_.grid : delta;
    snapped = std::clamp(snapped, -minBeat, std::max(-minBeat, c->length - e_.grid - maxBeat));
    deltaKey = std::clamp(deltaKey, -minKey, 127 - maxKey);
    for (auto& n : c->notes)
        if (selection.contains(n.id)) {
            auto it = std::find_if(original_.begin(), original_.end(),
                                   [&](const Note& old) { return old.id == n.id; });
            if (it == original_.end())
                continue;
            if (resizing_)
                n.duration = std::max(
                    e_.grid,
                    it->duration + (e_.snap ? std::round(delta / e_.grid) * e_.grid : delta));
            else {
                n.beat = it->beat + snapped;
                n.key = it->key + deltaKey;
            }
        }
    update();
}
void PianoRoll::mouseReleaseEvent(QMouseEvent*) {
    if (changed_)
        e_.endEdit();
    drag_ = 0;
    changed_ = false;
    marquee_ = false;
    update();
}
void PianoRoll::erase() {
    auto* c = findClip(e_.project, e_.selectedClip);
    if (!c || selection.empty())
        return;
    e_.edit(
        [&] { std::erase_if(c->notes, [&](const Note& n) { return selection.contains(n.id); }); });
    selection.clear();
    update();
}
void PianoRoll::quantize() {
    auto* c = findClip(e_.project, e_.selectedClip);
    if (!c)
        return;
    e_.edit([&] {
        for (auto& n : c->notes)
            if (selection.empty() || selection.contains(n.id)) {
                n.beat = std::clamp(std::round(n.beat / e_.grid) * e_.grid, 0.,
                                    std::max(0., c->length - e_.grid));
                n.duration = std::max(e_.grid, std::round(n.duration / e_.grid) * e_.grid);
            }
    });
}
void PianoRoll::setVelocity(double value) {
    auto* c = findClip(e_.project, e_.selectedClip);
    if (!c || selection.empty())
        return;
    e_.edit([&] {
        for (auto& n : c->notes)
            if (selection.contains(n.id))
                n.velocity = float(value);
    });
}
void PianoRoll::keyPressEvent(QKeyEvent* ev) {
    if (ev->key() == Qt::Key_Delete || ev->key() == Qt::Key_Backspace) {
        erase();
        return;
    }
    if (ev->matches(QKeySequence::SelectAll)) {
        if (auto* c = findClip(e_.project, e_.selectedClip))
            for (const auto& n : c->notes)
                selection.insert(n.id);
        update();
        return;
    }
    if (ev->key() == Qt::Key_Q) {
        quantize();
        return;
    }
    QWidget::keyPressEvent(ev);
}
} // namespace reverie
