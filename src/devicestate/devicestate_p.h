/*!
 * @file devicestate_p.h
 * @brief Contains DeviceStatePrivate

   <p>
   Copyright (c) 2009-2011 Nokia Corporation
   Copyright (c) 2015 - 2020 Jolla Ltd.
   Copyright (c) 2020 Open Mobile Platform LLC.

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
#ifndef DEVICESTATE_P_H
#define DEVICESTATE_P_H

#include "devicestate.h"
#include "dsme/dsme_dbus_if.h"

#include <QMutex>

class QDBusServiceWatcher;

#define SIGNAL_SYSTEM_STATE 0

namespace DeviceState
{

    class DeviceStatePrivate : public QObject
    {
        Q_OBJECT

    public:
        DeviceStatePrivate() {
            connectCount[SIGNAL_SYSTEM_STATE] = 0;
            userManagerWatcher = nullptr;
        }

        ~DeviceStatePrivate() {
        }

        QMutex connectMutex;
        size_t connectCount[1];
        QDBusServiceWatcher *userManagerWatcher;

    Q_SIGNALS:

        void systemStateChanged(DeviceState::DeviceState::StateIndication what);
        void nextUserChanged(uint uid);

    private Q_SLOTS:

        void emitShutdown() {
            emit systemStateChanged(DeviceState::Shutdown);
        }

        void emitThermalShutdown(QString thermalState) {
            // TODO: hardcoded "fatal"
            if (thermalState == "fatal") {
                emit systemStateChanged(DeviceState::ThermalStateFatal);
            }
        }

        void emitBatteryShutdown() {
            emit systemStateChanged(DeviceState::BatteryStateEmpty);
        }

        void emitSaveData() {
            emit systemStateChanged(DeviceState::SaveData);
        }

        void emitShutdownDenied(QString reqType, QString reason) {
            // XXX: Move hardcoded strings somewere else
            if (reason == "usb") {
                if (reqType == "shutdown") {
                    emit systemStateChanged(DeviceState::ShutdownDeniedUSB);
                } else if (reqType == "reboot") {
                    emit systemStateChanged(DeviceState::RebootDeniedUSB);
                }
            }
        }

        void emitStateChangeInd(QString state) {
            // TODO: hardcoded "REBOOT"
            if (state == "REBOOT") {
                emit systemStateChanged(DeviceState::Reboot);
            }
        }

        void emitUserSwitching(uint uid) {
            emit nextUserChanged(uid);
            emit systemStateChanged(DeviceState::UserSwitching);
        }

        void emitUserSwitchingFailed(uint uid) {
            Q_UNUSED(uid)
            emit systemStateChanged(DeviceState::UserSwitchingFailed);
        }
    };
}
#endif // DEVICESTATE_P_H
