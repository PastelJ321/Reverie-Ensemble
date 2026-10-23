// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include <QWidget>
namespace reverie {
class Editor;
class Timeline final : public QWidget {
  public:
    static constexpr int header = 204, ruler = 34, row = 82;
    explicit Timeline(Editor&);
    void updateExtent();
    QRectF clipRect(int, const Clip&) const;

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

  private:
    Editor& e_;
    Id drag_ = 0;
    int sourceTrack_ = 0;
    double startBeat_ = 0, startLength_ = 0;
    QPointF press_;
    bool resizing_ = false, changed_ = false;
    int trackAt(double y) const;
    Id hit(QPointF, int&) const;
};
} // namespace reverie
