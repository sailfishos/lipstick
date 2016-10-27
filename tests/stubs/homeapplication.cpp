/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Aaron Kennedy <aaron.kennedy@jollamobile.com>
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

#include "homeapplication.h"

HomeApplication::HomeApplication(int &argc, char **argv, const QString &qmlPath)
    : QGuiApplication(argc, argv)
{
    Q_UNUSED(qmlPath);
}

bool HomeApplication::event(QEvent *)
{
    qFatal("Not implemented");
    return true;
}

void HomeApplication::sendHomeReadySignalIfNotAlreadySent()
{
}

void HomeApplication::sendStartupNotifications()
{
}

void HomeApplication::connectFrameSwappedSignal(bool)
{
}

void HomeApplication::registerVpnAgent(const QString &)
{
}

void HomeApplication::unregisterVpnAgent(const QString &)
{
}
