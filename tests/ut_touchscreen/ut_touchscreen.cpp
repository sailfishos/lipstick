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

#include <QtTest/QtTest>
#include <QMouseEvent>

#include "ut_touchscreen.h"
#include "touchscreen.h"
#include "homeapplication.h"
#include "displaystate_stub.h"
#include "lipsticktest.h"

#include <mce/mode-names.h>

TouchScreen *gTouchScreen = 0;

HomeApplication::~HomeApplication()
{
}

HomeApplication *HomeApplication::instance()
{
    return qobject_cast<HomeApplication *>(qApp);
}

TouchScreen *HomeApplication::touchScreen() const
{
    return gTouchScreen;
}

void Ut_TouchScreen::init()
{
    gTouchScreen = new TouchScreen;
}

void Ut_TouchScreen::cleanup()
{
    delete gTouchScreen;
}

void Ut_TouchScreen::initTestCase()
{
}

void Ut_TouchScreen::cleanupTestCase()
{
}

void Ut_TouchScreen::testEnabled()
{
    fakeDisplayOnAndReady();
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(), Qt::NoButton, 0, 0);

    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    touchScreen->setEnabled(false);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    touchScreen->setEnabled(true);
    QCOMPARE(touchScreen->eventFilter(0, &event), false);
}

void Ut_TouchScreen::testTouchBlocking()
{
    HomeApplication::instance()->touchScreen()->setEnabled(true);

    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    updateDisplayState(DeviceState::DisplayStateMonitor::Unknown, DeviceState::DisplayStateMonitor::Off);
    touchScreen->inputPolicyChanged(MCE_INPUT_POLICY_DISABLED);
    QVERIFY(touchScreen->touchBlocked());

    QMouseEvent event(QEvent::MouseButtonPress, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseMove, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseButtonRelease, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseButtonDblClick, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    QTouchEvent touch(QEvent::TouchBegin);
    QCOMPARE(touchScreen->eventFilter(0, &touch), true);

    touch = QTouchEvent(QEvent::TouchUpdate);
    QCOMPARE(touchScreen->eventFilter(0, &touch), true);

    touch = QTouchEvent(QEvent::TouchEnd);
    QCOMPARE(touchScreen->eventFilter(0, &touch), true);

    // Do not filter TouchCancel.
    touch = QTouchEvent(QEvent::TouchCancel);
    QCOMPARE(touchScreen->eventFilter(0, &touch), false);

    updateDisplayState(DeviceState::DisplayStateMonitor::Off, DeviceState::DisplayStateMonitor::On);
    QVERIFY(touchScreen->touchBlocked());
    touchScreen->inputPolicyChanged(MCE_INPUT_POLICY_ENABLED);
    QSignalSpy touchBlockingSpy(touchScreen, SIGNAL(touchBlockedChanged()));
    touchBlockingSpy.wait();
    QVERIFY(!touchScreen->touchBlocked());

    event = QMouseEvent(QEvent::MouseButtonPress, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseMove, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseButtonRelease, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    event = QMouseEvent(QEvent::MouseButtonDblClick, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), true);

    touch = QTouchEvent(QEvent::TouchUpdate);
    QCOMPARE(touchScreen->eventFilter(0, &touch), true);

    touch = QTouchEvent(QEvent::TouchEnd);
    QCOMPARE(touchScreen->eventFilter(0, &touch), true);

    // New touch sequence starts after turning display on.
    touch = QTouchEvent(QEvent::TouchBegin);
    QCOMPARE(touchScreen->eventFilter(0, &touch), false);

    touch = QTouchEvent(QEvent::TouchUpdate);
    QCOMPARE(touchScreen->eventFilter(0, &touch), false);

    touch = QTouchEvent(QEvent::TouchEnd);
    QCOMPARE(touchScreen->eventFilter(0, &touch), false);

    event = QMouseEvent(QEvent::MouseButtonPress, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), false);

    event = QMouseEvent(QEvent::MouseMove, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), false);

    event = QMouseEvent(QEvent::MouseButtonRelease, QPointF(), Qt::NoButton, 0, 0);
    QCOMPARE(touchScreen->eventFilter(0, &event), false);
}

void Ut_TouchScreen::updateDisplayState(DeviceState::DisplayStateMonitor::DisplayState oldState, DeviceState::DisplayStateMonitor::DisplayState newState)
{
    emit gDisplayStateMonitorStub->displayState->displayStateChanged(oldState);
    emit gDisplayStateMonitorStub->displayState->displayStateChanged(newState);
}

void Ut_TouchScreen::fakeDisplayOnAndReady()
{
    // Fake display state change.
    TouchScreen *touchScreen = HomeApplication::instance()->touchScreen();
    updateDisplayState(DeviceState::DisplayStateMonitor::Off, DeviceState::DisplayStateMonitor::On);
    touchScreen->inputPolicyChanged(MCE_INPUT_POLICY_ENABLED);
    QSignalSpy touchBlockingSpy(touchScreen, SIGNAL(touchBlockedChanged()));
    touchBlockingSpy.wait();

    QVERIFY(!touchScreen->touchBlocked());
    touchScreen->setEnabled(true);
    QTouchEvent touch(QEvent::TouchBegin);
    QCOMPARE(touchScreen->eventFilter(0, &touch), false);
}

LIPSTICK_TEST_MAIN(Ut_TouchScreen)
