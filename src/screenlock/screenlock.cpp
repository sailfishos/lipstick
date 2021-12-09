/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QTimer>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QTextStream>
#include <QCursor>
#include <QDebug>
#include <QTouchEvent>
#include <MGConfItem>

#include <mce/mode-names.h>

#include "homeapplication.h"
#include "screenlock.h"
#include "touchscreen/touchscreen.h"
#include "utilities/closeeventeater.h"

ScreenLock::ScreenLock(TouchScreen *touch, QObject* parent) :
    QObject(parent),
    m_touchScreen(touch),
    m_shuttingDown(false),
    m_lockscreenVisible(false),
    m_deviceIsLocked(false),
    m_allowSkippingLockScreen(false),
    m_lowPowerMode(false),
    m_mceBlankingPolicy("default"),
    m_interactionExpectedTimer(0),
    m_interactionExpectedCurrent(false),
    m_interactionExpectedEmitted(-1)

{
    /* Setup idle timer for signaling interaction expected changes on D-Bus */
    m_interactionExpectedTimer = new QTimer(this);
    m_interactionExpectedTimer->setSingleShot(true);
    m_interactionExpectedTimer->setInterval(0);
    connect(m_interactionExpectedTimer, &QTimer::timeout,
            this, &ScreenLock::interactionExpectedBroadcast);

    connect(m_touchScreen, SIGNAL(touchBlockedChanged()), this, SIGNAL(touchBlockedChanged()));

    auto systemBus = QDBusConnection::systemBus();
    systemBus.connect(QString(),
            "/com/nokia/mce/signal",
            "com.nokia.mce.signal",
            "lpm_ui_mode_ind",
            this,
            SLOT(handleLpmModeChange(QString)));
    systemBus.connect(QString(),
            "/com/nokia/mce/signal",
            "com.nokia.mce.signal",
            "display_blanking_policy_ind",
            this,
            SLOT(handleBlankingPolicyChange(QString)));

    m_allowSkippingLockScreen = MGConfItem("/lipstick/skip_lock_screen").value(false).toBool();
}

ScreenLock::~ScreenLock()
{
    m_touchScreen = 0;
}

int ScreenLock::tklock_open(const QString &service, const QString &path, const QString &interface, const QString &method, uint mode, bool, bool)
{
    if (m_shuttingDown) {
        // Don't show the touch screen lock while shutting down
        return TkLockReplyOk;
    }

    // Store the callback method name
    m_callbackMethod = QDBusMessage::createMethodCall(service, path, interface, method);

    // MCE needs a response ASAP, so the actions are executed with single shot timers
    switch (mode) {
    case TkLockModeEnable:
        // Create the lock screen already so that it's readily available
        QTimer::singleShot(0, this, SLOT(showScreenLock()));
        break;

    case TkLockModeOneInput:
        QTimer::singleShot(0, this, SLOT(showEventEater()));
        break;

    case TkLockEnableVisual:
        // Raise the lock screen window on top if it isn't already
        QTimer::singleShot(0, this, SLOT(showScreenLock()));
        break;

    case TkLockEnableLowPowerMode:
        // Raise the lock screen window on top if it isn't already
        // (XXX: Low power mode is now handled via lpm_ui_mode_ind)
        QTimer::singleShot(0, this, SLOT(showLowPowerMode()));
        break;

    case TkLockRealBlankMode:
        QTimer::singleShot(0, this, SLOT(setDisplayOffMode()));
        break;

    default:
        break;
    }

    return TkLockReplyOk;
}

int ScreenLock::tklock_close(bool)
{
    QTimer::singleShot(0, this, SLOT(hideScreenLock()));

    return TkLockReplyOk;
}

void ScreenLock::interactionExpectedBroadcast()
{
    if (m_interactionExpectedEmitted != m_interactionExpectedCurrent) {
        m_interactionExpectedEmitted = m_interactionExpectedCurrent;
        emit interaction_expected(m_interactionExpectedEmitted);
    }
}

