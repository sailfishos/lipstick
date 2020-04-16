/***************************************************************************
**
** Copyright (c) 2012-2014 Jolla Ltd.
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
#ifndef THERMAL_STUB
#define THERMAL_STUB

#include "thermal.h"
#include <stubbase.h>


// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class ThermalStub : public StubBase
{
public:
    virtual void ThermalConstructor(QObject *parent);
    virtual void ThermalDestructor();
    virtual DeviceState::Thermal::ThermalState get() const;
    virtual void connectNotify(const QMetaMethod &signal);
    virtual void disconnectNotify(const QMetaMethod &signal);
};

// 2. IMPLEMENT STUB
void ThermalStub::ThermalConstructor(QObject *parent)
{
    Q_UNUSED(parent);

}
void ThermalStub::ThermalDestructor()
{

}
DeviceState::Thermal::ThermalState ThermalStub::get() const
{
    stubMethodEntered("get");
    return stubReturnValue<DeviceState::Thermal::ThermalState>("get");
}
void ThermalStub::connectNotify(const QMetaMethod &signal)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QMetaMethod &>(signal));
    stubMethodEntered("connectNotify", params);
}
void ThermalStub::disconnectNotify(const QMetaMethod &signal)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QMetaMethod &>(signal));
    stubMethodEntered("disconnectNotify", params);
}



// 3. CREATE A STUB INSTANCE
ThermalStub gDefaultThermalStub;
ThermalStub *gThermalStub = &gDefaultThermalStub;

namespace DeviceState {

// 4. CREATE A PROXY WHICH CALLS THE STUB
Thermal::Thermal(QObject *parent)
{
    gThermalStub->ThermalConstructor(parent);
}

Thermal::~Thermal()
{
    gThermalStub->ThermalDestructor();
}

DeviceState::Thermal::ThermalState Thermal::get() const
{
    return gThermalStub->get();
}

void Thermal::connectNotify(const QMetaMethod &signal)
{
    gThermalStub->connectNotify(signal);
}

void Thermal::disconnectNotify(const QMetaMethod &signal)
{
    gThermalStub->disconnectNotify(signal);
}

}

#endif
