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
#ifndef PRIVILEGEDDBUSCONTEXT_H
#define PRIVILEGEDDBUSCONTEXT_H

#include <QDBusContext>
#include "lipstickglobal.h"

class LIPSTICK_EXPORT PrivilegedDBusContext : public QDBusContext
{
protected:
    /*!
     * Identifies whether the caller of a dbus function belongs to the privileged group.
     *
     * \return true if the caller belongs to the privileged group.
     */
    bool isPrivileged();

    /*!
     * Identifies the caller of a dbus function.
     *
     * \return The PID of the calling process.
     */
    uint callerProcessId() const;
};

#endif
