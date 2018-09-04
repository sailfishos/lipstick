/***************************************************************************
**
** Copyright (C) 2018 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
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

#include "lipstickwindow.h"
#include "homewindow.h"

LipstickWindow::LipstickWindow(QObject *parent)
    : QObject(parent)
    , m_window(nullptr)
{

}

LipstickWindow::~LipstickWindow()
{
    delete m_window;
    m_window = nullptr;
}

bool LipstickWindow::windowVisible() const
{
    return m_window && m_window->isVisible();
}

void LipstickWindow::setWindowVisible(bool visible)
{
    if (visible) {
        if (!m_window) {
            createWindow();
        }

        if (!m_window->isVisible()) {
            m_window->showFullScreen();
            emit windowVisibleChanged();
        }
    } else if (windowVisible()) {
        m_window->hide();
        emit windowVisibleChanged();
    }
}
