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

#include "pulseaudiocontrol.h"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusArgument>
#include <QDBusServiceWatcher>
#include <QTimer>
#include <QDebug>

static const QString VOLUME_SERVICE = "com.Meego.MainVolume2";
static const QString VOLUME_PATH = "/com/meego/mainvolume2";
static const QString VOLUME_INTERFACE = "com.Meego.MainVolume2";

static const QString PA_BUS_NAME= "pulse_bus";

static const quint32 PA_RECONNECT_TIMEOUT_MS (2000);

PulseAudioControl::PulseAudioControl(QObject *parent) :
    QObject(parent),
    m_reconnectTimeout(PA_RECONNECT_TIMEOUT_MS),
    m_serviceWatcher(0)
{
}

PulseAudioControl::~PulseAudioControl()
{
    delete m_dbusConnection;
}

void PulseAudioControl::pulseRegistered(const QString &service)
{
    Q_UNUSED(service);
    openConnection();
}

void PulseAudioControl::pulseUnregistered(const QString &service)
{
    Q_UNUSED(service);
    delete m_dbusConnection;
    m_dbusConnection = nullptr;
    m_reconnectTimeout = PA_RECONNECT_TIMEOUT_MS;
}

void PulseAudioControl::openConnection()
{
    // For the first time connection is opened connect to session bus
    // for tracking PulseAudio server state and to do the peer to peer
    // address lookup later
    if (!m_serviceWatcher) {
        m_serviceWatcher = new QDBusServiceWatcher(QStringLiteral("org.pulseaudio.Server"),
                                                   QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForRegistration
                                                   | QDBusServiceWatcher::WatchForUnregistration,
                                                   this);

        connect(m_serviceWatcher, SIGNAL(serviceRegistered(const QString&)),
                this, SLOT(pulseRegistered(const QString&)));
        connect(m_serviceWatcher, SIGNAL(serviceUnregistered(const QString&)),
                this, SLOT(pulseUnregistered(const QString&)));
    }

    // If the connection already exists, do nothing
    if (m_dbusConnection) {
        return;
    }

    // Establish a connection to the server
    QByteArray paBusAddress = qgetenv("PULSE_DBUS_SERVER");
    QByteArray addressArray;
    if (paBusAddress.isEmpty()) {
        QDBusMessage message = QDBusMessage::createMethodCall("org.pulseaudio.Server",
                                                              "/org/pulseaudio/server_lookup1",
                                                              "org.freedesktop.DBus.Properties", "Get");
        message.setArguments(QVariantList() << "org.PulseAudio.ServerLookup1" << "Address");
        QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() > 0) {
            addressArray = reply.arguments().first().value<QDBusVariant>().variant().toString().toLatin1();
            paBusAddress = addressArray.data();
        }
    }

    m_dbusConnection = new QDBusConnection(QDBusConnection::connectToPeer(paBusAddress, PA_BUS_NAME));
    addSignalMatch();

    if (!m_dbusConnection->isConnected()) {
        QTimer::singleShot(m_reconnectTimeout, this, &PulseAudioControl::update);
        m_reconnectTimeout += PA_RECONNECT_TIMEOUT_MS; // next reconnect waits for reconnect timeout longer
    }
}