void ScreenLock::setInteractionExpected(bool expected)
{
    /* The qml side property evaluation produces some jitter.
     * To avoid duplicating it at dbus level, delay signal
     * broadcasting until we settle on some value. */

    m_interactionExpectedCurrent = expected;

    if (m_interactionExpectedEmitted != m_interactionExpectedCurrent) {
        m_interactionExpectedTimer->start();
    } else {
        m_interactionExpectedTimer->stop();
    }
}

void ScreenLock::lockScreen(bool immediate)
{
    QDBusMessage message = QDBusMessage::createMethodCall("com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request", "req_tklock_mode_change");
    message.setArguments(QVariantList() << (immediate ? MCE_TK_LOCKED : MCE_TK_LOCKED_DELAY));
    QDBusConnection::systemBus().asyncCall(message);

    showScreenLock();
}

void ScreenLock::unlockScreen()
{
    hideScreenLockAndEventEater();

    if (m_callbackMethod.type() == QDBusMessage::MethodCallMessage) {
        m_callbackMethod.setArguments({ TkLockUnlock });
        QDBusConnection::systemBus().call(m_callbackMethod, QDBus::NoBlock);
    }
}

void ScreenLock::showScreenLock()
{
    setScreenLocked(true);
    setEventEaterEnabled(false);
}

void ScreenLock::showLowPowerMode()
{
    setScreenLocked(true);
    setEventEaterEnabled(false);
}

void ScreenLock::setDisplayOffMode()
{
    setScreenLocked(true);
    setEventEaterEnabled(false);
}

void ScreenLock::hideScreenLock()
{
    setScreenLocked(false);
}

void ScreenLock::hideScreenLockAndEventEater()
{
    setScreenLocked(false);
    setEventEaterEnabled(false);
}

void ScreenLock::showEventEater()
{
    setEventEaterEnabled(true);
}

void ScreenLock::hideEventEater()
{
    setEventEaterEnabled(false);
}

void ScreenLock::setScreenLocked(bool value)
{
    // TODO Make the view a lock screen view (title? stacking layer?)
    if (m_lockscreenVisible != value) {
        if (m_allowSkippingLockScreen) {
            // Allow disabling lock screen, but only if the device is not locked
            if (value && !m_deviceIsLocked) {
                return;
            }
        }
        m_lockscreenVisible = value;
        emit screenLockedChanged(value);
    }
}

bool ScreenLock::isDeviceLocked() const
{
    return m_deviceIsLocked;
}

void ScreenLock::setDeviceIsLocked(bool locked)
{
    if (m_deviceIsLocked != locked) {
        m_deviceIsLocked = locked;
        emit deviceIsLockedChanged(locked);

        if (m_allowSkippingLockScreen) {
            if (locked && displayState() == TouchScreen::DisplayOff) {
                lockScreen();
            }
        }
    }
}

void ScreenLock::setEventEaterEnabled(bool value)
{
    Q_ASSERT(m_touchScreen);
    m_touchScreen->setEnabled(!value);
}

bool ScreenLock::isScreenLocked() const
{
    return m_lockscreenVisible;
}

bool ScreenLock::isLowPowerMode() const
{
    return m_lowPowerMode;
}

QString ScreenLock::blankingPolicy() const
{
    return m_mceBlankingPolicy;
}

bool ScreenLock::touchBlocked() const
{
    Q_ASSERT(m_touchScreen);
    return m_touchScreen->touchBlocked();
}

TouchScreen::DisplayState ScreenLock::displayState() const
{
    Q_ASSERT(m_touchScreen);
    return m_touchScreen->currentDisplayState();
}

void ScreenLock::handleLpmModeChange(const QString &state)
{
    bool enabled = (state == "enabled");

    if (!enabled && state != "disabled") {
        qWarning() << "Invalid LPM state value from mce:" << state;
    }

    if (m_lowPowerMode != enabled) {
        m_lowPowerMode = enabled;
        emit lowPowerModeChanged();
    }
}

void ScreenLock::handleBlankingPolicyChange(const QString &policy)
{
    if (m_mceBlankingPolicy != policy) {
        m_mceBlankingPolicy = policy;
        emit blankingPolicyChanged(m_mceBlankingPolicy);
    }
}
