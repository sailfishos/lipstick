/***************************************************************************
**
** Copyright (c) 2012 - 2023 Jolla Ltd.
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
#include "thermalnotifier.h"

#include <dsme/thermalmanager_dbus_if.h>

#include <QDBusConnection>
#include <QDebug>

ThermalNotifier::ThermalNotifier(QObject *parent)
    : QObject(parent)
{
    QDBusConnection::systemBus().connect(QString(),
                                         thermalmanager_path,
                                         thermalmanager_interface,
                                         thermalmanager_state_change_ind,
                                         this,
                                         SLOT(applyThermalState(const QString&)));
}

ThermalNotifier::~ThermalNotifier()
{
    QDBusConnection::systemBus().disconnect(QString(),
                                            thermalmanager_path,
                                            thermalmanager_interface,
                                            thermalmanager_state_change_ind,
                                            this,
                                            SLOT(applyThermalState(const QString&)));
}

void ThermalNotifier::applyThermalState(const QString &state)
{
    if (state == thermalmanager_thermal_status_warning) {
        //% "Device is getting hot. Close all apps."
        publishTemperatureNotification(qtTrId("qtn_shut_high_temp_warning"));
    } else if (state == thermalmanager_thermal_status_alert) {
        //% "Device is overheating. turn it off."
        publishTemperatureNotification(qtTrId("qtn_shut_high_temp_alert"));
    } else if (state == thermalmanager_thermal_status_low) {
        //% "Low temperature warning"
        publishTemperatureNotification(qtTrId("qtn_shut_low_temp_warning"));
    }
}

void ThermalNotifier::publishTemperatureNotification(const QString &body)
{
    NotificationManager *manager = NotificationManager::instance();

    QVariantHash hints;
    hints.insert(LipstickNotification::HINT_URGENCY, LipstickNotification::Critical);
    hints.insert(LipstickNotification::HINT_TRANSIENT, true);
    hints.insert(LipstickNotification::HINT_FEEDBACK, "general_warning");

    manager->Notify(manager->systemApplicationName(),
                    0,
                    QLatin1String("icon-system-warning"),
                    QString(),
                    body,
                    QStringList(),
                    hints,
                    -1);
}
