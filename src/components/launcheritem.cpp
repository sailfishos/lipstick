
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
// Copyright (c) 2011, Robin Burchell
// Copyright (c) 2012, Timur Kristóf <venemo@fedoraproject.org>

#include <QProcess>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QTimerEvent>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <mdesktopentry.h>

#ifdef HAVE_CONTENTACTION
#include <contentaction.h>
#endif

#include "launcheritem.h"
#include "launchermodel.h"

LauncherItem::LauncherItem(const QString &filePath, QObject *parent)
    : QObject(parent)
    , m_isLaunching(false)
    , m_isUpdating(false)
    , m_isTemporary(false)
    , m_packageName("")
    , m_updatingProgress(-1)
    , m_customTitle("")
    , m_customIconFilename("")
    , m_serial(0)
    , m_isBlacklisted(false)
{
    if (!filePath.isEmpty()) {
        setFilePath(filePath);
    }

    // TODO: match the PID of the window thumbnails with the launcher processes
    // Launching animation will be disabled if the window of the launched app shows up
}

LauncherItem::LauncherItem(const QString &packageName, const QString &label,
        const QString &iconPath, const QString &desktopFile, QObject *parent)
    : QObject(parent)
    , m_isLaunching(false)
    , m_isUpdating(false)
    , m_isTemporary(false)
    , m_packageName(packageName)
    , m_updatingProgress(-1)
    , m_customTitle(label)
    , m_customIconFilename(iconPath)
    , m_serial(0)
    , m_isBlacklisted(false)
{
    if (!desktopFile.isEmpty()) {
        setFilePath(desktopFile);
    }
}

LauncherItem::~LauncherItem()
{
}

LauncherModel::ItemType LauncherItem::type() const
{
    return LauncherModel::Application;
}

void LauncherItem::setFilePath(const QString &filePath)
{
    if (!filePath.isEmpty() && QFile(filePath).exists()) {
        m_desktopEntry = QSharedPointer<MDesktopEntry>(new MDesktopEntry(filePath));
    } else {
        m_desktopEntry.clear();
    }

    emit this->itemChanged();
}

QString LauncherItem::filePath() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->fileName() : QString();
}

QString LauncherItem::fileID() const
{
    if (m_desktopEntry.isNull()) {
        return QString();
    }

    // Retrieve the file ID according to
    // http://standards.freedesktop.org/desktop-entry-spec/latest/ape.html
    QRegularExpression re(".*applications/(.*.desktop)");
    QRegularExpressionMatch match = re.match(m_desktopEntry->fileName());
    if (!match.hasMatch()) {
        return filename();
    }

    QString id = match.captured(1);
    id.replace('/', '-');
    return id;
}

QString LauncherItem::filename() const
{
    QString filename = filePath();
    int sep = filename.lastIndexOf('/');
    if (sep == -1)
        return QString();

    return filename.mid(sep+1);
}

QString LauncherItem::exec() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->exec() : QString();
}

bool LauncherItem::dBusActivated() const
{
    return (!m_desktopEntry.isNull() && !m_desktopEntry->xMaemoService().isEmpty());
}

QString LauncherItem::title() const
{
    if (m_isTemporary) {
        return m_customTitle;
    }

    return !m_desktopEntry.isNull() ? m_desktopEntry->name() : QString();
}

QString LauncherItem::entryType() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->type() : QString();
}

QString LauncherItem::iconId() const
{
    if (!m_customIconFilename.isEmpty()) {
        return QString("%1#serial=%2").arg(m_customIconFilename).arg(m_serial);
    }

    return getOriginalIconId();
}

QStringList LauncherItem::desktopCategories() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->categories() : QStringList();
}

QStringList LauncherItem::mimeType() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->mimeType() : QStringList();
}

QString LauncherItem::titleUnlocalized() const
{
    if (m_isTemporary) {
        return m_customTitle;
    }

    return !m_desktopEntry.isNull() ? m_desktopEntry->nameUnlocalized() : QString();
}

bool LauncherItem::shouldDisplay() const
{
    if (m_desktopEntry.isNull()) {
        return m_isTemporary;
    } else {
        return !m_desktopEntry->noDisplay() && !m_desktopEntry->notShowIn().contains(QStringLiteral("X-MeeGo"));
    }
}

