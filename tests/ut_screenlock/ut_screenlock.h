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

#ifndef UT_SCREENLOCK_H
#define UT_SCREENLOCK_H

#include <QObject>
#include <displaystate.h>

class ScreenLock;

class Ut_ScreenLock : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void initTestCase();
    void cleanupTestCase();

    void testToggleScreenLockUI();
    void testToggleEventEater();
    void testUnlockScreenWhenLocked();
    void testUnlockScreenWhenNotLocked();
    void testTkLockOpen_data();
    void testTkLockOpen();
    void testTkLockClose();

private:
    void updateDisplayState(DeviceState::DisplayStateMonitor::DisplayState oldState, DeviceState::DisplayStateMonitor::DisplayState newState);
    void fakeDisplayOnAndReady();

    ScreenLock *screenLock;
};

#endif
