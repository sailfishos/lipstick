/*!
 * @file devicestate.cpp
 * @brief DeviceState

   <p>
   Copyright (c) 2009-2011 Nokia Corporation
   Copyright (c) 2015 - 2020 Jolla Ltd.
   Copyright (c) 2020 Open Mobile Platform LLC.

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
    : QObject(parent)
    , d_ptr(new DeviceStatePrivate)
{
    Q_D(DeviceState);

    connect(d, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)),
            this, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)));
    connect(d, SIGNAL(nextUserChanged(uint)),
            this, SIGNAL(nextUserChanged(uint)));
}

DeviceState::~DeviceState()
{
    Q_D(DeviceState);

    disconnect(d, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)),
               this, SIGNAL(systemStateChanged(DeviceState::DeviceState::StateIndication)));
    disconnect(d, SIGNAL(nextUserChanged(uint)),
               this, SIGNAL(nextUserChanged(uint)));

    delete d_ptr;
}

void DeviceState::connectNotify(const QMetaMethod &signal)
{
    Q_D(DeviceState);

    /* QObject::connect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DeviceState::systemStateChanged)) {
        if (0 == d->connectCount[SIGNAL_SYSTEM_STATE]) {
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_shutdown_ind,
                                                 d,
                                                 SLOT(emitShutdown()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_save_unsaved_data_ind,
                                                 d,
                                                 SLOT(emitSaveData()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_battery_empty_ind,
                                                 d,
                                                 SLOT(emitBatteryShutdown()));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_state_req_denied_ind,
                                                 d,
                                                 SLOT(emitShutdownDenied(QString, QString)));
            QDBusConnection::systemBus().connect(dsme_service,
                                                 dsme_sig_path,
                                                 dsme_sig_interface,
                                                 dsme_state_change_ind,
                                                 d,
                                                 SLOT(emitStateChangeInd(QString)));
            QDBusConnection::systemBus().connect(thermalmanager_service,
                                                 thermalmanager_path,
                                                 thermalmanager_interface,
                                                 thermalmanager_state_change_ind,
                                                 d,
                                                 SLOT(emitThermalShutdown(QString)));
            d->userManagerWatcher = new QDBusServiceWatcher(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                                               QDBusConnection::systemBus(),
                                                               QDBusServiceWatcher::WatchForRegistration
                                                               | QDBusServiceWatcher::WatchForUnregistration,
                                                               this);
            connect(d->userManagerWatcher, &QDBusServiceWatcher::serviceRegistered,
                    this, &DeviceState::connectUserManager);
            connect(d->userManagerWatcher, &QDBusServiceWatcher::serviceUnregistered,
                    this, &DeviceState::disconnectUserManager);
            if (QDBusConnection::systemBus().interface()->isServiceRegistered(SAILFISH_USERMANAGER_DBUS_INTERFACE))
                connectUserManager();
        }
        d->connectCount[SIGNAL_SYSTEM_STATE]++;
    }
}

void DeviceState::disconnectNotify(const QMetaMethod &signal)
{
    Q_D(DeviceState);

    /* QObject::disconnect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DeviceState::systemStateChanged)) {
        d->connectCount[SIGNAL_SYSTEM_STATE]--;

        if (0 == d->connectCount[SIGNAL_SYSTEM_STATE]) {
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_shutdown_ind,
                                                    d,
                                                    SLOT(emitShutdown()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_save_unsaved_data_ind,
                                                    d,
                                                    SLOT(emitSaveData()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_battery_empty_ind,
                                                    d,
                                                    SLOT(emitBatteryShutdown()));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_state_req_denied_ind,
                                                    d,
                                                    SLOT(emitShutdownDenied(QString, QString)));
            QDBusConnection::systemBus().disconnect(dsme_service,
                                                    dsme_sig_path,
                                                    dsme_sig_interface,
                                                    dsme_state_change_ind,
                                                    d,
                                                    SLOT(emitStateChangeInd(QString)));
            QDBusConnection::systemBus().disconnect(thermalmanager_service,
                                                    thermalmanager_path,
                                                    thermalmanager_interface,
                                                    thermalmanager_state_change_ind,
                                                    d,
                                                    SLOT(emitThermalShutdown(QString)));
            disconnectUserManager();
            d->userManagerWatcher->deleteLater();
            d->userManagerWatcher = nullptr;
        }
    }
}

void DeviceState::connectUserManager()
{
    Q_D(DeviceState);
    QDBusConnection::systemBus().connect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                         SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                         SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                         "aboutToChangeCurrentUser",
                                         d,
                                         SLOT(emitUserSwitching(uint)));
    QDBusConnection::systemBus().connect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                         SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                         SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                         "currentUserChangeFailed",
                                         d,
                                         SLOT(emitUserSwitchingFailed(uint)));
}

void DeviceState::disconnectUserManager()
{
    Q_D(DeviceState);
    QDBusConnection::systemBus().disconnect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                            SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            "aboutToChangeCurrentUser",
                                            d,
                                            SLOT(emitUserSwitching(uint)));
    QDBusConnection::systemBus().disconnect(SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            SAILFISH_USERMANAGER_DBUS_OBJECT_PATH,
                                            SAILFISH_USERMANAGER_DBUS_INTERFACE,
                                            "currentUserChangeFailed",
                                            d,
                                            SLOT(emitUserSwitchingFailed(uint)));
}

} // DeviceState namespace
