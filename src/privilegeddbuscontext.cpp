/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012 Jolla Ltd.
** Contact: Robin Burchell <robin.burchell@jollamobile.com>
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

#include "privilegeddbuscontext.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QFileInfo>

uint PrivilegedDBusContext::callerProcessId() const
{
    if (calledFromDBus()) {
        return connection().interface()->servicePid(message().service()).value();
    } else {
        return QCoreApplication::applicationPid();
    }
}

bool PrivilegedDBusContext::isPrivileged()
{
    if (!calledFromDBus()) {
        // Local function calls are always privileged
        return true;
    }

    // Get the PID of the calling process
    pid_t pid = connection().interface()->servicePid(message().service());

    // The /proc/<pid> directory is owned by EUID:EGID of the process
    QFileInfo info(QStringLiteral("/proc/%1").arg(pid));
    if (info.group() != QLatin1String("privileged") && info.owner() != QLatin1String("root")) {
        sendErrorReply(QDBusError::AccessDenied,
                QStringLiteral("PID %1 is not in privileged group").arg(pid));
        return false;
    }

    return true;
}
