/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012 - 2020 Jolla Ltd.
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
#include <QtTest/QtTest>
#include "thermalnotifier.h"
#include "homeapplication.h"
#include "displaystate_stub.h"
#include "thermal_stub.h"
#include "notificationmanager_stub.h"
#include "lipsticknotification.h"
#include "ut_thermalnotifier.h"

HomeApplication::~HomeApplication()
{
}

HomeApplication *HomeApplication::instance()
{
    return 0;
}

void HomeApplication::restoreSignalHandlers()
{
}

int argc = 1;
char *argv[] = { (char *) "./ut_thermalnotifier", NULL };

void Ut_ThermalNotifier::initTestCase()
{
}

void Ut_ThermalNotifier::cleanupTestCase()
{
}

void Ut_ThermalNotifier::init()
{
    thermalNotifier = new ThermalNotifier;

    gNotificationManagerStub->stubReset();
    gNotificationManagerStub->stubSetReturnValue("Notify", (uint)1);
    gThermalStub->stubReset();
}

void Ut_ThermalNotifier::cleanup()
{
    delete thermalNotifier;
    gNotificationManagerStub->stubReset();
}

void Ut_ThermalNotifier::testConnections()
{
    QCOMPARE(disconnect(thermalNotifier->m_thermalState, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)), thermalNotifier, SLOT(applyThermalState(DeviceState::Thermal::ThermalState))), true);
    QCOMPARE(disconnect(thermalNotifier->m_displayState, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)), thermalNotifier, SLOT(applyDisplayState(DeviceState::DisplayStateMonitor::DisplayState))), true);
}

void Ut_ThermalNotifier::testThermalState()
{
    thermalNotifier->applyThermalState(DeviceState::Thermal::Warning);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 1);
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_CATEGORY).toString(), QString("x-nemo.battery.temperature"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_PREVIEW_BODY).toString(), qtTrId("qtn_shut_high_temp_warning"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QString>(2), QString());

    thermalNotifier->applyThermalState(DeviceState::Thermal::Alert);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 2);
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_CATEGORY).toString(), QString("x-nemo.battery.temperature"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_PREVIEW_BODY).toString(), qtTrId("qtn_shut_high_temp_alert"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QString>(2), QString());

    thermalNotifier->applyThermalState(DeviceState::Thermal::LowTemperatureWarning);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 3);
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_CATEGORY).toString(), QString("x-nemo.battery.temperature"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QVariantHash>(6).value(LipstickNotification::HINT_PREVIEW_BODY).toString(), qtTrId("qtn_shut_low_temp_warning"));
    QCOMPARE(gNotificationManagerStub->stubLastCallTo("Notify").parameter<QString>(2), QString());
}

void Ut_ThermalNotifier::testDisplayStateOffDoesNothing()
{
    gThermalStub->stubSetReturnValue("get", DeviceState::Thermal::Warning);

    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::Off);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 0);

    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::Dimmed);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 0);

    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::Unknown);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 0);
}

void Ut_ThermalNotifier::testDisplayStateOnAppliesThermalState()
{
    gDisplayStateMonitorStub->stubSetReturnValue("get", DeviceState::DisplayStateMonitor::On);
    gThermalStub->stubSetReturnValue("get", DeviceState::Thermal::Warning);

    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::On);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 1);

    // The same thermal state should not get shown again
    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::On);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 1);

    // The different thermal state should get shown
    gThermalStub->stubSetReturnValue("get", DeviceState::Thermal::Alert);
    thermalNotifier->applyDisplayState(DeviceState::DisplayStateMonitor::On);
    QCOMPARE(gNotificationManagerStub->stubCallCount("Notify"), 2);
}

QTEST_MAIN (Ut_ThermalNotifier)
