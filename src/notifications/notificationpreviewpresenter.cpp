/***************************************************************************
**
** Copyright (c) 2012-2019 Jolla Ltd.
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
#include <QWaylandSurface>

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
        ScreenLock *screenLock, NemoDeviceLock::DeviceLock *deviceLock, QObject *parent)
    : QObject(parent)
    , m_window(nullptr)
    , m_currentNotification(nullptr)
    , m_notificationFeedbackPlayer(new NotificationFeedbackPlayer(this))
    , m_screenLock(screenLock)
    , m_deviceLock(deviceLock)
    , m_currentInBackground(false)
{
    connect(NotificationManager::instance(), &NotificationManager::notificationAdded,
            this, &NotificationPreviewPresenter::updateNotification);
    connect(NotificationManager::instance(), &NotificationManager::notificationModified,
            this, &NotificationPreviewPresenter::updateNotification);
    connect(NotificationManager::instance(), &NotificationManager::notificationRemoved,
            this, &NotificationPreviewPresenter::removeNotification);

    QTimer::singleShot(0, this, SLOT(createWindowIfNecessary()));

    m_backgroundNotificationTimer.setSingleShot(true);
    m_backgroundNotificationTimer.setInterval(2000);
    connect(&m_backgroundNotificationTimer, &QTimer::timeout,
            this, [=]() {
        setCurrentNotification(nullptr);
        showNextNotification();
    });
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
        if (m_window != nullptr && m_window->isVisible()) {
            m_window->hide();
        }

        setCurrentNotification(nullptr);
    } else {
        LipstickNotification *notification = m_notificationQueue.takeFirst();
        bool show = notificationShouldBeShown(notification);

        if (!show) {
            // Suppress feedback if locked out.
            bool playingFeedback = false;
            if (m_deviceLock->state() != NemoDeviceLock::DeviceLock::ManagerLockout) {
                playingFeedback = m_notificationFeedbackPlayer->addNotification(notification->id());
            }

            if (playingFeedback) {
                // Give a non-popup notification that plays some feedback a small timeslot
                // to avoid worst type of overlapping sounds played on notification burst.
                // TODO: should eventually get some information from feedbackplayer/libngf at which
                // point sound and vibra type of feedback components have stopped, and continue based on that
                setCurrentNotification(notification, true);
                m_backgroundNotificationTimer.start();
            } else {
                setCurrentNotification(nullptr);
                showNextNotification();
            }
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

    // Add the notification to the queue if not already there or the current notification
    if (notification != nullptr
            && m_currentNotification != notification
            && !m_notificationQueue.contains(notification)) {
        m_notificationQueue.append(notification);

        // Show the notification if no notification currently being shown
        if (m_currentNotification == nullptr) {
            showNextNotification();
        }
    }
}

void NotificationPreviewPresenter::removeNotification(uint id)
{
    // Remove the notification from the queue
    LipstickNotification *notification = NotificationManager::instance()->notification(id);

    if (notification != nullptr) {
        m_notificationQueue.removeAll(notification);

        // If the notification is currently being shown hide it
        // - the next notification will be shown after the current one has been hidden
        if (m_currentNotification == notification) {
            m_currentNotification = nullptr;
            emit notificationChanged();
        }
    }
}

void NotificationPreviewPresenter::createWindowIfNecessary()
{
    if (m_window != nullptr) {
        return;
    }

    m_window = new HomeWindow;
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
    if (notification->restored()
            || (notification->urgency() == LipstickNotification::Low && !notification->isTransient())
            || (notification->previewBody().isEmpty() && notification->previewSummary().isEmpty())) {
        return false;
    }

    if (notification->hasProgress()) {
        return false; // would show up constantly as preview
    }

    const bool screenLocked = m_screenLock->isScreenLocked();
    const bool deviceLocked = m_deviceLock->state() >= NemoDeviceLock::DeviceLock::Locked;
    const bool notificationIsCritical = notification->urgency() >= LipstickNotification::Critical
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
    if (surface != nullptr) {
        mode = surface->windowProperties().value("NOTIFICATION_PREVIEWS_DISABLED", uint(AllNotificationsEnabled)).toUInt();
    }

    return (mode == AllNotificationsEnabled
            || (mode == ApplicationNotificationsDisabled && notificationIsCritical)
            || (mode == SystemNotificationsDisabled && !notificationIsCritical));
}

void NotificationPreviewPresenter::setCurrentNotification(LipstickNotification *notification, bool background)
{
    if (m_currentNotification != notification) {
        if (m_currentNotification && !m_currentInBackground) {
            // mark displayed only the ones that were actually shown on the screen
            NotificationManager::instance()->markNotificationDisplayed(m_currentNotification->id());
        }

        m_currentNotification = notification;
        m_currentInBackground = background;

        emit notificationChanged();

        if (notification) {
            // Ask mce to turn the screen on if requested
            const bool notificationIsCritical = notification->urgency() >= LipstickNotification::Critical;
            const bool displayOnRequested = notification->hints().value(LipstickNotification::HINT_DISPLAY_ON).toBool()
                    && !m_notificationFeedbackPlayer->doNotDisturbMode();

            if (notificationIsCritical || displayOnRequested) {
                QString mceIdToAdd = QString("lipstick_notification_") + QString::number(notification->id());
                QDBusMessage msg = QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF,
                                                                  MCE_NOTIFICATION_BEGIN);
                msg.setArguments(QVariantList() << mceIdToAdd << MCE_DURATION << MCE_EXTEND_DURATION);
                QDBusConnection::systemBus().asyncCall(msg);
            }
        }
    }
}
