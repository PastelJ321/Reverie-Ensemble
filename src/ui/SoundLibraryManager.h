// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QDialog>
class QListWidget;
class QLabel;
class QProgressBar;
class QPushButton;
namespace reverie {
class SoundBrowser;
class SoundLibraryManager final : public QDialog {
    SoundBrowser& browser_;
    QListWidget *available_, *installed_;
    QLabel* details_;
    QProgressBar* progress_;
    QPushButton *download_, *cancel_;
    bool cancelled_ = false, busy_ = false;
    void reload();
    void download();

  public:
    explicit SoundLibraryManager(SoundBrowser&);

  protected:
    void reject() override;
};
} // namespace reverie
