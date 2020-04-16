/*!
 * @file displaystate_p.h
 * @brief Contains DisplayStatePrivate

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
#ifndef DISPLAYSTATE_P_H
#define DISPLAYSTATE_P_H

#include "displaystate.h"

#include <QMutex>

#include "mce/dbus-names.h"
#include "mce/mode-names.h"

#define SIGNAL_DISPLAY_STATE 0

namespace DeviceState
{
    class DisplayStateMonitorPrivate : public QObject
    {
        Q_OBJECT;
        MEEGO_DECLARE_PUBLIC(DisplayStateMonitor)

    public:
        DisplayStateMonitorPrivate() {
            connectCount[SIGNAL_DISPLAY_STATE] = 0;
        }

        ~DisplayStateMonitorPrivate() {
        }

        QMutex connectMutex;
        size_t connectCount[1];

    Q_SIGNALS:
        void displayStateChanged(DeviceState::DisplayStateMonitor::DisplayState);

    private Q_SLOTS:

        void slotDisplayStateChanged(const QString& state) {
            if (state == MCE_DISPLAY_OFF_STRING)
                emit displayStateChanged(DisplayStateMonitor::Off);
            else if (state == MCE_DISPLAY_DIM_STRING)
                emit displayStateChanged(DisplayStateMonitor::Dimmed);
            else if (state == MCE_DISPLAY_ON_STRING)
                emit displayStateChanged(DisplayStateMonitor::On);
        }
    };
}
#endif // DISPLAYSTATE_P_H
