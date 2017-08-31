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
#ifndef SCREENLOCK_STUB
#define SCREENLOCK_STUB

#include "screenlock.h"
#include <stubbase.h>


// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class ScreenLockStub : public StubBase
{
public:
    virtual void ScreenLockConstructor(TouchScreen *touchScreen, QObject *parent);
    virtual void ScreenLockDestructor();
    virtual void displayStatusChanged(const QString &mode);
    virtual int tklock_open(const QString &service, const QString &path, const QString &interface, const QString &method, uint mode, bool silent, bool flicker);
    virtual int tklock_close(bool silent);
    virtual void setScreenLocked(bool value);
    virtual void setEventEaterEnabled(bool value);
    virtual void lockScreen(bool immediate = false);
    virtual void setInteractionExpected(bool expected);
    virtual void interactionExpectedBroadcast();
    virtual void unlockScreen();
    virtual void showScreenLock();
    virtual void showLowPowerMode();
    virtual void setDisplayOffMode();
    virtual void hideScreenLock();
    virtual void hideScreenLockAndEventEater();
    virtual void showEventEater();
    virtual void hideEventEater();
    virtual void handleLpmModeChange(const QString &);
    virtual void handleBlankingPolicyChange(const QString &);
    virtual bool isScreenLocked();
    virtual bool eventFilter(QObject *, QEvent *event);
    virtual void timerEvent(QTimerEvent *);
    virtual bool isLowPowerMode() const;
    virtual QString blankingPolicy() const;
    virtual bool touchBlocked() const;
};

// 2. IMPLEMENT STUB
void ScreenLockStub::ScreenLockConstructor(TouchScreen *touchScreen, QObject *parent)
{
    Q_UNUSED(touchScreen)
    Q_UNUSED(parent);
}
void ScreenLockStub::ScreenLockDestructor()
{

}

void ScreenLockStub::displayStatusChanged(const QString &mode)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QString &>(mode));
    stubMethodEntered("displayStatusChanged", params);
}

int ScreenLockStub::tklock_open(const QString &service, const QString &path, const QString &interface, const QString &method, uint mode, bool silent, bool flicker)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QString & >(service));
    params.append( new Parameter<const QString & >(path));
    params.append( new Parameter<const QString & >(interface));
    params.append( new Parameter<const QString & >(method));
    params.append( new Parameter<uint >(mode));
    params.append( new Parameter<bool >(silent));
    params.append( new Parameter<bool >(flicker));
    stubMethodEntered("tklock_open", params);
    return stubReturnValue<int>("tklock_open");
}

int ScreenLockStub::tklock_close(bool silent)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<bool >(silent));
    stubMethodEntered("tklock_close", params);
    return stubReturnValue<int>("tklock_close");
}

void ScreenLockStub::setScreenLocked(bool value)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<bool >(value));
    stubMethodEntered("setScreenLocked", params);
}

void ScreenLockStub::setEventEaterEnabled(bool value)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<bool >(value));
    stubMethodEntered("setEventEaterEnabled", params);
}

void ScreenLockStub::lockScreen(bool immediate)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<bool >(immediate));
    stubMethodEntered("lockScreen");
}

void ScreenLockStub::setInteractionExpected(bool expected)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<bool >(expected));
    stubMethodEntered("setInteractionExpected");
}

void ScreenLockStub::interactionExpectedBroadcast()
{
    stubMethodEntered("interactionExpectedBroadcast");
}

void ScreenLockStub::unlockScreen()
{
    stubMethodEntered("unlockScreen");
}

void ScreenLockStub::showScreenLock()
{
    stubMethodEntered("showScreenLock");
}

void ScreenLockStub::showLowPowerMode()
{
    stubMethodEntered("showLowPowerMode");
}

void ScreenLockStub::setDisplayOffMode()
{
    stubMethodEntered("setDisplayOffMode");
}

void ScreenLockStub::hideScreenLock()
{
    stubMethodEntered("hideScreenLock");
}

void ScreenLockStub::hideScreenLockAndEventEater()
{
    stubMethodEntered("hideScreenLockAndEventEater");
}

void ScreenLockStub::showEventEater()
{
    stubMethodEntered("showEventEater");
}

void ScreenLockStub::hideEventEater()
{
    stubMethodEntered("hideEventEater");
}

void ScreenLockStub::handleLpmModeChange(const QString &state)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QString>(state));
    stubMethodEntered("handleLpmModeChange", params);
}

