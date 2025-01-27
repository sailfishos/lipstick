
// This file is part of lipstick, a QML desktop library
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License version 2.1 as published by the Free Software Foundation
// and appearing in the file LICENSE.LGPL included in the packaging
// of this file.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// Copyright (c) 2012, Robin Burchell <robin+nemo@viroteck.net>
//

#include <QGuiApplication>
#include <QScreen>
#include <QDebug>
#include <MDConfItem>

#include "screenlock/screenlock.h"
#include "homeapplication.h"
#include "lipsticksettings.h"

Q_GLOBAL_STATIC(LipstickSettings, settingsInstance)

LipstickSettings::LipstickSettings()
    : QObject()
    , m_screenLock(0)
{
}

LipstickSettings *LipstickSettings::instance()
{
    return settingsInstance();
}

void LipstickSettings::setScreenLock(ScreenLock *screenLock)
{
    // TODO: Disconnect from previous screenlock signals?

    m_screenLock = screenLock;
    connect(screenLock, SIGNAL(screenLockedChanged(bool)), this, SIGNAL(lockscreenVisibleChanged()));
    connect(screenLock, SIGNAL(lowPowerModeChanged()), this, SIGNAL(lowPowerModeChanged()));
    connect(screenLock, SIGNAL(blankingPolicyChanged(QString)), this, SIGNAL(blankingPolicyChanged()));
}

bool LipstickSettings::lockscreenVisible() const
{
    return m_screenLock != 0 ? m_screenLock->isScreenLocked() : false;
}

void LipstickSettings::setLockscreenVisible(bool lockscreenVisible)
{
    if (m_screenLock != 0 && lockscreenVisible != m_screenLock->isScreenLocked()) {
        if (lockscreenVisible) {
            m_screenLock->lockScreen();
        } else {
            m_screenLock->unlockScreen();
        }
    }
}

bool LipstickSettings::lowPowerMode() const
{
    return (m_screenLock && m_screenLock->isLowPowerMode());
}

void LipstickSettings::lockScreen(bool immediate)
{
    if (m_screenLock != 0 && (!m_screenLock->isScreenLocked() || immediate)) {
        m_screenLock->lockScreen(immediate);
    }
}

void LipstickSettings::setInteractionExpected(bool expected)
{
    if (m_screenLock != 0) {
        m_screenLock->setInteractionExpected(expected);
    }
}

QSize LipstickSettings::screenSize()
{
    return QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->size() : QSize();
}

void LipstickSettings::exportScreenProperties()
{
    const int defaultValue = 0;
    MDConfItem widthConf("/lipstick/screen/primary/width");
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (!primaryScreen) {
        qWarning() << Q_FUNC_INFO << "No screen found";
        return;
    }

    QSize primaryScreenSize = primaryScreen->size();
    if (widthConf.value(defaultValue) != primaryScreenSize.width()) {
        widthConf.set(primaryScreenSize.width());
        widthConf.sync();
    }
    MDConfItem heightConf("/lipstick/screen/primary/height");
    if (heightConf.value(defaultValue) != primaryScreenSize.height()) {
        heightConf.set(primaryScreenSize.height());
        heightConf.sync();
    }
    MDConfItem physicalDotsPerInchConf("/lipstick/screen/primary/physicalDotsPerInch");
    if (physicalDotsPerInchConf.value(defaultValue) != primaryScreen->physicalDotsPerInch()) {
        physicalDotsPerInchConf.set(primaryScreen->physicalDotsPerInch());
        physicalDotsPerInchConf.sync();
    }
    MDConfItem physicalDotsPerInchXConf("/lipstick/screen/primary/physicalDotsPerInchX");
    if (physicalDotsPerInchXConf.value(defaultValue) != primaryScreen->physicalDotsPerInchX()) {
        physicalDotsPerInchXConf.set(primaryScreen->physicalDotsPerInchX());
        physicalDotsPerInchXConf.sync();
    }
    MDConfItem physicalDotsPerInchYConf("/lipstick/screen/primary/physicalDotsPerInchY");
    if (physicalDotsPerInchYConf.value(defaultValue) != primaryScreen->physicalDotsPerInchY()) {
        physicalDotsPerInchYConf.set(primaryScreen->physicalDotsPerInchY());
        physicalDotsPerInchYConf.sync();
    }
}

QString LipstickSettings::blankingPolicy()
{
    if (m_screenLock) {
        return m_screenLock->blankingPolicy();
    }

    return "default";
}

