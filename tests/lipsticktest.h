/***************************************************************************
**
** Copyright (c) 2015 Jolla Ltd.
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

#ifndef LIPSTICK_TEST
#define LIPSTICK_TEST

#include <QtTest/QtTest>
#include <QString>
#include "homeapplication.h"

#define LIPSTICK_TEST_MAIN(TestObject) \
int main(int argc, char *argv[]) \
{ \
    HomeApplication app(argc, argv, QString()); \
    app.setAttribute(Qt::AA_Use96Dpi, true); \
    TestObject tc; \
    return QTest::qExec(&tc, argc, argv); \
}

#endif // LIPSTICK_TEST