void ScreenLockStub::handleBlankingPolicyChange(const QString &state)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QString>(state));
    stubMethodEntered("handleBlankingPolicyChange", params);
}

bool ScreenLockStub::isScreenLocked()
{
    stubMethodEntered("isScreenLocked");
    return stubReturnValue<bool>("isScreenLocked");
}

bool ScreenLockStub::eventFilter(QObject *object, QEvent *event)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<QObject * >(object));
    params.append( new Parameter<QEvent * >(event));
    stubMethodEntered("eventFilter", params);
    return stubReturnValue<bool>("eventFilter");
}

void ScreenLockStub::timerEvent(QTimerEvent *event)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QEvent *>(event));
    stubMethodEntered("timerEvent", params);
}

bool ScreenLockStub::isLowPowerMode() const
{
    stubMethodEntered("isLowPowerMode");
    return stubReturnValue<bool>("isLowPowerMode");
}

QString ScreenLockStub::blankingPolicy() const
{
    stubMethodEntered("blankingPolicy");
    return stubReturnValue<QString>("blankingPolicy");
}

bool ScreenLockStub::touchBlocked() const
{
    stubMethodEntered("touchBlocked");
    return stubReturnValue<bool>("touchBlocked");
}

// 3. CREATE A STUB INSTANCE
ScreenLockStub gDefaultScreenLockStub;
ScreenLockStub *gScreenLockStub = &gDefaultScreenLockStub;


// 4. CREATE A PROXY WHICH CALLS THE STUB
ScreenLock::ScreenLock(TouchScreen *touchScreen, QObject *parent)
{
    gScreenLockStub->ScreenLockConstructor(touchScreen, parent);
}

ScreenLock::~ScreenLock()
{
    gScreenLockStub->ScreenLockDestructor();
}

int ScreenLock::tklock_open(const QString &service, const QString &path, const QString &interface, const QString &method, uint mode, bool silent, bool flicker)
{
    return gScreenLockStub->tklock_open(service, path, interface, method, mode, silent, flicker);
}

int ScreenLock::tklock_close(bool silent)
{
    return gScreenLockStub->tklock_close(silent);
}

void ScreenLock::setScreenLocked(bool value)
{
    gScreenLockStub->setScreenLocked(value);
}

void ScreenLock::setEventEaterEnabled(bool value)
{
    gScreenLockStub->setEventEaterEnabled(value);
}

void ScreenLock::lockScreen(bool immediate)
{
    gScreenLockStub->lockScreen(immediate);
}

void ScreenLock::setInteractionExpected(bool expected)
{
    gScreenLockStub->setInteractionExpected(expected);
}

void ScreenLock::interactionExpectedBroadcast()
{
    gScreenLockStub->interactionExpectedBroadcast();
}

void ScreenLock::unlockScreen()
{
    gScreenLockStub->unlockScreen();
}

void ScreenLock::showScreenLock()
{
    gScreenLockStub->showScreenLock();
}

void ScreenLock::showLowPowerMode()
{
    gScreenLockStub->showLowPowerMode();
}

void ScreenLock::setDisplayOffMode()
{
    gScreenLockStub->setDisplayOffMode();
}

void ScreenLock::hideScreenLock()
{
    gScreenLockStub->hideScreenLock();
}

void ScreenLock::hideScreenLockAndEventEater()
{
    gScreenLockStub->hideScreenLockAndEventEater();
}

void ScreenLock::showEventEater()
{
    gScreenLockStub->showEventEater();
}

void ScreenLock::hideEventEater()
{
    gScreenLockStub->hideEventEater();
}

void
ScreenLock::handleLpmModeChange(const QString &enabled)
{
    gScreenLockStub->handleLpmModeChange(enabled);
}

void ScreenLock::handleBlankingPolicyChange(const QString &policy)
{
    gScreenLockStub->handleBlankingPolicyChange(policy);
}

bool ScreenLock::isScreenLocked() const
{
    return gScreenLockStub->isScreenLocked();
}

bool ScreenLock::isLowPowerMode() const
{
    return gScreenLockStub->isLowPowerMode();
}

QString ScreenLock::blankingPolicy() const
{
    return gScreenLockStub->blankingPolicy();
}

bool ScreenLock::touchBlocked() const
{
    return gScreenLockStub->touchBlocked();
}
#endif
