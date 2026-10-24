// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Export.h"
#include <QDialog>
#include <atomic>
#include <future>
class QComboBox;
class QLineEdit;
class QCheckBox;
class QLabel;
class QProgressBar;
class QPushButton;
namespace reverie {
class ExportDialog final : public QDialog {
  public:
    ExportDialog(const Project&, double selectionStart, double selectionEnd,
                 QWidget* parent = nullptr);
    ~ExportDialog() override;
    ExportOptions options() const;
    void startRender();

  protected:
    void reject() override;

  private:
    Project project_;
    double selectionStart_, selectionEnd_;
    QLineEdit* path_;
    QComboBox *rate_, *depth_, *channels_, *range_;
    QCheckBox *tail_, *normalize_, *dither_;
    QLabel* status_;
    QProgressBar* progress_;
    QPushButton* render_;
    std::future<ExportResult> job_;
    std::atomic<bool> cancel_{false};
    std::atomic<double> percent_{0};
    bool running_ = false, done_ = false;
    void updateEstimate();
};
} // namespace reverie