void PulseAudioControl::update()
{
    openConnection();

    if (!m_dbusConnection || !m_dbusConnection->isConnected()) {
        return;
    }
    QDBusMessage message = QDBusMessage::createMethodCall(VOLUME_SERVICE,
                                                          VOLUME_PATH,
                                                          "org.freedesktop.DBus.Properties",
                                                          "GetAll");
    message.setArguments(QList<QVariant>{VOLUME_INTERFACE});
    QDBusMessage reply = m_dbusConnection->call(message, QDBus::Block);

    int currentStep = -1, stepCount = -1, highVolumeStep = -1;
    QString mediaState;

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Error fetching pulseaudio state" << reply.errorMessage();
    } else if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().length() > 0) {
        const QDBusArgument &dbusArg = reply.arguments().at(0).value<QDBusArgument>();
        dbusArg.beginMap();

        // Recurse into the array [array of dicts]
        while (!dbusArg.atEnd()) {
            dbusArg.beginMapEntry();
            QString key;
            QVariant value;
            dbusArg >> key;
            dbusArg >> value;

            if (key == "StepCount") {
                stepCount = value.toInt();
            } else if (key == "CurrentStep") {
                currentStep = value.toInt();
            } else if (key == "HighVolumeStep") {
                highVolumeStep = value.toInt();
            } else if (key == "MediaState") {
                mediaState = value.toString();
            }

            dbusArg.endMapEntry();
        }
        dbusArg.endMap();
    }

    if (currentStep != -1 && stepCount != -1) {
        setSteps(currentStep, stepCount);
    }

    if (highVolumeStep != -1) {
        emit highVolume(highVolumeStep);
    }

    if (!mediaState.isEmpty()) {
        emit mediaStateChanged(mediaState);
    }
}

void PulseAudioControl::addSignalMatch()
{
    if (!m_dbusConnection) {
        return;
    }

    QVector<QString> signalNames = {"com.Meego.MainVolume2.StepsUpdated", "com.Meego.MainVolume2.NotifyHighVolume",
                                    "com.Meego.MainVolume2.NotifyListeningTime", "com.Meego.MainVolume2.CallStateChanged",
                                    "com.Meego.MainVolume2.MediaStateChanged"};
    for (QString signalName : signalNames) {
        QDBusMessage message = QDBusMessage::createMethodCall(QString(),
                                                              "/org/pulseaudio/core1",
                                                              QString(),
                                                              "ListenForSignal");

        message << signalName;
        message << QVariant::fromValue(QList<QDBusObjectPath>());
        m_dbusConnection->call(message, QDBus::NoBlock);
    }

    m_dbusConnection->connect(
        QString(),
        QString("/org/pulseaudio/core1"),
        QString("com.Meego.MainVolume2"),
        QString("StepsUpdated"),
        this,
        SLOT(setSteps(quint32,quint32)));
    m_dbusConnection->connect(
        QString(),
        QString("/org/pulseaudio/core1"),
        QString("com.Meego.MainVolume2"),
        QString("NotifyHighVolume"),
        this,
        SLOT(setHighestSafeVolume(quint32)));
    m_dbusConnection->connect(
        QString(),
        QString("/org/pulseaudio/core1"),
        QString("com.Meego.MainVolume2"),
        QString("NotifyListeningTime"),
        this,
        SLOT(setListeningTime(quint32)));
    m_dbusConnection->connect(
        QString(),
        QString("/org/pulseaudio/core1"),
        QString("com.Meego.MainVolume2"),
        QString("CallStateChanged"),
        this,
        SIGNAL(callActiveChanged(bool)));
    m_dbusConnection->connect(
        QString(),
        QString("/org/pulseaudio/core1"),
        QString("com.Meego.MainVolume2"),
        QString("MediaStateChanged"),
        this,
        SIGNAL(mediaStateChanged(QString)));
}

void PulseAudioControl::setSteps(quint32 currentStep, quint32 stepCount)
{
    // The pulseaudio API reports the step count (starting from 0), so the maximum volume is stepCount - 1
    emit volumeChanged(currentStep, stepCount - 1);
}

void PulseAudioControl::setVolume(int volume)
{
    // Check the connection, maybe PulseAudio restarted meanwhile
    this->openConnection();

    // Don't try to set the volume via D-bus when it isn't available
    if (!m_dbusConnection || !m_dbusConnection->isConnected()) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(VOLUME_SERVICE,
                                                          VOLUME_PATH,
                                                          "org.freedesktop.DBus.Properties",
                                                          "Set");
    message << VOLUME_INTERFACE
            << "CurrentStep"
            << volume;
    m_dbusConnection->call(message, QDBus::NoBlock);
}

void PulseAudioControl::setHighestSafeVolume(quint32 highestSafeVolume)
{
    this->highVolume(highestSafeVolume);
}

void PulseAudioControl::setListeningTime(quint32 listeningTime)
{
    this->longListeningTime(listeningTime);
}
