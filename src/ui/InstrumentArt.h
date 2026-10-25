// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <QPixmap>
#include <QWidget>
namespace reverie {
QString instrumentFamily(const Preset&);
QPixmap instrumentThumbnail(const Preset&, QSize, qreal dpr = 1, bool artwork = false);
QPixmap instrumentDrawing(const QString& family, QSize size, bool detailed = false, qreal dpr = 1);
class InstrumentArtwork final : public QWidget {
  public:
    explicit InstrumentArtwork(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(145);
        setMaximumHeight(240);
        setObjectName("instrumentArtwork");
    }
    void setPreset(const Preset&);
    void setFamily(QString family) {
        ++request_;
        localPath_.clear();
        localImage_ = QImage();
        family_ = std::move(family);
        update();
    }

  protected:
    void paintEvent(QPaintEvent*) override;

  private:
    QString family_ = "synth";
    QString localPath_;
    QImage localImage_;
    std::uint64_t request_ = 0;
};
} // namespace reverie
