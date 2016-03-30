/****************************************************************************
 **
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (directui@nokia.com)
 **
 ** This file is part of systemui.
 **
 ** If you have questions regarding the use of this file, please contact
 ** Nokia at directui@nokia.com.
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License version 2.1 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.LGPL included in the packaging
 ** of this file.
 **
 ****************************************************************************/
#include "lowbatterynotifier.h"
#include <QTimer>

static const int CALL_ACTIVE_NOTIFICATION_INTERVAL = 2 * 60 * 1000;
static const int DEVICE_ACTIVE_NOTIFICATION_INTERVAL = 5 * 60 * 1000;
static const int DEVICE_INACTIVE_NOTIFICATION_INTERVAL = 30 * 60 * 1000;

LowBatteryNotifier::LowBatteryNotifier(QObject *parent) :
    QObject(parent),
    m_displayState(new MeeGo::QmDisplayState(this)),
#ifdef HAVE_CONTEXTSUBSCRIBER
    m_callContextItem("Phone.Call"),
#endif
    m_notificationTimer(new QTimer(this)),
    m_previousNotificationTime(QTime::currentTime()),
    m_notificationInterval(DEVICE_ACTIVE_NOTIFICATION_INTERVAL),
    m_deviceInactive(false),
    m_touchScreenLockActive(false),
    m_callActive(false)
{
    connect(m_notificationTimer, SIGNAL(timeout()), this, SLOT(sendLowBatteryAlert()));

    setNotificationInterval();

    connect(m_displayState, SIGNAL(displayStateChanged(MeeGo::QmDisplayState::DisplayState)), this, SLOT(setNotificationInterval()));

#ifdef HAVE_CONTEXTSUBSCRIBER
    connect(&m_callContextItem, SIGNAL(valueChanged()), this, SLOT(setNotificationInterval()));
    m_callContextItem.subscribe();
#endif
}

LowBatteryNotifier::~LowBatteryNotifier()
{
}

void LowBatteryNotifier::sendLowBatteryAlert()
{
    emit lowBatteryAlert();

    m_previousNotificationTime.start();
    m_notificationTimer->start(m_notificationInterval);
}

void LowBatteryNotifier::setNotificationInterval()
{
    bool deviceCurrentlyInactive = m_touchScreenLockActive;
#ifdef HAVE_CONTEXTSUBSCRIBER
    bool callCurrentlyActive = m_callContextItem.value().toString() == "active";
#else
    bool callCurrentlyActive = false;
#endif

    // Device can be considered inactive only if the touch screen lock is active AND the display is off
    deviceCurrentlyInactive &= m_displayState->get() == MeeGo::QmDisplayState::Off;

    if (deviceCurrentlyInactive != m_deviceInactive || callCurrentlyActive != m_callActive) {
        // Device activity or call activity has changed
        m_deviceInactive = deviceCurrentlyInactive;
        m_callActive = callCurrentlyActive;

        // Set the new notification interval based on the device and call state
        if (m_callActive) {
            m_notificationInterval = CALL_ACTIVE_NOTIFICATION_INTERVAL;
        } else {
            m_notificationInterval = m_deviceInactive ? DEVICE_INACTIVE_NOTIFICATION_INTERVAL : DEVICE_ACTIVE_NOTIFICATION_INTERVAL;
        }

        if (m_previousNotificationTime.elapsed() < m_notificationInterval) {
            // Elapsed time has not reached the notification interval so just set the new interval
            m_notificationTimer->setInterval(m_notificationInterval - m_previousNotificationTime.elapsed());
        } else {
            // Elapsed time has reached the notification interval, so send the notification immediately (which will also set the new interval)
            sendLowBatteryAlert();
        }
    }
}

void LowBatteryNotifier::setTouchScreenLockActive(bool active)
{
    m_touchScreenLockActive = active;
    setNotificationInterval();
}
