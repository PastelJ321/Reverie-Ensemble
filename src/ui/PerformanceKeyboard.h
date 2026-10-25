// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QMap>
#include <QSet>
#include <QWidget>
class QLabel;
class QSpinBox;
class QCheckBox;
namespace reverie {
class Editor;
class PerformanceKeyboard final : public QWidget {
    Q_OBJECT
    Editor& editor_;
    QMap<int, int> typingNotes_;
    QSet<int> pressed_;
    QSpinBox *octave_, *velocity_;
    QCheckBox *typing_, *sustain_;
    QLabel* target_;
    QWidget* keys_;
    int mouseKey_ = -1;
    std::string targetIdentity_;
    int keyAt(QPointF) const;
    QRectF keyRect(int) const;
    void mouseNote(int);

  public:
    explicit PerformanceKeyboard(Editor&);
    ~PerformanceKeyboard() override;
    void releaseAll();
    void refresh();
    void setTyping(bool);
    void showActivity(int key, bool on);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    void hideEvent(QHideEvent*) override;
};
} // namespace reverie
