/***************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include "connectivitymonitor.h"

#include "connmanmanagerproxy.h"
#include "connmanserviceproxy.h"

#include <QDBusMessage>
#include <QDBusConnection>

ConnectivityMonitor::ConnectivityMonitor(QObject *parent)
    : QObject(parent)
    , connmanManager_(new ConnmanManagerProxy("net.connman", "/", QDBusConnection::systemBus(), this))
{
    qDBusRegisterMetaType<PathProperties>();
    qDBusRegisterMetaType<PathPropertiesArray>();

    connect(connmanManager_, &ConnmanManagerProxy::ServicesChanged,
            this, [this](const PathPropertiesArray &services, const QList<QDBusObjectPath> &removedPaths) {
        const QList<QString> original(activeConnectionTypes());

        foreach (const QDBusObjectPath &objectPath, removedPaths) {
            const QString path(objectPath.path());
            serviceInactive(path);
            forgetService(path);
        }

        for (const PathProperties &service : services) {
            const QString &path(service.first.path());
            const QVariantMap &properties(service.second);

            const QString type(properties[QStringLiteral("Type")].toString());
            if (type == QStringLiteral("cellular") || type == QStringLiteral("wifi")) {
                const QString state(properties[QStringLiteral("State")].toString());
                if (state == QStringLiteral("online")) {
                    serviceActive(type, path);
                } else {
                    serviceInactive(path);
                }

                monitorService(type, path);
            }
        }

        if (activeConnectionTypes() != original) {
            emit connectivityChanged(activeConnectionTypes());
        }
    });

    fetchServiceList();
}

ConnectivityMonitor::~ConnectivityMonitor()
{
}

QList<QString> ConnectivityMonitor::activeConnectionTypes() const
{
    return activeServices_.keys().toSet().toList();
}

void ConnectivityMonitor::fetchServiceList()
{
    QDBusPendingCall call = connmanManager_->GetServices();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<PathPropertiesArray> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qWarning() << "Unable to fetch Connman services:" << reply.error().message();
        } else {
            const PathPropertiesArray &services(reply.value());

            for (const PathProperties &service : services) {
                const QString &path(service.first.path());
                const QVariantMap &properties(service.second);

                const QString type(properties[QStringLiteral("Type")].toString());
                if (type == QStringLiteral("cellular") || type == QStringLiteral("wifi")) {
                    const QString state(properties[QStringLiteral("State")].toString());
                    if (state == QStringLiteral("online")) {
                        const QString name(properties[QStringLiteral("Name")].toString());
                        serviceActive(type, path);
                    }

                    monitorService(type, path);
                }
            }

            if (!activeServices_.isEmpty()) {
                emit connectivityChanged(activeConnectionTypes());
            }
        }
    });
}

void ConnectivityMonitor::monitorService(const QString &type, const QString &path)
{
    auto it = connmanServices_.find(path);
    if (it == connmanServices_.end()) {
        ConnmanServiceProxy *proxy = new ConnmanServiceProxy("net.connman", path, QDBusConnection::systemBus(), nullptr);
        connmanServices_.insert(path, proxy);

        connect(proxy, &ConnmanServiceProxy::PropertyChanged, this, [this, type, path](const QString &name, const QDBusVariant &value) {
            if (name == QStringLiteral("State")) {
                const QList<QString> original(activeConnectionTypes());

                const QString state = value.variant().toString();
                if (state == QStringLiteral("online")) {
                    serviceActive(type, path);
                } else {
                    serviceInactive(path);
                }

                if (activeConnectionTypes() != original) {
                    emit connectivityChanged(activeConnectionTypes());
                }
            }
        });
    }
}

void ConnectivityMonitor::forgetService(const QString &path)
{
    auto it = connmanServices_.find(path);
    if (it != connmanServices_.end()) {
        ConnmanServiceProxy *proxy = (*it);
        delete proxy;

        connmanServices_.erase(it);
    }
}

void ConnectivityMonitor::serviceActive(const QString &type, const QString &path)
{
    QList<QString> &services(activeServices_[type]);
    if (!services.contains(path)) {
        services.append(path);
    }
}

void ConnectivityMonitor::serviceInactive(const QString &path)
{
    for (auto ait = activeServices_.begin(); ait != activeServices_.end(); ) {
        QList<QString> &services(ait.value());
        if ((services.removeAll(path) > 0) && services.isEmpty()) {
            ait = activeServices_.erase(ait);
        } else {
            ++ait;
        }
    }
}

