// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#include "ui/Theme.h"
#include "ui/DesignTokens.h"
#include <QApplication>
#include <QDir>
#include <QFontDatabase>
#include <QPainter>
#include <QPainterPath>
namespace reverie {
QString assetPath() {
    auto bundled = QCoreApplication::applicationDirPath() + "/../Resources/assets";
    if (QDir(bundled).exists())
        return QDir(bundled).absolutePath();
    return QString::fromUtf8(REVERIE_ASSET_DIR);
}
void initializeTheme() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    for (const auto& name : {"IBMPlexSansKR-Regular.ttf", "IBMPlexSansKR-Medium.ttf",
                             "IBMPlexSansKR-SemiBold.ttf", "IBMPlexMono-Regular.ttf"})
        QFontDatabase::addApplicationFont(assetPath() + "/fonts/" + name);
    QFont font("IBM Plex Sans KR");
    if (!QFontDatabase::families().contains(font.family()))
        font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPixelSize(12);
    QApplication::setFont(font);
    QApplication::setStyle("Fusion");
}
QString themeStyle() {
    QString style = R"(
QWidget{background:@shell;color:@text;font-size:12px;} QMainWindow{background:@shell;}
QToolBar{background:@surface;border:0;border-bottom:1px solid @border;spacing:5px;padding:4px;}
QPushButton,QToolButton{background:@control;border:1px solid @border;border-radius:2px;padding:4px 7px;min-height:18px;}
QPushButton:hover,QToolButton:hover{background:#5A534A;} QPushButton:pressed,QToolButton:pressed{background:@shell;}
QPushButton:checked,QToolButton:checked{background:#665342;border:1px solid @selection;}
QPushButton:focus,QToolButton:focus,QLineEdit:focus,QDoubleSpinBox:focus,QComboBox:focus{border:1px solid @text;}
QWidget:disabled{color:#8A8175;} QPushButton:disabled{background:@surface;border-color:@control;}
QSpinBox,QDoubleSpinBox,QComboBox,QLineEdit{background:@shell;border:1px solid @border;border-radius:2px;padding:3px;min-height:20px;selection-background-color:@selection;selection-color:@ink;}
QSpinBox,QDoubleSpinBox{font-family:"IBM Plex Mono";}
QDoubleSpinBox::up-button,QDoubleSpinBox::down-button{width:12px;} QDockWidget::title{background:@surface;padding:7px;border-bottom:1px solid @border;}
QListWidget,QTreeWidget{background:@surface;border:0;outline:0;} QListWidget::item{padding:7px 5px;border-bottom:1px solid @control;}
QListWidget::item:selected,QTreeWidget::item:selected{background:#665342;color:@text;} QListWidget::item:hover{background:@control;}
QMenuBar,QMenu,QStatusBar{background:@shell;} QMenu::item:selected{background:@control;}
QScrollArea{border:0;} QScrollBar{background:@surface;width:9px;height:9px;} QScrollBar::handle{background:@border;min-width:24px;min-height:24px;border-radius:2px;}
QScrollBar::add-line,QScrollBar::sub-line{width:0;height:0;} QSplitter::handle{background:@border;height:3px;width:3px;}
QCheckBox{spacing:5px;} QTabBar::tab{background:@surface;padding:7px 12px;border-bottom:2px solid @surface;} QTabBar::tab:selected{border-bottom:2px solid @selection;}
QSlider::groove:vertical{background:@shell;width:5px;} QSlider::handle:vertical{background:@secondary;border:1px solid @border;height:16px;margin:0 -8px;border-radius:2px;}
QProgressDialog,QMessageBox,QInputDialog{background:@surface;} QToolTip{background:@control;color:@text;border:1px solid @border;padding:4px;}
 )";
    for (const auto& entry :
         std::vector<std::pair<QString, QColor>>{{"@shell", design::shell},
                                                 {"@surface", design::surface},
                                                 {"@control", design::control},
                                                 {"@border", design::border},
                                                 {"@text", design::text},
                                                 {"@secondary", design::secondary},
                                                 {"@paper", design::paper},
                                                 {"@ink", design::ink},
                                                 {"@mutedInk", design::mutedInk},
                                                 {"@grid", design::grid},
                                                 {"@selection", design::selection},
                                                 {"@signal", design::signal},
                                                 {"@record", design::record},
                                                 {"@error", design::error}})
        style.replace(entry.first, entry.second.name());
    return style;
}

QIcon toolIcon(const QString& name) {
    QPixmap pm(40, 40);
    pm.setDevicePixelRatio(2);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(design::text, 1.5));
    p.setBrush(design::text);
    if (name == "play") {
        QPainterPath path;
        path.moveTo(6, 4);
        path.lineTo(16, 10);
        path.lineTo(6, 16);
        path.closeSubpath();
        p.drawPath(path);
    } else if (name == "stop")
        p.drawRect(5, 5, 10, 10);
    else if (name == "record") {
        p.setBrush(design::record);
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, 12, 12);
    } else if (name == "pause") {
        p.drawRect(5, 4, 3, 12);
        p.drawRect(12, 4, 3, 12);
    } else if (name == "loop") {
        p.setBrush(Qt::NoBrush);
        p.drawArc(3, 3, 14, 14, 30 * 16, 280 * 16);
        p.drawLine(15, 4, 16, 9);
        p.drawLine(16, 9, 11, 8);
    } else if (name == "metronome") {
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(QPolygonF({{4, 17}, {8, 3}, {12, 3}, {16, 17}}));
        p.drawLine(10, 13, 15, 4);
    } else {
        p.setBrush(Qt::NoBrush);
        p.drawRect(4, 3, 12, 14);
        p.drawLine(7, 7, 13, 7);
        p.drawLine(7, 11, 13, 11);
    }
    return QIcon(pm);
}
} // namespace reverie
