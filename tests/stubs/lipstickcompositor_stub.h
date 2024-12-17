/***************************************************************************
**
** Copyright (c) 2013-2019 Jolla Ltd.
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

#ifndef LIPSTICKCOMPOSITOR_STUB
#define LIPSTICKCOMPOSITOR_STUB

#include "lipstickcompositor.h"
#include "touchscreen/touchscreen.h"
#include <stubbase.h>

// 1. DECLARE STUB
// FIXME - stubgen is not yet finished
class LipstickCompositorStub : public StubBase
{
public:
    virtual void LipstickCompositorConstructor();
    virtual void LipstickCompositorDestructor();
    virtual LipstickCompositor *instance();
    virtual void classBegin();
    virtual void componentComplete();
    virtual void surfaceCreated(QWaylandSurface *surface);
    virtual bool openUrl(WaylandClient *, const QUrl &);
    virtual bool openUrl(const QUrl &);
    virtual void retainedSelectionReceived(QMimeData *mimeData);
    virtual int windowCount() const;
    virtual int ghostWindowCount() const;
    virtual bool homeActive() const;
    virtual void setHomeActive(bool);
    virtual void setFullscreenSurface(QWaylandSurface *surface);
    virtual void setTopmostWindowId(int id);
    virtual void setTopmostWindowOrientation(Qt::ScreenOrientation topmostWindowOrientation);
    virtual void setScreenOrientation(Qt::ScreenOrientation screenOrientation);
    virtual bool displayDimmed() const;
    virtual LipstickKeymap *keymap() const;
    virtual void setKeymap(LipstickKeymap *keymap);
    virtual void updateKeymap();
    virtual QObject *clipboard() const;
    virtual bool debug() const;
    virtual QObject *windowForId(int) const;
    virtual void closeClientForWindowId(int);
    virtual void clearKeyboardFocus();
    virtual void setDisplayOff();
    virtual LipstickCompositorProcWindow *mapProcWindow(const QString &title, const QString &category, const QRect &);
    virtual QWaylandSurface *surfaceForId(int) const;
    virtual void surfaceMapped();
    virtual void surfaceUnmapped();
    virtual void surfaceSizeChanged();
    virtual void surfaceTitleChanged();
    virtual void surfaceRaised();
    virtual void surfaceLowered();
    virtual void surfaceDamaged(const QRegion &);
    virtual void windowSwapped();
    virtual void windowDestroyed();
    virtual void windowPropertyChanged(const QString &);
    virtual void reactOnDisplayStateChanges(TouchScreen::DisplayState oldState, TouchScreen::DisplayState newState);
    virtual void setScreenOrientationFromSensor();
    virtual void clipboardDataChanged();
    virtual void onVisibleChanged(bool visible);
    virtual void keymapChanged();
    virtual QWaylandSurfaceView *createView(QWaylandSurface *surf);
    virtual void readContent();
    virtual void initialize();
    virtual bool completed();
    virtual bool synthesizeBackEvent() const;
    virtual void setSynthesizeBackEvent(bool enable);
    virtual void timerEvent(QTimerEvent *e);
    virtual bool event(QEvent *e);
    virtual void sendKeyEvent(QEvent::Type type, Qt::Key key, quint32 nativeScanCode);
};

// 2. IMPLEMENT STUB
void LipstickCompositorStub::LipstickCompositorConstructor()
{

}
void LipstickCompositorStub::LipstickCompositorDestructor()
{

}
LipstickCompositor *LipstickCompositorStub::instance()
{
    stubMethodEntered("instance");
    return stubReturnValue<LipstickCompositor *>("instance");
}

void LipstickCompositorStub::classBegin()
{
    stubMethodEntered("classBegin");
}

void LipstickCompositorStub::componentComplete()
{
    stubMethodEntered("componentComplete");
}

void LipstickCompositorStub::surfaceCreated(QWaylandSurface *surface)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QWaylandSurface * >(surface));
    stubMethodEntered("surfaceCreated", params);
}

bool LipstickCompositorStub::openUrl(WaylandClient *client, const QUrl &url)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<WaylandClient * >(client));
    params.append(new Parameter<const QUrl &>(url));
    stubMethodEntered("openUrl", params);
    return stubReturnValue<bool>("openUrl");
}

bool LipstickCompositorStub::openUrl(const QUrl &url)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<const QUrl &>(url));
    stubMethodEntered("openUrl", params);
    return stubReturnValue<bool>("openUrl");
}

void LipstickCompositorStub::retainedSelectionReceived(QMimeData *mimeData)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QMimeData *>(mimeData));
    stubMethodEntered("retainedSelectionReceived", params);
}

int LipstickCompositorStub::windowCount() const
{
    stubMethodEntered("windowCount");
    return stubReturnValue<int>("windowCount");
}

int LipstickCompositorStub::ghostWindowCount() const
{
    stubMethodEntered("ghostWindowCount");
    return stubReturnValue<int>("ghostWindowCount");
}

bool LipstickCompositorStub::homeActive() const
{
    stubMethodEntered("homeActive");
    return stubReturnValue<bool>("homeActive");
}

void LipstickCompositorStub::setHomeActive(bool homeActive)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<bool >(homeActive));
    stubMethodEntered("setHomeActive", params);
}

void LipstickCompositorStub::setFullscreenSurface(QWaylandSurface *surface)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QWaylandSurface * >(surface));
    stubMethodEntered("setFullscreenSurface", params);
}

void LipstickCompositorStub::setTopmostWindowId(int id)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<int >(id));
    stubMethodEntered("setTopmostWindowId", params);
}

void LipstickCompositorStub::setTopmostWindowOrientation(Qt::ScreenOrientation topmostWindowOrientation)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<Qt::ScreenOrientation >(topmostWindowOrientation));
    stubMethodEntered("setTopmostWindowOrientation", params);
}

void LipstickCompositorStub::setScreenOrientation(Qt::ScreenOrientation screenOrientation)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<Qt::ScreenOrientation >(screenOrientation));
    stubMethodEntered("setScreenOrientation", params);
}

bool LipstickCompositorStub::displayDimmed() const
{
    stubMethodEntered("displayDimmed");
    return stubReturnValue<bool>("displayDimmed");
}

LipstickKeymap *LipstickCompositorStub::keymap() const
{
    stubMethodEntered("keymap");
    return stubReturnValue<LipstickKeymap *>("keymap");
}

void LipstickCompositorStub::setKeymap(LipstickKeymap *keymap)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<LipstickKeymap *>(keymap));
    stubMethodEntered("setKeymap", params);
}

void LipstickCompositorStub::updateKeymap()
{
    stubMethodEntered("updateKeymap");
}

QObject *LipstickCompositorStub::clipboard() const
{
    stubMethodEntered("clipboard");
    return stubReturnValue<QObject *>("clipboard");
}

bool LipstickCompositorStub::debug() const
{
    stubMethodEntered("debug");
    return stubReturnValue<bool>("debug");
}

QObject *LipstickCompositorStub::windowForId(int id) const
{
    QList<ParameterBase *> params;
    params.append(new Parameter<int >(id));
    stubMethodEntered("windowForId", params);
    return stubReturnValue<QObject *>("windowForId");
}

void LipstickCompositorStub::closeClientForWindowId(int id)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<int >(id));
    stubMethodEntered("closeClientForWindowId", params);
}

void LipstickCompositorStub::clearKeyboardFocus()
{
    stubMethodEntered("clearKeyboardFocus");
}

void LipstickCompositorStub::setDisplayOff()
{
    stubMethodEntered("setDisplayOff");
}

LipstickCompositorProcWindow *LipstickCompositorStub::mapProcWindow(const QString &title, const QString &category, const QRect &rect)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<const QString & >(title));
    params.append(new Parameter<const QString & >(category));
    params.append(new Parameter<const QRect & >(rect));
    stubMethodEntered("mapProcWindow", params);
    return stubReturnValue<LipstickCompositorProcWindow *>("mapProcWindow");
}

QWaylandSurface *LipstickCompositorStub::surfaceForId(int id) const
{
    QList<ParameterBase *> params;
    params.append(new Parameter<int >(id));
    stubMethodEntered("surfaceForId", params);
    return stubReturnValue<QWaylandSurface *>("surfaceForId");
}

void LipstickCompositorStub::surfaceMapped()
{
    stubMethodEntered("surfaceMapped");
}

void LipstickCompositorStub::surfaceUnmapped()
{
    stubMethodEntered("surfaceUnmapped");
}

void LipstickCompositorStub::surfaceSizeChanged()
{
    stubMethodEntered("surfaceSizeChanged");
}

void LipstickCompositorStub::surfaceTitleChanged()
{
    stubMethodEntered("surfaceTitleChanged");
}

void LipstickCompositorStub::surfaceRaised()
{
    stubMethodEntered("surfaceRaised");
}

void LipstickCompositorStub::surfaceLowered()
{
    stubMethodEntered("surfaceLowered");
}

void LipstickCompositorStub::readContent()
{
    stubMethodEntered("readContent");
}

void LipstickCompositorStub::initialize()
{
    stubMethodEntered("initialize");
}

bool LipstickCompositorStub::completed()
{
    stubMethodEntered("completed");
    return true;
}

bool LipstickCompositorStub::synthesizeBackEvent() const
{
    stubMethodEntered("synthesizeBackEvent");
    return true;
}

void LipstickCompositorStub::setSynthesizeBackEvent(bool enable)
{
    Q_UNUSED(enable);
    stubMethodEntered("setSynthesizeBackEvent");
}

void LipstickCompositorStub::timerEvent(QTimerEvent *e)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QTimerEvent *>(e));
    stubMethodEntered("timerEvent", params);
}

bool LipstickCompositorStub::event(QEvent *e)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QEvent *>(e));
    stubMethodEntered("event", params);
    return true;
}

void LipstickCompositorStub::sendKeyEvent(QEvent::Type type, Qt::Key key, quint32 nativeScanCode)
{
  QList<ParameterBase*> params;
  params.append(new Parameter<QEvent::Type>(type));
  params.append(new Parameter<Qt::Key>(key));
  params.append(new Parameter<quint32>(nativeScanCode));
  stubMethodEntered("sendKeyEvent", params);
}

void LipstickCompositorStub::surfaceDamaged(const QRegion &rect)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QRegion>(rect));
    stubMethodEntered("surfaceDamaged", params);
}

void LipstickCompositorStub::windowSwapped()
{
    stubMethodEntered("windowSwapped");
}

void LipstickCompositorStub::windowDestroyed()
{
    stubMethodEntered("windowDestroyed");
}

void LipstickCompositorStub::windowPropertyChanged(const QString &property)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<const QString & >(property));
    stubMethodEntered("windowPropertyChanged", params);
}

void LipstickCompositorStub::reactOnDisplayStateChanges(TouchScreen::DisplayState oldState, TouchScreen::DisplayState newState)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<TouchScreen::DisplayState>(oldState));
    params.append(new Parameter<TouchScreen::DisplayState>(newState));
    stubMethodEntered("reactOnDisplayStateChanges", params);
}

void LipstickCompositorStub::setScreenOrientationFromSensor( )
{
    stubMethodEntered("setScreenOrientationFromSensor");
}

void LipstickCompositorStub::clipboardDataChanged()
{
    stubMethodEntered("clipboardDataChanged");
}

void LipstickCompositorStub::onVisibleChanged(bool v)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<bool>(v));
    stubMethodEntered("onVisibleChanged", params);
}

void LipstickCompositorStub::keymapChanged()
{
    stubMethodEntered("keymapChanged");
}

QWaylandSurfaceView *LipstickCompositorStub::createView(QWaylandSurface *surf)
{
    QList<ParameterBase *> params;
    params.append(new Parameter<QWaylandSurface *>(surf));
    stubMethodEntered("createView", params);
    return stubReturnValue<QWaylandSurfaceView *>("createView");
}

// 3. CREATE A STUB INSTANCE
LipstickCompositorStub gDefaultLipstickCompositorStub;
LipstickCompositorStub *gLipstickCompositorStub = &gDefaultLipstickCompositorStub;

// 4. CREATE A PROXY WHICH CALLS THE STUB
LipstickCompositor::LipstickCompositor()
{
    gLipstickCompositorStub->LipstickCompositorConstructor();
}

LipstickCompositor::~LipstickCompositor()
{
    gLipstickCompositorStub->LipstickCompositorDestructor();
}

LipstickCompositor *LipstickCompositor::instance()
{
    return gLipstickCompositorStub->instance();
}

void LipstickCompositor::classBegin()
{
    gLipstickCompositorStub->classBegin();
}

void LipstickCompositor::componentComplete()
{
    gLipstickCompositorStub->componentComplete();
}

void LipstickCompositor::surfaceCreated(QWaylandSurface *surface)
{
    gLipstickCompositorStub->surfaceCreated(surface);
}

bool LipstickCompositor::openUrl(WaylandClient *client, const QUrl &url)
{
    return gLipstickCompositorStub->openUrl(client, url);
}

bool LipstickCompositor::openUrl(const QUrl &url)
{
    return gLipstickCompositorStub->openUrl(url);
}

void LipstickCompositor::retainedSelectionReceived(QMimeData *mimeData)
{
    gLipstickCompositorStub->retainedSelectionReceived(mimeData);
}

int LipstickCompositor::windowCount() const
{
    return gLipstickCompositorStub->windowCount();
}

int LipstickCompositor::ghostWindowCount() const
{
    return gLipstickCompositorStub->ghostWindowCount();
}

bool LipstickCompositor::homeActive() const
{
    return gLipstickCompositorStub->homeActive();
}

void LipstickCompositor::setHomeActive(bool homeActive)
{
    gLipstickCompositorStub->setHomeActive(homeActive);
}

void LipstickCompositor::setTopmostWindowId(int id)
{
    gLipstickCompositorStub->setTopmostWindowId(id);
}

void LipstickCompositor::setTopmostWindowOrientation(Qt::ScreenOrientation topmostWindowOrientation)
{
    gLipstickCompositorStub->setTopmostWindowOrientation(topmostWindowOrientation);
}

void LipstickCompositor::setScreenOrientation(Qt::ScreenOrientation screenOrientation)
{
    gLipstickCompositorStub->setScreenOrientation(screenOrientation);
}

bool LipstickCompositor::displayDimmed() const
{
    return gLipstickCompositorStub->displayDimmed();
}

LipstickKeymap *LipstickCompositor::keymap() const
{
    return gLipstickCompositorStub->keymap();
}

void LipstickCompositor::setKeymap(LipstickKeymap *keymap)
{
    gLipstickCompositorStub->setKeymap(keymap);
}

void LipstickCompositor::updateKeymap()
{
    gLipstickCompositorStub->updateKeymap();
}

QObject *LipstickCompositor::clipboard() const
{
    return gLipstickCompositorStub->clipboard();
}

bool LipstickCompositor::debug() const
{
    return gLipstickCompositorStub->debug();
}

QObject *LipstickCompositor::windowForId(int id) const
{
    return gLipstickCompositorStub->windowForId(id);
}

void LipstickCompositor::closeClientForWindowId(int id)
{
    gLipstickCompositorStub->closeClientForWindowId(id);
}

void LipstickCompositor::clearKeyboardFocus()
{
    gLipstickCompositorStub->clearKeyboardFocus();
}

void LipstickCompositor::setDisplayOff()
{
    gLipstickCompositorStub->setDisplayOff();
}

LipstickCompositorProcWindow *LipstickCompositor::mapProcWindow(const QString &title, const QString &category, const QRect &rect)
{
    return gLipstickCompositorStub->mapProcWindow(title, category, rect);
}

QWaylandSurface *LipstickCompositor::surfaceForId(int id) const
{
    return gLipstickCompositorStub->surfaceForId(id);
}

void LipstickCompositor::surfaceMapped()
{
    gLipstickCompositorStub->surfaceMapped();
}

void LipstickCompositor::surfaceUnmapped()
{
    gLipstickCompositorStub->surfaceUnmapped();
}

void LipstickCompositor::surfaceSizeChanged()
{
    gLipstickCompositorStub->surfaceSizeChanged();
}

void LipstickCompositor::surfaceTitleChanged()
{
    gLipstickCompositorStub->surfaceTitleChanged();
}

void LipstickCompositor::surfaceRaised()
{
    gLipstickCompositorStub->surfaceRaised();
}

void LipstickCompositor::surfaceLowered()
{
    gLipstickCompositorStub->surfaceLowered();
}

void LipstickCompositor::surfaceDamaged(const QRegion &rect)
{
    gLipstickCompositorStub->surfaceDamaged(rect);
}

void LipstickCompositor::windowSwapped()
{
    gLipstickCompositorStub->windowSwapped();
}

void LipstickCompositor::windowDestroyed()
{
    gLipstickCompositorStub->windowDestroyed();
}

void LipstickCompositor::windowPropertyChanged(const QString &property)
{
    gLipstickCompositorStub->windowPropertyChanged(property);
}

void LipstickCompositor::reactOnDisplayStateChanges(TouchScreen::DisplayState oldState, TouchScreen::DisplayState newState)
{
    gLipstickCompositorStub->reactOnDisplayStateChanges(oldState, newState);
}

void LipstickCompositor::homeApplicationAboutToDestroy()
{
}

void LipstickCompositor::setScreenOrientationFromSensor()
{
    gLipstickCompositorStub->setScreenOrientationFromSensor();
}

void LipstickCompositor::clipboardDataChanged()
{
    gLipstickCompositorStub->clipboardDataChanged();
}

void LipstickCompositor::onVisibleChanged(bool v)
{
    gLipstickCompositorStub->onVisibleChanged(v);
}

QWaylandSurfaceView *LipstickCompositor::createView(QWaylandSurface *surf)
{
    return gLipstickCompositorStub->createView(surf);
}

void LipstickCompositor::readContent()
{
    gLipstickCompositorStub->readContent();
}

void LipstickCompositor::initialize()
{
    gLipstickCompositorStub->initialize();
}

bool LipstickCompositor::completed()
{
    return gLipstickCompositorStub->completed();
}

bool LipstickCompositor::synthesizeBackEvent() const
{
    return gLipstickCompositorStub->synthesizeBackEvent();
}

void LipstickCompositor::setSynthesizeBackEvent(bool enable)
{
    gLipstickCompositorStub->setSynthesizeBackEvent(enable);
}

void LipstickCompositor::timerEvent(QTimerEvent *e)
{
    gLipstickCompositorStub->timerEvent(e);
}

bool LipstickCompositor::event(QEvent *e)
{
    return gLipstickCompositorStub->event(e);
}

void LipstickCompositor::sendKeyEvent(QEvent::Type type, Qt::Key key, quint32 nativeScanCode)
{
  gLipstickCompositorStub->sendKeyEvent(type, key, nativeScanCode);
}

void LipstickCompositor::processQueuedSetUpdatesEnabledCalls()
{
}

#endif
