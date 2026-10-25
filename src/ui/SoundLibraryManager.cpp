// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/SoundLibraryManager.h"
#include "core/Library.h"
#include "ui/SoundBrowser.h"
#include "ui/Theme.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QVBoxLayout>
#include <chrono>
#include <future>
namespace reverie {
static QString packRoot() {
    return soundPackPath();
}
static QJsonObject manifest(QString path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("Cannot read pack catalog");
    auto j = QJsonDocument::fromJson(f.readAll());
    if (!j.isObject())
        throw std::runtime_error("Invalid catalog JSON");
    return j.object();
}
SoundLibraryManager::SoundLibraryManager(SoundBrowser& browser)
    : QDialog(&browser), browser_(browser) {
    setObjectName("soundLibraryManager");
    setWindowTitle("Sound Library Manager");
    resize(760, 560);
    setMinimumSize(620, 440);
    auto* layout = new QVBoxLayout(this);
    auto* title = new QLabel("ENSEMBLE SOUND LIBRARY");
    title->setStyleSheet("font-size:19px;font-weight:600;color:#E8E1D5");
    layout->addWidget(title);
    layout->addWidget(
        new QLabel("Verified catalogs · original patch mappings · explicit downloads only"));
    auto* columns = new QHBoxLayout;
    auto* a = new QVBoxLayout;
    a->addWidget(new QLabel("AVAILABLE PACKS"));
    available_ = new QListWidget;
    available_->setObjectName("availableSoundPacks");
    a->addWidget(available_);
    columns->addLayout(a);
    auto* b = new QVBoxLayout;
    b->addWidget(new QLabel("INSTALLED"));
    installed_ = new QListWidget;
    installed_->setObjectName("installedSoundPacks");
    b->addWidget(installed_);
    columns->addLayout(b);
    layout->addLayout(columns, 1);
    details_ = new QLabel;
    details_->setObjectName("libraryInstallStatus");
    details_->setTextFormat(Qt::PlainText);
    details_->setWordWrap(true);
    details_->setMinimumHeight(95);
    layout->addWidget(details_);
    progress_ = new QProgressBar;
    progress_->setRange(0, 1000);
    layout->addWidget(progress_);
    auto* controls = new QHBoxLayout;
    download_ = new QPushButton("Download / Install / Update");
    download_->setObjectName("downloadSoundPack");
    controls->addWidget(download_);
    auto* local = new QPushButton("Install local folder…");
    controls->addWidget(local);
    auto* remove = new QPushButton("Remove optional pack…");
    controls->addWidget(remove);
    cancel_ = new QPushButton("Cancel download");
    cancel_->setEnabled(false);
    controls->addWidget(cancel_);
    layout->addLayout(controls);
    auto* close = new QPushButton("Close");
    layout->addWidget(close);
    connect(close, &QPushButton::clicked, this, &SoundLibraryManager::reject);
    connect(cancel_, &QPushButton::clicked, this, [this] { cancelled_ = true; });
    connect(download_, &QPushButton::clicked, this, [this] { download(); });
    connect(local, &QPushButton::clicked, this, [this] {
        if (busy_)
            return;
        auto folder =
            QFileDialog::getExistingDirectory(this, "Install local verified pack (pack.json)");
        if (folder.isEmpty())
            return;
        try {
            browser_.installFolder(folder);
            reload();
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Install failed", e.what());
        }
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (busy_ || !installed_->currentItem())
            return;
        auto path = installed_->currentItem()->data(Qt::UserRole).toString();
        if (path.isEmpty())
            return;
        if (browser_.packInUse(path)) {
            QMessageBox::information(
                this, "Pack is in use",
                "Change or remove this project’s instruments before removing their sample pack.");
            return;
        }
        if (QMessageBox::question(this, "Remove optional sound pack",
                                  "Remove installed files? Projects referencing this pack will "
                                  "need it reinstalled. Bundled factory sounds are protected.") !=
            QMessageBox::Yes)
            return;
        auto root = std::filesystem::weakly_canonical(packRoot().toStdString());
        auto target = std::filesystem::weakly_canonical(path.toStdString());
        if (target.parent_path() != root) {
            QMessageBox::warning(this, "Remove failed", "Pack path is outside the managed library");
            return;
        }
        std::error_code ec;
        std::filesystem::remove_all(target, ec);
        if (ec)
            QMessageBox::warning(this, "Remove failed", QString::fromStdString(ec.message()));
        reload();
        browser_.reload();
    });
    connect(available_, &QListWidget::currentRowChanged, this, [this] {
        if (!available_->currentItem())
            return;
        try {
            auto j = manifest(available_->currentItem()->data(Qt::UserRole).toString());
            details_->setText(
                j["name"].toString() + " · v" + j["version"].toString() + "\n" +
                j["license"].toString() + " · " + j["author"].toString() +
                QString("\nDownload / install: %1 MiB · %2 patches · Backend: sfizz\n")
                    .arg(j["installSize"].toDouble() / 1048576., 0, 'f', 1)
                    .arg(j["instruments"].toArray().size()) +
                j["source"].toString());
        } catch (const std::exception& e) {
            details_->setText(e.what());
        }
    });
    reload();
}
void SoundLibraryManager::reload() {
    available_->clear();
    installed_->clear();
    QDir catalogs(qEnvironmentVariable("REVERIE_CATALOG_ROOT", assetPath() + "/sounds/catalog"));
    for (auto name : catalogs.entryList({"*.json"}, QDir::Files)) {
        try {
            auto j = manifest(catalogs.filePath(name));
            auto* item = new QListWidgetItem(
                j["name"].toString() + QString("\n%1 MiB · %2")
                                           .arg(j["installSize"].toDouble() / 1048576., 0, 'f', 1)
                                           .arg(j["license"].toString()),
                available_);
            item->setData(Qt::UserRole, catalogs.filePath(name));
        } catch (...) {
        }
    }
    auto add = [this](QString path, bool bundled) {
        try {
            auto p = readSoundPack(path.toStdString(), false);
            auto* item = new QListWidgetItem(QString::fromStdString(p.name + " · " + p.version) +
                                                 (bundled ? " · Bundled" : ""),
                                             installed_);
            if (!bundled)
                item->setData(Qt::UserRole, path);
        } catch (...) {
        }
    };
    add(assetPath() + "/sounds/factory", true);
    add(assetPath() + "/sounds/essentials03", true);
    QDir root(packRoot());
    for (auto name : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (!name.startsWith('.'))
            add(root.filePath(name), false);
    if (available_->count())
        available_->setCurrentRow(0);
    else
        details_->setText("Offline: install a verified local sound pack folder. Catalog generation "
                          "is documented in scripts/.");
}
void SoundLibraryManager::reject() {
    if (busy_) {
        cancelled_ = true;
        return;
    }
    QDialog::reject();
}
void SoundLibraryManager::download() {
    if (busy_ || !available_->currentItem())
        return;
    QString staging, outcome;
    try {
        auto path = available_->currentItem()->data(Qt::UserRole).toString();
        auto j = manifest(path);
        auto id = j["id"].toString();
        if (!QRegularExpression("^[a-z0-9][a-z0-9.-]{0,95}$").match(id).hasMatch())
            throw std::runtime_error("Invalid pack ID");
        if (browser_.packInUse(packRoot() + "/" + id))
            throw std::runtime_error(
                "Pack is in use by this project; change its instruments before updating");
        auto files = j["files"].toArray();
        qint64 total = qint64(j["installSize"].toDouble()), sum = 0;
        if (total <= 0 || total > 32LL * 1024 * 1024 * 1024)
            throw std::runtime_error("Invalid pack size");
        QSet<QString> paths;
        for (auto value : files) {
            auto f = value.toObject();
            auto rel = f["path"].toString();
            auto url = QUrl(f["url"].toString());
            auto n = qint64(f["size"].toDouble());
            if (rel.isEmpty() || QDir::isAbsolutePath(rel) || rel.contains('\\') ||
                rel.split('/').contains("..") || paths.contains(rel) || rel == "pack.json" ||
                n < 0 || n > total - sum || url.scheme() != "https" ||
                !QRegularExpression("^[0-9a-f]{64}$").match(f["sha256"].toString()).hasMatch())
                throw std::runtime_error("Unsafe catalog inventory");
            sum += n;
            paths.insert(rel);
        }
        if (sum != total)
            throw std::runtime_error("Catalog size mismatch");
        QDir().mkpath(packRoot());
        QStorageInfo space(packRoot());
        if (space.bytesAvailable() < total * 2 + 64 * 1024 * 1024)
            throw std::runtime_error("Not enough disk space (staging + installed copy required)");
        if (QMessageBox::question(
                this, "Download sound pack",
                details_->text() +
                    "\n\nDownload these files and accept the displayed asset license?") !=
            QMessageBox::Yes)
            return;
        busy_ = true;
        cancelled_ = false;
        cancel_->setEnabled(true);
        download_->setEnabled(false);
        available_->setEnabled(false);
        QTemporaryDir stage(packRoot() + "/.download-" + id + "-XXXXXX");
        if (!stage.isValid())
            throw std::runtime_error("Cannot create isolated staging directory");
        stage.setAutoRemove(false);
        staging = stage.path();
        QNetworkAccessManager network;
        qint64 done = 0;
        for (auto value : files) {
            if (cancelled_)
                throw std::runtime_error("Installation cancelled; staging removed");
            auto f = value.toObject();
            auto rel = f["path"].toString();
            auto filePath = staging + "/" + rel;
            QDir().mkpath(QFileInfo(filePath).path());
            QFile out(filePath);
            if (!out.open(QIODevice::WriteOnly | QIODevice::NewOnly))
                throw std::runtime_error("Cannot create staged file");
            QNetworkRequest request(QUrl(f["url"].toString()));
            request.setTransferTimeout(60000);
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                 QNetworkRequest::NoLessSafeRedirectPolicy);
            auto* reply = network.get(request);
            QCryptographicHash hash(QCryptographicHash::Sha256);
            qint64 received = 0, expected = qint64(f["size"].toDouble());
            bool writeFailed = false;
            auto consume = [&] {
                if (!reply->isOpen())
                    return;
                auto bytes = reply->readAll();
                if (bytes.size() > expected - received) {
                    writeFailed = true;
                    reply->abort();
                    return;
                }
                received += bytes.size();
                hash.addData(bytes);
                if (out.write(bytes) != bytes.size()) {
                    writeFailed = true;
                    reply->abort();
                }
            };
            connect(reply, &QIODevice::readyRead, this, consume);
            while (!reply->isFinished()) {
                QApplication::processEvents(QEventLoop::AllEvents, 10);
                if (cancelled_)
                    reply->abort();
                progress_->setValue(int((done + received) * 1000 / total));
                details_->setText(QString("%1\n%2 / %3 MiB · %4")
                                      .arg(j["name"].toString())
                                      .arg((done + received) / 1048576., 0, 'f', 1)
                                      .arg(total / 1048576., 0, 'f', 1)
                                      .arg(rel));
            }
            consume();
            out.close();
            auto error = reply->error();
            delete reply;
            if (cancelled_)
                throw std::runtime_error("Installation cancelled; staging removed");
            if (writeFailed || error != QNetworkReply::NoError || received != expected ||
                hash.result().toHex() != f["sha256"].toString().toLatin1())
                throw std::runtime_error("Download failed or SHA-256 mismatch");
            done += received;
        }
        QFile pack(staging + "/pack.json");
        if (!pack.open(QIODevice::WriteOnly))
            throw std::runtime_error("Cannot write manifest");
        pack.write(QJsonDocument(j).toJson());
        pack.close();
        auto check = std::async(std::launch::async,
                                [staging] { return readSoundPack(staging.toStdString(), true); });
        details_->setText("Verifying inventory and SFZ references…");
        while (check.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready)
            QApplication::processEvents();
        (void)check.get();
        if (cancelled_)
            throw std::runtime_error("Installation cancelled");
        QString final = packRoot() + "/" + id, backup = packRoot() + "/.previous-" + id;
        if (QFileInfo(final).isSymLink() || QFileInfo(backup).isSymLink())
            throw std::runtime_error("Symlinked installation target rejected");
        if (QDir(backup).exists())
            QDir(backup).removeRecursively();
        if (QDir(final).exists() && !QDir().rename(final, backup))
            throw std::runtime_error("Cannot stage previous pack for update");
        if (!QDir().rename(staging, final)) {
            QDir().rename(backup, final);
            throw std::runtime_error("Cannot activate installed pack");
        }
        staging.clear();
        if (QDir(backup).exists())
            QDir(backup).removeRecursively();
        progress_->setValue(1000);
        details_->setText("Installed and verified. Instruments are available immediately.");
        browser_.reload();
    } catch (const std::exception& e) {
        details_->setText(QString::fromUtf8(e.what()));
    }
    outcome = details_->text();
    if (!staging.isEmpty())
        QDir(staging).removeRecursively();
    busy_ = false;
    cancel_->setEnabled(false);
    download_->setEnabled(true);
    available_->setEnabled(true);
    reload();
    details_->setText(outcome);
}
} // namespace reverie
