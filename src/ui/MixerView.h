// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QWidget>
namespace reverie {
class Editor;
class MixerView : public QWidget {
  public:
    explicit MixerView(Editor&);
    void refresh();

  private:
    Editor& e_;
    QWidget* channels_ = nullptr;
};
} // namespace reverie
