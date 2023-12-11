
/***************************************************************************
**
** Copyright (c) 2012 - 2019 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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
#include <QDataStream>
#include <QtDebug>

namespace {
// deprecated
const char *HINT_ICON = "x-nemo-icon";
const char *HINT_PREVIEW_ICON = "x-nemo-preview-icon";

const char *INTERNAL_HINT_PRIVILEGED = "privileged";
}

const char *LipstickNotification::HINT_URGENCY = "urgency";
const char *LipstickNotification::HINT_CATEGORY = "category";
const char *LipstickNotification::HINT_TRANSIENT = "transient";
const char *LipstickNotification::HINT_RESIDENT = "resident";
const char *LipstickNotification::HINT_IMAGE_PATH = "image-path";
const char *LipstickNotification::HINT_IMAGE_DATA = "image-data";
const char *LipstickNotification::HINT_SUPPRESS_SOUND = "suppress-sound";
const char *LipstickNotification::HINT_SOUND_FILE = "sound-file";
const char *LipstickNotification::HINT_SOUND_NAME = "sound-name";
const char *LipstickNotification::HINT_ITEM_COUNT = "x-nemo-item-count";
const char *LipstickNotification::HINT_PRIORITY = "x-nemo-priority";
const char *LipstickNotification::HINT_TIMESTAMP = "x-nemo-timestamp";
const char *LipstickNotification::HINT_PREVIEW_BODY = "x-nemo-preview-body";
const char *LipstickNotification::HINT_PREVIEW_SUMMARY = "x-nemo-preview-summary";
const char *LipstickNotification::HINT_SUB_TEXT = "x-nemo-sub-text";
const char *LipstickNotification::HINT_REMOTE_ACTION_PREFIX = "x-nemo-remote-action-";
const char *LipstickNotification::HINT_REMOTE_ACTION_ICON_PREFIX = "x-nemo-remote-action-icon-";
const char *LipstickNotification::HINT_REMOTE_ACTION_TYPE_PREFIX = "x-nemo-remote-action-type-";
const char *LipstickNotification::HINT_USER_REMOVABLE = "x-nemo-user-removable";
const char *LipstickNotification::HINT_FEEDBACK = "x-nemo-feedback";
const char *LipstickNotification::HINT_DISPLAY_ON = "x-nemo-display-on";
const char *LipstickNotification::HINT_ORIGIN_PACKAGE = "x-nemo-origin-package";
const char *LipstickNotification::HINT_OWNER = "x-nemo-owner";
const char *LipstickNotification::HINT_PROGRESS = "x-nemo-progress";
const char *LipstickNotification::HINT_VIBRA = "x-nemo-vibrate";
const char *LipstickNotification::HINT_VISIBILITY = "x-nemo-visibility";

LipstickNotification::LipstickNotification(const QString &appName, const QString &explicitAppName,
                                           const QString &disambiguatedAppName, uint id,
                                           const QString &appIcon, const QString &summary, const QString &body,
                                           const QStringList &actions, const QVariantHash &hints, int expireTimeout,
                                           QObject *parent)
    : QObject(parent),
      m_appName(appName),
      m_explicitAppName(explicitAppName),
      m_disambiguatedAppName(disambiguatedAppName),
      m_id(id),
      m_appIcon(appIcon),
      m_summary(summary),
      m_body(body),
      m_actions(actions),
      m_hints(hints),
      m_expireTimeout(expireTimeout),
      m_priority(hints.value(LipstickNotification::HINT_PRIORITY).toInt()),
      m_timestamp(hints.value(LipstickNotification::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch()),
      m_activeProgressTimer(0)
{
    updateHintValues();
}

LipstickNotification::LipstickNotification(QObject *parent)
    : QObject(parent),
      m_id(0),
      m_expireTimeout(-1),
      m_priority(0),
      m_timestamp(0),
      m_activeProgressTimer(0)
{
}

LipstickNotification::LipstickNotification(const LipstickNotification &notification)
    : QObject(notification.parent()),
      m_appName(notification.m_appName),
      m_explicitAppName(notification.m_explicitAppName),
      m_disambiguatedAppName(notification.m_disambiguatedAppName),
      m_id(notification.m_id),
      m_appIcon(notification.m_appIcon),
      m_appIconOrigin(notification.m_appIconOrigin),
      m_summary(notification.m_summary),
      m_body(notification.m_body),
      m_actions(notification.m_actions),
      m_hints(notification.m_hints),
      m_hintValues(notification.m_hintValues),
      m_expireTimeout(notification.m_expireTimeout),
      m_priority(notification.m_priority),
      m_timestamp(notification.m_timestamp),
      m_activeProgressTimer(0) // not caring for d-bus serialization
{
}

QString LipstickNotification::appName() const
{
    return m_appName;
}

QString LipstickNotification::explicitAppName() const
{
    return m_explicitAppName;
}

QString LipstickNotification::disambiguatedAppName() const
{
    return m_disambiguatedAppName;
}

void LipstickNotification::setAppName(const QString &appName)
{
    m_appName = appName;
}

void LipstickNotification::setExplicitAppName(const QString &appName)
{
    m_explicitAppName = appName;
}

void LipstickNotification::setDisambiguatedAppName(const QString &disambiguatedAppName)
{
    m_disambiguatedAppName = disambiguatedAppName;
}

uint LipstickNotification::id() const
{
    return m_id;
}

QString LipstickNotification::appIcon() const
{
    return m_appIcon;
}

void LipstickNotification::setAppIcon(const QString &appIcon, int source)
{
    bool iconChanged = false;
    bool sourceChanged = false;

    if (appIcon != m_appIcon) {
        iconChanged = true;
        m_appIcon = appIcon;
    }

    if (source != m_appIconOrigin) {
        sourceChanged = true;
        m_appIconOrigin = source;
    }

    if (iconChanged) {
        emit appIconChanged();
    }
    if (sourceChanged) {
        emit appIconOriginChanged();
    }
}

int LipstickNotification::appIconOrigin() const
{
    return m_appIconOrigin;
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
    if (m_actions != actions) {
        m_actions = actions;
        emit remoteActionsChanged();
    }
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
    QString oldAppIcon = appIcon();
    quint64 oldTimestamp = m_timestamp;
    QString oldPreviewSummary = previewSummary();
    QString oldPreviewBody = previewBody();
    QString oldSubText = subText();
    int oldUrgency = urgency();
    int oldItemCount = itemCount();
    int oldPriority = m_priority;
    QString oldCategory = category();
    qreal oldProgress = progress();
    bool oldHasProgress = hasProgress();
    bool oldIsTransient = isTransient();
    QString oldColor = color();

    m_hints = hints;
    updateHintValues();

    if (oldAppIcon != appIcon()) {
        emit appIconChanged();
    }

    m_timestamp = m_hints.value(LipstickNotification::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch();
    if (oldTimestamp != m_timestamp) {
        emit timestampChanged();
    }

    if (oldPreviewSummary != previewSummary()) {
        emit previewSummaryChanged();
    }

    if (oldPreviewBody != previewBody()) {
        emit previewBodyChanged();
    }

    if (oldSubText != subText()) {
        emit subTextChanged();
    }

    if (oldUrgency != urgency()) {
        emit urgencyChanged();
    }

    if (oldItemCount != itemCount()) {
        emit itemCountChanged();
    }

    m_priority = m_hints.value(LipstickNotification::HINT_PRIORITY).toInt();
    if (oldPriority != m_priority) {
        emit priorityChanged();
    }

    if (oldCategory != category()) {
        emit categoryChanged();
    }

    if (oldHasProgress != hasProgress()) {
        emit hasProgressChanged();
    }

    if (oldProgress != progress()) {
        emit progressChanged();
    }

    if (oldIsTransient != isTransient()) {
        emit isTransientChanged();
    }

    if (oldColor != color()) {
        emit colorChanged();
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

QDateTime LipstickNotification::timestamp() const
{
    return QDateTime::fromMSecsSinceEpoch(m_timestamp);
}

QString LipstickNotification::previewSummary() const
{
    return m_hints.value(LipstickNotification::HINT_PREVIEW_SUMMARY).toString();
}

QString LipstickNotification::previewBody() const
{
    return m_hints.value(LipstickNotification::HINT_PREVIEW_BODY).toString();
}

QString LipstickNotification::subText() const
{
    return m_hints.value(LipstickNotification::HINT_SUB_TEXT).toString();
}

int LipstickNotification::urgency() const
{
    return m_hints.value(LipstickNotification::HINT_URGENCY, LipstickNotification::Normal).toInt();
}

int LipstickNotification::itemCount() const
{
    return m_hints.value(LipstickNotification::HINT_ITEM_COUNT).toInt();
}

int LipstickNotification::priority() const
{
    return m_priority;
}

QString LipstickNotification::category() const
{
    return m_hints.value(LipstickNotification::HINT_CATEGORY).toString();
}

bool LipstickNotification::isTransient() const
{
    return m_hints.value(LipstickNotification::HINT_TRANSIENT).toBool();
}

QString LipstickNotification::color() const
{
    return m_hints.value(QLatin1String("x-nemo-color")).toString();
}

bool LipstickNotification::isUserRemovable() const
{
    if (hasProgress() && m_activeProgressTimer && m_activeProgressTimer->isActive()) {
        return false;
    } else {
        return isUserRemovableByHint();
    }
}

bool LipstickNotification::isUserRemovableByHint() const
{
    return (m_hints.value(LipstickNotification::HINT_USER_REMOVABLE, QVariant(true)).toBool());
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

        QVariantMap vm;
        vm.insert(QStringLiteral("name"), name);
        if (!displayName.isEmpty()) {
            vm.insert(QStringLiteral("displayName"), displayName);
        }

        const QString icon(m_hints.value(LipstickNotification::HINT_REMOTE_ACTION_ICON_PREFIX + name).toString());
        if (!icon.isEmpty()) {
            vm.insert(QStringLiteral("icon"), icon);
        }

        const QString type(m_hints.value(LipstickNotification::HINT_REMOTE_ACTION_TYPE_PREFIX + name).toString());
        vm.insert(QStringLiteral("type"), type);

        const QString hint(m_hints.value(LipstickNotification::HINT_REMOTE_ACTION_PREFIX + name).toString());
        if (!hint.isEmpty()) {
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
        }

        rv.append(vm);
    }

    return rv;
}

QString LipstickNotification::owner() const
{
    return m_hints.value(LipstickNotification::HINT_OWNER).toString();
}

bool LipstickNotification::restored() const
{
    return m_restored;
}

void LipstickNotification::setRestored(bool restored)
{
    m_restored = restored;
}

qreal LipstickNotification::progress() const
{
    return m_hints.value(LipstickNotification::HINT_PROGRESS).toReal();
}

bool LipstickNotification::hasProgress() const
{
    return m_hints.contains(LipstickNotification::HINT_PROGRESS);
}

quint64 LipstickNotification::internalTimestamp() const
{
    return m_timestamp;
}

void LipstickNotification::restartProgressTimer()
{
    // if this is notification with progress, have it non-removable for some time so updates don't instantly re-add it.
    // if no updates come soon, it can be considered stalled and removable.
    //
    // TODO: if we had an explicit hint like Android notifications' setOngoing(bool) we could use that for
    // temporarily marking notification non-removable, maybe using a lot longer timer.
    if (hasProgress()) {
        bool wasUserRemovable = isUserRemovable();
        if (!m_activeProgressTimer) {
            m_activeProgressTimer = new QTimer(this);
            m_activeProgressTimer->setSingleShot(true);
            connect(m_activeProgressTimer, &QTimer::timeout, this, &LipstickNotification::userRemovableChanged);
        }
        m_activeProgressTimer->start(60000); // just need some rough value here

        if (!wasUserRemovable) {
            emit userRemovableChanged();
        }
    }
}

QVariantHash LipstickNotification::internalHints() const
{
    return m_internalHints;
}

void LipstickNotification::setInternalHints(const QVariantHash &hints)
{
    m_internalHints = hints;
}

void LipstickNotification::setPrivilegedSource(bool privileged)
{
    m_internalHints[INTERNAL_HINT_PRIVILEGED] = privileged;
}

bool LipstickNotification::privilegedSource() const
{
    return m_internalHints.value(INTERNAL_HINT_PRIVILEGED, false).toBool();
}

void LipstickNotification::updateHintValues()
{
    m_hintValues.clear();

    QVariantHash::const_iterator it = m_hints.constBegin(), end = m_hints.constEnd();
    for ( ; it != end; ++it) {
        // Filter out the hints that are represented by other properties
        const QString &hint(it.key());

        if (hint == HINT_ICON) {
            qWarning() << "Notification sets deprecated hint" << HINT_ICON
                       << "to" << it.value() << ", use app_icon parameter or"
                       << LipstickNotification::HINT_IMAGE_PATH << "instead";
        } else if (hint == HINT_PREVIEW_ICON) {
            qWarning() << "Notification sets deprecated hint" << HINT_PREVIEW_ICON
                       << "to" << it.value() << ", use app_icon parameter or"
                       << LipstickNotification::HINT_IMAGE_PATH << "instead";
        }

        if (hint.compare(LipstickNotification::HINT_TIMESTAMP, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_PREVIEW_SUMMARY, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_PREVIEW_BODY, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_SUB_TEXT, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_URGENCY, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_ITEM_COUNT, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_PRIORITY, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_CATEGORY, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_USER_REMOVABLE, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_OWNER, Qt::CaseInsensitive) != 0 &&
            hint.compare(LipstickNotification::HINT_PROGRESS, Qt::CaseInsensitive) &&
            !hint.startsWith(LipstickNotification::HINT_REMOTE_ACTION_PREFIX, Qt::CaseInsensitive) &&
            !hint.startsWith(LipstickNotification::HINT_REMOTE_ACTION_ICON_PREFIX, Qt::CaseInsensitive) &&
            !hint.startsWith(LipstickNotification::HINT_REMOTE_ACTION_TYPE_PREFIX, Qt::CaseInsensitive)) {
            m_hintValues.insert(hint, it.value());
        }
    }
}

QDBusArgument &operator<<(QDBusArgument &argument, const LipstickNotification &notification)
{
    argument.beginStructure();
    argument << notification.m_appName;
    argument << notification.m_id;
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
    argument >> notification.m_id;
    argument >> notification.m_appIcon;
    argument >> notification.m_summary;
    argument >> notification.m_body;
    argument >> notification.m_actions;
    argument >> notification.m_hints;
    argument >> notification.m_expireTimeout;
    argument.endStructure();

    notification.m_priority = notification.m_hints.value(LipstickNotification::HINT_PRIORITY).toInt();
    notification.m_timestamp = notification.m_hints.value(LipstickNotification::HINT_TIMESTAMP).toDateTime().toMSecsSinceEpoch();
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
            if (lhs.id() > rhs.id()) {
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
