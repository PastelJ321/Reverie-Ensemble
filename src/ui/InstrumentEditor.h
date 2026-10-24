// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QWidget>
class QDoubleSpinBox;
class QLabel;
namespace reverie {
class Editor;
class InstrumentEditor final : public QWidget {
  public:
    explicit InstrumentEditor(Editor&);
    void refresh();

  private:
    Editor& e_;
    QDoubleSpinBox *tone_, *time_, *space_;
    QLabel* timeLabel_;
    bool updating_ = false;
};
} // namespace reverie
