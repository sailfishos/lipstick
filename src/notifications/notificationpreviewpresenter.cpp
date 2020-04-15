/***************************************************************************
**
** Copyright (c) 2012 Jolla Ltd.
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

#include "homewindow.h"
#include "lipsticksettings.h"
#include "utilities/closeeventeater.h"
#include "notifications/notificationmanager.h"
#include "notifications/notificationfeedbackplayer.h"
#include "notifications/lipsticknotification.h"
#include <QScreen> // should be included by lipstickcompositor.h
#include "compositor/lipstickcompositor.h"
#include "notificationpreviewpresenter.h"
#include "lipstickqmlpath.h"

#include "screenlock/screenlock.h"

#include <mce/dbus-names.h>
#include <nemo-devicelock/devicelock.h>

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QGuiApplication>
#include <QQmlContext>

namespace {

const QString MCE_NOTIFICATION_BEGIN(QStringLiteral("notification_begin_req"));
const QString MCE_NOTIFICATION_END(QStringLiteral("notification_end_req"));

const qint32 MCE_DURATION(6000);
const qint32 MCE_EXTEND_DURATION(2000);

enum PreviewMode {
    AllNotificationsEnabled = 0,
    ApplicationNotificationsDisabled,
    SystemNotificationsDisabled,
    AllNotificationsDisabled
};

}

NotificationPreviewPresenter::NotificationPreviewPresenter(
        ScreenLock *screenLock, NemoDeviceLock::DeviceLock *deviceLock, QObject *parent) :
    QObject(parent),
    m_window(0),
    m_currentNotification(0),
    m_notificationFeedbackPlayer(new NotificationFeedbackPlayer(this)),
    m_screenLock(screenLock),
    m_deviceLock(deviceLock)
{
    connect(NotificationManager::instance(), &NotificationManager::notificationAdded,
            this, &NotificationPreviewPresenter::updateNotification);
    connect(NotificationManager::instance(), &NotificationManager::notificationModified,
            this, &NotificationPreviewPresenter::updateNotification);
    connect(NotificationManager::instance(), &NotificationManager::notificationRemoved,
            this, [=](uint id) {
            removeNotification(id);
    });
    QTimer::singleShot(0, this, SLOT(createWindowIfNecessary()));
}

NotificationPreviewPresenter::~NotificationPreviewPresenter()
{
    delete m_window;
}

void NotificationPreviewPresenter::showNextNotification()
{
    if (!LipstickCompositor::instance() && !m_notificationQueue.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(showNextNotification()));
        return;
    }

    if (m_notificationQueue.isEmpty()) {
        // No more notifications to show: hide the notification window if it's visible
        if (m_window != 0 && m_window->isVisible()) {
            m_window->hide();
        }

        setCurrentNotification(0);
    } else {
        LipstickNotification *notification = m_notificationQueue.takeFirst();
        bool show = notificationShouldBeShown(notification);

        if (!show) {
            if (m_deviceLock->state() != NemoDeviceLock::DeviceLock::ManagerLockout) { // Suppress feedback if locked out.
                m_notificationFeedbackPlayer->addNotification(notification->id());
            }

            setCurrentNotification(0);

            showNextNotification();
        } else {
            // Show the notification window and the first queued notification in it
            if (!m_window->isVisible()) {
                m_window->show();
            }

            m_notificationFeedbackPlayer->addNotification(notification->id());

            setCurrentNotification(notification);
        }
    }
}

LipstickNotification *NotificationPreviewPresenter::notification() const
{
    return m_currentNotification;
}

void NotificationPreviewPresenter::updateNotification(uint id)
{
    LipstickNotification *notification = NotificationManager::instance()->notification(id);

    if (notification != 0) {
        if (notificationShouldBeShown(notification)) {
            // Add the notification to the queue if not already there or the current notification
            if (m_currentNotification != notification && !m_notificationQueue.contains(notification)) {
                m_notificationQueue.append(notification);

                // Show the notification if no notification currently being shown
                if (m_currentNotification == 0) {
                    showNextNotification();
                }
            }
        } else {
            m_notificationFeedbackPlayer->addNotification(id);

            // Remove updated notification only from the queue so that a currently visible notification won't suddenly disappear
            removeNotification(id, true);

            if (m_currentNotification != notification) {
                NotificationManager::instance()->markNotificationDisplayed(id);
            }
        }
    }
}

void NotificationPreviewPresenter::removeNotification(uint id, bool onlyFromQueue)
{
    // Remove the notification from the queue
    LipstickNotification *notification = NotificationManager::instance()->notification(id);

    if (notification != 0) {
        m_notificationQueue.removeAll(notification);

        // If the notification is currently being shown hide it - the next notification will be shown after the current one has been hidden
        if (!onlyFromQueue && m_currentNotification == notification) {
            m_currentNotification = 0;
            emit notificationChanged();
        }
    }
}

void NotificationPreviewPresenter::createWindowIfNecessary()
{
    if (m_window != 0) {
        return;
    }

    m_window = new HomeWindow();
    m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
    m_window->setCategory(QLatin1String("notification"));
    m_window->setWindowTitle("Notification");
    m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
    m_window->setContextProperty("LipstickSettings", LipstickSettings::instance());
    m_window->setContextProperty("notificationPreviewPresenter", this);
    m_window->setContextProperty("notificationFeedbackPlayer", m_notificationFeedbackPlayer);
    m_window->setSource(QmlPath::to("notifications/NotificationPreview.qml"));
    m_window->installEventFilter(new CloseEventEater(this));
}

bool NotificationPreviewPresenter::notificationShouldBeShown(LipstickNotification *notification)
{
    if (notification->hidden()
            || notification->restored()
            || (notification->previewBody().isEmpty() && notification->previewSummary().isEmpty())) {
        return false;
    }

    if (notification->hasProgress()) {
        return false; // would show up constantly as preview
    }

    const bool screenLocked = m_screenLock->isScreenLocked();
    const bool deviceLocked = m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked;
    const bool notificationIsCritical = notification->urgency() >= 2
            || notification->hints().value(LipstickNotification::HINT_DISPLAY_ON).toBool();
    const bool notificationIsPublic = notification->hints().value(LipstickNotification::HINT_VISIBILITY).toString()
            .compare(QLatin1String("public"), Qt::CaseInsensitive) == 0;

    bool show = true;

    if (notificationIsPublic) {
    } else if (deviceLocked) {
        if (!notificationIsCritical) {
            show = false;
        } else {
            show = m_deviceLock->showNotifications();
        }
    } else if (screenLocked) {
        if (!notificationIsCritical) {
            show = false;
        }
    }

    if (!show) {
        return false;
    }

    uint mode = AllNotificationsEnabled;
    QWaylandSurface *surface = LipstickCompositor::instance()->surfaceForId(LipstickCompositor::instance()->topmostWindowId());
    if (surface != 0) {
        mode = surface->windowProperties().value("NOTIFICATION_PREVIEWS_DISABLED", uint(AllNotificationsEnabled)).toUInt();
    }

    return (mode == AllNotificationsEnabled
            || (mode == ApplicationNotificationsDisabled && notificationIsCritical)
            || (mode == SystemNotificationsDisabled && !notificationIsCritical));
}

void NotificationPreviewPresenter::setCurrentNotification(LipstickNotification *notification)
{
    if (m_currentNotification != notification) {
        if (m_currentNotification) {
            NotificationManager::instance()->markNotificationDisplayed(m_currentNotification->id());
        }
        m_currentNotification = notification;
        emit notificationChanged();

        if (notification) {
            // Ask mce to turn the screen on if requested
            const bool notificationIsCritical = notification->urgency() >= 2 ||
                                                notification->hints().value(LipstickNotification::HINT_DISPLAY_ON).toBool();
            const bool notificationCanUnblank = !notification->hints().value(LipstickNotification::HINT_SUPPRESS_DISPLAY_ON).toBool();

            if (notificationIsCritical && notificationCanUnblank) {
                QString mceIdToAdd = QString("lipstick_notification_") + QString::number(notification->id());
                QDBusMessage msg = QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_NOTIFICATION_BEGIN);
                msg.setArguments(QVariantList() << mceIdToAdd << MCE_DURATION << MCE_EXTEND_DURATION);
                QDBusConnection::systemBus().asyncCall(msg);
            }
        }
    }
}
