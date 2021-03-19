/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012-2015 Jolla Ltd.
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
#include <QQmlContext>
#include <QScreen>
#include <usbmodeselector.h>
#include <qusbmoded.h>

#include "ut_usbmodeselector.h"
#include "lipsticknotification.h"
#include "closeeventeater_stub.h"
#include "lipstickqmlpath_stub.h"

#include <nemo-devicelock/devicelock.h>

int argc = 1;
char *argv[] = { (char *) "./ut_usbmodeselector", NULL };

void Ut_USBModeSelector::initTestCase()
{
}

void Ut_USBModeSelector::cleanupTestCase()
{
}

void Ut_USBModeSelector::init()
{
    deviceLock = new NemoDeviceLock::DeviceLock(this);
    usbModeSelector = new USBModeSelector(deviceLock);
    usbModeSelector->m_usbMode->setCurrentMode(QUsbModed::Mode::Undefined);
}

void Ut_USBModeSelector::cleanup()
{
    delete usbModeSelector;
    delete deviceLock;
}

void Ut_USBModeSelector::testShowDialog_data()
{
    QTest::addColumn<QString>("mode");

    QTest::newRow("Ask") << QUsbModed::Mode::Ask;
}

void Ut_USBModeSelector::testShowDialog()
{
    QFETCH(QString, mode);

    QSignalSpy spy(usbModeSelector, SIGNAL(dialogShown()));
    usbModeSelector->setMode(mode);

    // Check that the window was shown
    QCOMPARE(usbModeSelector->windowVisible(), true);
    QCOMPARE(spy.count(), 1);
}

void Ut_USBModeSelector::testHideDialog_data()
{
    QTest::addColumn<QString>("mode");

    QTest::newRow("PC Suite") << QUsbModed::Mode::PCSuite;
    QTest::newRow("Mass Storage") << QUsbModed::Mode::MassStorage;
    QTest::newRow("MTP") << QUsbModed::Mode::MTP;
    QTest::newRow("Developer") << QUsbModed::Mode::Developer;
    QTest::newRow("Adb") << QUsbModed::Mode::Adb;
    QTest::newRow("Diag") << QUsbModed::Mode::Diag;
    QTest::newRow("Host") << QUsbModed::Mode::Host;
    QTest::newRow("Cellular connection sharing") << QUsbModed::Mode::ConnectionSharing;
}

void Ut_USBModeSelector::testHideDialog()
{
    QFETCH(QString, mode);

    usbModeSelector->m_usbMode->setConfigMode(QUsbModed::Mode::Ask);
    usbModeSelector->m_usbMode->setCurrentMode(QUsbModed::Mode::Ask);
    usbModeSelector->handleUSBState();

    QCOMPARE(usbModeSelector->windowVisible(), true);

    usbModeSelector->m_usbMode->setConfigMode(mode);
    usbModeSelector->m_usbMode->setCurrentMode(mode);

    usbModeSelector->handleUSBState();

    QCOMPARE(usbModeSelector->windowVisible(), false);
}

void Ut_USBModeSelector::testUSBNotifications_data()
{
    QTest::addColumn<QString>("mode");
    QTest::addColumn<USBModeSelector::Notification>("notification");

    QTest::newRow("PC Suite") << QUsbModed::Mode::PCSuite << USBModeSelector::PCSuite;
    QTest::newRow("Mass Storage") << QUsbModed::Mode::MassStorage << USBModeSelector::MassStorage;
    QTest::newRow("Developer") << QUsbModed::Mode::Developer << USBModeSelector::Developer;
    QTest::newRow("MTP") << QUsbModed::Mode::MTP << USBModeSelector::MTP;
    QTest::newRow("Adb") << QUsbModed::Mode::Adb << USBModeSelector::Adb;
    QTest::newRow("Diag") << QUsbModed::Mode::Diag << USBModeSelector::Diag;
    QTest::newRow("Host") << QUsbModed::Mode::Host << USBModeSelector::Host;
    QTest::newRow("Cellular connection sharing") << QUsbModed::Mode::ConnectionSharing << USBModeSelector::ConnectionSharing;
}

void Ut_USBModeSelector::testUSBNotifications()
{
    QFETCH(QString, mode);
    QFETCH(USBModeSelector::Notification, notification);

    usbModeSelector->m_usbMode->setCurrentMode(QUsbModed::Mode::Ask);
    usbModeSelector->handleUSBState();

    QSignalSpy spy(usbModeSelector, SIGNAL(showNotification(USBModeSelector::Notification)));

    usbModeSelector->setMode(mode);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.last().at(0).value<USBModeSelector::Notification>(), notification);
}

void Ut_USBModeSelector::testConnectingUSBWhenDeviceIsLockedEmitsDialogShown_data()
{
    QTest::addColumn<NemoDeviceLock::DeviceLock::LockState>("deviceLocked");
    QTest::addColumn<int>("dialogShownCount");
    QTest::newRow("Device locked") << NemoDeviceLock::DeviceLock::Locked << 1;
    QTest::newRow("Device not locked") << NemoDeviceLock::DeviceLock::Unlocked << 0;
}

void Ut_USBModeSelector::testConnectingUSBWhenDeviceIsLockedEmitsDialogShown()
{
    QFETCH(NemoDeviceLock::DeviceLock::LockState, deviceLocked);
    QFETCH(int, dialogShownCount);

    QSignalSpy spy(usbModeSelector, SIGNAL(dialogShown()));
    deviceLock->setState(deviceLocked);
    usbModeSelector->handleUSBEvent(QUsbModed::Mode::Connected);
    QCOMPARE(spy.count(), dialogShownCount);
}

void Ut_USBModeSelector::testSetUSBMode()
{
    usbModeSelector->setMode(QUsbModed::Mode::Charging);
    QCOMPARE(usbModeSelector->m_usbMode->currentMode(), QUsbModed::Mode::Charging);
}

QTEST_MAIN (Ut_USBModeSelector)
