/*!
 * @file displaystate.cpp
 * @brief DisplayStateMonitor

   <p>
   Copyright (c) 2009-2011 Nokia Corporation
   Copyright (c) 2015 - 2020 Jolla Ltd.
   Copyright (c) 2020 Open Mobile Platform LLC.

   @author Antonio Aloisio <antonio.aloisio@nokia.com>
   @author Ilya Dogolazky <ilya.dogolazky@nokia.com>
   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>
   @author Ustun Ergenoglu <ext-ustun.ergenoglu@nokia.com>
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
#include "displaystate.h"
#include "displaystate_p.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QMetaMethod>

namespace DeviceState {

DisplayStateMonitor::DisplayStateMonitor(QObject *parent)
    : QObject(parent)
    , d_ptr(new DisplayStateMonitorPrivate)
{
    Q_D(DisplayStateMonitor);

    connect(d, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)),
            this, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)));
}

DisplayStateMonitor::~DisplayStateMonitor()
{
    Q_D(DisplayStateMonitor);

    disconnect(d, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)),
               this, SIGNAL(displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState)));

    delete d_ptr;
}

void DisplayStateMonitor::connectNotify(const QMetaMethod &signal)
{
    Q_D(DisplayStateMonitor);

    /* QObject::connect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DisplayStateMonitor::displayStateChanged)) {
        if (0 == d->connectCount[SIGNAL_DISPLAY_STATE]) {
            QDBusConnection::systemBus().connect(MCE_SERVICE,
                                                 MCE_SIGNAL_PATH,
                                                 MCE_SIGNAL_IF,
                                                 MCE_DISPLAY_SIG,
                                                 d,
                                                 SLOT(slotDisplayStateChanged(QString)));

            QDBusConnection::systemBus().callWithCallback(
                        QDBusMessage::createMethodCall(
                            MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, MCE_DISPLAY_STATUS_GET),
                            d,
                            SLOT(slotDisplayStateChanged(QString)));
        }
        d->connectCount[SIGNAL_DISPLAY_STATE]++;
    }
}

void DisplayStateMonitor::disconnectNotify(const QMetaMethod &signal)
{
    Q_D(DisplayStateMonitor);

    /* QObject::disconnect() needs to be thread-safe */
    QMutexLocker locker(&d->connectMutex);

    if (signal == QMetaMethod::fromSignal(&DisplayStateMonitor::displayStateChanged)) {
        d->connectCount[SIGNAL_DISPLAY_STATE]--;

        if (0 == d->connectCount[SIGNAL_DISPLAY_STATE]) {
            QDBusConnection::systemBus().disconnect(MCE_SERVICE,
                                                    MCE_SIGNAL_PATH,
                                                    MCE_SIGNAL_IF,
                                                    MCE_DISPLAY_SIG,
                                                    d,
                                                    SLOT(slotDisplayStateChanged(QString)));
        }
    }
}

DisplayStateMonitor::DisplayState DisplayStateMonitor::get() const
{
    DisplayStateMonitor::DisplayState state = Unknown;
    QDBusReply<QString> displayStateReply
            = QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF,
                                               MCE_DISPLAY_STATUS_GET));
    if (!displayStateReply.isValid()) {
        return state;
    }

    QString stateStr = displayStateReply.value();

    if (stateStr == MCE_DISPLAY_DIM_STRING) {
        state = Dimmed;
    } else if (stateStr == MCE_DISPLAY_ON_STRING) {
        state = On;
    } else if (stateStr == MCE_DISPLAY_OFF_STRING) {
        state = Off;
    }
    return state;
}

bool DisplayStateMonitor::set(DisplayStateMonitor::DisplayState state)
{
    QString method;

    switch (state) {
        case Off:
            method = QString(MCE_DISPLAY_OFF_REQ);
            break;
        case Dimmed:
            method = QString(MCE_DISPLAY_DIM_REQ);
            break;
        case On:
            method = QString(MCE_DISPLAY_ON_REQ);
            break;
        default:
            return false;
    }

    QDBusMessage displayStateSetCall
            = QDBusMessage::createMethodCall(MCE_SERVICE, MCE_REQUEST_PATH, MCE_REQUEST_IF, method);
    QDBusConnection::systemBus().call(displayStateSetCall, QDBus::NoBlock);

    return true;
}

}
