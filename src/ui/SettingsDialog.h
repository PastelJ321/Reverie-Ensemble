// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QDialog>
namespace reverie {
class Editor;
class SettingsDialog final : public QDialog {
  public:
    explicit SettingsDialog(Editor&);
};
} // namespace reverie
