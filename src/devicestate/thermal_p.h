/*!
 * @file thermal_p.h
 * @brief Contains ThermalPrivate

   <p>
   Copyright (c) 2009-2011 Nokia Corporation

   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>

   @scope Private

   This file is part of SystemSW QtAPI.

   SystemSW QtAPI is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   SystemSW QtAPI is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with SystemSW QtAPI.  If not, see <http://www.gnu.org/licenses/>.
   </p>
 */
#ifndef THERMAL_P_H
#define THERMAL_P_H

#include "thermal.h"
#include "ipcinterface_p.h"

#include <dsme/thermalmanager_dbus_if.h>

#include <QMutex>


#define SIGNAL_THERMAL_STATE 0

namespace DeviceState
{
    class ThermalPrivate : public QObject
    {
        Q_OBJECT

    public:
        ThermalPrivate() {
            If = new IPCInterface(thermalmanager_service,
                                  thermalmanager_path,
                                  thermalmanager_interface);

            connectCount[SIGNAL_THERMAL_STATE] = 0;
        }

        ~ThermalPrivate() {
            if (If) {
                delete If, If = 0;
            }
        }

        static Thermal::ThermalState stringToState(const QString& state) {
            Thermal::ThermalState mState = Thermal::Unknown;

            if (state == thermalmanager_thermal_status_normal) {
                mState = Thermal::Normal;
            } else if (state == thermalmanager_thermal_status_warning) {
                mState = Thermal::Warning;
            } else if (state == thermalmanager_thermal_status_alert) {
                mState = Thermal::Alert;
            } else if (state == thermalmanager_thermal_status_low) {
                mState = Thermal::LowTemperatureWarning;
            }
            return mState;
        }

        QMutex connectMutex;
        size_t connectCount[1];
        IPCInterface *If;

    Q_SIGNALS:
        void thermalChanged(DeviceState::Thermal::ThermalState);

    private Q_SLOTS:
        void thermalStateChanged(const QString &state) {
            emit thermalChanged(ThermalPrivate::stringToState(state));
        }
    };
}
#endif // THERMAL_P_H
