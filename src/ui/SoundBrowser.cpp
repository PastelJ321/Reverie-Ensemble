// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/SoundBrowser.h"
#include "ui/Editor.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDrag>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <chrono>
#include <future>
namespace reverie {
template <class F> static auto libraryJob(QWidget* parent, const QString& label, F job) {
    if (!parent->isVisible())
        return job();
    QProgressDialog progress(label, QString(), 0, 0, parent);
    progress.setCancelButton(nullptr);
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    progress.show();
    auto result = std::async(std::launch::async, std::move(job));
    while (result.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready)
        QApplication::processEvents();
    return result.get();
}
class PresetList : public QListWidget {
  public:
    using QListWidget::QListWidget;
    void startDrag(Qt::DropActions) override {
        auto* item = currentItem();
        if (!item)
            return;
        auto* drag = new QDrag(this);
        auto* mime = new QMimeData;
        mime->setData("application/x-reverie-instrument", item->data(Qt::UserRole).toByteArray());
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }
};
SoundBrowser::SoundBrowser(Editor& e) : editor_(e) {
    setObjectName("soundBrowser");
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);
    search_ = new QLineEdit;
    search_->setObjectName("soundSearch");
    search_->setPlaceholderText("Search instruments / tags");
    search_->setClearButtonEnabled(true);
    layout->addWidget(search_);
    categories_ = new QComboBox;
    categories_->setObjectName("soundCategory");
    categories_->addItems({"All instruments", "Favorites", "Piano & Keys", "Synthesizers",
                           "Guitar & Bass", "Strings", "Brass", "Woodwinds", "Drums & Percussion",
                           "Melodic Percussion", "Orchestral", "Pads & Textures",
                           "User Instruments"});
    layout->addWidget(categories_);
    list_ = new PresetList;
    list_->setObjectName("soundList");
    list_->setDragEnabled(true);
    layout->addWidget(list_, 1);
    detail_ = new QLabel;
    detail_->setTextFormat(Qt::PlainText);
    detail_->setWordWrap(true);
    detail_->setMinimumHeight(55);
    layout->addWidget(detail_);
    auto* actions = new QHBoxLayout;
    auto* listen = new QPushButton("Listen");
    listen->setIcon(toolIcon("play"));
    actions->addWidget(listen);
    auto* apply = new QPushButton("Apply");
    actions->addWidget(apply);
    auto* favorite = new QPushButton("Favorite");
    actions->addWidget(favorite);
    layout->addLayout(actions);
    auto* add = new QPushButton("New instrument track");
    layout->addWidget(add);
    auto* install = new QPushButton("Install local sound pack…");
    layout->addWidget(install);
    auto* sources = new QPushButton("Available collections…");
    layout->addWidget(sources);
    connect(sources, &QPushButton::clicked, this, [sources] {
        QMenu menu;
        for (const auto& pair : std::vector<std::pair<QString, QString>>{
                 {"VSCO 2 CE · CC0 · ~3 GB · external",
                  "https://versilian-studios.com/vsco-community/"},
                 {"VCSL · CC0 · ~5 GB · external", "https://versilian-studios.com/vcsl/"},
                 {"Karoryfer · external catalog / per-asset license",
                  "https://shop.karoryfer.com/pages/free-samples"}})
            menu.addAction(pair.first,
                           [url = pair.second] { QDesktopServices::openUrl(QUrl(url)); });
        menu.exec(sources->mapToGlobal(QPoint(0, sources->height())));
    });
    auto* import = new QPushButton("Import user SFZ…");
    layout->addWidget(import);
    auto* wav = new QPushButton("Import audio WAV…");
    layout->addWidget(wav);
    status_ = new QLabel;
    status_->setWordWrap(true);
    status_->setStyleSheet("color:#BEB2A4;font-size:11px");
    layout->addWidget(status_);
    connect(search_, &QLineEdit::textChanged, this, [this] { filter(); });
    connect(categories_, &QComboBox::currentIndexChanged, this, [this] { filter(); });
    connect(list_, &QListWidget::currentRowChanged, this, [this] {
        int index = currentIndex();
        if (index >= 0) {
            const auto& entry = entries_[index];
            detail_->setToolTip(QString::fromStdString(entry.preset.source).toHtmlEscaped());
            detail_->setText(QString::fromStdString(entry.category) +
                             (entry.sfz.empty()
                                  ? " / synthesis"
                                  : " / recorded multisample · " +
                                        QString::fromStdString(entry.preset.license)) +
                             "\n" + QString::fromStdString(entry.name));
        }
    });
    connect(list_, &QListWidget::itemDoubleClicked, this, [this] { applyCurrent(); });
    connect(listen, &QPushButton::clicked, this, [this] { preview(); });
    connect(apply, &QPushButton::clicked, this, [this] { applyCurrent(); });
    connect(add, &QPushButton::clicked, this, [this] { applyCurrent(true); });
    connect(wav, &QPushButton::clicked, this, [this] { editor_.importWav(); });
    connect(favorite, &QPushButton::clicked, this, [this] {
        int i = currentIndex();
        if (i < 0)
            return;
        entries_[i].favorite = !entries_[i].favorite;
        QSettings settings;
        QStringList ids;
        for (const auto& e : entries_)
            if (e.favorite)
                ids << QString::fromStdString(e.id);
        settings.setValue("library/favorites", ids);
        filter();
    });
    connect(install, &QPushButton::clicked, this, [this] {
        auto folder =
            QFileDialog::getExistingDirectory(this, "Select unpacked sound pack (pack.json)");
        if (folder.isEmpty())
            return;
        try {
            auto p = libraryJob(this, "Validating pack integrity and mappings…",
                                [folder] { return readSoundPack(folder.toStdString()); });
            auto text =
                QString::fromStdString(p.name + "\n" + p.author + "\nLicense: " + p.license +
                                       "\nSource: " + p.source) +
                QString("\nInstall size: %1 MiB\nInstall this local pack?")
                    .arg(p.installSize / 1048576., 0, 'f', 1);
            if (QMessageBox::question(this, "Install sound pack", text) == QMessageBox::Yes)
                installFolder(folder);
        } catch (const std::exception& ex) {
            editor_.message(ex.what());
        }
    });
    connect(import, &QPushButton::clicked, this, [this] {
        auto path =
            QFileDialog::getOpenFileName(this, "Load supported SFZ instrument", {}, "SFZ (*.sfz)");
        if (path.isEmpty())
            return;
        try {
            auto p = libraryJob(this, "Loading user SFZ…",
                                [this, path] { return loadSfz(path.toStdString(), cache_); });
            p.category = "User Instruments";
            p.source = "User supplied SFZ";
            p.license = "User-managed; verify redistribution rights";
            editor_.applyInstrument(p);
        } catch (const std::exception& ex) {
            editor_.message(ex.what());
        }
    });
    reload();
}
int SoundBrowser::currentIndex() const {
    int row = list_->currentRow();
    return row >= 0 && row < int(visible_.size()) ? visible_[row] : -1;
}
void SoundBrowser::reload() {
    entries_.clear();
    int n = 0;
    for (auto p : factoryPresets()) {
        std::string category = n == 0 || n == 5 || n == 6 ? "Piano & Keys"
                               : n == 1                   ? "Pads & Textures"
                               : n == 8                   ? "Drums & Percussion"
                                                          : "Synthesizers";
        p.category = category;
        p.instrumentId = "quadraj.synth." + std::to_string(n++);
        entries_.push_back(
            {p.name, category, "synth oscillator " + p.name, p, {}, p.instrumentId, false});
    }
    std::uint64_t size = 0;
    int packs = 0;
    auto addPack = [&](const std::filesystem::path& folder) {
        try {
            auto pack = readSoundPack(folder, false);
            for (auto i : pack.instruments) {
                Preset p;
                p.source = pack.source;
                p.license = pack.license;
                p.instrumentId = i.id;
                entries_.push_back({i.name, i.category, i.tags, p, i.file, i.id, false});
            }
            size += pack.installSize;
            ++packs;
        } catch (const std::exception& ex) {
            editor_.message(QString("Library unavailable: ") + ex.what());
        }
    };
    addPack((assetPath() + "/sounds/factory").toStdString());
    QDir user(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
              "/SoundPacks");
    for (const auto& name : user.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        addPack(user.filePath(name).toStdString());
    std::stable_sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        return !a.sfz.empty() && b.sfz.empty();
    });
    QSettings settings;
    auto favorites = settings.value("library/favorites").toStringList();
    for (auto& e : entries_)
        e.favorite = favorites.contains(QString::fromStdString(e.id));
    status_->setText(
        QString("%1 installed pack(s) · %2 MiB\n%3 playable instruments · Offline "
                "ready\nAdditional packs: install licensed local folders. No automatic downloads.")
            .arg(packs)
            .arg(size / 1048576., 0, 'f', 1)
            .arg(entries_.size()));
    filter();
}
void SoundBrowser::filter() {
    list_->clear();
    visible_.clear();
    QString query = search_->text().trimmed().toLower(), category = categories_->currentText();
    for (int i = 0; i < int(entries_.size()); ++i) {
        const auto& e = entries_[i];
        if (category == "Favorites" && !e.favorite)
            continue;
        if (category != "All instruments" && category != "Favorites" &&
            QString::fromStdString(e.category) != category)
            continue;
        if (!QString::fromStdString(e.name + " " + e.tags + " " + e.category)
                 .toLower()
                 .contains(query))
            continue;
        visible_.push_back(i);
        auto* item = new QListWidgetItem(QString::fromStdString(e.name) + "\n" +
                                             (e.sfz.empty() ? "SYNTH" : "RECORDED") +
                                             (e.favorite ? " · Favorite" : ""),
                                         list_);
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString::fromStdString(e.category + " · " + e.tags));
    }
    if (list_->count())
        list_->setCurrentRow(0);
}
Preset SoundBrowser::presetAt(int i) {
    if (i < 0 || i >= int(entries_.size()))
        throw std::runtime_error("No instrument selected");
    auto& e = entries_[i];
    if (e.sfz.empty())
        return e.preset;
    auto p = libraryJob(this, "Preparing recorded instrument…",
                        [this, path = e.sfz] { return loadSfz(path, cache_); });
    p.name = e.name;
    p.instrumentId = e.id;
    p.category = e.category;
    p.tags = e.tags;
    p.source = e.preset.source;
    p.license = e.preset.license;
    return p;
}
void SoundBrowser::preview() {
    try {
        auto p = presetAt(currentIndex());
        auto t = makeTrack(TrackKind::Midi, 0);
        t.preset = p;
        t.volume = .7;
        editor_.engine.audition(t, p.category == "Drums & Percussion" ? 38 : 60, .8, .65);
    } catch (const std::exception& ex) {
        editor_.message(ex.what());
    }
}
void SoundBrowser::applyCurrent(bool newTrack) {
    try {
        editor_.applyInstrument(presetAt(currentIndex()), newTrack);
    } catch (const std::exception& ex) {
        editor_.message(ex.what());
    }
}
void SoundBrowser::installFolder(const QString& folder) {
    auto path =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/SoundPacks";
    libraryJob(this, "Installing verified sound pack…", [folder, path] {
        return installSoundPack(folder.toStdString(), path.toStdString());
    });
    reload();
}
} // namespace reverie
