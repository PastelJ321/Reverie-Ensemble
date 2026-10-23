// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/AudioEditor.h"
#include "ui/DesignTokens.h"
#include "ui/Editor.h"
#include <QMouseEvent>
#include <QPainter>
namespace reverie {
AudioEditor::AudioEditor(Editor& e) : e_(e) {
    setObjectName("audioEditor");
    setMinimumHeight(160);
    setCursor(Qt::CrossCursor);
}
void AudioEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), design::paper);
    auto* c = findClip(e_.project, e_.selectedClip);
    p.setPen(design::ink);
    p.drawText(12, 22, "AUDIO / NON-DESTRUCTIVE SOURCE");
    if (!c)
        return;
    p.drawText(12, 44,
               QString::fromStdString(c->name) + QString("    Source in %1 s · Length %2 beats")
                                                     .arg(c->sourceOffset, 0, 'f', 3)
                                                     .arg(c->length, 0, 'f', 2));
    if (!c->sample) {
        p.drawText(12, 90, "Source WAV unavailable");
        return;
    }
    double frames = c->length * 60 / e_.project.bpm * c->sample->sampleRate,
           offset = c->sourceOffset * c->sample->sampleRate;
    double center = (height() + 50) / 2.;
    p.setPen(design::grid);
    p.drawLine(0, int(center), width(), int(center));
    p.setPen(design::ink);
    for (int x = 0; x < width(); ++x) {
        auto start = std::size_t(offset + x * frames / std::max(1, width())) / 256,
             end = std::size_t(offset + (x + 1) * frames / std::max(1, width())) / 256;
        float lo = 0, hi = 0;
        for (auto i = start; i <= end && i < c->sample->peaks.size(); ++i) {
            lo = std::min(lo, c->sample->peaks[i].first);
            hi = std::max(hi, c->sample->peaks[i].second);
        }
        p.drawLine(QPointF(x, center + lo * (height() - 70) * .45),
                   QPointF(x, center + hi * (height() - 70) * .45));
    }
}
void AudioEditor::mousePressEvent(QMouseEvent* ev) {
    if (auto* c = findClip(e_.project, e_.selectedClip))
        e_.engine.seek(TempoMap{e_.project.bpm, e_.engine.sampleRate()}.sampleAt(
            c->beat + std::clamp(ev->position().x() / std::max(1, width()), 0., 1.) * c->length));
}
} // namespace reverie
