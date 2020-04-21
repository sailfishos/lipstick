/*!
 * @file ipcinterface.cpp
 * @brief IPCInterface

   <p>
   Copyright (c) 2009-2011 Nokia Corporation
   Copyright (c) 2015 - 2020 Jolla Ltd.
   Copyright (c) 2020 Open Mobile Platform LLC.

   @author Antonio Aloisio <antonio.aloisio@nokia.com>
   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>
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
#include "ipcinterface_p.h"

namespace DeviceState {

// Note: QDBusAbstractInterface is used instead of QDBusInterface for performance reasons --
// QDBusInterface uses blocking D-Bus call in constructor (http://bugreports.qt.nokia.com/browse/QTBUG-14485)
IPCInterface::IPCInterface(const char* service,
                               const char* path,
                               const char* interface,
                               QObject *parent)
              :  QDBusAbstractInterface(service,
                                        path,
                                        interface,
                                        QDBusConnection::systemBus(),
                                        parent) {
}

IPCInterface::~IPCInterface() {
}

void IPCInterface::callAsynchronously(const QString& method,
                                        const QVariant& arg1,
                                        const QVariant& arg2 ) {
    // As no feedback is needed on the D-Bus call, calling QDBusAbstractInterface
    // with QDBus::NoBlock is faster than calling asyncCall() with QDBusPendingCall.
    (void)call(QDBus::NoBlock, method, arg1, arg2);
}

QList<QVariant> IPCInterface::get(const QString& method,
                                    const QVariant& arg1,
                                    const QVariant& arg2) {
    QList<QVariant> results;
    QDBusMessage msg = call(method, arg1, arg2);
    if (msg.type() == QDBusMessage::ReplyMessage) {
        results  = msg.arguments();
    }
    return results;
}

} // Namespace DeviceState
