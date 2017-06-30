/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QTimer>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QTextStream>
#include <QCursor>
#include <QDebug>
#include <QTouchEvent>

#include <mce/mode-names.h>

#include "homeapplication.h"
#include "screenlock.h"
#include "touchscreen/touchscreen.h"
#include "utilities/closeeventeater.h"

ScreenLock::ScreenLock(TouchScreen *touch, QObject* parent) :
    QObject(parent),
    m_callbackInterface(NULL),
    m_touchScreen(touch),
    m_shuttingDown(false),
    m_lockscreenVisible(false),
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

    // Create a D-Bus interface if one doesn't exist or the D-Bus callback details have changed
    if (m_callbackInterface == NULL || m_callbackInterface->service() != service || m_callbackInterface->path() != path || m_callbackInterface->interface() != interface) {
        delete m_callbackInterface;
        m_callbackInterface = new QDBusInterface(service, path, interface, QDBusConnection::systemBus(), this);
    }

    // Store the callback method name
    m_callbackMethod = method;

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

    if (m_callbackInterface != NULL && !m_callbackMethod.isEmpty()) {
        m_callbackInterface->call(QDBus::NoBlock, m_callbackMethod, TkLockUnlock);
    }
}

void ScreenLock::showScreenLock()
{
    toggleScreenLockUI(true);
    toggleEventEater(false);
}

void ScreenLock::showLowPowerMode()
{
    toggleScreenLockUI(true);
    toggleEventEater(false);
}

void ScreenLock::setDisplayOffMode()
{
    toggleScreenLockUI(true);
    toggleEventEater(false);
}

void ScreenLock::hideScreenLock()
{
    toggleScreenLockUI(false);
}

void ScreenLock::hideScreenLockAndEventEater()
{
    toggleScreenLockUI(false);
    toggleEventEater(false);
}

void ScreenLock::showEventEater()
{
    toggleEventEater(true);
}

void ScreenLock::hideEventEater()
{
    toggleEventEater(false);
}

void ScreenLock::toggleScreenLockUI(bool toggle)
{
    // TODO Make the view a lock screen view (title? stacking layer?)
    if (m_lockscreenVisible != toggle) {
        m_lockscreenVisible = toggle;
        emit screenIsLocked(toggle);
    }
}

void ScreenLock::toggleEventEater(bool toggle)
{
    Q_ASSERT(m_touchScreen);
    m_touchScreen->setEnabled(!toggle);
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
