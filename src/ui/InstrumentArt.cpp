// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/InstrumentArt.h"
#include "ui/DesignTokens.h"
#include <QPainter>
#include <QPainterPath>
namespace reverie {
QString instrumentFamily(const Preset& p) {
    if (!p.family.empty())
        return QString::fromStdString(p.family);
    auto s = QString::fromStdString(p.instrumentId + " " + p.name).toLower();
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
    return "synth";
}
QPixmap instrumentDrawing(const QString& family, QSize size, bool detail, qreal dpr) {
    QPixmap image(size * dpr);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    double scale = std::min(size.width() / 200., size.height() / 110.);
    p.translate((size.width() - 200 * scale) / 2, (size.height() - 110 * scale) / 2);
    p.scale(scale, scale);
    QColor graphite("#403B34"), copper("#B68E66"), paper("#E8E1D5");
    p.setPen(QPen(graphite, detail ? 1.6 : 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(detail ? QColor("#C7B399") : graphite);
    auto line = [&](double a, double b, double c, double d) {
        p.drawLine(QPointF(a, b), QPointF(c, d));
    };
    auto keyboard = [&](QRectF rect) {
        p.setBrush(paper);
        p.drawRect(rect);
        for (int i = 1; i < 15; ++i)
            line(rect.left() + rect.width() * i / 15, rect.top(),
                 rect.left() + rect.width() * i / 15, rect.bottom());
        p.setBrush(graphite);
        for (int i = 0; i < 14; ++i)
            if (i % 7 != 2 && i % 7 != 6)
                p.drawRect(QRectF(rect.left() + rect.width() * (i + .7) / 15, rect.top(),
                                  rect.width() * .55 / 15, rect.height() * .58));
    };
    if (family == "grand") {
        QPainterPath body;
        body.moveTo(55, 85);
        body.lineTo(55, 27);
        body.cubicTo(52, 1, 120, 5, 133, 24);
        body.cubicTo(155, 46, 107, 48, 141, 85);
        body.closeSubpath();
        p.drawPath(body);
        if (detail) {
            p.setBrush(Qt::NoBrush);
            p.drawPath(body.translated(4, -3));
            for (int i = 0; i < 10; ++i)
                line(65 + i * 5, 27, 74 + i * 5, 77);
            line(50, 22, 148, 72);
        }
        keyboard({53, 78, 90, 18});
        line(60, 96, 58, 105);
        line(135, 96, 139, 105);
    } else if (family == "upright" || family == "keys" || family == "synth") {
        if (family == "synth" || family == "keys") {
            p.translate(0, 25);
            p.scale(1, .65);
        }
        p.drawRect(QRectF(38, 25, 124, 56));
        keyboard({39, 68, 122, 22});
        line(44, 90, 40, 105);
        line(156, 90, 160, 105);
        if (detail) {
            p.setBrush(graphite);
            if (family == "synth") {
                for (int i = 0; i < 6; ++i)
                    p.drawEllipse(QPointF(53 + i * 18, 43), 4, 4);
                p.drawRect(QRectF(90, 50, 48, 9));
            } else {
                line(45, 31, 155, 31);
                line(45, 60, 155, 60);
                p.drawEllipse(QPointF(100, 49), 2, 2);
            }
        }
    } else if (family == "guitar" || family == "bass") {
        p.save();
        p.translate(95, 65);
        if (family == "bass")
            p.scale(.8, 1.12);
        p.rotate(30);
        QPainterPath body;
        body.moveTo(-8, -13);
        body.cubicTo(-31, -35, -42, -8, -25, 2);
        body.cubicTo(-51, 37, -25, 53, 0, 45);
        body.cubicTo(32, 52, 42, 23, 19, 5);
        body.cubicTo(36, -18, 19, -32, 8, -13);
        body.closeSubpath();
        p.drawPath(body);
        p.drawRect(QRectF(-4, -55, 8, 62));
        p.drawRect(QRectF(-7, -68, 14, 15));
        if (detail) {
            p.setBrush(graphite);
            p.drawRect(QRectF(-13, 19, 26, 4));
            for (int i = 0; i < (family == "bass" ? 4 : 6); ++i)
                line(-3 + i * 1.3, -64, -3 + i * 1.3, 26);
            for (int i = 0; i < 8; ++i)
                line(-4, -50 + i * 6, 4, -50 + i * 6);
            p.drawEllipse(QPointF(20, 23), 2, 2);
        }
        p.restore();
    } else if (family == "strings" || family == "cello" || family == "double-bass") {
        QPainterPath body;
        body.moveTo(90, 42);
        body.cubicTo(66, 20, 57, 44, 79, 57);
        body.cubicTo(54, 67, 67, 96, 96, 89);
        body.cubicTo(124, 96, 137, 67, 111, 57);
        body.cubicTo(133, 42, 124, 22, 102, 42);
        body.closeSubpath();
        p.drawPath(body);
        p.drawRect(QRectF(92, 10, 7, 51));
        p.drawEllipse(QRectF(90, 3, 11, 10));
        line(143, 13, 119, 99);
        line(147, 14, 123, 100);
        if (family == "cello" || family == "double-bass")
            line(96, 90, 96, 108);
        if (detail) {
            for (int i = 0; i < 4; ++i)
                line(93 + i, 12, 93 + i, 82);
            line(85, 75, 108, 75);
            p.drawText(QPointF(78, 72), "f");
            p.drawText(QPointF(110, 72), "f");
        }
    } else if (family == "brass") {
        QPainterPath bell;
        bell.moveTo(135, 47);
        bell.lineTo(165, 28);
        bell.lineTo(165, 76);
        bell.lineTo(135, 57);
        bell.closeSubpath();
        p.drawPath(bell);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(48, 49, 91, 31), 14, 14);
        line(28, 49, 135, 49);
        for (int i = 0; i < 3; ++i) {
            p.drawRect(QRectF(75 + i * 16, 39, 7, 30));
            line(73 + i * 16, 35, 84 + i * 16, 35);
        }
        if (detail) {
            line(41, 46, 139, 46);
            line(34, 45, 34, 54);
        }
    } else if (family == "wind") {
        p.drawRoundedRect(QRectF(23, 50, 156, 10), 3, 3);
        p.setBrush(paper);
        for (int i = 0; i < 9; ++i)
            p.drawEllipse(QPointF(57 + i * 12, 55), 4, 4);
        if (detail) {
            line(31, 47, 40, 47);
            for (int i = 0; i < 6; ++i)
                line(70 + i * 14, 47, 70 + i * 14, 63);
        }
    } else if (family == "drums") {
        p.drawEllipse(QRectF(73, 49, 57, 57));
        p.drawEllipse(QRectF(41, 43, 33, 26));
        p.drawEllipse(QRectF(126, 40, 35, 32));
        p.drawEllipse(QRectF(76, 27, 26, 23));
        p.drawEllipse(QRectF(104, 27, 26, 23));
        line(37, 32, 37, 99);
        line(163, 28, 163, 99);
        p.setBrush(copper);
        p.drawEllipse(QRectF(12, 25, 49, 7));
        p.drawEllipse(QRectF(139, 20, 50, 8));
        line(37, 99, 25, 106);
        line(163, 99, 176, 106);
        if (detail) {
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QRectF(78, 54, 47, 47));
            line(100, 90, 106, 104);
        }
    } else if (family == "mallet") {
        for (int i = 0; i < 10; ++i)
            p.drawRect(QRectF(35 + i * 13, 27 + i * 2, 11, 48 - i * 2));
        line(42, 79, 39, 105);
        line(152, 79, 157, 105);
        if (detail) {
            line(52, 12, 110, 88);
            line(135, 10, 80, 87);
            p.drawEllipse(QPointF(52, 12), 4, 4);
            p.drawEllipse(QPointF(135, 10), 4, 4);
        }
    }
    return image;
}
void InstrumentArtwork::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), design::paper);
    auto image = instrumentDrawing(family_, size() - QSize(12, 12), true, devicePixelRatioF());
    p.drawPixmap(6, 6, image);
    p.setPen(design::grid);
    p.drawLine(0, height() - 1, width(), height() - 1);
}
} // namespace reverie
