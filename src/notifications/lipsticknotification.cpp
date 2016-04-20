/***************************************************************************
**
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

#include "notificationmanager.h"
#include "lipsticknotification.h"

#include <QDBusArgument>
#include <QtDebug>

LipstickNotification::LipstickNotification(const QString &appName, uint replacesId, const QString &appIcon, const QString &summary, const QString &body, const QStringList &actions, const QVariantHash &hints, int expireTimeout, QObject *parent) :
    QObject(parent),
    m_appName(appName),
    m_replacesId(replacesId),
    m_appIcon(appIcon),
    m_summary(summary),
    m_body(body),
    m_actions(actions),
    m_hints(hints),
    m_expireTimeout(expireTimeout),
    m_priority(hints.value(NotificationManager::HINT_PRIORITY).toInt()),
    m_timestamp(hints.value(NotificationManager::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch())
{
    updateHintValues();
}

LipstickNotification::LipstickNotification(QObject *parent) :
    QObject(parent),
    m_replacesId(0),
    m_expireTimeout(-1),
    m_priority(0),
    m_timestamp(0)
{
}

LipstickNotification::LipstickNotification(const LipstickNotification &notification) :
    QObject(notification.parent()),
    m_appName(notification.m_appName),
    m_replacesId(notification.m_replacesId),
    m_appIcon(notification.m_appIcon),
    m_summary(notification.m_summary),
    m_body(notification.m_body),
    m_actions(notification.m_actions),
    m_hints(notification.m_hints),
    m_hintValues(notification.m_hintValues),
    m_expireTimeout(notification.m_expireTimeout),
    m_priority(notification.m_priority),
    m_timestamp(notification.m_timestamp)
{
}

QString LipstickNotification::appName() const
{
    return m_appName;
}

void LipstickNotification::setAppName(const QString &appName)
{
    m_appName = appName;
}

uint LipstickNotification::replacesId() const
{
    return m_replacesId;
}

QString LipstickNotification::appIcon() const
{
    return m_appIcon;
}

void LipstickNotification::setAppIcon(const QString &appIcon)
{
    m_appIcon = appIcon;
}

QString LipstickNotification::summary() const
{
    return m_summary;
}

void LipstickNotification::setSummary(const QString &summary)
{
    if (m_summary != summary) {
        m_summary = summary;
        emit summaryChanged();
    }
}

QString LipstickNotification::body() const
{
    return m_body;
}

void LipstickNotification::setBody(const QString &body)
{
    if (m_body != body) {
        m_body = body;
        emit bodyChanged();
    }
}

QStringList LipstickNotification::actions() const
{
    return m_actions;
}

void LipstickNotification::setActions(const QStringList &actions)
{
    m_actions = actions;
}

QVariantHash LipstickNotification::hints() const
{
    return m_hints;
}

QVariantMap LipstickNotification::hintValues() const
{
    return m_hintValues;
}

void LipstickNotification::setHints(const QVariantHash &hints)
{
    QString oldIcon = icon();
    quint64 oldTimestamp = m_timestamp;
    QString oldPreviewIcon = previewIcon();
    QString oldPreviewSummary = previewSummary();
    QString oldPreviewBody = previewBody();
    int oldUrgency = urgency();
    int oldItemCount = itemCount();
    int oldPriority = m_priority;
    QString oldCategory = category();

    m_hints = hints;
    updateHintValues();

    if (oldIcon != icon()) {
        emit iconChanged();
    }

    m_timestamp = m_hints.value(NotificationManager::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch();
    if (oldTimestamp != m_timestamp) {
        emit timestampChanged();
    }

    if (oldPreviewIcon != previewIcon()) {
        emit previewIconChanged();
    }

    if (oldPreviewSummary != previewSummary()) {
        emit previewSummaryChanged();
    }

    if (oldPreviewBody != previewBody()) {
        emit previewBodyChanged();
    }

    if (oldUrgency != urgency()) {
        emit urgencyChanged();
    }

    if (oldItemCount != itemCount()) {
        emit itemCountChanged();
    }

    m_priority = m_hints.value(NotificationManager::HINT_PRIORITY).toInt();
    if (oldPriority != m_priority) {
        emit priorityChanged();
    }

    if (oldCategory != category()) {
        emit categoryChanged();
    }

    emit hintsChanged();
}

int LipstickNotification::expireTimeout() const
{
    return m_expireTimeout;
}

void LipstickNotification::setExpireTimeout(int expireTimeout)
{
    m_expireTimeout = expireTimeout;
}

QString LipstickNotification::icon() const
{
    QString rv(m_hints.value(NotificationManager::HINT_ICON).toString());
    if (rv.isEmpty()) {
        rv = m_hints.value(NotificationManager::HINT_IMAGE_PATH).toString();
    }
    return rv;
}

QDateTime LipstickNotification::timestamp() const
{
    return QDateTime::fromMSecsSinceEpoch(m_timestamp);
}

QString LipstickNotification::previewIcon() const
{
    return m_hints.value(NotificationManager::HINT_PREVIEW_ICON).toString();
}

QString LipstickNotification::previewSummary() const
{
    return m_hints.value(NotificationManager::HINT_PREVIEW_SUMMARY).toString();
}

QString LipstickNotification::previewBody() const
{
    return m_hints.value(NotificationManager::HINT_PREVIEW_BODY).toString();
}

int LipstickNotification::urgency() const
{
    return m_hints.value(NotificationManager::HINT_URGENCY).toInt();
}

int LipstickNotification::itemCount() const
{
    return m_hints.value(NotificationManager::HINT_ITEM_COUNT).toInt();
}

int LipstickNotification::priority() const
{
    return m_priority;
}

QString LipstickNotification::category() const
{
    return m_hints.value(NotificationManager::HINT_CATEGORY).toString();
}

bool LipstickNotification::isUserRemovable() const
{
    return m_hints.value(NotificationManager::HINT_USER_REMOVABLE, QVariant(true)).toBool();
}

bool LipstickNotification::hidden() const
{
    return m_hints.value(NotificationManager::HINT_HIDDEN, QVariant(false)).toBool();
}

QVariantList LipstickNotification::remoteActions() const
{
    QVariantList rv;

    QStringList::const_iterator it = m_actions.constBegin(), end = m_actions.constEnd();
    while (it != end) {
        const QString name(*it);
        QString displayName;
        if (++it != end) {
            displayName = *it;
            ++it;
        }

        const QString hint(m_hints.value(NotificationManager::HINT_REMOTE_ACTION_PREFIX + name).toString());
        if (!hint.isEmpty()) {
            const QString icon(m_hints.value(NotificationManager::HINT_REMOTE_ACTION_ICON_PREFIX + name).toString());

            QVariantMap vm;
            vm.insert(QStringLiteral("name"), name);
            if (!displayName.isEmpty()) {
                vm.insert(QStringLiteral("displayName"), displayName);
            }
            if (!icon.isEmpty()) {
                vm.insert(QStringLiteral("icon"), icon);
            }

            // Extract the element of the DBus call
            QStringList elements(hint.split(' ', QString::SkipEmptyParts));
            if (elements.size() <= 3) {
                qWarning() << "Unable to decode invalid remote action:" << hint;
            } else {
                int index = 0;
                vm.insert(QStringLiteral("service"), elements.at(index++));
                vm.insert(QStringLiteral("path"), elements.at(index++));
                vm.insert(QStringLiteral("iface"), elements.at(index++));
                vm.insert(QStringLiteral("method"), elements.at(index++));

                QVariantList args;
                while (index < elements.size()) {
                    const QString &arg(elements.at(index++));
                    const QByteArray buffer(QByteArray::fromBase64(arg.toUtf8()));

                    QDataStream stream(buffer);
                    QVariant var;
                    stream >> var;
                    args.append(var);
                }
                vm.insert(QStringLiteral("arguments"), args);
            }

            rv.append(vm);
        }
    }

    return rv;
}

QString LipstickNotification::origin() const
{
    return m_hints.value(NotificationManager::HINT_ORIGIN).toString();
}

QString LipstickNotification::owner() const
{
    return m_hints.value(NotificationManager::HINT_OWNER).toString();
}

int LipstickNotification::maxContentLines() const
{
    return m_hints.value(NotificationManager::HINT_MAX_CONTENT_LINES).toInt();
}

bool LipstickNotification::restored() const
{
    return m_hints.value(NotificationManager::HINT_RESTORED).toBool();
}

quint64 LipstickNotification::internalTimestamp() const
{
    return m_timestamp;
}

void LipstickNotification::updateHintValues()
{
    m_hintValues.clear();

    QVariantHash::const_iterator it = m_hints.constBegin(), end = m_hints.constEnd();
    for ( ; it != end; ++it) {
        // Filter out the hints that are represented by other properties
        const QString &hint(it.key());
        if (hint.compare(NotificationManager::HINT_ICON, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_IMAGE_PATH, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_TIMESTAMP, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_PREVIEW_ICON, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_PREVIEW_SUMMARY, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_PREVIEW_BODY, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_URGENCY, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_ITEM_COUNT, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_PRIORITY, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_CATEGORY, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_USER_REMOVABLE, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_HIDDEN, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_ORIGIN, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_OWNER, Qt::CaseInsensitive) != 0 &&
            hint.compare(NotificationManager::HINT_MAX_CONTENT_LINES, Qt::CaseInsensitive) != 0 &&
            !hint.startsWith(NotificationManager::HINT_REMOTE_ACTION_PREFIX, Qt::CaseInsensitive) &&
            !hint.startsWith(NotificationManager::HINT_REMOTE_ACTION_ICON_PREFIX, Qt::CaseInsensitive)) {
            m_hintValues.insert(hint, it.value());
        }
    }
}

QDBusArgument &operator<<(QDBusArgument &argument, const LipstickNotification &notification)
{
    argument.beginStructure();
    argument << notification.m_appName;
    argument << notification.m_replacesId;
    argument << notification.m_appIcon;
    argument << notification.m_summary;
    argument << notification.m_body;
    argument << notification.m_actions;
    argument << notification.m_hints;
    argument << notification.m_expireTimeout;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, LipstickNotification &notification)
{
    argument.beginStructure();
    argument >> notification.m_appName;
    argument >> notification.m_replacesId;
    argument >> notification.m_appIcon;
    argument >> notification.m_summary;
    argument >> notification.m_body;
    argument >> notification.m_actions;
    argument >> notification.m_hints;
    argument >> notification.m_expireTimeout;
    argument.endStructure();

    notification.m_priority = notification.m_hints.value(NotificationManager::HINT_PRIORITY).toInt();
    notification.m_timestamp = notification.m_hints.value(NotificationManager::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch();
    notification.updateHintValues();

    return argument;
}

namespace {

int comparePriority(const LipstickNotification &lhs, const LipstickNotification &rhs)
{
    const int lhsPriority(lhs.priority()), rhsPriority(rhs.priority());
    if (lhsPriority < rhsPriority) {
        return -1;
    }
    if (rhsPriority < lhsPriority) {
        return 1;
    }
    return 0;
}

int compareTimestamp(const LipstickNotification &lhs, const LipstickNotification &rhs)
{
    const quint64 lhsTimestamp(lhs.internalTimestamp()), rhsTimestamp(rhs.internalTimestamp());
    if (lhsTimestamp < rhsTimestamp) {
        return -1;
    }
    if (rhsTimestamp < lhsTimestamp) {
        return 1;
    }
    return 0;
}

}

bool operator<(const LipstickNotification &lhs, const LipstickNotification &rhs)
{
    int priorityComparison(comparePriority(lhs, rhs));
    if (priorityComparison > 0) {
        // Higher priority notifications sort first
        return true;
    } else if (priorityComparison == 0) {
        int timestampComparison(compareTimestamp(lhs, rhs));
        if (timestampComparison > 0) {
            // Later notifications sort first
            return true;
        } else if (timestampComparison == 0) {
            // For matching timestamps, sort the higher ID first
            if (lhs.replacesId() > rhs.replacesId()) {
                return true;
            }
        }
    }
    return false;
}

NotificationList::NotificationList()
{
}

NotificationList::NotificationList(const QList<LipstickNotification *> &notificationList) :
    m_notificationList(notificationList)
{
}

NotificationList::NotificationList(const NotificationList &notificationList) :
    m_notificationList(notificationList.m_notificationList)
{
}

QList<LipstickNotification *> NotificationList::notifications() const
{
    return m_notificationList;
}

QDBusArgument &operator<<(QDBusArgument &argument, const NotificationList &notificationList)
{
    argument.beginArray(qMetaTypeId<LipstickNotification>());
    foreach (LipstickNotification *notification, notificationList.m_notificationList) {
        argument << *notification;
    }
    argument.endArray();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, NotificationList &notificationList)
{
    argument.beginArray();
    notificationList.m_notificationList.clear();
    while (!argument.atEnd()) {
        LipstickNotification *notification = new LipstickNotification;
        argument >> *notification;
        notificationList.m_notificationList.append(notification);
    }
    argument.endArray();
    return argument;
}
