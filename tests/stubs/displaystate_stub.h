/***************************************************************************
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2012 - 2020 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
**
** This file is part of lipstick.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/
#ifndef DISPLAYSTATE_STUB
#define DISPLAYSTATE_STUB

#include <displaystate.h>
#include <stubbase.h>


/*
 * XXX: This stub contains only those methods which are
 * used by the  DeviceModeBusinessLogic class ...
 */


/** DECLARE STUB */
class DisplayStateMonitorStub : public StubBase
{
public:
    // enum DisplayStateMonitor { Off=-1, Dimmed=0, On=1, Unknown } ;
    virtual void DisplayStateMonitorConstructor(QObject *parent);
    virtual void DisplayStateMonitorDestructor();
    virtual DeviceState::DisplayStateMonitor::DisplayState get() const;
    virtual bool set(DeviceState::DisplayStateMonitor::DisplayState state);
    virtual void connectNotify(const QMetaMethod &signal);
    virtual void disconnectNotify(const QMetaMethod &signal);

    DeviceState::DisplayStateMonitor *displayState;
};

// 2. IMPLEMENT STUB
void DisplayStateMonitorStub::DisplayStateMonitorConstructor(QObject *parent)
{
    Q_UNUSED(parent);

}
void DisplayStateMonitorStub::DisplayStateMonitorDestructor()
{

}
DeviceState::DisplayStateMonitor::DisplayState DisplayStateMonitorStub::get() const
{
    stubMethodEntered("get");
    return stubReturnValue<DeviceState::DisplayStateMonitor::DisplayState>("get");
}

bool DisplayStateMonitorStub::set(DeviceState::DisplayStateMonitor::DisplayState state)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<DeviceState::DisplayStateMonitor::DisplayState >(state));
    stubMethodEntered("set", params);
    return stubReturnValue<bool>("set");
}

void DisplayStateMonitorStub::connectNotify(const QMetaMethod &signal)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QMetaMethod &>(signal));
    stubMethodEntered("connectNotify", params);
}


void DisplayStateMonitorStub::disconnectNotify(const QMetaMethod &signal)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QMetaMethod &>(signal));
    stubMethodEntered("disconnectNotify", params);
}






// 3. CREATE A STUB INSTANCE
DisplayStateMonitorStub gDefaultDisplayStateMonitorStub;
DisplayStateMonitorStub *gDisplayStateMonitorStub = &gDefaultDisplayStateMonitorStub;

namespace DeviceState {

// 4. CREATE A PROXY WHICH CALLS THE STUB
DisplayStateMonitor::DisplayStateMonitor(QObject *parent)
{
    gDisplayStateMonitorStub->DisplayStateMonitorConstructor(parent);
    gDisplayStateMonitorStub->displayState = this;
}

DisplayStateMonitor::~DisplayStateMonitor()
{
    gDisplayStateMonitorStub->DisplayStateMonitorDestructor();
}

DeviceState::DisplayStateMonitor::DisplayState DisplayStateMonitor::get() const
{
    return gDisplayStateMonitorStub->get();
}

bool DisplayStateMonitor::set(DisplayState state)
{
    return gDisplayStateMonitorStub->set(state);
}

void DisplayStateMonitor::connectNotify(const QMetaMethod &signal)
{
    gDisplayStateMonitorStub->connectNotify(signal);
}
void DisplayStateMonitor::disconnectNotify(const QMetaMethod &signal)
{
    gDisplayStateMonitorStub->disconnectNotify(signal);
}

} // Namespace DeviceState

#endif
