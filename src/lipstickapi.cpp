/***************************************************************************
**
** Copyright (c) 2013-2014 Jolla Ltd.
**
** This file is part of lipstick.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "lipstickapi.h"

#include "homeapplication.h"
#include "compositor/lipstickcompositor.h"
#include "notifications/notificationmanager.h"

LipstickApi::LipstickApi(QObject *parent)
    : QObject(parent)
{
    HomeApplication *homeApp = HomeApplication::instance();
    if (homeApp) {
        QObject::connect(homeApp, SIGNAL(homeActiveChanged()), this, SIGNAL(activeChanged()));
    }
}

bool LipstickApi::active() const
{
    return HomeApplication::instance()->homeActive();
}

QObject *LipstickApi::compositor() const
{
    return LipstickCompositor::instance();
}

ScreenshotResult *LipstickApi::takeScreenshot(const QString &path)
{
    return ScreenshotService::saveScreenshot(path);
}

QString LipstickApi::notificationSystemApplicationName() const
{
    return NotificationManager::instance(false)->systemApplicationName();
}
