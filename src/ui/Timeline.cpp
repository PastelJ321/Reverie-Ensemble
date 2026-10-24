// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "ui/Timeline.h"
#include "ui/DesignTokens.h"
#include "ui/Editor.h"
#include "ui/InstrumentArt.h"
#include "ui/SoundBrowser.h"
#include <QColorDialog>
#include <QDragEnterEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
namespace reverie {
Timeline::Timeline(Editor& e) : e_(e) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAcceptDrops(true);
    setObjectName("timeline");
    updateExtent();
}
void Timeline::updateExtent() {
    setMinimumSize(header + int(std::max(32., e_.project.endBeat() + 8) * e_.pixelsPerBeat),
                   ruler + std::max(4, int(e_.project.tracks.size())) * rowHeight() + 60);
    resize(minimumSize());
    update();
}
QRectF Timeline::clipRect(int t, const Clip& c) const {
    return {header + c.beat * e_.pixelsPerBeat, double(ruler + t * rowHeight() + 7),
            c.length * e_.pixelsPerBeat, double(rowHeight() - 14)};
}
int Timeline::trackAt(double y) const {
    int i = int((y - ruler) / rowHeight());
    return y >= ruler && i >= 0 && i < int(e_.project.tracks.size()) ? i : -1;
}
Id Timeline::hit(QPointF p, int& track) const {
    track = trackAt(p.y());
    if (p.x() < horizontalOffset() + header)
        return 0;
    if (track < 0)
        return 0;
    for (auto it = e_.project.tracks[track].clips.rbegin();
         it != e_.project.tracks[track].clips.rend(); ++it)
        if (clipRect(track, *it).contains(p))
            return it->id;
    return 0;
}
void Timeline::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), design::paper);
    const double ppb = e_.pixelsPerBeat;
    double bar = e_.project.numerator * 4. / e_.project.denominator;
    const auto visible = event->rect();
    int first = std::max(0, int((visible.left() - header) / ppb));
    int last = int((visible.right() - header) / ppb) + 1;
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int beat = first; beat <= last; ++beat) {
        int x = header + int(beat * ppb);
        bool major = std::abs(std::remainder(double(beat), bar)) < .001;
        p.setPen(QColor(major ? "#B2A593" : "#CFC5B7"));
        p.drawLine(x, ruler, x, height());
    }
    p.setRenderHint(QPainter::Antialiasing, true);
    p.save();
    p.setClipRect(QRect(horizontalOffset() + header, 0, width(), height()));
    for (int i = 0; i < int(e_.project.tracks.size()); ++i)
        for (const auto& c : e_.project.tracks[i].clips) {
            auto r = clipRect(i, c);
            if (!r.intersects(visible))
                continue;
            QColor color = QColor::fromRgba(e_.project.tracks[i].color);
            color.setAlpha(e_.project.tracks[i].mute ? 70 : 150);
            p.setPen(QPen(c.id == e_.selectedClip ? design::ink : color.lighter(125),
                          c.id == e_.selectedClip ? 2
                          : c.id == hover_        ? 1.5
                                                  : 1));
            p.setBrush(color.lighter(135));
            p.drawRoundedRect(r.adjusted(1, 0, -1, 0), 2, 2);
            p.save();
            p.setClipRect(r.adjusted(4, 3, -4, -3));
            p.setPen(design::ink);
            p.drawText(r.adjusted(9, 3, -6, 0), Qt::AlignTop | Qt::AlignLeft,
                       QString::fromStdString(c.name));
            if (c.sample) {
                p.setPen(QColor("#49473F"));
                int start = std::max(int(r.left() + 3), visible.left()),
                    end = std::min(int(r.right() - 3), visible.right());
                double framesPerPixel = 60. / e_.project.bpm / ppb * c.sample->sampleRate;
                double offset = c.sourceOffset * c.sample->sampleRate;
                for (int x = start; x < end; ++x) {
                    double f = offset + (x - r.left()) * framesPerPixel;
                    auto a = std::size_t(std::max(0., f) / 256),
                         b = std::size_t(std::max(0., f + framesPerPixel) / 256);
                    float lo = 0, hi = 0;
                    for (auto j = a; j <= b && j < c.sample->peaks.size(); ++j) {
                        lo = std::min(lo, c.sample->peaks[j].first);
                        hi = std::max(hi, c.sample->peaks[j].second);
                    }
                    double center = r.center().y() + 8;
                    p.drawLine(QPointF(x, center + lo * 20), QPointF(x, center + hi * 20));
                }
            } else if (!c.mediaPath.empty()) {
                p.drawText(r.adjusted(8, 25, -6, 0), "Missing WAV — relink by importing source");
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(design::control);
                for (const auto& n : c.notes)
                    if (n.beat < c.length) {
                        double x = r.left() + n.beat * ppb,
                               w = std::min(n.duration, c.length - n.beat) * ppb;
                        double y = r.bottom() - 8 - (n.key - 36) * .55;
                        p.drawRect(QRectF(x, y, std::max(2., w), 3));
                    }
            }
            p.restore();
        }
    p.restore();
    p.save();
    p.translate(horizontalOffset(), 0);
    for (int i = 0; i < int(e_.project.tracks.size()); ++i) {
        const auto& t = e_.project.tracks[i];
        int y = ruler + i * rowHeight();
        p.fillRect(QRect(0, y, header, rowHeight()),
                   QColor(i == e_.selectedTrack ? "#DCD2C3" : (i % 2 ? "#E5DCCE" : "#E8E1D5")));
        p.setPen(design::grid);
        p.drawLine(0, y + rowHeight(), header, y + rowHeight());
        p.fillRect(QRect(0, y, 4, rowHeight()), QColor::fromRgba(t.color));
        p.setPen(design::ink);
        p.drawText(QRect(46, y + 10, header - 54, 25), Qt::AlignVCenter,
                   p.fontMetrics().elidedText(QString::fromStdString(t.name), Qt::ElideRight,
                                              header - 56));
        p.setPen(design::mutedInk);
        p.drawText(14, y + rowHeight() - 20, t.kind == TrackKind::Midi ? "MIDI" : "AUDIO");
        p.drawPixmap(10, y + 10,
                     instrumentDrawing(instrumentFamily(t.preset), QSize(30, 24), false,
                                       devicePixelRatioF()));
        for (int b = 0; b < 2; ++b) {
            QRect r(header - 72 + b * 31, y + rowHeight() - 34, 25, 23);
            p.setBrush((b ? t.solo : t.mute) ? QColor(b ? "#D4A67A" : "#BC655C") : design::grid);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(r, 2, 2);
            p.setPen(design::ink);
            p.drawText(r, Qt::AlignCenter, b ? "S" : "M");
        }
        float meter = std::min(1.f, e_.engine.trackPeak(i));
        p.fillRect(QRect(14, y + rowHeight() - 6, int((header - 30) * meter), 3), design::signal);
    }
    p.restore();
    p.fillRect(QRect(0, 0, width(), ruler), QColor("#DDD3C4"));
    p.setPen(design::mutedInk);
    p.fillRect(QRect(horizontalOffset(), 0, header, ruler), design::surface);
    p.setPen(design::text);
    p.drawText(horizontalOffset() + 14, 22, "ARRANGEMENT");
    p.save();
    p.setClipRect(QRect(horizontalOffset() + header, 0, width(), height()));
    for (int b = std::max(0, int(first / bar)); b <= int(last / bar) + 1; ++b) {
        int x = header + int(b * bar * ppb);
        p.setPen(design::mutedInk);
        p.drawLine(x, 20, x, ruler);
        p.drawText(x + 6, 17, QString::number(b + 1));
    }
    if (e_.project.loopEnabled) {
        int start = header + int(e_.project.loopStart * ppb),
            end = header + int(e_.project.loopEnd * ppb);
        p.fillRect(QRect(start, 2, end - start, 7), design::selection);
        p.fillRect(QRect(start, 1, 5, 12), design::ink);
        p.fillRect(QRect(end - 4, 1, 5, 12), design::ink);
    }
    double beat =
        TempoMap{e_.project.bpm, e_.engine.sampleRate()}.beatAt(e_.engine.displayPosition());
    int playX = header + int(beat * ppb);
    p.setPen(QPen(QColor("#443D34"), 1.5));
    p.drawLine(playX, 0, playX, height());
    p.setBrush(design::ink);
    p.drawPolygon(QPolygonF{QPointF(playX - 6, 20), QPointF(playX + 6, 20), QPointF(playX, 29)});
    p.restore();
    if (e_.project.tracks.empty()) {
        p.setPen(design::mutedInk);
        p.drawText(QRect(header + 20, 65, 600, 40),
                   "Add a MIDI track to begin. Double-click a track lane to create a clip.");
    }
}
void Timeline::mousePressEvent(QMouseEvent* ev) {
    setFocus();
    if (ev->button() != Qt::LeftButton)
        return;
    auto pt = ev->position();
    if (pt.y() < ruler && pt.x() >= horizontalOffset() + header) {
        double start = header + e_.project.loopStart * e_.pixelsPerBeat,
               end = header + e_.project.loopEnd * e_.pixelsPerBeat;
        if (pt.y() < 15 && (std::abs(pt.x() - start) < 10 || std::abs(pt.x() - end) < 10)) {
            loopDrag_ = std::abs(pt.x() - start) < std::abs(pt.x() - end) ? 1 : 2;
            e_.beginEdit();
            changed_ = true;
        } else if (ev->modifiers() & Qt::ShiftModifier) {
            loopDrag_ = 3;
            loopAnchor_ = std::max(0., e_.snapped((pt.x() - header) / e_.pixelsPerBeat));
            e_.beginEdit();
            changed_ = true;
        } else
            seeking_ = true;
        moveRuler(pt, ev->modifiers());
        return;
    }
    int t;
    auto clip = hit(pt, t);
    if (t < 0)
        return;
    e_.select(t, clip);
    if (pt.x() < horizontalOffset() + header) {
        if (pt.y() >= ruler + t * rowHeight() + rowHeight() - 34 &&
            pt.y() < ruler + t * rowHeight() + rowHeight() - 11) {
            if (pt.x() - horizontalOffset() >= header - 72 &&
                pt.x() - horizontalOffset() < header - 47)
                e_.edit([&] { e_.project.tracks[t].mute = !e_.project.tracks[t].mute; });
            if (pt.x() - horizontalOffset() >= header - 41 &&
                pt.x() - horizontalOffset() < header - 16)
                e_.edit([&] { e_.project.tracks[t].solo = !e_.project.tracks[t].solo; });
        }
        return;
    }
    if (clip) {
        drag_ = clip;
        sourceTrack_ = t;
        const auto* c = findClip(e_.project, clip);
        startBeat_ = c->beat;
        startLength_ = c->length;
        press_ = pt;
        resizing_ = pt.x() > clipRect(t, *c).right() - 10;
        changed_ = false;
    }
}
void Timeline::moveRuler(QPointF pt, Qt::KeyboardModifiers modifiers) {
    double beat = std::max(0., (pt.x() - header) / e_.pixelsPerBeat);
    if (!(modifiers & Qt::AltModifier))
        beat = e_.snapped(beat);
    if (seeking_)
        e_.engine.seek(TempoMap{e_.project.bpm, e_.engine.sampleRate()}.sampleAt(beat));
    else if (loopDrag_) {
        auto& p = e_.project;
        p.loopEnabled = true;
        if (loopDrag_ == 1)
            p.loopStart = std::min(beat, p.loopEnd - e_.grid);
        else if (loopDrag_ == 2)
            p.loopEnd = std::max(beat, p.loopStart + e_.grid);
        else {
            p.loopStart = std::min(beat, loopAnchor_);
            p.loopEnd = std::max(std::max(beat, loopAnchor_), p.loopStart + e_.grid);
        }
    }
    update();
}
void Timeline::mouseMoveEvent(QMouseEvent* ev) {
    if (seeking_ || loopDrag_) {
        moveRuler(ev->position(), ev->modifiers());
        setCursor(Qt::SizeHorCursor);
        return;
    }
    if (ev->position().y() < ruler && ev->position().x() >= horizontalOffset() + header) {
        setCursor(Qt::SizeHorCursor);
        return;
    }
    if (!drag_) {
        int t;
        auto id = hit(ev->position(), t);
        if (hover_ != id) {
            hover_ = id;
            update();
        }
        auto* c = findClip(e_.project, id);
        setCursor(c && ev->position().x() > clipRect(t, *c).right() - 10 ? Qt::SizeHorCursor
                                                                         : Qt::ArrowCursor);
        return;
    }
    auto* c = findClip(e_.project, drag_);
    if (!c)
        return;
    const auto pt = ev->position();
    if (!changed_ && (pt - press_).manhattanLength() < 3)
        return;
    if (!changed_) {
        e_.beginEdit();
        changed_ = true;
    }
    double delta = (pt.x() - press_.x()) / e_.pixelsPerBeat;
    if (resizing_)
        c->length = std::max(e_.grid, e_.snapped(startLength_ + delta));
    else {
        c->beat = std::max(0., e_.snapped(startBeat_ + delta));
        int dest = trackAt(pt.y());
        if (dest >= 0 && dest != sourceTrack_ &&
            e_.project.tracks[dest].kind == e_.project.tracks[sourceTrack_].kind) {
            auto moving = *c;
            auto& clips = e_.project.tracks[sourceTrack_].clips;
            std::erase_if(clips, [&](const Clip& x) { return x.id == drag_; });
            e_.project.tracks[dest].clips.push_back(std::move(moving));
            sourceTrack_ = dest;
            e_.selectedTrack = dest;
        }
    }
    update();
}
void Timeline::mouseReleaseEvent(QMouseEvent*) {
    if (changed_)
        e_.endEdit();
    drag_ = 0;
    seeking_ = false;
    loopDrag_ = 0;
    changed_ = false;
}
void Timeline::mouseDoubleClickEvent(QMouseEvent* ev) {
    int t;
    auto id = hit(ev->position(), t);
    if (t < 0)
        return;
    if (ev->position().x() < horizontalOffset() + header) {
        bool ok = false;
        auto name = QInputDialog::getText(this, "Track name", "Name", QLineEdit::Normal,
                                          QString::fromStdString(e_.project.tracks[t].name), &ok);
        if (ok && !name.isEmpty())
            e_.edit([&] { e_.project.tracks[t].name = name.toStdString(); });
        return;
    }
    if (id) {
        if (e_.project.tracks[t].kind == TrackKind::Midi)
            e_.openPiano(id);
        return;
    }
    double beat = std::max(0., e_.snapped((ev->position().x() - header) / e_.pixelsPerBeat));
    if (e_.project.tracks[t].kind == TrackKind::Audio) {
        e_.importWav({}, beat);
        return;
    }
    e_.edit([&] {
        Clip c;
        c.beat = beat;
        e_.selectedClip = c.id;
        e_.project.tracks[t].clips.push_back(c);
    });
    e_.openPiano(e_.selectedClip);
}
void Timeline::contextMenuEvent(QContextMenuEvent* ev) {
    int t;
    auto id = hit(ev->pos(), t);
    if (t < 0)
        return;
    e_.select(t, id);
    QMenu menu;
    if (id) {
        menu.addAction("Duplicate clip", [&] { e_.duplicateClip(); });
        menu.addAction("Delete clip", [&] { e_.deleteSelection(); });
        menu.addAction("Rename clip", [&] {
            auto* c = findClip(e_.project, id);
            if (!c)
                return;
            bool ok;
            auto name = QInputDialog::getText(this, "Rename clip", "Name", QLineEdit::Normal,
                                              QString::fromStdString(c->name), &ok);
            if (ok)
                e_.edit([&] { findClip(e_.project, id)->name = name.toStdString(); });
        });
        menu.addSeparator();
    }
    menu.addAction("Track color…", [&] {
        auto color = QColorDialog::getColor(QColor::fromRgba(e_.project.tracks[t].color), this);
        if (color.isValid())
            e_.edit([&] { e_.project.tracks[t].color = color.rgba(); });
    });
    menu.addAction("Move track up", [&] { e_.moveTrack(-1); });
    menu.addAction("Move track down", [&] { e_.moveTrack(1); });
    menu.addAction("Delete track", [&] {
        e_.selectedClip = 0;
        e_.deleteSelection();
    });
    menu.exec(ev->globalPos());
}
void Timeline::keyPressEvent(QKeyEvent* ev) {
    if (ev->key() == Qt::Key_Delete || ev->key() == Qt::Key_Backspace) {
        e_.deleteSelection();
        return;
    }
    if (ev->matches(QKeySequence::Copy)) {
        e_.duplicateClip();
        return;
    }
    QWidget::keyPressEvent(ev);
}
int Timeline::horizontalOffset() const {
    auto* area =
        qobject_cast<QScrollArea*>(parentWidget() ? parentWidget()->parentWidget() : nullptr);
    return area ? area->horizontalScrollBar()->value() : 0;
}
int Timeline::rowHeight() const {
    return e_.trackHeight;
}
void Timeline::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-reverie-instrument"))
        event->acceptProposedAction();
}
void Timeline::dropEvent(QDropEvent* event) {
    auto* browser = e_.findChild<SoundBrowser*>("soundBrowser");
    if (!browser)
        return;
    try {
        auto preset =
            browser->presetAt(event->mimeData()->data("application/x-reverie-instrument").toInt());
        int t = trackAt(event->position().y());
        if (t >= 0)
            e_.select(t);
        e_.applyInstrument(preset, t < 0);
        event->acceptProposedAction();
    } catch (const std::exception& ex) {
        e_.message(ex.what());
    }
}
} // namespace reverie
