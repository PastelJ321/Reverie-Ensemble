// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include <QSet>
#include <QWidget>
namespace reverie {
class Editor;
class PianoRoll final : public QWidget {
  public:
    static constexpr int keys = 60, ruler = 30, keyHeight = 15, topKey = 95, bottomKey = 24;
    explicit PianoRoll(Editor&);
    QSet<Id> selection;
    void updateExtent();
    void quantize();
    void erase();
    void setVelocity(double);
    QRectF noteRect(const Note&) const;

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

  private:
    Editor& e_;
    QPointF press_;
    Id drag_ = 0;
    bool resizing_ = false, changed_ = false, marquee_ = false;
    QRectF selectionRect_;
    std::vector<Note> original_;
};
} // namespace reverie