bool LauncherItem::isSandboxed() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->isSandboxed() : false;
}

bool LauncherItem::isValid() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->isValid() : m_isTemporary;
}

bool LauncherItem::isLaunching() const
{
    return m_isLaunching;
}

void LauncherItem::setIsLaunching(bool isLaunching)
{
    if (isLaunching) {
        // This is a failsafe to allow launching again after 5 seconds in case the application crashes on startup and no window is ever created
        m_launchingTimeout.start(5000, this);
    } else {
        m_launchingTimeout.stop();
    }
    if (m_isLaunching != isLaunching) {
        m_isLaunching = isLaunching;
        emit this->isLaunchingChanged();
    }
}

void LauncherItem::setIsUpdating(bool isUpdating)
{
    if (m_isUpdating != isUpdating) {
        m_isUpdating = isUpdating;
        emit isUpdatingChanged();
    }
}

void LauncherItem::setIsTemporary(bool isTemporary)
{
    if (m_isTemporary != isTemporary) {
        m_isTemporary = isTemporary;
        emit isTemporaryChanged();
    }
}

void LauncherItem::launchApplication()
{
    if (m_isUpdating) {
        LauncherModel *model = static_cast<LauncherModel *>(parent());
        model->requestLaunch(m_packageName);
        return;
    }

    if (m_desktopEntry.isNull())
        return;

#if defined(HAVE_CONTENTACTION)
    LAUNCHER_DEBUG("launching content action for" << m_desktopEntry->name());
    ContentAction::Action action = ContentAction::Action::launcherAction(m_desktopEntry, QStringList());
    action.trigger();
#else
    LAUNCHER_DEBUG("launching exec line for" << m_desktopEntry->name());

    // Get the command text from the desktop entry
    QString commandText = m_desktopEntry->exec();

    // Take care of the freedesktop standards things

    commandText.replace(QRegExp("%k"), filePath());
    commandText.replace(QRegExp("%c"), m_desktopEntry->name());
    commandText.remove(QRegExp("%[fFuU]"));

    if (!m_desktopEntry->icon().isEmpty())
        commandText.replace(QRegExp("%i"), QString("--icon ") + m_desktopEntry->icon());

    // DETAILS: http://standards.freedesktop.org/desktop-entry-spec/latest/index.html
    // DETAILS: http://standards.freedesktop.org/desktop-entry-spec/latest/ar01s06.html

    // Launch the application
    QProcess::startDetached(commandText);
#endif

    setIsLaunching(true);
}

bool LauncherItem::isStillValid()
{
    if (m_isTemporary) {
        return true;
    }

    // Force a reload of m_desktopEntry
    setFilePath(filePath());
    return isValid();
}

QString LauncherItem::getOriginalIconId() const
{
    return !m_desktopEntry.isNull() ? m_desktopEntry->icon() : QString();
}

void LauncherItem::setIconFilename(const QString &path)
{
    m_customIconFilename = path;
    if (!path.isEmpty()) {
        m_serial++;
    }
    emit itemChanged();
}

QString LauncherItem::iconFilename() const
{
    return m_customIconFilename;
}

void LauncherItem::setPackageName(QString packageName)
{
    if (m_packageName != packageName) {
        m_packageName = packageName;
        emit packageNameChanged();
    }
}

void LauncherItem::setUpdatingProgress(int updatingProgress)
{
    if (m_updatingProgress != updatingProgress) {
        m_updatingProgress = updatingProgress;
        emit updatingProgressChanged();
    }
}

bool LauncherItem::isBlacklisted() const
{
    return m_isBlacklisted;
}

void LauncherItem::setIsBlacklisted(bool isBlacklisted) {
    if (m_isBlacklisted != isBlacklisted) {
        m_isBlacklisted = isBlacklisted;
        emit isBlacklistedChanged();
    }
}

void LauncherItem::setCustomTitle(QString customTitle)
{
    if (m_customTitle != customTitle) {
        m_customTitle = customTitle;
        emit itemChanged();
    }
}

QString LauncherItem::readValue(const QString &key) const
{
    if (m_desktopEntry.isNull())
        return QString();

    return m_desktopEntry->value("Desktop Entry", key);
}

void LauncherItem::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_launchingTimeout.timerId()) {
        setIsLaunching(false);
    } else {
        QObject::timerEvent(event);
    }
}
