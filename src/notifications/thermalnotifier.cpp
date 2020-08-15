/***************************************************************************
**
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
#include "notifications/notificationmanager.h"
#include "notifications/lipsticknotification.h"
#include "homeapplication.h"
#include "thermalnotifier.h"

ThermalNotifier::ThermalNotifier(QObject *parent) :
    QObject(parent),
    m_thermalState(new DeviceState::Thermal(this)),
    m_displayState(new DeviceState::DisplayStateMonitor(this)),
    m_thermalStateNotifiedWhileScreenIsOn(DeviceState::Thermal::Normal)
{
    connect(m_thermalState, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)), this, SLOT(applyThermalState(DeviceState::Thermal::ThermalState)));
    connect(m_displayState, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)), this, SLOT(applyDisplayState(DeviceState::DisplayStateMonitor::DisplayState)));
}

void ThermalNotifier::applyThermalState(DeviceState::Thermal::ThermalState state)
{
    switch (state) {
    case DeviceState::Thermal::Warning:
        //% "Device is getting hot. Close all apps."
        publishTemperatureNotification(qtTrId("qtn_shut_high_temp_warning"));
        break;
    case DeviceState::Thermal::Alert:
        //% "Device is overheating. turn it off."
        publishTemperatureNotification(qtTrId("qtn_shut_high_temp_alert"));
        break;
    case DeviceState::Thermal::LowTemperatureWarning:
        //% "Low temperature warning"
        publishTemperatureNotification(qtTrId("qtn_shut_low_temp_warning"));
        break;
    default:
        break;
    }

    if (m_displayState->get() != DeviceState::DisplayStateMonitor::Off) {
        m_thermalStateNotifiedWhileScreenIsOn = state;
    }
}

void ThermalNotifier::applyDisplayState(DeviceState::DisplayStateMonitor::DisplayState state)
{
    if (state == DeviceState::DisplayStateMonitor::On) {
        DeviceState::Thermal::ThermalState currentThermalState = m_thermalState->get();
        if (m_thermalStateNotifiedWhileScreenIsOn != currentThermalState) {
            applyThermalState(currentThermalState);
        }
    }
}

void ThermalNotifier::publishTemperatureNotification(const QString &body)
{
    NotificationManager *manager = NotificationManager::instance();

    QVariantHash hints;
    hints.insert(LipstickNotification::HINT_APP_ICON, "icon-system-warning");
    hints.insert(LipstickNotification::HINT_URGENCY, LipstickNotification::Critical);
    hints.insert(LipstickNotification::HINT_TRANSIENT, true);
    hints.insert(LipstickNotification::HINT_FEEDBACK, "general_warning");

    manager->Notify(manager->systemApplicationName(),
                    0,
                    QString(),
                    QString(),
                    body,
                    QStringList(),
                    hints,
                    -1);
}
