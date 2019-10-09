/***************************************************************************
**
** Copyright (c) 2016 - 2019 Jolla Ltd.
** Copyright (c) 2019 Open Mobile Platform LLC.
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

#include "settingsvpnmodel.h"

#include "lipstickglobal.h"

class HomeWindow;

class LIPSTICK_EXPORT VpnAgent : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(bool windowVisible READ windowVisible WRITE setWindowVisible NOTIFY windowVisibleChanged)

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

    /*!
     * Returns whether the window is visible or not.
     *
     * \return \c true if the window is visible, \c false otherwise
     */
    bool windowVisible() const;

    /*!
     * Sets the visibility of the window.
     *
     * \param visible \c true if the window should be visible, \c false otherwise
     */
    void setWindowVisible(bool visible);

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
    void createWindow();

signals:
    //! Sent when the visibility of the window has changed.
    void windowVisibleChanged();

private:
    HomeWindow *m_window;
    SettingsVpnModel *m_connections;

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
