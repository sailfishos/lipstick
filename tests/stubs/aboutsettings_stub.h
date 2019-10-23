/***************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
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

#ifndef ABOUTSETTINGS_H_STUB
#define ABOUTSETTINGS_H_STUB

#include "aboutsettings.h"
#include <stubbase.h>

static const auto TotalDiskSpace = QStringLiteral("totalDiskSpace");
static const auto AvailableDiskSpace = QStringLiteral("availableDiskSpace");
static const auto DiskUsageModel = QStringLiteral("diskUsageModel");
static const auto RefreshStorageModels = QStringLiteral("refreshStorageModels");

static const auto WlanMacAddress = QStringLiteral("wlanMacAddress");
static const auto Imei = QStringLiteral("imei");
static const auto Serial = QStringLiteral("serial");
static const auto LocalizedOperatingSystemName = QStringLiteral("localizedOperatingSystemName");
static const auto OperatingSystemName = QStringLiteral("operatingSystemName");
static const auto LocalizedSoftwareVersion = QStringLiteral("localizedSoftwareVersion");
static const auto BaseOperatingSystemName = QStringLiteral("baseOperatingSystemName");
static const auto SoftwareVersion = QStringLiteral("softwareVersion");
static const auto SoftwareVersionId = QStringLiteral("softwareVersionId");
static const auto AdaptationVersion = QStringLiteral("adaptationVersion");
static const auto VendorName = QStringLiteral("vendorName");
static const auto VendorVersion = QStringLiteral("vendorVersion");

class AboutSettingsStub : public StubBase
{
public:
    virtual void AboutSettingsConstructor(QObject *parent);
    virtual void AboutSettingsDestructor();
    virtual QString operatingSystemName() const;
    virtual QString localizedSoftwareVersion() const;

    virtual qlonglong totalDiskSpace() const;
    virtual qlonglong availableDiskSpace() const;
    virtual QVariant diskUsageModel() const;
    virtual void refreshStorageModels();

    virtual QString wlanMacAddress() const;
    virtual QString imei() const;
    virtual QString serial() const;
    virtual QString localizedOperatingSystemName() const;
    virtual QString baseOperatingSystemName() const;
    virtual QString softwareVersion() const;
    virtual QString softwareVersionId() const;
    virtual QString adaptationVersion() const;

    virtual QString vendorName() const;
    virtual QString vendorVersion() const;
};


void AboutSettingsStub::AboutSettingsConstructor(QObject *parent)
{
    Q_UNUSED(parent)
}

void AboutSettingsStub::AboutSettingsDestructor()
{

}

QString AboutSettingsStub::operatingSystemName() const
{
    stubMethodEntered(OperatingSystemName);
    return stubReturnValue<QString>(OperatingSystemName);
}

QString AboutSettingsStub::localizedSoftwareVersion() const
{
    stubMethodEntered(LocalizedSoftwareVersion);
    return stubReturnValue<QString>(LocalizedSoftwareVersion);
}

qlonglong AboutSettingsStub::totalDiskSpace() const
{
    stubMethodEntered(TotalDiskSpace);
    return stubReturnValue<qlonglong>(TotalDiskSpace);
}

qlonglong AboutSettingsStub::availableDiskSpace() const
{
    stubMethodEntered(AvailableDiskSpace);
    return stubReturnValue<qlonglong>(AvailableDiskSpace);
}

QVariant AboutSettingsStub::diskUsageModel() const
{
    stubMethodEntered(DiskUsageModel);
    return stubReturnValue<QVariant>(DiskUsageModel);
}

void AboutSettingsStub::refreshStorageModels()
{
    // Do nothing
}

QString AboutSettingsStub::wlanMacAddress() const
{
    stubMethodEntered(WlanMacAddress);
    return stubReturnValue<QString>(WlanMacAddress);
}

QString AboutSettingsStub::imei() const
{
    stubMethodEntered(Imei);
    return stubReturnValue<QString>(Imei);
}

QString AboutSettingsStub::serial() const
{
    stubMethodEntered(Serial);
    return stubReturnValue<QString>(Serial);
}

QString AboutSettingsStub::localizedOperatingSystemName() const
{
    stubMethodEntered(LocalizedOperatingSystemName);
    return stubReturnValue<QString>(LocalizedOperatingSystemName);
}

QString AboutSettingsStub::baseOperatingSystemName() const
{
    stubMethodEntered(BaseOperatingSystemName);
    return stubReturnValue<QString>(BaseOperatingSystemName);
}

QString AboutSettingsStub::softwareVersion() const
{
    stubMethodEntered(SoftwareVersion);
    return stubReturnValue<QString>(SoftwareVersion);
}

QString AboutSettingsStub::softwareVersionId() const
{
    stubMethodEntered(SoftwareVersionId);
    return stubReturnValue<QString>(SoftwareVersionId);
}

QString AboutSettingsStub::adaptationVersion() const
{
    stubMethodEntered(AdaptationVersion);
    return stubReturnValue<QString>(AdaptationVersion);
}

QString AboutSettingsStub::vendorName() const
{
    stubMethodEntered(VendorName);
    return stubReturnValue<QString>(VendorName);
}

QString AboutSettingsStub::vendorVersion() const
{
    stubMethodEntered(VendorVersion);
    return stubReturnValue<QString>(VendorVersion);
}

AboutSettingsStub gDefaultAboutSettingsStub;
AboutSettingsStub *gAboutSettingsStub = &gDefaultAboutSettingsStub;

AboutSettings::AboutSettings(QObject *parent)
{
    gAboutSettingsStub->AboutSettingsConstructor(parent);
}

AboutSettings::~AboutSettings()
{
    gAboutSettingsStub->AboutSettingsDestructor();
}

QString AboutSettings::operatingSystemName() const
{
    return gAboutSettingsStub->operatingSystemName();
}

QString AboutSettings::localizedSoftwareVersion() const
{
    return gAboutSettingsStub->localizedSoftwareVersion();
}

qlonglong AboutSettings::totalDiskSpace() const
{
    return gAboutSettingsStub->totalDiskSpace();
}

qlonglong AboutSettings::availableDiskSpace() const
{
    return gAboutSettingsStub->availableDiskSpace();
}

QVariant AboutSettings::diskUsageModel() const
{
    return gAboutSettingsStub->diskUsageModel();
}

void AboutSettings::refreshStorageModels()
{
    gAboutSettingsStub->refreshStorageModels();
}

QString AboutSettings::wlanMacAddress() const
{
    return gAboutSettingsStub->wlanMacAddress();
}

QString AboutSettings::imei() const
{
    return gAboutSettingsStub->imei();
}

QString AboutSettings::serial() const
{
    return gAboutSettingsStub->serial();
}

QString AboutSettings::localizedOperatingSystemName() const
{
    return gAboutSettingsStub->localizedOperatingSystemName();
}

QString AboutSettings::baseOperatingSystemName() const
{
    return gAboutSettingsStub->baseOperatingSystemName();
}

QString AboutSettings::softwareVersion() const
{
    return gAboutSettingsStub->softwareVersion();
}

QString AboutSettings::softwareVersionId() const
{
    return gAboutSettingsStub->softwareVersionId();
}

QString AboutSettings::adaptationVersion() const
{
    return gAboutSettingsStub->adaptationVersion();
}

QString AboutSettings::vendorName() const
{
    return gAboutSettingsStub->vendorName();
}

QString AboutSettings::vendorVersion() const
{
    return gAboutSettingsStub->vendorVersion();
}

#endif
