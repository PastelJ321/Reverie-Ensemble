// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Library.h"
#include <QWidget>
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;
class QSplitter;
class QListWidget;
class QLabel;
namespace reverie {
class InstrumentArtwork;
class Editor;
class SoundBrowser final : public QWidget {
    Q_OBJECT
  public:
    explicit SoundBrowser(Editor&);
    ~SoundBrowser() override;
    void setExpanded(bool);
    bool expanded() const {
        return expanded_;
    }
    void showManager();
    void importInstrument(const QString&);
    bool packInUse(const QString&) const;
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
        LibraryInstrument metadata;
        SoundPack pack;
        bool installed = true;
    };
    Editor& editor_;
    SampleCache cache_;
    std::vector<Entry> entries_, userEntries_;
    std::vector<int> visible_;
    QLineEdit* search_;
    QListWidget* categories_;
    QWidget* categoryColumn_;
    QWidget* detailColumn_;
    QSplitter* columns_;
    QComboBox* packs_;
    QCheckBox* installedOnly_;
    QPushButton* expand_;
    QPushButton* pin_;
    bool expanded_ = false;
    QListWidget* list_;
    QLabel* status_;
    QLabel* detail_;
    QLabel* detailTitle_;
    InstrumentArtwork* artwork_;
    QListWidget* patches_;
    QStringList recent_;
    void updateDetails(bool rebuildPatches = true);
    int currentIndex() const;
};
} // namespace reverie
