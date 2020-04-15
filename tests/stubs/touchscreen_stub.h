/***************************************************************************
**
** Copyright (c) 2016 Jolla Ltd.
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

#ifndef TOUCHSCREEN_STUB_H
#define TOUCHSCREEN_STUB_H

#include "touchscreen/touchscreen.h"
#include <stubbase.h>

// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class TouchScreenStub : public StubBase
{
public:
    virtual void TouchScreenStubConstructor(QObject *parent = 0);
    virtual void TouchScreenStubDestructor();

    virtual bool touchBlocked() const;
    virtual void setEnabled(bool);
    virtual void setDisplayOff();
    virtual TouchScreen::DisplayState currentDisplayState() const;

    virtual bool eventFilter(QObject *, QEvent *);
    virtual void timerEvent(QTimerEvent *);

    virtual void inputPolicyChanged(const QString &status);
    virtual void inputPolicyReply(QDBusPendingCallWatcher *watcher);
};

// 2. IMPLEMENT STUB
void TouchScreenStub::TouchScreenStubConstructor(QObject *parent)
{
    Q_UNUSED(parent)
}

void TouchScreenStub::TouchScreenStubDestructor()
{

}

bool TouchScreenStub::touchBlocked() const
{
    stubMethodEntered("touchBlocked");
    return stubReturnValue<bool>("touchBlocked");
}

void TouchScreenStub::setEnabled(bool enable)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<bool>(enable));
    stubMethodEntered("setEnabled", params);
}

void TouchScreenStub::setDisplayOff()
{
    stubMethodEntered("setDisplayOff");
}

TouchScreen::DisplayState TouchScreenStub::currentDisplayState() const
{
    stubMethodEntered("currentDisplayState");
    return stubReturnValue<TouchScreen::DisplayState>("currentDisplayState");
}

bool TouchScreenStub::eventFilter(QObject *, QEvent *)
{
    stubMethodEntered("eventFilter");
    return stubReturnValue<bool>("eventFilter");
}

void TouchScreenStub::timerEvent(QTimerEvent *)
{
    stubMethodEntered("timerEvent");
}

void TouchScreenStub::inputPolicyChanged(const QString &status)
{
    Q_UNUSED(status);
    stubMethodEntered("inputPolicyChanged");
}

void TouchScreenStub::inputPolicyReply(QDBusPendingCallWatcher *watcher)
{
    Q_UNUSED(watcher);
    stubMethodEntered("inputPolicyReply");
}

// 3. CREATE A STUB INSTANCE
TouchScreenStub gDefaultTouchScreenStub;
TouchScreenStub *gTouchScreenStub = &gDefaultTouchScreenStub;

// 4. CREATE A PROXY WHICH CALLS THE STUB
TouchScreen::TouchScreen(QObject *parent)
{
    gTouchScreenStub->TouchScreenStubConstructor(parent);
}

TouchScreen::~TouchScreen()
{
    gTouchScreenStub->TouchScreenStubDestructor();
}

bool TouchScreen::touchBlocked() const
{
    return gTouchScreenStub->touchBlocked();
}

void TouchScreen::setEnabled(bool enabled)
{
    gTouchScreenStub->setEnabled(enabled);
}

void TouchScreen::setDisplayOff()
{
    gTouchScreenStub->setDisplayOff();
}

TouchScreen::DisplayState TouchScreen::currentDisplayState() const
{
    return gTouchScreenStub->currentDisplayState();
}

bool TouchScreen::eventFilter(QObject *o, QEvent *event)
{
    return gTouchScreenStub->eventFilter(o, event);
}

void TouchScreen::timerEvent(QTimerEvent *e)
{
    gTouchScreenStub->timerEvent(e);
}

void TouchScreen::inputPolicyChanged(const QString &status)
{
    gTouchScreenStub->inputPolicyChanged(status);
}

void TouchScreen::inputPolicyReply(QDBusPendingCallWatcher *watcher)
{
    gTouchScreenStub->inputPolicyReply(watcher);
}

#endif // TOUCHSCREEN_STUB_H
