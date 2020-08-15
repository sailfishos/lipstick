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
#ifndef THERMALNOTIFIER_H
#define THERMALNOTIFIER_H

#include <thermal.h>
#include <displaystate.h>

class ThermalNotifier : public QObject
{
    Q_OBJECT
public:
    explicit ThermalNotifier(QObject *parent = 0);

    /*!
     * Shows a system notification.
     *
     * \param body the body text of the notification
     */
    static void publishTemperatureNotification(const QString &body);

private slots:
    /*!
     * Reacts to thermal state changes by showing the
     * related notification.
     *
     * \param state the new thermal state
     */
    void applyThermalState(DeviceState::Thermal::ThermalState state);

    /*!
     * Reacts to display state changes by showing the
     * related notification if not displayed yet.
     *
     * \param state the new display state
     */
    void applyDisplayState(DeviceState::DisplayStateMonitor::DisplayState state);

private:
    //! For getting the thermal state
    DeviceState::Thermal *m_thermalState;

    //! For getting the display state
    DeviceState::DisplayStateMonitor *m_displayState;

    //! Thermal state for which a notification has been displayed while the screen was on
    DeviceState::Thermal::ThermalState m_thermalStateNotifiedWhileScreenIsOn;

#ifdef UNIT_TEST
    friend class Ut_ThermalNotifier;
#endif
};

#endif // THERMALNOTIFIER_H
