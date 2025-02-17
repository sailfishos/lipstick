/***************************************************************************
**
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

#include <QtTest/QtTest>

#include "screenlock_stub.h"
#include "touchscreen_stub.h"
#include "homeapplication.h"
#include "touchscreen/touchscreen.h"
#include "lipsticksettings.h"
#include "ut_lipsticksettings.h"

void Ut_LipstickSettings::initTestCase()
{
}

void Ut_LipstickSettings::cleanupTestCase()
{
}

void Ut_LipstickSettings::testSetLockScreenVisible()
{
    LipstickSettings settings;

    // No screen lock set: calling setLockscreenVisible() does nothing
    settings.setLockscreenVisible(false);
    QCOMPARE(gScreenLockStub->stubCallCount("unlockScreen"), 0);
    settings.setLockscreenVisible(true);
    QCOMPARE(gScreenLockStub->stubCallCount("lockScreen"), 0);

    // Screen lock set: calling setLockscreenVisible() with a changed lock state calls the appropriate function
    ScreenLock screenLock(new TouchScreen(this));
    settings.setScreenLock(&screenLock);
    gScreenLockStub->stubSetReturnValue("isScreenLocked", false);
    settings.setLockscreenVisible(true);
    QCOMPARE(gScreenLockStub->stubCallCount("lockScreen"), 1);

    gScreenLockStub->stubSetReturnValue("isScreenLocked", true);
    settings.setLockscreenVisible(false);
    QCOMPARE(gScreenLockStub->stubCallCount("unlockScreen"), 1);

    // Screen lock set: calling setLockscreenVisible() with the same lock state does nothing
    settings.setLockscreenVisible(true);
    QCOMPARE(gScreenLockStub->stubCallCount("lockScreen"), 1);

    gScreenLockStub->stubSetReturnValue("isScreenLocked", false);
    settings.setLockscreenVisible(false);
    QCOMPARE(gScreenLockStub->stubCallCount("unlockScreen"), 1);
}

QTEST_MAIN(Ut_LipstickSettings)
