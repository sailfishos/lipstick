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

#include "homewindow.h"
#include "lipsticksettings.h"
#include "utilities/closeeventeater.h"
#include "notifications/notificationmanager.h"
#include "notifications/notificationfeedbackplayer.h"
#include <QScreen> // should be included by lipstickcompositor.h
#include "compositor/lipstickcompositor.h"
#include "notificationpreviewpresenter.h"
#include "lipstickqmlpath.h"

#include <qmdisplaystate.h>
#include <qmlocks.h>
#include "screenlock/screenlock.h"

#include <mce/dbus-names.h>
#include <nemo-devicelock/devicelock.h>

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QGuiApplication>
#include <QQmlContext>
#include <QSettings>

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
    connect(NotificationManager::instance(), SIGNAL(notificationAdded(uint)), this, SLOT(updateNotification(uint)));
    connect(NotificationManager::instance(), SIGNAL(notificationRemoved(uint)), this, SLOT(removeNotification(uint)));
    connect(this, SIGNAL(notificationPresented(uint)), m_notificationFeedbackPlayer, SLOT(addNotification(uint)));

    connect(NotificationManager::instance(), &NotificationManager::aboutToUpdateNotification,
            this, &NotificationPreviewPresenter::updateNotification);
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

        const bool screenLocked = m_screenLock->isScreenLocked() && m_screenLock->displayState() == TouchScreen::DisplayOff;
        const bool deviceLocked = m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked;
        const bool notificationIsCritical = notification->urgency() >= 2 || notification->hints().value(NotificationManager::HINT_DISPLAY_ON).toBool();

        bool show = true;
        if (deviceLocked) {
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
            if (m_deviceLock->state() != NemoDeviceLock::DeviceLock::ManagerLockout) { // Suppress feedback if locked out.
                // Don't show the notification but just remove it from the queue
                emit notificationPresented(notification->replacesId());
            }

            setCurrentNotification(0);

            showNextNotification();
        } else {
            // Show the notification window and the first queued notification in it
            if (!m_window->isVisible()) {
                m_window->show();
            }

            emit notificationPresented(notification->replacesId());

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
            // Remove updated notification only from the queue so that a currently visible notification won't suddenly disappear
            emit notificationPresented(id);

            removeNotification(id, true);

            if (m_currentNotification != notification) {
                NotificationManager::instance()->MarkNotificationDisplayed(id);
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
    if (notification->hidden() || notification->restored() || (notification->previewBody().isEmpty() && notification->previewSummary().isEmpty()))
        return false;

    const bool screenLocked = m_screenLock->isScreenLocked();
    const bool deviceLocked = m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked;
    const bool notificationIsCritical = notification->urgency() >= 2 || notification->hints().value(NotificationManager::HINT_DISPLAY_ON).toBool();

    uint mode = AllNotificationsEnabled;
    QWaylandSurface *surface = LipstickCompositor::instance()->surfaceForId(LipstickCompositor::instance()->topmostWindowId());
    if (surface != 0) {
        mode = surface->windowProperties().value("NOTIFICATION_PREVIEWS_DISABLED", uint(AllNotificationsEnabled)).toUInt();
    }

    return ((!screenLocked && !deviceLocked) || notificationIsCritical) &&
            (mode == AllNotificationsEnabled ||
             (mode == ApplicationNotificationsDisabled && notificationIsCritical) ||
             (mode == SystemNotificationsDisabled && !notificationIsCritical));
}

void NotificationPreviewPresenter::setCurrentNotification(LipstickNotification *notification)
{
    if (m_currentNotification != notification) {
        if (m_currentNotification) {
            NotificationManager::instance()->MarkNotificationDisplayed(m_currentNotification->replacesId());
        }
        m_currentNotification = notification;
        emit notificationChanged();

        if (notification) {
            // Ask mce to turn the screen on if requested
            const bool notificationIsCritical = notification->urgency() >= 2 ||
                                                notification->hints().value(NotificationManager::HINT_DISPLAY_ON).toBool();
            const bool  notificationCanUnblank = !notification->hints().value(NotificationManager::HINT_SUPPRESS_DISPLAY_ON).toBool();

            if (notificationIsCritical && notificationCanUnblank) {
                QString mceIdToAdd = QString("lipstick_notification_") + QString::number(notification->replacesId());
                QDBusMessage msg = QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_NOTIFICATION_BEGIN);
                msg.setArguments(QVariantList() << mceIdToAdd << MCE_DURATION << MCE_EXTEND_DURATION);
                QDBusConnection::systemBus().asyncCall(msg);
            }
        }
    }
}
