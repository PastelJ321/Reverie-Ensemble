// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QIcon>
#include <QString>
class QWidget;
namespace reverie {
QString assetPath();
QString soundPackPath();
void initializeTheme();
QString themeStyle();
QIcon toolIcon(const QString& name);
} // namespace reverie
