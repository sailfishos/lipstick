/*!
 * @file thermal.cpp
 * @brief Thermal

   <p>
   Copyright (c) 2009-2011 Nokia Corporation

   @author Antonio Aloisio <antonio.aloisio@nokia.com>
   @author Ilya Dogolazky <ilya.dogolazky@nokia.com>
   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>

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
#include "thermal.h"
#include "thermal_p.h"
#include <QMetaMethod>

namespace DeviceState {

Thermal::Thermal(QObject *parent)
             : QObject(parent)
             , d_ptr(new ThermalPrivate) {
    Q_D(Thermal);

    connect(d, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)),
            this, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)));
}

Thermal::~Thermal() {
    Q_D(Thermal);

    disconnect(d, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)),
               this, SIGNAL(thermalChanged(DeviceState::Thermal::ThermalState)));

    delete d_ptr;
}

void Thermal::connectNotify(const QMetaMethod &signal) {
    Q_D(Thermal);

    /* QObject::connect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&Thermal::thermalChanged)) {
        if (0 == d->connectCount[SIGNAL_THERMAL_STATE]) {
            QDBusConnection::systemBus().connect("",
                                                 thermalmanager_path,
                                                 thermalmanager_interface,
                                                 thermalmanager_state_change_ind,
                                                 d,
                                                 SLOT(thermalStateChanged(const QString&)));
        }
        d->connectCount[SIGNAL_THERMAL_STATE]++;
    }
}

void Thermal::disconnectNotify(const QMetaMethod &signal) {
    Q_D(Thermal);

    /* QObject::disconnect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&Thermal::thermalChanged)) {
        d->connectCount[SIGNAL_THERMAL_STATE]--;

        if (0 == d->connectCount[SIGNAL_THERMAL_STATE]) {
            QDBusConnection::systemBus().disconnect("",
                                                    thermalmanager_path,
                                                    thermalmanager_interface,
                                                    thermalmanager_state_change_ind,
                                                    d,
                                                    SLOT(thermalStateChanged(const QString&)));
        }
    }
}

Thermal::ThermalState Thermal::get() const {
    Q_D(const Thermal);
    QString state;
    QList<QVariant> resp;

    resp = d->If->get(thermalmanager_get_thermal_state);

    if (resp.isEmpty()) {
        return Error;
    }

    state = resp[0].toString();
    return ThermalPrivate::stringToState(state);
}

} // DeviceState namespace
