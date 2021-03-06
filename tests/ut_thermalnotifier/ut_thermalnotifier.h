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
#ifndef _UT_THERMALNOTIFIER_H
#define _UT_THERMALNOTIFIER_H

#include <QObject>

class ThermalNotifier;

class Ut_ThermalNotifier : public QObject
{
    Q_OBJECT

private slots:
    // Executed once before every test case
    void init();

    // Executed once after every test case
    void cleanup();

    // Executed once before first test case
    void initTestCase();

    // Executed once after last test case
    void cleanupTestCase();

    // Test cases
    void testConnections();
    void testThermalState();
    void testDisplayStateOffDoesNothing();
    void testDisplayStateOnAppliesThermalState();

private:
    ThermalNotifier *thermalNotifier;
};

#endif
