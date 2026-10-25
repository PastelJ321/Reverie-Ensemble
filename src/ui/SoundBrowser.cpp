// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/SoundBrowser.h"
#include "core/InstrumentBackend.h"
#include "core/Serializer.h"
#include "ui/Editor.h"
#include "ui/InstrumentArt.h"
#include "ui/SoundLibraryManager.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QDir>
#include <QDockWidget>
#include <QDrag>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
#include <QSplitter>
#include <QStandardPaths>
#include <QTimer>
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
    auto* modeBar = new QHBoxLayout;
    expand_ = new QPushButton("Expand ↔");
    expand_->setObjectName("expandBrowser");
    modeBar->addWidget(expand_);
    pin_ = new QPushButton("Pin");
    pin_->setCheckable(true);
    pin_->setChecked(QSettings().value("library/pinned", true).toBool());
    modeBar->addWidget(pin_);
    connect(pin_, &QPushButton::toggled, this,
            [](bool value) { QSettings().setValue("library/pinned", value); });
    connect(expand_, &QPushButton::clicked, this, [this] { setExpanded(!expanded_); });
    layout->addLayout(modeBar);
    search_ = new QLineEdit;
    search_->setObjectName("soundSearch");
    search_->setPlaceholderText("Search instruments / tags");
    search_->setClearButtonEnabled(true);
    layout->addWidget(search_);
    packs_ = new QComboBox;
    packs_->setObjectName("soundPackFilter");
    packs_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    packs_->setMinimumContentsLength(12);
    packs_->setMinimumWidth(0);
    packs_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    packs_->addItem("All packs");
    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(packs_, 1);
    installedOnly_ = new QCheckBox("Installed");
    installedOnly_->setObjectName("installedInstrumentsOnly");
    installedOnly_->setChecked(QSettings().value("library/installedOnly", true).toBool());
    filterRow->addWidget(installedOnly_);
    layout->addLayout(filterRow);
    connect(installedOnly_, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue("library/installedOnly", on);
        filter();
    });
    connect(packs_, &QComboBox::currentTextChanged, this, [this] { filter(); });
    columns_ = new QSplitter;
    columns_->setObjectName("browserColumns");
    categories_ = new QListWidget;
    categories_->setObjectName("soundCategory");
    categories_->addItems({"All instruments", "Favorites", "Recently used", "Piano & Keys",
                           "Guitar & Bass", "Drums & Percussion", "Strings", "Brass", "Woodwinds",
                           "Synthesizers", "Orchestral", "World Instruments",
                           "Sound Effects & Textures", "User Instruments"});
    categories_->setCurrentRow(0);
    categories_->setMinimumWidth(130);
    categories_->setMaximumWidth(180);
    categoryColumn_ = categories_;
    columns_->addWidget(categories_);
    auto* instrumentColumn = new QWidget;
    auto* instrumentLayout = new QVBoxLayout(instrumentColumn);
    instrumentLayout->setContentsMargins(0, 0, 0, 0);
    list_ = new PresetList;
    list_->setObjectName("soundList");
    list_->setDragEnabled(true);
    list_->setIconSize(QSize(70, 48));
    list_->setMinimumWidth(150);
    instrumentLayout->addWidget(list_, 1);
    columns_->addWidget(instrumentColumn);
    detailColumn_ = new QWidget;
    auto* detailsLayout = new QVBoxLayout(detailColumn_);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailColumn_->setMinimumWidth(175);
    columns_->addWidget(detailColumn_);
    layout->addWidget(columns_, 1);
    detailTitle_ = new QLabel;
    detailTitle_->setObjectName("instrumentDetailTitle");
    detailTitle_->setWordWrap(true);
    detailTitle_->setStyleSheet("font-size:17px;font-weight:600;color:#E8E1D5");
    detailsLayout->addWidget(detailTitle_);
    artwork_ = new InstrumentArtwork;
    detailsLayout->addWidget(artwork_);
    auto* patchHeading = new QLabel("PATCHES");
    detailsLayout->addWidget(patchHeading);
    patches_ = new QListWidget;
    patches_->setObjectName("patchList");
    patches_->setMinimumHeight(64);
    patches_->setMaximumHeight(180);
    detailsLayout->addWidget(patches_);
    detail_ = new QLabel;
    detail_->setTextFormat(Qt::PlainText);
    detail_->setWordWrap(true);
    detail_->setMinimumHeight(55);
    detailsLayout->addWidget(detail_);
    auto* actions = new QHBoxLayout;
    auto* listen = new QPushButton("Listen");
    listen->setObjectName("browserPreview");
    listen->setIcon(toolIcon("play"));
    actions->addWidget(listen);
    auto* apply = new QPushButton("Apply");
    apply->setObjectName("browserApply");
    actions->addWidget(apply);
    auto* favorite = new QPushButton("Favorite");
    favorite->setObjectName("browserFavorite");
    actions->addWidget(favorite);
    detailsLayout->addLayout(actions);
    auto* add = new QPushButton("New instrument track");
    add->setObjectName("browserNewTrack");
    detailsLayout->addWidget(add);
    detailsLayout->addStretch();
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
    auto* import = new QPushButton("Import SFZ / SoundFont…");
    layout->addWidget(import);
    auto* wav = new QPushButton("Import audio WAV…");
    layout->addWidget(wav);
    status_ = new QLabel;
    status_->setWordWrap(true);
    status_->setStyleSheet("color:#BEB2A4;font-size:11px");
    layout->addWidget(status_);
    connect(search_, &QLineEdit::textChanged, this, [this] { filter(); });
    connect(categories_, &QListWidget::currentRowChanged, this, [this] { filter(); });
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
        auto path = QFileDialog::getOpenFileName(this, "Load SFZ or SoundFont", {},
                                                 "Instruments (*.sfz *.sf2 *.sf3)");
        if (path.isEmpty())
            return;
        importInstrument(path);
    });
    auto* tools = new QPushButton("Library tools…");
    tools->setText("⋯");
    tools->setToolTip("Import instrument / local pack tools");
    tools->setMaximumWidth(30);
    modeBar->addWidget(tools);
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
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    patches_->setContextMenuPolicy(Qt::CustomContextMenu);
    auto context = [this, favorite](QWidget* view, QPoint pt) {
        QMenu menu;
        int selected = currentIndex();
        bool ready = selected >= 0 && entries_[selected].installed;
        menu.addAction("Preview", this, [this] { preview(); })->setEnabled(ready);
        menu.addAction("Apply", this, [this] { applyCurrent(); })->setEnabled(ready);
        menu.addAction("Create New Track", this, [this] { applyCurrent(true); })->setEnabled(ready);
        menu.addAction("Add / Remove Favorite", favorite, &QPushButton::click);
        menu.addAction("Show Instrument Details", this, [this] { setExpanded(true); });
        menu.addAction("Show Source / License", this, [this] {
            int i = currentIndex();
            if (i >= 0) {
                auto& e = entries_[i];
                QMessageBox::information(
                    this, "Instrument source and license",
                    QString::fromStdString(e.name + "\n" + e.id + "\n" + e.pack.author + "\n" +
                                           e.preset.source + "\nLicense: " + e.preset.license));
            }
        });
        menu.addAction("Reveal Installed Files", this, [this] {
            int i = currentIndex();
            if (i >= 0 && !entries_[i].sfz.empty())
                QDesktopServices::openUrl(QUrl::fromLocalFile(
                    QString::fromStdString(entries_[i].sfz.parent_path().string())));
        });
        menu.addAction("Manage / Remove Optional Packs…", this, [this] { showManager(); });
        menu.exec(view->mapToGlobal(pt));
    };
    connect(list_, &QListWidget::customContextMenuRequested, this, [this, context](QPoint pt) {
        if (auto* item = list_->itemAt(pt))
            list_->setCurrentItem(item);
        context(list_, pt);
    });
    connect(patches_, &QListWidget::customContextMenuRequested, this, [this, context](QPoint pt) {
        if (auto* item = patches_->itemAt(pt))
            patches_->setCurrentItem(item);
        context(patches_, pt);
    });
    auto* manager = new QPushButton("Sound Library Manager…");
    manager->setObjectName("libraryManagerButton");
    layout->addWidget(manager);
    connect(manager, &QPushButton::clicked, this, [this] { showManager(); });
    reload();
    QTimer::singleShot(180, this, [this] { filter(); });
    QTimer::singleShot(
        0, this, [this] { setExpanded(QSettings().value("library/expanded", false).toBool()); });
}
void SoundBrowser::importInstrument(const QString& path) {
    if (path.isEmpty())
        return;
    try {
        auto p = libraryJob(this, "Loading user SFZ…", [this, path] {
            if (path.endsWith(".sfz", Qt::CaseInsensitive)) {
                if (sfizzAvailable())
                    return externalPreset(path.toStdString(), InstrumentKind::Sfizz);
                return loadSfz(path.toStdString(), cache_);
            }
            auto programs = soundFontPrograms(path.toStdString());
            if (programs.empty())
                throw std::runtime_error("SoundFont contains no programs");
            return externalPreset(path.toStdString(), InstrumentKind::SoundFont, programs[0].bank,
                                  programs[0].program);
        });
        p.category = "User Instruments";
        p.source = "User supplied instrument";
        p.license = "User-managed; verify redistribution rights";
        if (p.kind == InstrumentKind::SoundFont) {
            auto programs = libraryJob(this, "Reading SoundFont bank / programs…",
                                       [path] { return soundFontPrograms(path.toStdString()); });
            QStringList options;
            for (auto& v : programs)
                options << QString("%1:%2 · %3")
                               .arg(v.bank)
                               .arg(v.program)
                               .arg(QString::fromStdString(v.name));
            bool ok = false;
            auto chosen = QInputDialog::getItem(this, "SoundFont Instrument Selection",
                                                "Bank : Program", options, 0, false, &ok);
            if (!ok)
                return;
            auto& v = programs[options.indexOf(chosen)];
            p.soundFontBank = v.bank;
            p.soundFontProgram = v.program;
            p.patchId = std::to_string(v.bank) + ":" + std::to_string(v.program);
            p.name = v.name;
        }
        auto prepared = libraryJob(this, "Checking instrument backend…",
                                   [p] { return prepareBackend(p, 48000); });
        if (p.kind == InstrumentKind::SoundFont) {
            auto programs = libraryJob(this, "Cataloging SoundFont programs…",
                                       [path] { return soundFontPrograms(path.toStdString()); });
            for (auto& v : programs) {
                auto patch = p;
                patch.name = v.name;
                patch.soundFontBank = v.bank;
                patch.soundFontProgram = v.program;
                patch.patchId = std::to_string(v.bank) + ":" + std::to_string(v.program);
                LibraryInstrument m;
                m.name = v.name;
                m.id = p.instrumentId + ":" + patch.patchId;
                m.instrumentId = p.instrumentId;
                m.patchId = patch.patchId;
                m.patchName =
                    std::to_string(v.bank) + ":" + std::to_string(v.program) + " · " + v.name;
                m.category = p.category;
                m.format = "sf2";
                m.file = p.backendPath;
                Entry entry{p.name, p.category, p.tags, patch, p.backendPath, m.id, false, m, {}};
                entries_.push_back(entry);
                userEntries_.push_back(entry);
            }
        } else {
            Entry entry{p.name, p.category, p.tags, p, {}, p.instrumentId + ":" + p.patchId,
                        false,  {},         {}};
            entries_.push_back(entry);
            userEntries_.push_back(entry);
        }
        auto registry = soundPackPath() + "/.user-instruments";
        QDir().mkpath(registry);
        for (const auto& entry : userEntries_) {
            auto key = QCryptographicHash::hash(QByteArray::fromStdString(entry.id),
                                                QCryptographicHash::Sha256)
                           .toHex();
            ProjectSerializer::savePreset(entry.preset,
                                          (registry + "/" + key + ".json").toStdString());
        }
        filter();
        editor_.applyInstrument(p);
    } catch (const std::exception& ex) {
        editor_.message(ex.what());
    }
}
int SoundBrowser::currentIndex() const {
    if (auto* patch = patches_->currentItem())
        return patch->data(Qt::UserRole).toInt();
    int row = list_->currentRow();
    return row >= 0 && row < int(visible_.size()) ? visible_[row] : -1;
}
void SoundBrowser::reload() {
    entries_.clear();
    QSignalBlocker packBlock(packs_);
    packs_->clear();
    packs_->addItem("All packs");
    int n = 0;
    for (auto p :
         calibratedFactoryPresets((assetPath() + "/sounds/synth-calibration.json").toStdString())) {
        std::string category = n == 0 || n == 5 || n == 6 ? "Piano & Keys"
                               : n == 1                   ? "Sound Effects & Textures"
                               : n == 8                   ? "Drums & Percussion"
                                                          : "Synthesizers";
        p.category = category;
        p.instrumentId = "quadraj.synth." + std::to_string(n++);
        entries_.push_back(
            {p.name, category, "synth oscillator " + p.name, p, {}, p.instrumentId, false, {}, {}});
    }
    std::uint64_t size = 0;
    int packs = 0;
    auto addPack = [&](const std::filesystem::path& folder) {
        try {
            auto pack = readSoundPack(folder, false);
            packs_->addItem(QString::fromStdString(pack.name));
            for (auto i : pack.instruments) {
                Preset p;
                p.source = i.source.empty() ? pack.source : i.source;
                p.license = pack.license;
                p.instrumentId = i.id;
                p.family = i.family;
                p.name = i.name;
                p.artworkUri = i.artworkUri;
                p.artworkId = i.artworkId;
                entries_.push_back(
                    {i.name, i.category == "Melodic Percussion" ? "Drums & Percussion" : i.category,
                     i.tags, p, i.file, i.id, false, i, pack});
            }
            size += pack.installSize;
            ++packs;
        } catch (const std::exception& ex) {
            editor_.message(QString("Library unavailable: ") + ex.what());
        }
    };
    addPack((assetPath() + "/sounds/factory").toStdString());
    addPack((assetPath() + "/sounds/essentials03").toStdString());
    if (QDir(assetPath() + "/sounds/community04").exists())
        addPack((assetPath() + "/sounds/community04").toStdString());
    QDir user(soundPackPath());
    for (const auto& name : user.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        addPack(user.filePath(name).toStdString());
    QDir registry(soundPackPath() + "/.user-instruments");
    std::set<std::string> userIds;
    for (auto& entry : userEntries_)
        userIds.insert(entry.id);
    QSet<QString> knownFiles;
    for (auto& id : userIds)
        knownFiles.insert(
            QCryptographicHash::hash(QByteArray::fromStdString(id), QCryptographicHash::Sha256)
                .toHex() +
            ".json");
    for (auto filename : registry.entryList({"*.json"}, QDir::Files)) {
        if (knownFiles.contains(filename))
            continue;
        try {
            auto p = libraryJob(this, "Reading user instrument registry…", [&] {
                return ProjectSerializer::loadPreset(registry.filePath(filename).toStdString());
            });
            auto id = p.instrumentId + ":" + p.patchId;
            if (!userIds.insert(id).second)
                continue;
            LibraryInstrument m;
            m.id = id;
            m.name = p.name;
            m.instrumentId = p.instrumentId;
            m.patchId = p.patchId;
            m.patchName = p.kind == InstrumentKind::SoundFont
                              ? std::to_string(p.soundFontBank) + ":" +
                                    std::to_string(p.soundFontProgram) + " · " + p.name
                              : p.name;
            m.category = "User Instruments";
            m.file = p.backendPath;
            m.source = p.source;
            m.family = p.family;
            std::filesystem::path file = p.backendPath;
            auto name = p.kind == InstrumentKind::SoundFont ? file.stem().string() : p.name;
            userEntries_.push_back({name, "User Instruments", p.tags, p, file, id, false, m, {}});
        } catch (const std::exception& ex) {
            editor_.message(QString("User instrument unavailable: ") + ex.what());
        }
    }
    entries_.insert(entries_.end(), userEntries_.begin(), userEntries_.end());
    std::set<std::string> installedPacks;
    for (auto& e : entries_)
        if (!e.pack.id.empty())
            installedPacks.insert(e.pack.id);
    QDir catalogs(qEnvironmentVariable("REVERIE_CATALOG_ROOT", assetPath() + "/sounds/catalog"));
    for (auto name : catalogs.entryList({"*.json"}, QDir::Files)) {
        QFile file(catalogs.filePath(name));
        if (!file.open(QIODevice::ReadOnly))
            continue;
        auto j = QJsonDocument::fromJson(file.readAll()).object();
        auto packId = j["id"].toString().toStdString();
        if (installedPacks.contains(packId))
            continue;
        SoundPack pack;
        pack.id = packId;
        pack.name = j["name"].toString().toStdString();
        pack.version = j["version"].toString().toStdString();
        pack.source = j["source"].toString().toStdString();
        pack.license = j["license"].toString().toStdString();
        pack.author = j["author"].toString().toStdString();
        packs_->addItem(QString::fromStdString(pack.name));
        for (auto value : j["instruments"].toArray()) {
            auto i = value.toObject();
            LibraryInstrument m;
            m.id = i["id"].toString().toStdString();
            m.instrumentId = i["instrumentId"].toString().toStdString();
            m.name = i["name"].toString().toStdString();
            m.patchName = i["patchName"].toString().toStdString();
            m.patchId = i["patchId"].toString().toStdString();
            m.category = i["category"].toString().toStdString();
            m.family = i["family"].toString().toStdString();
            m.articulation = i["articulation"].toString().toStdString();
            m.source = pack.source;
            Preset p;
            p.name = m.name;
            p.family = m.family;
            p.category = m.category;
            p.source = pack.source;
            p.license = pack.license;
            entries_.push_back({m.name, m.category, "available sfizz original " + m.patchName, p,
                                std::filesystem::path(i["file"].toString().toStdString()), m.id,
                                false, m, pack, false});
        }
    }
    std::stable_sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        return !a.sfz.empty() && b.sfz.empty();
    });
    QSettings settings;
    auto favorites = settings.value("library/favorites").toStringList();
    recent_ = settings.value("library/recent").toStringList();
    for (auto& e : entries_)
        e.favorite = favorites.contains(QString::fromStdString(e.id));
    std::set<std::string> instrumentIds;
    std::size_t installedPatchCount = 0;
    for (auto& e : entries_)
        if (e.installed) {
            ++installedPatchCount;
            instrumentIds.insert(e.sfz.empty() ? e.id : e.metadata.instrumentId);
        }
    status_->setText(QString("%1 installed pack(s) · %2 MiB\n%3 instruments / %4 patches")
                         .arg(packs)
                         .arg(size / 1048576., 0, 'f', 1)
                         .arg(instrumentIds.size())
                         .arg(installedPatchCount));
    filter();
}
void SoundBrowser::filter() {
    QString selected =
        list_->currentItem() ? list_->currentItem()->data(Qt::UserRole + 1).toString() : QString();
    QSignalBlocker listBlock(list_);
    list_->clear();
    visible_.clear();
    patches_->clear();
    std::set<std::string> shown;
    QString query = search_->text().trimmed().toLower(),
            category =
                categories_->currentItem() ? categories_->currentItem()->text() : "All instruments";
    for (int i = 0; i < int(entries_.size()); ++i) {
        const auto& e = entries_[i];
        if (installedOnly_->isChecked() && !e.installed)
            continue;
        if (packs_->currentIndex() > 0 &&
            QString::fromStdString(e.pack.name) != packs_->currentText())
            continue;
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
                                             (!e.installed    ? "DOWNLOAD"
                                              : e.sfz.empty() ? "SYNTH"
                                                              : "RECORDED") +
                                             (e.favorite ? " · Favorite" : ""),
                                         list_);
        auto glyph = instrumentThumbnail(e.preset, QSize(70, 48), devicePixelRatioF(), true);
        item->setIcon(QIcon(glyph));
        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, QString::fromStdString(e.id));
        item->setToolTip(QString::fromStdString(e.category + " · " + e.tags));
    }
    if (list_->count())
        list_->setCurrentRow(0);
    for (int i = 0; i < list_->count(); ++i)
        if (list_->item(i)->data(Qt::UserRole + 1).toString() == selected)
            list_->setCurrentRow(i);
    updateDetails();
}
Preset SoundBrowser::presetAt(int i) {
    if (i < 0 || i >= int(entries_.size()))
        throw std::runtime_error("No instrument selected");
    auto& e = entries_[i];
    if ((e.metadata.format == "sfizz" && !sfizzAvailable()) ||
        ((e.metadata.format == "sf2" || e.metadata.format == "sf3") && !fluidSynthAvailable()))
        throw std::runtime_error("This build does not include the required instrument backend");
    if (!e.installed)
        throw std::runtime_error("Install this collection in Sound Library Manager first");
    if (e.sfz.empty() || !e.preset.backendPath.empty())
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
    for (auto name : {"browserPreview", "browserApply", "browserNewTrack"})
        findChild<QPushButton*>(name)->setEnabled(e.installed);
    detailTitle_->setText(QString::fromStdString(e.name));
    artwork_->setPreset(e.preset);
    detail_->setText(
        QString::fromStdString(e.category + " · " +
                               (e.sfz.empty() ? "Synthesis" : e.metadata.articulation)) +
        "\n" + QString::fromStdString(e.preset.license) +
        (e.installed ? " · Installed\n" : " · Download required\n") +
        QString::fromStdString(e.pack.name) + "\n" + QString::fromStdString(e.preset.source) +
        (e.metadata.unsupportedOpcodes.empty()
             ? ""
             : "\nIgnored SFZ metadata: " + QString::fromStdString(e.metadata.unsupportedOpcodes)));
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
        Project phrase;
        phrase.bpm = 120;
        phrase.master = .45f;
        t.clips.clear();
        Clip c;
        c.length = 4;
        int root = p.kind == InstrumentKind::Sfizz ? p.rootNote : 60;
        auto name = QString::fromStdString(p.name).toLower();
        bool kit = p.kind == InstrumentKind::Drums || name.contains("kit") ||
                   name.contains("gm percussion");
        std::vector<int> keys = kit ? std::vector<int>{36, 42, 38, 46}
                                : p.category == "Drums & Percussion"
                                    ? std::vector<int>{root, root, root, root}
                                    : std::vector<int>{root, root + 4, root + 7, root + 12};
        for (int i = 0; i < int(keys.size()); ++i) {
            Note n;
            n.key = std::clamp(keys[i], 0, 127);
            n.beat = i * .5;
            n.duration = .45;
            n.velocity = i % 2 ? .8f : .65f;
            c.notes.push_back(n);
        }
        t.clips.push_back(c);
        phrase.tracks.push_back(t);
        auto graph = libraryJob(this, "Preparing instrument audition…",
                                [phrase = std::move(phrase), rate = editor_.engine.sampleRate()] {
                                    return std::make_unique<AudioProcessingGraph>(phrase, rate);
                                });
        editor_.engine.publishAudition(std::move(graph));
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
        if (expanded_ && !pin_->isChecked())
            setExpanded(false);
    } catch (const std::exception& ex) {
        editor_.message(ex.what());
    }
}
void SoundBrowser::installFolder(const QString& folder) {
    auto path = soundPackPath();
    libraryJob(this, "Installing verified sound pack…", [folder, path] {
        return installSoundPack(folder.toStdString(), path.toStdString());
    });
    reload();
}
void SoundBrowser::setExpanded(bool value) {
    expanded_ = value;
    expand_->setText(value ? "Compact ↔" : "Expand ↔");
    categoryColumn_->setVisible(value);
    detailColumn_->setVisible(value);
    detailTitle_->setVisible(value);
    detail_->setVisible(value);
    findChild<QPushButton*>("browserNewTrack")->setVisible(value);
    findChild<QPushButton*>("browserFavorite")->setVisible(value);
    patches_->setMaximumHeight(value ? 180 : 54);
    list_->setMinimumHeight(value ? 180 : 130);
    if (!value) {
        // Compact keeps patch access through a small detail section below the instrument list.
        columns_->setOrientation(Qt::Vertical);
        detailColumn_->show();
        artwork_->hide();
        categories_->setMaximumHeight(65);
        categories_->setMaximumWidth(16777215);
        categoryColumn_->show();
        detailColumn_->setMaximumHeight(125);
        columns_->setSizes({65, 300, 125});
        setMinimumWidth(230);
        setMaximumWidth(900);
    } else {
        detailColumn_->setMaximumHeight(16777215);
        columns_->setOrientation(Qt::Horizontal);
        categories_->setMaximumHeight(16777215);
        categories_->setMaximumWidth(180);
        artwork_->show();
        columns_->setSizes({140, 210, 240});
    }
    if (auto* dock = qobject_cast<QDockWidget*>(parentWidget())) {
        if (value) {
            dock->setMaximumWidth(1000);
            dock->setMinimumWidth(660);
            dock->setFloating(true);
            dock->resize(QSettings().value("library/expandedSize", QSize(740, 680)).toSize());
        } else {
            if (dock->isFloating())
                QSettings().setValue("library/expandedSize", dock->size());
            dock->setMinimumWidth(230);
            dock->setMaximumWidth(300);
            dock->setFloating(false);
            QTimer::singleShot(
                0, this, [this, dock] { editor_.resizeDocks({dock}, {260}, Qt::Horizontal); });
        }
        dock->show();
        editor_.resizeDocks({dock}, {value ? 740 : 260}, Qt::Horizontal);
    }
    QSettings().setValue("library/expanded", value);
}
SoundBrowser::~SoundBrowser() {
    QSettings prefs;
    prefs.setValue("library/expanded", expanded_);
    if (expanded_ && parentWidget())
        prefs.setValue("library/expandedSize", parentWidget()->size());
}
bool SoundBrowser::packInUse(const QString& folder) const {
    auto root = std::filesystem::weakly_canonical(folder.toStdString());
    auto inside = [&](const std::string& ref) {
        if (ref.empty())
            return false;
        auto path = std::filesystem::weakly_canonical(ref).lexically_relative(root);
        return !path.empty() && *path.begin() != "..";
    };
    for (auto& t : editor_.project.tracks) {
        if (inside(t.preset.backendPath) || inside(t.preset.samplePath))
            return true;
        for (auto& z : t.preset.zones)
            if (inside(z.path))
                return true;
    }
    return false;
}
void SoundBrowser::showManager() {
    SoundLibraryManager manager(*this);
    manager.exec();
    reload();
}
} // namespace reverie
