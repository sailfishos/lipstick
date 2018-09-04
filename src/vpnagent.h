/***************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Matt Vogt <matthew.vogt@jollamobile.com>
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

#ifndef VPNAGENT_H
#define VPNAGENT_H

#include <QObject>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QList>

#include "vpnmodel.h"

#include <lipstickglobal.h>
#include <lipstickwindow.h>

class LIPSTICK_EXPORT VpnAgent : public LipstickWindow, protected QDBusContext
{
    Q_OBJECT

public:
    /*!
     * Creates a connection selector.
     *
     * \param parent the parent object
     */
    explicit VpnAgent(QObject *parent = 0);

    /*!
     * Destroys the connection selector.
     */
    virtual ~VpnAgent();

signals:
    void inputRequested(const QString &path, const QVariantMap &details);
    void requestCanceled(const QString &path);
    void errorReported(const QString &path, const QString &message);

public slots:
    void respond(const QString &path, const QVariantMap &response);
    void decline(const QString &path);

    void Release();
    void ReportError(const QDBusObjectPath &path, const QString &message);
    QVariantMap RequestInput(const QDBusObjectPath &path, const QVariantMap &details);
    void Cancel();

private slots:
    /*!
     * Creates the window.
     */
    void createWindow() override;

private:
    VpnModel *m_connections;

    struct Request {
        Request(const QString &path, const QVariantMap &details, const QDBusMessage &request);

        QString path;
        QVariantMap details;
        QDBusMessage reply;
        QDBusMessage cancelReply;
    };

    QList<Request> m_pending;
};

#endif
