/*!
 * @file thermal.h
 * @brief Contains Thermal which provides information on device thermal states.

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

   @scope Nokia Meego

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
#ifndef THERMAL_H
#define THERMAL_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

namespace DeviceState{

class ThermalPrivate;

/*!
 * @scope Nokia Meego
 *
 * @class Thermal
 * @brief Thermal provides information on device thermal states.
 */
class Thermal : public QObject
{
    Q_OBJECT
    Q_ENUMS(Thermal)
    Q_PROPERTY(ThermalState state READ get)

public:
    //! Possible thermal states - the state transitions are not necessarily linear
    enum ThermalState
    {
        Normal = 0, //!< Normal
        Warning,    //!< Warning state
        Alert,      //!< Alert state
        Unknown,    //!< State not known, just ignore !
        Error,      //!< State could not be retrieved (for get method only)
        LowTemperatureWarning     //!< Low temperature warning state
    };


public:
    /*!
     * @brief Constructor
     * @param parent The possible parent object
     */
    Thermal(QObject *parent = 0);

    /*!
     * @brief Destructor
     */
    ~Thermal();

    /*!
     * @brief Gets the current thermal state.
     * @return Current thermal state
     */
    ThermalState get() const;

Q_SIGNALS:
    /*!
     * @brief Sent when device thermal state has changed.
     * @param state Current thermal state
     */
    void thermalChanged(DeviceState::Thermal::ThermalState state);

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);

private:
    Q_DISABLE_COPY(Thermal)
    Q_DECLARE_PRIVATE(Thermal)
    ThermalPrivate *d_ptr;
};

} // DeviceState namespace

QT_END_HEADER

#endif // THERMAL_H
