// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#pragma once
#include "core/Model.h"
#include "ui/DesignTokens.h"
#include <QWidget>
namespace reverie {
class Editor;
class Timeline final : public QWidget {
  public:
    static constexpr int header = design::headerWidth, ruler = 34, row = design::trackHeight;
    explicit Timeline(Editor&);
    void updateExtent();
    bool interacting() const {
        return drag_ || seeking_ || loopDrag_;
    }
    QRectF clipRect(int, const Clip&) const;

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;

  private:
    int horizontalOffset() const;
    int rowHeight() const;
    Editor& e_;
    Clip clipboard_;
    bool clipboardValid_ = false;
    Id hover_ = 0;
    Id drag_ = 0;
    int sourceTrack_ = 0;
    double startBeat_ = 0, startLength_ = 0;
    QPointF press_;
    bool resizing_ = false, changed_ = false, seeking_ = false;
    int loopDrag_ = 0;
    double loopAnchor_ = 0;
    void moveRuler(QPointF, Qt::KeyboardModifiers);
    int trackAt(double y) const;
    Id hit(QPointF, int&) const;
};
} // namespace reverie
