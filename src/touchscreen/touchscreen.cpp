/***************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
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

bool userInteracting(const QEvent *event) {
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
    , touchUnblockingDelayTimer(0)
    , displayState(new MeeGo::QmDisplayState(q))
    , q_ptr(q)
{
}

void TouchScreenPrivate::handleDisplayStateChange(TouchScreen::DisplayState state)
{
    Q_Q(TouchScreen);

    // Exited display off state. Let's wait for touch begin.
    if (currentDisplayState == TouchScreen::DisplayOff) {
        // Keep on filtering touch events to avoid unwanted display on, display off
        // sequences e.g. during a phone call.
        bool wasTouchBlocked = touchBlocked();
        touchUnblockingDelayTimer = q->startTimer(100);
        if (!wasTouchBlocked) {
            emit q->touchBlockedChanged();
        }
    } else if (state == TouchScreen::DisplayOff) {
        if (touchUnblockingDelayTimer > 0) {
            q->killTimer(touchUnblockingDelayTimer);
            touchUnblockingDelayTimer = 0;
        }
        waitForTouchBegin = true;
    }

    currentDisplayState = state;
    if (state == TouchScreen::DisplayDimmed)
        return;

    // Eating an event is meaningful only when the display is dimmed
    eatEvents = false;
}

bool TouchScreenPrivate::touchBlocked() const
{
    return (currentDisplayState == TouchScreen::DisplayOff || touchUnblockingDelayTimer > 0);
}

TouchScreen::TouchScreen(QObject *parent)
    : QObject(parent)
    , d_ptr(new TouchScreenPrivate(this))
{
    Q_D(TouchScreen);
    connect(d->displayState, &MeeGo::QmDisplayState::displayStateChanged, this, [=](MeeGo::QmDisplayState::DisplayState state) {
        TouchScreen::DisplayState newState = (TouchScreen::DisplayState)state;
        if (d->currentDisplayState != newState) {
            TouchScreen::DisplayState oldState = d->currentDisplayState;
            d->handleDisplayStateChange(newState);
            emit displayStateChanged(oldState, d->currentDisplayState);
        }
    });

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
    d->displayState->set(MeeGo::QmDisplayState::Off);
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
            if (event->type() != QEvent::TouchBegin) {
                event->accept();
                return true;
            }
            d->waitForTouchBegin = false;
        }
    }

    bool eat = d->eatEvents && (event->type() == QEvent::MouseButtonPress ||
                                event->type() == QEvent::TouchBegin ||
                                event->type() == QEvent::TouchUpdate ||
                                event->type() == QEvent::TouchEnd);
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
        if (!touchBlocked()) {
            emit touchBlockedChanged();
        }
    }
}

