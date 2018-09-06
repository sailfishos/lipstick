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

#include <QWaylandWlShellSurface>
#include "lipstickglobal.h"

#include <QPointer>

QT_BEGIN_NAMESPACE

namespace QtWayland {
    class ExtendedSurface;
}

class QWaylandXdgSurfaceV5;

QT_END_NAMESPACE

class AlienSurface;
class LipstickCompositor;
class LipstickCompositorWindowHwcNode;
class WindowPropertyMap;

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
    Q_PROPERTY(QString className READ className NOTIFY classNameChanged)

    Q_PROPERTY(QRect mouseRegionBounds READ mouseRegionBounds NOTIFY mouseRegionBoundsChanged)

    Q_PROPERTY(bool focusOnTouch READ focusOnTouch WRITE setFocusOnTouch NOTIFY focusOnTouchChanged)

    Q_PROPERTY(bool exposed READ isExposed WRITE setExposed NOTIFY exposedChanged)
    Q_PROPERTY(bool exposedAsCover READ isExposedAsCover WRITE setExposedAsCover NOTIFY exposedAsCoverChanged)

    Q_PROPERTY(WindowPropertyMap *properties READ windowProperties CONSTANT)

    Q_PROPERTY(LipstickCompositorWindow *transientParent READ transientParent NOTIFY transientParentChanged)
    Q_PROPERTY(QPoint transientPosition READ transientPosition NOTIFY transientPositionChanged)

public:
    LipstickCompositorWindow(int windowId, const QString &, QWaylandSurface *surface = nullptr);
    LipstickCompositorWindow(int windowId, const QString &, QWaylandWlShellSurface *wlSurface);
    LipstickCompositorWindow(int windowId, const QString &, QWaylandXdgSurfaceV5 *xdgSurface);
    LipstickCompositorWindow(int windowId, AlienSurface *alienSurface);
    ~LipstickCompositorWindow();

    QVariant userData() const;
    void setUserData(QVariant);

    int windowId() const;
    qint64 processId() const;

    bool delayRemove() const;
    void setDelayRemove(bool);

    bool isTransient() const;

    QString category() const;
    virtual QString title() const;
    virtual bool isInProcess() const;

    QString className() const;

    qint16 windowFlags();

    QRect mouseRegionBounds() const;

    bool eventFilter(QObject *object, QEvent *event);

    Q_INVOKABLE void terminateProcess(int killTimeout);

    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *);

    bool focusOnTouch() const;
    void setFocusOnTouch(bool focusOnTouch);

    bool isAlien() const;

    WindowPropertyMap *windowProperties();
    QVariant windowProperty(const QString &key);

    bool isExposed() const;
    void setExposed(bool exposed);

    bool isExposedAsCover() const;
    void setExposedAsCover(bool exposed);

    LipstickCompositorWindow *transientParent() const;
    QPoint transientPosition() const;

    void sendOomScore(int score);

    QtWayland::ExtendedSurface *extendedSurface();

    Q_INVOKABLE void resize(const QSize &size);
    Q_INVOKABLE void ping();
    Q_INVOKABLE void close();
    Q_INVOKABLE void closePopup();

protected:
    void setTitle(const QString &title);

    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    virtual bool event(QEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;
    virtual void touchEvent(QTouchEvent *event) override;

signals:
    void userDataChanged();
    void titleChanged();
    void classNameChanged();
    void delayRemoveChanged();
    void focusOnTouchChanged();
    void windowFlagsChanged();
    void mouseRegionBoundsChanged();
    void pong();
    void exposedChanged();
    void exposedAsCoverChanged();
    void transientParentChanged();
    void transientPositionChanged();

private slots:
    void handleTouchCancel();
    void killProcess();

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

    inline void updateMouseRegion(const QVariant &value);
    inline void updateGrabbedKeys(const QVariant &value);

    QString m_title;
    QString m_category;
    QVariant m_data;

    QVector<QQuickItem *> m_refs;
    QPointer<QtWayland::ExtendedSurface> m_extSurface;
    QPointer<QWaylandWlShellSurface> m_wlShellSurface;
    QPointer<QWaylandXdgSurfaceV5> m_xdgSurface;
    QPointer<AlienSurface> m_alienSurface;
    QPointer<LipstickCompositorWindow> m_transientParent;
    QScopedPointer<WindowPropertyMap> m_windowProperties;
    QList<int> m_grabbedKeys;
    QList<int> m_pressedGrabbedKeys;

    QRect m_mouseRegion;
    QPoint m_transientPosition;
    LipstickCompositor * const m_compositor;
    const int m_windowId;
    uint m_pingSerial = 0;
    bool m_isAlien : 1;
    bool m_delayRemove : 1;
    bool m_windowClosed : 1;
    bool m_removePosted : 1;
    bool m_interceptingTouch : 1;
    bool m_mapped : 1;
    bool m_noHardwareComposition: 1;
    bool m_focusOnTouch : 1;
    bool m_hasVisibleReferences : 1;
    bool m_exposed : 1;
    bool m_exposedAsCover : 1;
    bool m_explicitMouseRegion : 1;
};

#endif // LIPSTICKCOMPOSITORWINDOW_H
