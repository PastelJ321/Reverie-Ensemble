#include <QCoreApplication>
// SPDX-License-Identifier: Apache-2.0
#include "core/Plugin.h"
#include "core/Serializer.h"
#include "ui/Editor.h"
#include "ui/PluginBrowser.h"
#include "ui/SoundBrowser.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtWidgets>
namespace reverie {
static QString registry() {
    auto p = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(p);
    return p + "/plugins.json";
}
static QJsonArray entries() {
    QFile f(registry());
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).array();
}
static Preset preset(const QJsonObject& j) {
    Preset p;
    p.kind = InstrumentKind::Clap;
    p.backendPath = j["path"].toString().toStdString();
    p.pluginId = j["id"].toString().toStdString();
    p.pluginVersion = j["version"].toString().toStdString();
    p.pluginVendor = j["vendor"].toString().toStdString();
    p.name = j["name"].toString().toStdString();
    p.instrumentId = "clap:" + p.pluginId;
    p.category = "Audio Plugins";
    p.gain = 1;
    p.renderTailSeconds = 10;
    p.source = p.pluginVendor;
    p.license = "User-installed plugin; see manufacturer license";
    return p;
}
std::vector<Preset> registeredPluginInstruments() {
    std::vector<Preset> p;
    for (auto v : entries())
        if (v.toObject()["instrument"].toBool())
            p.push_back(preset(v.toObject()));
    return p;
}
void editPluginParameters(Editor& e, Preset& p, bool& accepted) {
    accepted = false;
    try {
        PluginInstance instance(p, 48000);
        auto params = instance.parameters();
        QDialog dialog(&e);
        dialog.setObjectName("pluginParameters");
        dialog.setWindowTitle(QString::fromStdString(p.name) +
                              QCoreApplication::translate("Ensemble", " · ") +
                              QCoreApplication::translate("Ensemble", "Parameters"));
        dialog.resize(620, 600);
        auto* l = new QVBoxLayout(&dialog);
        l->addWidget(new QLabel(
            QCoreApplication::translate("Ensemble", "Latency: %1 samples · Tail: %2 samples")
                .arg(instance.latency())
                .arg(instance.tail())));
        auto* filter = new QLineEdit;
        filter->setPlaceholderText(QCoreApplication::translate("Ensemble", "Search parameters"));
        l->addWidget(filter);
        auto* table = new QTableWidget(params.size(), 2);
        table->setHorizontalHeaderLabels({QCoreApplication::translate("Ensemble", "Parameter"),
                                          QCoreApplication::translate("Ensemble", "Value")});
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        l->addWidget(table);
        for (unsigned i = 0; i < params.size(); ++i) {
            auto& param = params[i];
            table->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(param.name)));
            auto* v = new QDoubleSpinBox;
            v->setDecimals(param.stepped ? 0 : 5);
            v->setRange(param.minimum, param.maximum);
            v->setValue(param.value);
            v->setEnabled(!param.readOnly);
            table->setCellWidget(i, 1, v);
            QObject::connect(
                v, &QDoubleSpinBox::valueChanged, &dialog,
                [&, id = param.id](double value) { instance.setParameter(id, value); });
        }
        QObject::connect(filter, &QLineEdit::textChanged, &dialog, [=](QString query) {
            for (int i = 0; i < table->rowCount(); ++i)
                table->setRowHidden(
                    i, !table->item(i, 0)->text().contains(query, Qt::CaseInsensitive));
        });
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
        l->addWidget(buttons);
        QObject::connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, &dialog,
                         &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            p.pluginState = instance.saveState();
            accepted = true;
        }
    } catch (const std::exception& ex) {
        e.message(ex.what());
    }
}
void showPlugins(Editor& e, bool effects) {
    QDialog d(&e);
    d.setObjectName("pluginBrowser");
    d.setWindowTitle(effects ? QCoreApplication::translate("Ensemble", "Effect plugins")
                             : QCoreApplication::translate("Ensemble", "Instrument plugins"));
    d.resize(760, 560);
    auto* l = new QVBoxLayout(&d);
    auto* q = new QLineEdit;
    q->setPlaceholderText(QCoreApplication::translate("Ensemble", "Search plugins"));
    l->addWidget(q);
    auto* list = new QListWidget;
    l->addWidget(list);
    auto* status = new QLabel;
    status->setWordWrap(true);
    l->addWidget(status);
    auto* row = new QHBoxLayout;
    l->addLayout(row);
    auto* scan = new QPushButton(QCoreApplication::translate("Ensemble", "Rescan"));
    auto* folder = new QPushButton(QCoreApplication::translate("Ensemble", "Add search folder…"));
    auto* load = new QPushButton(effects ? QCoreApplication::translate("Ensemble", "Insert")
                                         : QCoreApplication::translate("Ensemble", "Create track"));
    row->addWidget(scan);
    row->addWidget(folder);
    row->addStretch();
    row->addWidget(load);
    QJsonArray catalog = entries();
    auto fill = [&] {
        list->clear();
        for (auto v : catalog) {
            auto j = v.toObject();
            if (j["instrument"].toBool() == effects)
                continue;
            QString name = j["name"].toString() + " · " + j["vendor"].toString() + " · CLAP";
            if (!name.contains(q->text(), Qt::CaseInsensitive))
                continue;
            auto* i = new QListWidgetItem(name, list);
            i->setData(Qt::UserRole, j);
            i->setToolTip(j["path"].toString());
        }
        status->setText(QCoreApplication::translate(
            "Ensemble", "CLAP: stereo instruments and effects; parameter editor. Audio Units and "
                        "VST3 hosting are not available in this build."));
    };
    fill();
    QObject::connect(q, &QLineEdit::textChanged, &d, [&] { fill(); });
    auto rescan = [&] {
        QStringList roots = {QDir::homePath() + "/Library/Audio/Plug-Ins/CLAP",
                             "/Library/Audio/Plug-Ins/CLAP"};
        roots += QSettings().value("plugins/paths").toStringList();
        QSet<QString> paths;
        for (auto root : roots) {
            QDirIterator it(root, {"*.clap"}, QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);
            while (it.hasNext())
                paths.insert(it.next());
        }
        catalog = {};
        QStringList failures;
        scan->setEnabled(false);
        for (auto path : paths) {
            QProcess child;
            auto scanner = QCoreApplication::applicationDirPath() + "/reverie-plugin-scan";
            if (!QFileInfo::exists(scanner))
                scanner = QString::fromUtf8(REVERIE_SCANNER_PATH);
            child.start(scanner, {path});
            if (!child.waitForStarted(2000) || !child.waitForFinished(15000) ||
                child.exitStatus() != QProcess::NormalExit || child.exitCode() != 0) {
                child.kill();
                child.waitForFinished(1000);
                failures << QFileInfo(path).fileName();
                continue;
            }
            auto records = QJsonDocument::fromJson(child.readAllStandardOutput()).array();
            for (auto record : records)
                catalog.append(record);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
        QSaveFile file(registry());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(catalog).toJson());
            file.commit();
        }
        scan->setEnabled(true);
        fill();
        if (!failures.isEmpty())
            status->setText(QCoreApplication::translate("Ensemble", "Could not scan: %1")
                                .arg(failures.join(", ")));
        e.findChild<SoundBrowser*>("soundBrowser")->reload();
    };
    QObject::connect(scan, &QPushButton::clicked, &d, rescan);
    QObject::connect(folder, &QPushButton::clicked, &d, [&] {
        auto path = QFileDialog::getExistingDirectory(
            &d, QCoreApplication::translate("Ensemble", "Plugin search folder"));
        if (path.isEmpty())
            return;
        auto paths = QSettings().value("plugins/paths").toStringList();
        if (!paths.contains(path))
            paths << path;
        QSettings().setValue("plugins/paths", paths);
        rescan();
    });
    QObject::connect(load, &QPushButton::clicked, &d, [&] {
        if (!list->currentItem())
            return;
        auto p = preset(list->currentItem()->data(Qt::UserRole).toJsonObject());
        try {
            PluginInstance check(p, 48000);
            p.pluginState = check.saveState();
            p.renderTailSeconds = std::clamp(check.tail() / 48000.f, 2.f, 120.f);
            if (effects) {
                if (e.selectedTrack < 0)
                    return;
                e.edit([&] { e.project.tracks[e.selectedTrack].inserts.push_back({p, false, 1}); });
            } else
                e.applyInstrument(p, true);
            d.accept();
        } catch (const std::exception& ex) {
            e.message(ex.what());
        }
    });
    QObject::connect(list, &QListWidget::itemDoubleClicked, &d, [&] { load->click(); });
    d.exec();
}
void showInserts(Editor& e, int track) {
    if (track < 0 || track >= int(e.project.tracks.size()))
        return;
    e.select(track);
    QDialog d(&e);
    d.setWindowTitle(QCoreApplication::translate("Ensemble", "Track inserts"));
    d.setObjectName("insertRack");
    d.resize(560, 440);
    auto* l = new QVBoxLayout(&d);
    auto* list = new QListWidget;
    l->addWidget(list);
    auto fill = [&] {
        list->clear();
        for (auto& i : e.project.tracks[track].inserts)
            list->addItem(
                QString::fromStdString(i.plugin.name) +
                (i.bypass ? QCoreApplication::translate("Ensemble", " · Bypassed") : QString()));
    };
    fill();
    auto* row = new QHBoxLayout;
    l->addLayout(row);
    auto button = [&](QString text, auto fn) {
        auto* b = new QPushButton(text);
        row->addWidget(b);
        QObject::connect(b, &QPushButton::clicked, &d, fn);
    };
    button(QCoreApplication::translate("Ensemble", "Add…"), [&] {
        showPlugins(e, true);
        fill();
    });
    button(QCoreApplication::translate("Ensemble", "Parameters…"), [&] {
        int n = list->currentRow();
        if (n < 0)
            return;
        auto p = e.project.tracks[track].inserts[n].plugin;
        bool ok;
        editPluginParameters(e, p, ok);
        if (ok)
            e.edit([&] { e.project.tracks[track].inserts[n].plugin = p; });
    });
    button(QCoreApplication::translate("Ensemble", "Bypass"), [&] {
        int n = list->currentRow();
        if (n < 0)
            return;
        e.edit([&] {
            auto& i = e.project.tracks[track].inserts[n];
            i.bypass = !i.bypass;
        });
        fill();
    });
    button(QCoreApplication::translate("Ensemble", "Remove"), [&] {
        int n = list->currentRow();
        if (n < 0)
            return;
        e.edit([&] {
            auto& v = e.project.tracks[track].inserts;
            v.erase(v.begin() + n);
        });
        fill();
    });
    auto* wet = new QDoubleSpinBox;
    wet->setRange(0, 1);
    wet->setSingleStep(.05);
    wet->setPrefix(QCoreApplication::translate("Ensemble", "Wet "));
    l->addWidget(wet);
    QObject::connect(list, &QListWidget::currentRowChanged, &d, [&](int n) {
        QSignalBlocker b(wet);
        if (n >= 0)
            wet->setValue(e.project.tracks[track].inserts[n].wet);
    });
    QObject::connect(wet, &QDoubleSpinBox::editingFinished, &d, [&] {
        int n = list->currentRow();
        if (n >= 0)
            e.edit([&] { e.project.tracks[track].inserts[n].wet = wet->value(); });
    });
    auto* move = new QPushButton(QCoreApplication::translate("Ensemble", "Move up"));
    l->addWidget(move);
    QObject::connect(move, &QPushButton::clicked, &d, [&] {
        int n = list->currentRow();
        if (n > 0) {
            e.edit([&] {
                auto& v = e.project.tracks[track].inserts;
                std::swap(v[n], v[n - 1]);
            });
            fill();
            list->setCurrentRow(n - 1);
        }
    });
    d.exec();
}
} // namespace reverie
