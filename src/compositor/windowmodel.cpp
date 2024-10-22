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

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include "lipstickcompositorwindow.h"
#include "lipstickcompositor.h"
#include "windowmodel.h"

WindowModel::WindowModel()
    : m_complete(false)
{
    LipstickCompositor *c = LipstickCompositor::instance();
    if (!c) {
        qWarning("WindowModel: Compositor must be created before WindowModel");
    } else {
        c->m_windowModels.append(this);
    }

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(LIPSTICK_DBUS_WINDOW_MODEL_PATH,
                        this, QDBusConnection::ExportAllSlots);
    dbus.registerService(LIPSTICK_DBUS_SERVICE_NAME);
}

WindowModel::~WindowModel()
{
    LipstickCompositor *c = LipstickCompositor::instance();
    if (c) c->m_windowModels.removeAll(this);
}

int WindowModel::itemCount() const
{
    return m_items.count();
}

int WindowModel::windowId(int index) const
{
    if (index < 0 || index >= m_items.count())
        return 0;
    return m_items.at(index);
}

int WindowModel::rowCount(const QModelIndex &) const
{
    return m_items.count();
}

QVariant WindowModel::data(const QModelIndex &index, int role) const
{
    int idx = index.row();
    if (idx < 0 || idx >= m_items.count())
        return QVariant();

    LipstickCompositor *c = LipstickCompositor::instance();
    if (role == Qt::UserRole + 1) {
        return m_items.at(idx);
    } else if (role == Qt::UserRole + 2) {
        LipstickCompositorWindow *w = static_cast<LipstickCompositorWindow *>(c->windowForId(m_items.at(idx)));

        return w ? w->processId() : 0;
    } else if (role == Qt::UserRole + 3) {
        LipstickCompositorWindow *w = static_cast<LipstickCompositorWindow *>(c->windowForId(m_items.at(idx)));
        return w->title();
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> WindowModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::UserRole + 1] = "window";
    roles[Qt::UserRole + 2] = "processId";
    roles[Qt::UserRole + 3] = "title";
    return roles;
}

void WindowModel::classBegin()
{
}

void WindowModel::componentComplete()
{
    m_complete = true;
    refresh();
}

/*!
    Reimplement this method to provide custom filtering.
*/
bool WindowModel::approveWindow(LipstickCompositorWindow *window)
{
    return window->isInProcess() == false
            && window->category() != QLatin1String("overlay");
}

void WindowModel::addItem(int id)
{
    if (!m_complete)
        return;

    LipstickCompositor *c = LipstickCompositor::instance();
    LipstickCompositorWindow *window = static_cast<LipstickCompositorWindow *>(c->windowForId(id));
    if (!approveWindow(window))
        return;

    beginInsertRows(QModelIndex(), m_items.count(), m_items.count());
    m_items.append(id);
    endInsertRows();
    emit itemAdded(m_items.count() - 1);
    emit itemCountChanged();
}

void WindowModel::remItem(int id)
{
    if (!m_complete)
        return;

    int idx = m_items.indexOf(id);
    if (idx == -1)
        return;

    beginRemoveRows(QModelIndex(), idx, idx);
    m_items.removeAt(idx);
    endRemoveRows();
    emit itemCountChanged();
}

void WindowModel::titleChanged(int id)
{
    if (!m_complete)
        return;

    int idx = m_items.indexOf(id);
    if (idx == -1)
        return;

    emit dataChanged(index(idx, 0), index(idx, 0));
}

void WindowModel::refresh()
{
    LipstickCompositor *compositor = LipstickCompositor::instance();
    if (!m_complete || !compositor)
        return;

    beginResetModel();

    m_items.clear();

    for (QHash<int, LipstickCompositorWindow *>::ConstIterator iter = compositor->m_mappedSurfaces.begin();
         iter != compositor->m_mappedSurfaces.end(); ++iter) {

        if (approveWindow(iter.value()))
            m_items.append(iter.key());
    }

    endResetModel();
}

bool WindowModel::isPrivileged() const
{
    if (!calledFromDBus()) {
        return true;
    }

    uint pid = connection().interface()->servicePid(message().service()).value();
    QFileInfo info(QString("/proc/%1").arg(pid));
    if (info.group() != QLatin1String("privileged") && info.owner() != QLatin1String("root")) {
        QString errorString = QString("PID %1 is not in privileged group").arg(pid);
        sendErrorReply(QDBusError::AccessDenied, errorString);
        return false;
    }
    return true;
}

// used by mapplauncherd to bring a binary to the front
void WindowModel::launchProcess(const QString &binaryName)
{
    LipstickCompositor *c = LipstickCompositor::instance();
    if (!m_complete || !c || !isPrivileged())
        return;

    QStringList binaryParts = binaryName.split(QRegExp(QRegExp("\\s+")));

    for (QHash<int, LipstickCompositorWindow *>::ConstIterator iter = c->m_mappedSurfaces.begin();
        iter != c->m_mappedSurfaces.end(); ++iter) {

        LipstickCompositorWindow *win = iter.value();
        if (!approveWindow(win))
            continue;

        QString pidFile = QString::fromLatin1("/proc/%1/cmdline").arg(win->processId());
        QFile f(pidFile);
        if (!f.open(QIODevice::ReadOnly)) {
            qWarning() << Q_FUNC_INFO << "Cannot open cmdline for " << pidFile;
            continue;
        }

        // Command line arguments are split by '\0' in /proc/*/cmdline
        QStringList proc;
        QByteArray data = f.readAll();
        Q_FOREACH (const QByteArray &array, data.split('\0')) {
            QString part = QString::fromUtf8(array);
            if (part.size() > 0) {
                proc << part;
            }
        }

        // Cannot match, as the cmdline has less arguments than then binary part
        if (binaryParts.size() > proc.size()) {
            continue;
        }

        bool match = true;

        // All parts of binaryName must be contained in this order in the
        // process command line to match the given process
        for (int i=0; i<binaryParts.count(); i++) {
            if (proc[i] != binaryParts[i]) {
                match = false;
                break;
            }
        }

        if (match) {
            win->surface()->raiseRequested();
            break;
        }
    }
}

