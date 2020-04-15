/***************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
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

#include "connectionselector.h"

#include <QGuiApplication>
#include "homewindow.h"
#include <QQuickItem>
#include <QQmlContext>
#include <QScreen>
#include <QTimer>
#include "utilities/closeeventeater.h"
#include "connectionselector.h"
#include "lipstickqmlpath.h"

ConnectionSelector::ConnectionSelector(QObject *parent) :
    QObject(parent),
    m_window(0)
{
    QTimer::singleShot(0, this, SLOT(createWindow()));
}

ConnectionSelector::~ConnectionSelector()
{
    delete m_window;
}

void ConnectionSelector::createWindow()
{
    m_window = new HomeWindow();
    m_window->setGeometry(QRect(QPoint(), QGuiApplication::primaryScreen()->size()));
    m_window->setCategory(QLatin1String("dialog"));
    m_window->setWindowTitle("Connection");
    m_window->setContextProperty("connectionSelector", this);
    m_window->setContextProperty("initialSize", QGuiApplication::primaryScreen()->size());
    m_window->setSource(QmlPath::to("connectivity/ConnectionSelector.qml"));
    m_window->installEventFilter(new CloseEventEater(this));
}

void ConnectionSelector::setWindowVisible(bool visible)
{
    if (visible) {
        if (!m_window->isVisible()) {
            m_window->showFullScreen();
            emit windowVisibleChanged();
        }
    } else if (m_window != 0 && m_window->isVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}

bool ConnectionSelector::windowVisible() const
{
    return m_window != 0 && m_window->isVisible();
}
