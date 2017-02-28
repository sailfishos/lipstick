/***************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Aaron Kennedy <aaron.kennedy@jollamobile.com>
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

#ifndef LIPSTICKCOMPOSITORWINDOW_H
#define LIPSTICKCOMPOSITORWINDOW_H

#include <QWaylandQuickItem>
#include <QWaylandBufferRef>
#include <QPointer>
#include <QWaylandWlShellSurface>
#include "lipstickglobal.h"

namespace QtWayland {
    class ExtendedSurface;
}
class LipstickCompositorWindowHwcNode;

class LIPSTICK_EXPORT LipstickCompositorWindow : public QWaylandQuickItem
{
    Q_OBJECT

    Q_PROPERTY(int windowId READ windowId CONSTANT)
    Q_PROPERTY(bool isInProcess READ isInProcess CONSTANT)
    Q_PROPERTY(bool isAlien READ isAlien CONSTANT)

    Q_PROPERTY(bool delayRemove READ delayRemove WRITE setDelayRemove NOTIFY delayRemoveChanged)
    Q_PROPERTY(QVariant userData READ userData WRITE setUserData NOTIFY userDataChanged)

    Q_PROPERTY(QString category READ category CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(qint64 processId READ processId CONSTANT)
    Q_PROPERTY(qint16 windowFlags READ windowFlags NOTIFY windowFlagsChanged)

    Q_PROPERTY(bool focusOnTouch READ focusOnTouch WRITE setFocusOnTouch NOTIFY focusOnTouchChanged)

public:
    LipstickCompositorWindow(int windowId, const QString &, QWaylandSurface *surface, QQuickItem *parent = 0);
    ~LipstickCompositorWindow();

    QVariant userData() const;
    void setUserData(QVariant);

    int windowId() const;
    qint64 processId() const;

    bool delayRemove() const;
    void setDelayRemove(bool);

    QString category() const;
    virtual QString title() const;
    virtual bool isInProcess() const;

    qint16 windowFlags();

    bool eventFilter(QObject *object, QEvent *event);

    Q_INVOKABLE void terminateProcess(int killTimeout);

    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *);

    bool focusOnTouch() const;
    void setFocusOnTouch(bool focusOnTouch);

    QVariantMap windowProperties();

protected:
    void itemChange(ItemChange change, const ItemChangeData &data);

    virtual bool event(QEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void touchEvent(QTouchEvent *event);

signals:
    void userDataChanged();
    void titleChanged();
    void delayRemoveChanged();
    void committed();
    void focusOnTouchChanged();
    void windowFlagsChanged();

private slots:
    void handleTouchCancel();
    void killProcess();
    void connectSurfaceSignals();

private:
    friend class LipstickCompositor;
    friend class WindowModel;
    friend class WindowPixmapItem;
    void imageAddref(QQuickItem *item);
    void imageRelease(QQuickItem *item);
    void onSync();

    bool canRemove() const;
    void tryRemove();
    void handleTouchEvent(QTouchEvent *e);

    void setExtendedSurface(QtWayland::ExtendedSurface *extSurface);
    QtWayland::ExtendedSurface *extendedSurface();
    void setTitle(QString title);
    QString m_title;

    int m_windowId;
    bool m_isAlien;
    QString m_category;
    bool m_delayRemove:1;
    bool m_windowClosed:1;
    bool m_removePosted:1;
    bool m_interceptingTouch:1;
    bool m_mapped : 1;
    bool m_noHardwareComposition: 1;
    bool m_focusOnTouch : 1;
    bool m_hasVisibleReferences : 1;
    QVariant m_data;
    QList<QMetaObject::Connection> m_surfaceConnections;
    QVector<QQuickItem *> m_refs;
    QtWayland::ExtendedSurface *m_extSurface;
    QWaylandWlShellSurface *m_wlShellSurface;
};

#endif // LIPSTICKCOMPOSITORWINDOW_H
