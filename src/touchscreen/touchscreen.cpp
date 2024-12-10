/***************************************************************************
**
** Copyright (c) 2016 - 2020 Jolla Ltd.
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

#include "touchscreen.h"
#include "touchscreen_p.h"

#include <QTimerEvent>
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>
#include <QDebug>

#include <mce/dbus-names.h>
#include <mce/mode-names.h>

static bool userInteracting(const QEvent *event)
{
    switch(event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::GrabMouse:
    case QEvent::UngrabMouse:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonDblClick:
        return true;
    default:
        return false;
    }
}

TouchScreenPrivate::TouchScreenPrivate(TouchScreen *q)
    : eatEvents(false)
    , currentDisplayState(TouchScreen::DisplayUnknown)
    , waitForTouchBegin(true)
    , inputEnabled(true)
    , touchBlockedState(false)
    , touchUnblockingDelayTimer(0)
    , displayState(new DeviceState::DisplayStateMonitor(q))
    , q_ptr(q)
{
}

void TouchScreenPrivate::handleDisplayStateChange(TouchScreen::DisplayState state)
{
    Q_Q(TouchScreen);

    currentDisplayState = state;
    if (state == TouchScreen::DisplayDimmed)
        return;

    // Eating an event is meaningful only when the display is dimmed
    eatEvents = false;
}

void TouchScreenPrivate::handleInputPolicyChange(bool inputEnabled)
{
    Q_Q(TouchScreen);

    if (this->inputEnabled != inputEnabled) {
        this->inputEnabled = inputEnabled;

        if (touchUnblockingDelayTimer > 0) {
            q->killTimer(touchUnblockingDelayTimer);
            touchUnblockingDelayTimer = 0;
        }

        if (this->inputEnabled) {
            touchUnblockingDelayTimer = q->startTimer(100);
        } else {
            waitForTouchBegin = true;
        }

        evaluateTouchBlocked();
    }
}

void TouchScreenPrivate::evaluateTouchBlocked()
{
    Q_Q(TouchScreen);

    bool blocked = !inputEnabled || touchUnblockingDelayTimer > 0;

    if (touchBlockedState != blocked) {
        touchBlockedState = blocked;
        emit q->touchBlockedChanged();
    }
}

bool TouchScreenPrivate::touchBlocked() const
{
    return touchBlockedState;
}

TouchScreen::TouchScreen(QObject *parent)
    : QObject(parent)
    , d_ptr(new TouchScreenPrivate(this))
{
    Q_D(TouchScreen);
    connect(d->displayState, &DeviceState::DisplayStateMonitor::displayStateChanged,
            this, [=](DeviceState::DisplayStateMonitor::DisplayState state) {
        TouchScreen::DisplayState newState = (TouchScreen::DisplayState)state;
        if (d->currentDisplayState != newState) {
            TouchScreen::DisplayState oldState = d->currentDisplayState;
            d->handleDisplayStateChange(newState);
            emit displayStateChanged(oldState, d->currentDisplayState);
        }
    });

    QDBusConnection systemBus = QDBusConnection::systemBus();

    systemBus.connect(MCE_SERVICE,
                      MCE_SIGNAL_PATH,
                      MCE_SIGNAL_IF,
                      MCE_TOUCH_INPUT_POLICY_SIG,
                      this, SLOT(inputPolicyChanged(QString)));

    QDBusPendingReply<QString> query = systemBus.asyncCall(QDBusMessage::createMethodCall(
                MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_TOUCH_INPUT_POLICY_GET));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(query, this);
    connect(watcher, &QDBusPendingCallWatcher::finished,
            this, &TouchScreen::inputPolicyReply);

    qApp->installEventFilter(this);
}

TouchScreen::~TouchScreen()
{
    Q_D(TouchScreen);
    delete d->displayState;
    d->displayState = 0;

    delete d;
    d_ptr = 0;
}

void TouchScreen::inputPolicyChanged(const QString &status)
{
    Q_D(TouchScreen);

    bool inputEnabled = (status != MCE_INPUT_POLICY_DISABLED);

    d->handleInputPolicyChange(inputEnabled);
}

void TouchScreen::inputPolicyReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QString> reply = *watcher;
    if (reply.isError()) {
        qWarning() << MCE_TOUCH_INPUT_POLICY_GET"() failed!" << reply.error().name() << reply.error().message();
        /* Do not leave input disabled in error situations */
        inputPolicyChanged(QString(MCE_INPUT_POLICY_ENABLED));
    } else {
        inputPolicyChanged(reply.value());
    }
    watcher->deleteLater();
}

bool TouchScreen::touchBlocked() const
{
    Q_D(const TouchScreen);
    return d->touchBlocked();
}

void TouchScreen::setEnabled(bool enabled)
{
    Q_D(TouchScreen);
    d->eatEvents = !enabled;
}

void TouchScreen::setDisplayOff()
{
    Q_D(TouchScreen);
    d->displayState->set(DeviceState::DisplayStateMonitor::Off);
}

TouchScreen::DisplayState TouchScreen::currentDisplayState() const
{
    Q_D(const TouchScreen);
    return d->currentDisplayState;
}

bool TouchScreen::eventFilter(QObject *, QEvent *event)
{
    Q_D(TouchScreen);

    if (userInteracting(event)) {
        if (touchBlocked()) {
            event->accept();
            return true;
        }

        if (d->waitForTouchBegin) {
            switch (event->type()) {
            case QEvent::TouchBegin:
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
                d->waitForTouchBegin = false;
                break;
            default:
                event->accept();
                return true;
            }
        }
    }

    bool eat = d->eatEvents && (event->type() == QEvent::MouseButtonPress
                                || event->type() == QEvent::TouchBegin
                                || event->type() == QEvent::TouchUpdate
                                || event->type() == QEvent::TouchEnd);
    if (eat) {
        setEnabled(true);
    }

    return eat;
}

void TouchScreen::timerEvent(QTimerEvent *e)
{
    Q_D(TouchScreen);
    if (e->timerId() == d->touchUnblockingDelayTimer) {
        killTimer(d->touchUnblockingDelayTimer);
        d->touchUnblockingDelayTimer = 0;
        d->evaluateTouchBlocked();
    }
}
