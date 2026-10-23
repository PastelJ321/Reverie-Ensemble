// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.

#include "ui/Editor.h"
#include <QApplication>
#include <QDebug>
#include <QStyleFactory>
#include <QTimer>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Reverie Ensemble");
    QApplication::setApplicationVersion("0.1.0");
    QApplication::setOrganizationName("QuadraJ");
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    const auto args = app.arguments();
    reverie::Editor editor(!args.contains("--no-audio"));
    editor.show();
    if (args.contains("--smoke-test"))
        QTimer::singleShot(1800, &app, &QApplication::quit);
    int shot = args.indexOf("--screenshot");
    if (shot >= 0 && shot + 1 < args.size())
        QTimer::singleShot(700, &editor,
                           [&editor, path = args[shot + 1]] { editor.grab().save(path); });
    if (args.contains("--audio-smoke-test")) {
        QTimer::singleShot(200, &editor, [&] { editor.play(); });
        QTimer::singleShot(1600, &editor, [&] {
            qInfo() << "Audio probe: frame=" << editor.engine.position()
                    << "peak=" << editor.engine.masterPeak();
            app.exit(editor.engine.position() > 24000 && editor.engine.masterPeak() > 0 ? 0 : 3);
        });
    }
    for (int i = 1; i < argc; ++i)
        if (QString::fromLocal8Bit(argv[i]).endsWith(".reverie")) {
            editor.openProject(QString::fromLocal8Bit(argv[i]));
            break;
        }
    return app.exec();
}
