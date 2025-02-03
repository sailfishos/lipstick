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

#include "vpnagent.h"

#include <QGuiApplication>
#include "homewindow.h"
#include <QQuickItem>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>
#include <QDBusArgument>
#include <QDBusConnection>
#include "utilities/closeeventeater.h"
#include "lipstickqmlpath.h"

VpnAgent::VpnAgent(QObject *parent) :
    QObject(parent),
    m_window(0),
    m_connections(new SettingsVpnModel(this))
{
    QTimer::singleShot(0, this, SLOT(createWindow()));
}

VpnAgent::~VpnAgent()
{
    delete m_window;
}

void VpnAgent::createWindow()
{
    m_window = new HomeWindow();
    m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
    m_window->setCategory(QLatin1String("dialog"));
    m_window->setWindowTitle("VPN Agent");
    m_window->setContextProperty("vpnAgent", this);
    m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
    m_window->setSource(QmlPath::to("connectivity/VpnAgent.qml"));
    m_window->installEventFilter(new CloseEventEater(this));
}

void VpnAgent::setWindowVisible(bool visible)
{
    if (visible) {
        if (!m_window->isVisible()) {
            m_window->showFullScreen();
            emit windowVisibleChanged();
        }
    } else if (m_window != 0 && m_window->isVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}

bool VpnAgent::windowVisible() const
{
    return m_window != 0 && m_window->isVisible();
}

void VpnAgent::respond(const QString &path, const QVariantMap &details)
{
    bool storeCredentials = false;
    bool keepCredentials = false;

    // Marshall our response
    QVariantMap response;
    for (QVariantMap::const_iterator it = details.cbegin(), end = details.cend(); it != end; ++it) {
        const QString name(it.key());
        const QVariantMap &field(it.value().value<QVariantMap>());
        const QVariant fieldValue = field.value(QStringLiteral("Value"));
        const QString fieldRequirement = field.value(QStringLiteral("Requirement")).toString();
        const QString fieldType = field.value(QStringLiteral("Type")).toString();
        if ((name == QStringLiteral("storeCredentials"))
                && (fieldRequirement == QStringLiteral("control"))
                && (fieldType == QStringLiteral("boolean"))) {
            storeCredentials = fieldValue.toBool();
        } else if ((name == QStringLiteral("keepCredentials"))
                && (fieldRequirement == QStringLiteral("control"))
                && (fieldType == QStringLiteral("boolean"))) {
            keepCredentials = fieldValue.toBool();
        } else {
            if (fieldRequirement == QStringLiteral("mandatory")
                    || (fieldRequirement != QStringLiteral("informational")
                        && fieldRequirement != QStringLiteral("control")
                        && fieldValue.isValid())) {
                response.insert(it.key(), fieldValue);
            }
        }
    }

    if (storeCredentials) {
        m_connections->setConnectionCredentials(path, response);
    } else if (!keepCredentials) { // Clearing explicitly disabled
        m_connections->disableConnectionCredentials(path);
    }

    QList<Request>::iterator it = m_pending.begin(), end = m_pending.end();
    for ( ; it != end; ++it) {
        Request &request(*it);
        if (request.path == path) {
            request.reply << response;
            if (!QDBusConnection::systemBus().send(request.reply)) {
                qWarning() << "Unable to transmit response for:" << path;
            }
            m_pending.erase(it);

            if (!m_pending.isEmpty()) {
                const Request &nextRequest(m_pending.front());
                emit inputRequested(nextRequest.path, nextRequest.details);
            }
            return;
        }
    }

    qWarning() << "Unable to send response for:" << path;
}

void VpnAgent::decline(const QString &path)
{
    QList<Request>::iterator it = m_pending.begin(), end = m_pending.end();
    for ( ; it != end; ++it) {
        Request &request(*it);
        if (request.path == path) {
            if (!QDBusConnection::systemBus().send(request.cancelReply)) {
                qWarning() << "Unable to transmit cancel for:" << path;
            }
            m_pending.erase(it);

            if (!m_pending.isEmpty()) {
                const Request &nextRequest(m_pending.front());
                emit inputRequested(nextRequest.path, nextRequest.details);
            }
            return;
        }
    }

    qWarning() << "Unable to cancel pending request for:" << path;
}

void VpnAgent::Release()
{
}

void VpnAgent::ReportError(const QDBusObjectPath &path, const QString &message)
{
    emit errorReported(path.path(), message);
}

namespace {

template<typename T>
T extract(const QDBusArgument &arg)
{
    T rv;
    arg >> rv;
    return rv;
}

// Extracts a boolean value and removes it from the map
bool ExtractRequestBool(QVariantMap &extracted, const QString &key, bool defaultValue) {
    bool result = defaultValue;
    QVariantMap::iterator it = extracted.find(key);
    while ((it != extracted.end()) && (it.key() == key)) {
        const QVariantMap field(it.value().value<QVariantMap>());
        const QString type = field.value(QStringLiteral("Type")).toString();
        const QString requirement = field.value(QStringLiteral("Requirement")).toString();

        if ((type == QStringLiteral("boolean")) && (requirement == QStringLiteral("control"))) {
            result = field.value(QStringLiteral("Value")).toBool();
            it = extracted.erase(it);
            break;
        }
        it++;
    }
    return result;
}

void AddMissingAttributes(QVariantMap &field) {
    if (!field.contains(QStringLiteral("Requirement"))) {
        field.insert(QStringLiteral("Requirement"), QVariant(QStringLiteral("optional")));
    }
    if (!field.contains(QStringLiteral("Type"))) {
        field.insert(QStringLiteral("Type"), QVariant(QStringLiteral("string")));
    }
    if (!field.contains(QStringLiteral("Value"))) {
        const QString fieldType = field.value(QStringLiteral("Type")).toString();
        if (fieldType == "boolean") {
            field.insert(QStringLiteral("Value"), false);
        }
        else {
            field.insert(QStringLiteral("Value"), QStringLiteral(""));
        }
    }
}

}

QVariantMap VpnAgent::RequestInput(const QDBusObjectPath &path, const QVariantMap &details)
{
    // Extract the details from DBus marshalling
    QVariantMap extracted(details);
    for (QVariantMap::iterator it = extracted.begin(), end = extracted.end(); it != end; ++it) {
        QVariantMap field = extract<QVariantMap>(it.value().value<QDBusArgument>());
        AddMissingAttributes(field);
        *it = QVariant::fromValue(field);
    }

    const bool allowCredentialStorage(ExtractRequestBool(extracted, "AllowStoreCredentials", true));
    const bool allowCredentialRetrieval(ExtractRequestBool(extracted, "AllowRetrieveCredentials", true));
    /*
     * When the second request is something else that is to be kept in memory
     * only and is not for VPN agent to keep, the requester can set this flag
     * to avoid the credentials from being cleared out. By default this is false
     * and in such case is not saved.
     */
    const bool keepCredentials(ExtractRequestBool(extracted, "KeepCredentials", false));

    // Can we supply the requested data from stored credentials?
    const QString objectPath(path.path());
    const bool storeCredentials(m_connections->connectionCredentialsEnabled(objectPath));
    if (storeCredentials && allowCredentialRetrieval) {
        const QVariantMap credentials(m_connections->connectionCredentials(objectPath));

        bool satisfied(true);
        QVariantMap response;

        QList<QPair<QVariantMap::iterator, QString> > storedValues;
        QVariantMap::iterator failureIt = extracted.end();

        for (QVariantMap::iterator it = extracted.begin(), end = extracted.end(); it != end; ++it) {
            QVariantMap field(it.value().value<QVariantMap>());

            const QString &name(it.key());
            const QString fieldRequirement = field.value(QStringLiteral("Requirement")).toString();
            const bool mandatory(fieldRequirement == QStringLiteral("mandatory"));
            if (fieldRequirement != QStringLiteral("informational")) {
                auto cit = credentials.find(name);
                const QString storedValue(cit == credentials.end() ? QString() : cit->toString());
                if (storedValue.isEmpty()) {
                    if (mandatory) {
                        satisfied = false;
                    }
                } else {
                    response.insert(name, storedValue);
                    storedValues.append(qMakePair(it, storedValue));
                }
            } else {
                if (name == "VpnAgent.AuthFailure") {
                    // Our previous attempt failed, do not use the stored values
                    m_connections->setConnectionCredentials(objectPath, QVariantMap());
                    failureIt = it;
                    break;
                }
            }
        }

        if (failureIt != extracted.end() && !keepCredentials) {
            // Hide this property from the user agent
            extracted.erase(failureIt);
        } else {
            if (satisfied) {
                // We can respond immediately
                return response;
            }

            // Store the values we previously held
            for (auto vit = storedValues.cbegin(), vend = storedValues.cend(); vit != vend; ++vit) {
                QVariantMap::iterator it = vit->first;
                QString storedValue = vit->second;

                QVariantMap field(it.value().value<QVariantMap>());
                field.insert(QStringLiteral("Value"), QVariant::fromValue(storedValue));
                *it = QVariant::fromValue(field);
            }
        }
    }

    if (allowCredentialStorage) {
        QVariantMap field;
        field.insert(QStringLiteral("Requirement"), QVariant::fromValue(QStringLiteral("control")));
        field.insert(QStringLiteral("Type"), QVariant::fromValue(QStringLiteral("boolean")));
        field.insert(QStringLiteral("Value"), QVariant::fromValue(storeCredentials));
        extracted.insert(QStringLiteral("storeCredentials"), field);
    }

    /*
     * By default this is false if not set. This value needs to be explicitely
     * set to retain the credentials although storing and retrieval is disabled.
     * This is used with Private Key passwords, which are stored in memory but
     * not in VPN agent and the actual credentials are required to be stored.
     */
    if (keepCredentials) {
        QVariantMap field;
        field.insert(QStringLiteral("Requirement"), QVariant::fromValue(QStringLiteral("control")));
        field.insert(QStringLiteral("Type"), QVariant::fromValue(QStringLiteral("boolean")));
        field.insert(QStringLiteral("Value"), QVariant::fromValue(keepCredentials));
        extracted.insert(QStringLiteral("keepCredentials"), field);
    }

    // Inform the caller that the reponse will be asynchronous
    QDBusContext::setDelayedReply(true);

    m_pending.append(Request(objectPath, extracted, QDBusContext::message()));
    Request &newRequest(m_pending.back());

    if (m_pending.size() == 1) {
        // No request was previously in progress
        emit inputRequested(newRequest.path, newRequest.details);
    }

    return QVariantMap();
}

void VpnAgent::Cancel()
{
    if (!m_pending.isEmpty()) {
        Request canceledRequest(m_pending.takeFirst());
        emit requestCanceled(canceledRequest.path);

        if (!m_pending.isEmpty()) {
            const Request &nextRequest(m_pending.front());
            emit inputRequested(nextRequest.path, nextRequest.details);
        }
    }
}

VpnAgent::Request::Request(const QString &path, const QVariantMap &details, const QDBusMessage &request)
    : path(path)
    , details(details)
    , reply(request.createReply())
    , cancelReply(request.createErrorReply(QStringLiteral("net.connman.vpn.Agent.Error.Canceled"), QString()))
{
}

