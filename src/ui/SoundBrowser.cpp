// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/SoundBrowser.h"
#include "ui/Editor.h"
#include "ui/InstrumentArt.h"
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
#include <QPainter>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <chrono>
#include <future>
#include <set>
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
    categories_->addItems({"All instruments", "Favorites", "Recently used", "Piano & Keys",
                           "Synthesizers", "Guitar & Bass", "Strings", "Brass", "Woodwinds",
                           "Drums & Percussion", "Melodic Percussion", "Orchestral",
                           "Pads & Textures", "User Instruments"});
    layout->addWidget(categories_);
    list_ = new PresetList;
    list_->setObjectName("soundList");
    list_->setDragEnabled(true);
    layout->addWidget(list_, 1);
    artwork_ = new InstrumentArtwork;
    layout->addWidget(artwork_);
    auto* patchHeading = new QLabel("PATCHES");
    layout->addWidget(patchHeading);
    patches_ = new QListWidget;
    patches_->setObjectName("patchList");
    patches_->setMaximumHeight(76);
    layout->addWidget(patches_);
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
    connect(list_, &QListWidget::currentRowChanged, this, [this] { updateDetails(); });
    connect(patches_, &QListWidget::currentRowChanged, this, [this] { updateDetails(false); });
    connect(patches_, &QListWidget::itemDoubleClicked, this, [this] { applyCurrent(); });
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
    auto* tools = new QPushButton("Library tools…");
    layout->addWidget(tools);
    for (auto* b : {install, sources, import, wav}) {
        layout->removeWidget(b);
        b->hide();
    }
    connect(tools, &QPushButton::clicked, this, [=] {
        QMenu menu;
        for (auto* b : {install, sources, import, wav})
            menu.addAction(b->text(), [b] { b->click(); });
        menu.exec(tools->mapToGlobal(QPoint(0, tools->height())));
    });
    reload();
}
int SoundBrowser::currentIndex() const {
    if (auto* patch = patches_->currentItem())
        return patch->data(Qt::UserRole).toInt();
    int row = list_->currentRow();
    return row >= 0 && row < int(visible_.size()) ? visible_[row] : -1;
}
void SoundBrowser::reload() {
    entries_.clear();
    int n = 0;
    for (auto p :
         calibratedFactoryPresets((assetPath() + "/sounds/synth-calibration.json").toStdString())) {
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
                p.source = i.source.empty() ? pack.source : i.source;
                p.license = pack.license;
                p.instrumentId = i.id;
                p.family = i.family;
                entries_.push_back({i.name, i.category, i.tags, p, i.file, i.id, false, i, pack});
            }
            size += pack.installSize;
            ++packs;
        } catch (const std::exception& ex) {
            editor_.message(QString("Library unavailable: ") + ex.what());
        }
    };
    addPack((assetPath() + "/sounds/factory").toStdString());
    addPack((assetPath() + "/sounds/essentials03").toStdString());
    QDir user(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
              "/SoundPacks");
    for (const auto& name : user.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        addPack(user.filePath(name).toStdString());
    std::stable_sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        return !a.sfz.empty() && b.sfz.empty();
    });
    QSettings settings;
    auto favorites = settings.value("library/favorites").toStringList();
    recent_ = settings.value("library/recent").toStringList();
    for (auto& e : entries_)
        e.favorite = favorites.contains(QString::fromStdString(e.id));
    std::set<std::string> instrumentIds;
    for (auto& e : entries_)
        instrumentIds.insert(e.sfz.empty() ? e.id : e.metadata.instrumentId);
    status_->setText(
        QString("%1 installed pack(s) · %2 MiB\n%3 instruments / %4 patches · Offline "
                "ready\nAdditional packs: install licensed local folders. No automatic downloads.")
            .arg(packs)
            .arg(size / 1048576., 0, 'f', 1)
            .arg(instrumentIds.size())
            .arg(entries_.size()));
    filter();
}
void SoundBrowser::filter() {
    list_->clear();
    visible_.clear();
    patches_->clear();
    std::set<std::string> shown;
    QString query = search_->text().trimmed().toLower(), category = categories_->currentText();
    for (int i = 0; i < int(entries_.size()); ++i) {
        const auto& e = entries_[i];
        if (category == "Favorites" && !e.favorite)
            continue;
        if (category == "Recently used" && !recent_.contains(QString::fromStdString(e.id)))
            continue;
        if (category != "All instruments" && category != "Favorites" &&
            category != "Recently used" && QString::fromStdString(e.category) != category)
            continue;
        if (!QString::fromStdString(e.name + " " + e.tags + " " + e.category + " " +
                                    e.metadata.patchName + " " + e.metadata.articulation)
                 .toLower()
                 .contains(query))
            continue;
        auto instrument = e.sfz.empty() ? e.id : e.metadata.instrumentId;
        if (!shown.insert(instrument).second)
            continue;
        visible_.push_back(i);
        auto* item = new QListWidgetItem(QString::fromStdString(e.name) + "\n" +
                                             (e.sfz.empty() ? "SYNTH" : "RECORDED") +
                                             (e.favorite ? " · Favorite" : ""),
                                         list_);
        auto glyph = instrumentDrawing(instrumentFamily(e.preset), QSize(40, 28), false,
                                       devicePixelRatioF());
        {
            QPainter paint(&glyph);
            paint.setCompositionMode(QPainter::CompositionMode_SourceIn);
            paint.fillRect(glyph.rect(), QColor("#D4A67A"));
        }
        item->setIcon(QIcon(glyph));
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
                        [this, &e] { return loadLibraryPreset(e.metadata, e.pack, cache_); });
    return p;
}
void SoundBrowser::updateDetails(bool rebuild) {
    if (rebuild) {
        QSignalBlocker block(patches_);
        patches_->clear();
        int row = list_->currentRow();
        if (row < 0 || row >= int(visible_.size())) {
            detail_->clear();
            return;
        }
        int selected = visible_[row];
        const auto& entry = entries_[selected];
        for (int index = 0; index < int(entries_.size()); ++index) {
            const auto& patch = entries_[index];
            if ((!entry.sfz.empty() &&
                 patch.metadata.instrumentId == entry.metadata.instrumentId) ||
                (entry.sfz.empty() && index == selected)) {
                auto* item = new QListWidgetItem(
                    QString::fromStdString(patch.sfz.empty() ? patch.name
                                                             : patch.metadata.patchName),
                    patches_);
                item->setData(Qt::UserRole, index);
                if (index == selected)
                    patches_->setCurrentItem(item);
            }
        }
    }
    int i = currentIndex();
    if (i < 0)
        return;
    const auto& e = entries_[i];
    artwork_->setFamily(instrumentFamily(e.preset));
    detail_->setText(
        QString::fromStdString(e.category + " · " +
                               (e.sfz.empty() ? "Synthesis" : e.metadata.articulation)) +
        "\n" + QString::fromStdString(e.preset.license));
    detail_->setToolTip(
        QString::fromStdString(e.pack.name + "\n" + e.pack.author + "\n" + e.pack.source +
                               "\nCalibration v" + std::to_string(e.metadata.calibrationVersion) +
                               ": " + std::to_string(e.metadata.calibrationDb) + " dB"));
}
void SoundBrowser::preview() {
    try {
        auto p = presetAt(currentIndex());
        auto t = makeTrack(TrackKind::Midi, 0);
        t.preset = p;
        t.preset.calibrationGain *= p.auditionGain;
        t.volume = .7;
        editor_.engine.audition(t, p.category == "Drums & Percussion" ? 38 : 60, .8, .65);
    } catch (const std::exception& ex) {
        editor_.message(ex.what());
    }
}
void SoundBrowser::applyCurrent(bool newTrack) {
    try {
        auto index = currentIndex();
        auto p = presetAt(index);
        editor_.applyInstrument(p, newTrack);
        auto id = QString::fromStdString(entries_[index].id);
        recent_.removeAll(id);
        recent_.prepend(id);
        while (recent_.size() > 16)
            recent_.removeLast();
        QSettings().setValue("library/recent", recent_);
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
