/*!
 * @file ipcinterface_p.h
 * @brief Contains IPCInterface.

   <p>
   @copyright (C) 2009-2011 Nokia Corporation
   Copyright (c) 2015 - 2020 Jolla Ltd.
   Copyright (c) 2020 Open Mobile Platform LLC.
   @license LGPL Lesser General Public License

   @author Antonio Aloisio <antonio.aloisio@nokia.com>
   @author Ilya Dogolazky <ilya.dogolazky@nokia.com>
   @author Raimo Vuonnala <raimo.vuonnala@nokia.com>
   @author Timo Olkkonen <ext-timo.p.olkkonen@nokia.com>
   @author Timo Rongas <ext-timo.rongas.nokia.com>
   @author Matias Muhonen <ext-matias.muhonen@nokia.com>

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
#ifndef IPCINTERFACE_P_H
#define IPCINTERFACE_P_H

#include <QDBusAbstractInterface>

namespace DeviceState {

class IPCInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    IPCInterface(const char* service,
                   const char* path,
                   const char* interface,
                   QObject *parent = 0);
    virtual ~IPCInterface();

    QList<QVariant> get(const QString& method,
                        const QVariant& arg1 = QVariant(),
                        const QVariant& arg2 = QVariant());
    /*
     * Makes a non-blocking call. Note: the method does not give feedback
     * on success. Use the asyncCall method with QDBusPendingCall, if feedback
     * is needed.
     */
    void callAsynchronously(const QString& method,
                            const QVariant& arg1 = QVariant(),
                            const QVariant& arg2 = QVariant());
};

} // DeviceState namespace

#endif // IPCINTERFACE_P_H
