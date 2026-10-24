// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include "core/Model.h"
#include <QPixmap>
#include <QWidget>
namespace reverie {
QString instrumentFamily(const Preset&);
QPixmap instrumentDrawing(const QString& family, QSize size, bool detailed = false, qreal dpr = 1);
class InstrumentArtwork final : public QWidget {
  public:
    explicit InstrumentArtwork(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(108);
        setMaximumHeight(140);
        setObjectName("instrumentArtwork");
    }
    void setFamily(QString family) {
        family_ = std::move(family);
        update();
    }

  protected:
    void paintEvent(QPaintEvent*) override;

  private:
    QString family_ = "synth";
};
} // namespace reverie
