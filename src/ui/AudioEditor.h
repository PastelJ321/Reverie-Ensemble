// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QWidget>
namespace reverie {
class Editor;
class AudioEditor : public QWidget {
  public:
    explicit AudioEditor(Editor&);

  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

  private:
    Editor& e_;
};
} // namespace reverie
