/*!
 * @file devicestate.cpp
 * @brief DeviceState

   <p>
   Copyright (c) 2009-2011 Nokia Corporation

   @author Antonio Aloisio <antonio.aloisio@nokia.com>
   @author Ilya Dogolazky <ilya.dogolazky@nokia.com>
   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>
   @author Tuomo Tanskanen <ext-tuomo.1.tanskanen@nokia.com>
   @author Simo Piiroinen <simo.piiroinen@nokia.com>
   @author Matias Muhonen <ext-matias.muhonen@nokia.com>

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
#include "devicestate.h"
#include "devicestate_p.h"

#include <dsme/thermalmanager_dbus_if.h>
#include <sailfishusermanagerinterface.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QMetaMethod>


namespace DeviceState {

DeviceState::DeviceState(QObject *parent)
             : QObject(parent) {
    MEEGO_INITIALIZE(DeviceState);

    connect(priv, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)),
            this, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)));
    connect(priv, SIGNAL(nextUserChanged(uint)),
            this, SIGNAL(nextUserChanged(uint)));
}

DeviceState::~DeviceState() {
    MEEGO_PRIVATE(DeviceState)

    disconnect(priv, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)),
               this, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)));
    disconnect(priv, SIGNAL(nextUserChanged(uint)),
               this, SIGNAL(nextUserChanged(uint)));

    MEEGO_UNINITIALIZE(DeviceState);
}

void DeviceState::connectNotify(const QMetaMethod &signal) {
    MEEGO_PRIVATE(DeviceState)

    /* QObject::connect() needs to be thread-safe */
    QMutexLocker locker(&priv->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DeviceState::systemStateChanged)) {
        if (0 == priv->connectCount[SIGNAL_SYSTEM_STATE]) {
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_shutdown_ind,
                                                 priv,
                                                 SLOT(emitShutdown()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_save_unsaved_data_ind,
                                                 priv,
                                                 SLOT(emitSaveData()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_battery_empty_ind,
                                                 priv,
                                                 SLOT(emitBatteryShutdown()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_state_req_denied_ind,
                                                 priv,
                                                 SLOT(emitShutdownDenied(QString, QString)));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_state_change_ind,
                                                 priv,
                                                 SLOT(emitStateChangeInd(QString)));
            QDBusConnection::systemBus().connect(thermalmanager_service,
                                                 thermalmanager_path,
                                                 thermalmanager_interface,
                                                 thermalmanager_state_change_ind,
                                                 priv,
                                                 SLOT(emitThermalShutdown(QString)));
            priv->userManagerWatcher = new QDBusServiceWatcher(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                                               QDBusConnection::systemBus(),
                                                               QDBusServiceWatcher::WatchForRegistration
                                                               | QDBusServiceWatcher::WatchForUnregistration,
                                                               this);
            connect(priv->userManagerWatcher, &QDBusServiceWatcher::serviceRegistered,
                    this, &DeviceState::connectUserManager);
            connect(priv->userManagerWatcher, &QDBusServiceWatcher::serviceUnregistered,
                    this, &DeviceState::disconnectUserManager);
            if (QDBusConnection::systemBus().interface()->isServiceRegistered(SAILFISH_USERMANAGER_DBUS_INTERFACE))
                connectUserManager();
        }
        priv->connectCount[SIGNAL_SYSTEM_STATE]++;
    }
}

void DeviceState::disconnectNotify(const QMetaMethod &signal) {
    MEEGO_PRIVATE(DeviceState)

    /* QObject::disconnect() needs to be thread-safe */
    QMutexLocker locker(&priv->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DeviceState::systemStateChanged)) {
        priv->connectCount[SIGNAL_SYSTEM_STATE]--;

        if (0 == priv->connectCount[SIGNAL_SYSTEM_STATE]) {
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_shutdown_ind,
                                                    priv,
                                                    SLOT(emitShutdown()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_save_unsaved_data_ind,
                                                    priv,
                                                    SLOT(emitSaveData()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_battery_empty_ind,
                                                    priv,
                                                    SLOT(emitBatteryShutdown()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_state_req_denied_ind,
                                                    priv,
                                                    SLOT(emitShutdownDenied(QString, QString)));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_state_change_ind,
                                                    priv,
                                                    SLOT(emitStateChangeInd(QString)));
            QDBusConnection::systemBus().disconnect(thermalmanager_service,
                                                    thermalmanager_path,
                                                    thermalmanager_interface,
                                                    thermalmanager_state_change_ind,
                                                    priv,
                                                    SLOT(emitThermalShutdown(QString)));
            disconnectUserManager();
            priv->userManagerWatcher->deleteLater();
            priv->userManagerWatcher = nullptr;
        }
    }
}

void DeviceState::connectUserManager()
{
    MEEGO_PRIVATE(DeviceState)
    QDBusConnection::systemBus().connect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                             SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                             SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                             "aboutToChangeCurrentUser",
                                             priv,
                                             SLOT(emitUserSwitching(uint)));
}

void DeviceState::disconnectUserManager()
{
    MEEGO_PRIVATE(DeviceState)
    QDBusConnection::systemBus().disconnect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                            SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            "aboutToChangeCurrentUser",
                                            priv,
                                            SLOT(emitUserChanging(uint)));
}

} // DeviceState namespace
