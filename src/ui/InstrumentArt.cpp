// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/InstrumentArt.h"
#include "ui/Theme.h"
#include <QImageReader>
#include <QPainter>
#include <QPixmapCache>
#include <QPointer>
#include <QTimer>
#include <QUrl>
#include <future>
#ifdef REVERIE_HAS_QTSVG
#include <QSvgRenderer>
#endif
namespace reverie {
QString instrumentFamily(const Preset& p) {
    auto s = QString::fromStdString(p.instrumentId + " " + p.name).toLower();
    if (s.contains("contrabass") || s.contains("double bass"))
        return "double-bass";
    if (s.contains("viola"))
        return "viola";
    if (s.contains("bassoon"))
        return "bassoon";
    if (s.contains("clarinet"))
        return "clarinet";
    if (s.contains("oboe"))
        return "oboe";
    if (s.contains("horn"))
        return "horn";
    if (s.contains("trombone"))
        return "trombone";
    if (s.contains("tuba"))
        return "tuba";
    if (s.contains("piccolo"))
        return "piccolo";
    if (s.contains("organ"))
        return "organ";
    if (s.contains("vibraphone"))
        return "vibraphone";
    if (s.contains("kalimba") || s.contains("mbira") || s.contains("nyunga"))
        return "kalimba";
    if (s.contains("balafon"))
        return "balafon";
    if (s.contains("timpani"))
        return "timpani";
    if (s.contains("tubular"))
        return "tubular-bells";
    if (s.contains("harpsichord"))
        return "grand";
    if (s.contains("harp"))
        return "harp";
    if (s.contains("acoustic guitar"))
        return "acoustic-guitar";
    if (p.kind == InstrumentKind::Drums || s.contains("percussion") || s.contains("drum"))
        return "drums";
    if (s.contains("upright"))
        return "upright";
    if (s.contains("grand"))
        return "grand";
    if (s.contains("guitar"))
        return "guitar";
    if (s.contains("bass"))
        return "bass";
    if (s.contains("cello"))
        return "cello";
    if (s.contains("violin") || s.contains("strings"))
        return "strings";
    if (s.contains("trumpet"))
        return "brass";
    if (s.contains("flute"))
        return "wind";
    if (s.contains("marimba") || s.contains("bell"))
        return "mallet";
    return p.family.empty() ? "synth" : QString::fromStdString(p.family);
}

static const QImage* atlas(bool orchestra = false) {
    static auto job = std::async(
        std::launch::async, [] { return QImage(assetPath() + "/artwork/instrument-atlas.png"); });
    static auto orchestralJob = std::async(
        std::launch::async, [] { return QImage(assetPath() + "/artwork/orchestra-atlas.png"); });
    static QImage orchestralDecoded;
    if (orchestra) {
        if (orchestralDecoded.isNull() && orchestralJob.valid() &&
            orchestralJob.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            orchestralDecoded = orchestralJob.get();
        return orchestralDecoded.isNull() ? nullptr : &orchestralDecoded;
    }
    static QImage decoded;
    if (decoded.isNull() && job.valid() &&
        job.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        decoded = job.get();
    return decoded.isNull() ? nullptr : &decoded;
}
QPixmap instrumentThumbnail(const Preset& preset, QSize size, qreal dpr, bool artwork) {
    auto uri = QString::fromStdString(preset.thumbnailAsset);
    auto path = uri.startsWith("file:") ? QUrl(uri).toLocalFile() : uri;
    if (path.isEmpty() || path.startsWith("asset:"))
        return instrumentDrawing(instrumentFamily(preset), size, artwork, dpr);
    static QHash<QString, std::shared_ptr<std::future<QImage>>> pending;
    static QHash<QString, QImage> images;
    if (!pending.contains(path) && !images.contains(path) && pending.size() < 4)
        pending[path] =
            std::make_shared<std::future<QImage>>(std::async(std::launch::async, [path] {
                QImageReader reader(path);
                auto source = reader.size();
                if (source.width() > 8192 || source.height() > 8192 || source.isEmpty())
                    return QImage();
                reader.setScaledSize(source.scaled(256, 256, Qt::KeepAspectRatio));
                return reader.read();
            }));
    if (pending.contains(path) &&
        pending[path]->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        images[path] = pending[path]->get();
        pending.remove(path);
        if (images.size() > 128)
            images.erase(images.begin());
    }
    if (images.contains(path) && !images[path].isNull()) {
        auto image = QPixmap::fromImage(
            images[path].scaled(size * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        image.setDevicePixelRatio(dpr);
        return image;
    }
    return instrumentDrawing(instrumentFamily(preset), size, artwork, dpr);
}
QPixmap instrumentDrawing(const QString& family, QSize size, bool detail, qreal dpr) {
    QString key = "art04:" + QString::number(detail) + family + ":" +
                  QString::number(size.width()) + ":" + QString::number(size.height()) + ":" +
                  QString::number(dpr);
    QPixmap result;
    if (QPixmapCache::find(key, &result))
        return result;
    result = QPixmap(size * dpr);
    result.setDevicePixelRatio(dpr);
    result.fill(QColor("#E8E1D5"));
#ifdef REVERIE_HAS_QTSVG
    if (!detail) {
        auto iconFamily = family;
        static const QMap<QString, QString> aliases = {
            {"viola", "strings"},        {"violin", "strings"},   {"piccolo", "wind"},
            {"trombone", "other-brass"}, {"tuba", "other-brass"}, {"timpani", "percussion"},
            {"tubular-bells", "bells"}};
        iconFamily = aliases.value(iconFamily, iconFamily);
        QSvgRenderer svg(assetPath() + "/artwork/icons/" + iconFamily + ".svg");
        if (svg.isValid()) {
            result.fill(Qt::transparent);
            QPainter painter(&result);
            svg.render(&painter, QRectF(0, 0, size.width(), size.height()));
            painter.end();
            QPixmapCache::insert(key, result);
            return result;
        }
    }
#endif
    static const QMap<QString, int> orchestraCells = {
        {"cello", 0},    {"double-bass", 1}, {"viola", 2},    {"bassoon", 3},
        {"clarinet", 4}, {"oboe", 5},        {"horn", 6},     {"trombone", 7},
        {"tuba", 8},     {"piccolo", 9},     {"organ", 10},   {"vibraphone", 11},
        {"kalimba", 12}, {"balafon", 13},    {"timpani", 14}, {"tubular-bells", 15}};
    bool orchestra = orchestraCells.contains(family);
    const auto* source = atlas(orchestra);
    if (!source)
        return result;
    static const QMap<QString, int> cells = {
        {"grand", 0},   {"upright", 1}, {"acoustic-guitar", 2}, {"guitar", 3}, {"bass", 4},
        {"strings", 5}, {"violin", 5},  {"cello", 5},           {"drums", 6},  {"synth", 7},
        {"wind", 8},    {"brass", 9},   {"harp", 10},           {"mallet", 11}};
    int index = orchestra ? orchestraCells[family] : cells.value(family, 7);
    int w = source->width() / 4, h = source->height() / (orchestra ? 4 : 3);
    auto tile = source->copy((index % 4) * w, (index / 4) * h, w, h)
                    .scaled(size * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter paint(&result);
    paint.drawImage(QRectF((size.width() - tile.width() / dpr) / 2,
                           (size.height() - tile.height() / dpr) / 2, tile.width() / dpr,
                           tile.height() / dpr),
                    tile);
    paint.end();
    QPixmapCache::insert(key, result);
    return result;
}
void InstrumentArtwork::setPreset(const Preset& preset) {
    family_ = instrumentFamily(preset);
    auto uri = QString::fromStdString(preset.artworkUri);
    auto path = uri.startsWith("file:") ? QUrl(uri).toLocalFile() : uri;
    if (path.startsWith("asset:") || path.isEmpty()) {
        ++request_;
        localPath_.clear();
        localImage_ = QImage();
        update();
        return;
    }
    if (path == localPath_)
        return;
    localPath_ = path;
    localImage_ = QImage();
    auto serial = ++request_;
    auto result = std::make_shared<std::future<QImage>>(std::async(std::launch::async, [path] {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        auto size = reader.size();
        if (size.width() > 8192 || size.height() > 8192 || size.width() <= 0 || size.height() <= 0)
            return QImage();
        reader.setScaledSize(size.scaled(2048, 2048, Qt::KeepAspectRatio));
        return reader.read();
    }));
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, result, serial] {
        if (result->wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;
        auto image = result->get();
        if (serial == request_)
            localImage_ = std::move(image);
        timer->stop();
        timer->deleteLater();
        update();
    });
    timer->start(20);
    update();
}
void InstrumentArtwork::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor("#E8E1D5"));
    if (!localImage_.isNull()) {
        auto target = localImage_.size().scaled(size(), Qt::KeepAspectRatio);
        p.drawImage(
            QRect(QPoint((width() - target.width()) / 2, (height() - target.height()) / 2), target),
            localImage_);
    } else
        p.drawPixmap(0, 0, instrumentDrawing(family_, size(), true, devicePixelRatioF()));
    if (!atlas())
        QTimer::singleShot(30, this, qOverload<>(&QWidget::update));
}
} // namespace reverie
