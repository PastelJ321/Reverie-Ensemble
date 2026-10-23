// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QWidget>
class QFormLayout;
class QLabel;
class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QStackedWidget;
namespace reverie {
class Editor;
class InspectorPanel : public QWidget {
  public:
    explicit InspectorPanel(Editor&);
    void refresh();

  private:
    QFormLayout* form_;
    Editor& e_;
    bool updating_ = false;
    QLabel* title_;
    QLineEdit* name_;
    QDoubleSpinBox *beat_, *length_, *velocity_, *offset_;
    QSpinBox* pitch_;
};
} // namespace reverie
