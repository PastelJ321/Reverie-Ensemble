// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Library.h"
#include <QWidget>
class QLineEdit;
class QComboBox;
class QListWidget;
class QLabel;
namespace reverie {
class Editor;
class SoundBrowser final : public QWidget {
    Q_OBJECT
  public:
    explicit SoundBrowser(Editor&);
    void filter();
    void reload();
    void applyCurrent(bool newTrack = false);
    void preview();
    void installFolder(const QString&);
    Preset presetAt(int);

  private:
    struct Entry {
        std::string name, category, tags;
        Preset preset;
        std::filesystem::path sfz;
        std::string id;
        bool favorite = false;
    };
    Editor& editor_;
    SampleCache cache_;
    std::vector<Entry> entries_;
    std::vector<int> visible_;
    QLineEdit* search_;
    QComboBox* categories_;
    QListWidget* list_;
    QLabel* status_;
    QLabel* detail_;
    int currentIndex() const;
};
} // namespace reverie
