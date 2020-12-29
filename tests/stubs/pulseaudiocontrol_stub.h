/***************************************************************************
**
** Copyright (c) 2012 Jolla Ltd.
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
#ifndef PULSEAUDIOCONTROL_STUB
#define PULSEAUDIOCONTROL_STUB

#include "pulseaudiocontrol.h"
#include <stubbase.h>


// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class PulseAudioControlStub : public StubBase
{
public:
    virtual void PulseAudioControlConstructor(QObject *parent);
    virtual void PulseAudioControlDestructor();
    virtual void update();
    virtual void setVolume(int volume);
    virtual void pulseRegistered(const QString &service);
    virtual void pulseUnregistered(const QString &service);
    virtual void setSteps(quint32 currentStep, quint32 stepCount);
    virtual void setHighestSafeVolume(quint32 highestSafeVolume);
    virtual void setListeningTime(quint32 listeningTime);
};

// 2. IMPLEMENT STUB
void PulseAudioControlStub::PulseAudioControlConstructor(QObject *parent)
{
    Q_UNUSED(parent);

}
void PulseAudioControlStub::PulseAudioControlDestructor()
{

}
void PulseAudioControlStub::update()
{
    stubMethodEntered("update");
}

void PulseAudioControlStub::setVolume(int volume)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<int >(volume));
    stubMethodEntered("setVolume", params);
}

void PulseAudioControlStub::pulseRegistered(const QString &service)
{
    Q_UNUSED(service);
    stubMethodEntered("pulseRegistered");
}

void PulseAudioControlStub::pulseUnregistered(const QString &service)
{
    Q_UNUSED(service);
    stubMethodEntered("pulseUnregistered");
}

void PulseAudioControlStub::setSteps(quint32 currentStep, quint32 stepCount)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<quint32 >(currentStep));
    params.append( new Parameter<quint32 >(stepCount));
    stubMethodEntered("setSteps", params);
}

void PulseAudioControlStub::setHighestSafeVolume(quint32 highestSafeVolume)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<quint32 >(highestSafeVolume));
    stubMethodEntered("setHighestsafevolume", params);
}

void PulseAudioControlStub::setListeningTime(quint32 listeningTime)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<quint32 >(listeningTime));
    stubMethodEntered("setListeningtime", params);
}



// 3. CREATE A STUB INSTANCE
PulseAudioControlStub gDefaultPulseAudioControlStub;
PulseAudioControlStub *gPulseAudioControlStub = &gDefaultPulseAudioControlStub;


// 4. CREATE A PROXY WHICH CALLS THE STUB
PulseAudioControl::PulseAudioControl(QObject *parent)
{
    gPulseAudioControlStub->PulseAudioControlConstructor(parent);
}

PulseAudioControl::~PulseAudioControl()
{
    gPulseAudioControlStub->PulseAudioControlDestructor();
}

void PulseAudioControl::update()
{
    gPulseAudioControlStub->update();
}

void PulseAudioControl::setVolume(int volume)
{
    gPulseAudioControlStub->setVolume(volume);
}

void PulseAudioControl::pulseRegistered(const QString &service)
{
    gPulseAudioControlStub->pulseRegistered(service);
}
void PulseAudioControl::pulseUnregistered(const QString &service)
{
    gPulseAudioControlStub->pulseUnregistered(service);
}

void PulseAudioControl::setSteps(quint32 currentStep, quint32 stepCount)
{
    gPulseAudioControlStub->setSteps(currentStep, stepCount);
}

void PulseAudioControl::setHighestSafeVolume(quint32 highestSafeVolume)
{
    gPulseAudioControlStub->setHighestSafeVolume(highestSafeVolume);
}

void PulseAudioControl::setListeningTime(quint32 listeningTime)
{
    gPulseAudioControlStub->setListeningTime(listeningTime);
}


#endif
