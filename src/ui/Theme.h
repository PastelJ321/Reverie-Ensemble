// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 QuadraJ and contributors.
#pragma once
#include <QIcon>
#include <QString>
class QWidget;
namespace reverie {
QString assetPath();
QString soundPackPath();
QString factoryPackPath();
void loadUiLanguage();
QString translateEngineMessage(const QString& original);
void initializeTheme();
QString themeStyle();
QIcon toolIcon(const QString& name);
} // namespace reverie
