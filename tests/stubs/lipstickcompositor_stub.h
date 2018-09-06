#ifndef LIPSTICKCOMPOSITOR_STUB
#define LIPSTICKCOMPOSITOR_STUB

#include "lipstickcompositor.h"
#include "touchscreen/touchscreen.h"
#include "windowpropertymap.h"
#include <stubbase.h>

#include <QWaylandSurface>
#include <QWaylandXdgShellV5>

QT_BEGIN_NAMESPACE
namespace QtWayland {
class ExtendedSurface : public QObject {};
class SurfaceExtensionGlobal {};
class QtKeyExtensionGlobal {};
}
QT_END_NAMESPACE


class AlienManager {};

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
    virtual bool openUrl(QWaylandClient *, const QUrl &);
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
    virtual QObject *clipboard() const;
    virtual bool debug() const;
    virtual LipstickCompositorWindow *windowForId(int) const;
    virtual void closeClientForWindowId(int);
    virtual void clearKeyboardFocus();
    virtual void setDisplayOff();
    virtual LipstickCompositorProcWindow *mapProcWindow(const QString &title, const QString &category, const QRect &);
    virtual QWaylandSurface *surfaceForId(int) const;
    virtual void surfaceSizeChanged();
    virtual void surfaceTitleChanged();
    virtual void surfaceRaised();
    virtual void surfaceLowered();
    virtual void surfaceDamaged(const QRegion &);
    virtual void windowSwapped();
    virtual void windowPropertyChanged(const QString &);
    virtual void reactOnDisplayStateChanges(TouchScreen::DisplayState oldState, TouchScreen::DisplayState newState);
    virtual void setScreenOrientationFromSensor();
    virtual void clipboardDataChanged();
    virtual void onVisibleChanged(bool visible);
    virtual void readContent();
    virtual void initialize();
    virtual bool completed();
    virtual void timerEvent(QTimerEvent *e);
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

bool LipstickCompositorStub::openUrl(QWaylandClient *client, const QUrl &url)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<QWaylandClient * >(client));
    params.append( new Parameter<const QUrl &>(url));
    stubMethodEntered("openUrl", params);
    return stubReturnValue<bool>("openUrl");
}

bool LipstickCompositorStub::openUrl(const QUrl &url)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QUrl &>(url));
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
    params.append( new Parameter<bool >(homeActive));
    stubMethodEntered("setHomeActive", params);
}

void LipstickCompositorStub::setFullscreenSurface(QWaylandSurface *surface)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<QWaylandSurface * >(surface));
    stubMethodEntered("setFullscreenSurface", params);
}

void LipstickCompositorStub::setTopmostWindowId(int id)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<int >(id));
    stubMethodEntered("setTopmostWindowId", params);
}

void LipstickCompositorStub::setTopmostWindowOrientation(Qt::ScreenOrientation topmostWindowOrientation)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<Qt::ScreenOrientation >(topmostWindowOrientation));
    stubMethodEntered("setTopmostWindowOrientation", params);
}

void LipstickCompositorStub::setScreenOrientation(Qt::ScreenOrientation screenOrientation)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<Qt::ScreenOrientation >(screenOrientation));
    stubMethodEntered("setScreenOrientation", params);
}

bool LipstickCompositorStub::displayDimmed() const
{
    stubMethodEntered("displayDimmed");
    return stubReturnValue<bool>("displayDimmed");
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

LipstickCompositorWindow *LipstickCompositorStub::windowForId(int id) const
{
    QList<ParameterBase *> params;
    params.append( new Parameter<int >(id));
    stubMethodEntered("windowForId", params);
    return stubReturnValue<LipstickCompositorWindow *>("windowForId");
}

void LipstickCompositorStub::closeClientForWindowId(int id)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<int >(id));
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
    params.append( new Parameter<const QString & >(title));
    params.append( new Parameter<const QString & >(category));
    params.append( new Parameter<const QRect & >(rect));
    stubMethodEntered("mapProcWindow", params);
    return stubReturnValue<LipstickCompositorProcWindow *>("mapProcWindow");
}

QWaylandSurface *LipstickCompositorStub::surfaceForId(int id) const
{
    QList<ParameterBase *> params;
    params.append( new Parameter<int >(id));
    stubMethodEntered("surfaceForId", params);
    return stubReturnValue<QWaylandSurface *>("surfaceForId");
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

void LipstickCompositorStub::timerEvent(QTimerEvent *e)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<QTimerEvent *>(e));
    stubMethodEntered("timerEvent", params);
}

void LipstickCompositorStub::surfaceDamaged(const QRegion &rect)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<QRegion>(rect));
    stubMethodEntered("surfaceDamaged", params);
}

void LipstickCompositorStub::windowSwapped()
{
    stubMethodEntered("windowSwapped");
}

void LipstickCompositorStub::windowPropertyChanged(const QString &property)
{
    QList<ParameterBase *> params;
    params.append( new Parameter<const QString & >(property));
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
    params.append( new Parameter<bool>(v));
    stubMethodEntered("onVisibleChanged", params);
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

void LipstickCompositor::componentComplete()
{
    QWaylandQuickCompositor::componentComplete();
}

LipstickCompositor *LipstickCompositor::instance()
{
    return gLipstickCompositorStub->instance();
}

bool LipstickCompositor::openUrl(QWaylandClient *client, const QUrl &url)
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

void LipstickCompositor::setFullscreenSurface(QWaylandSurface *surface)
{
    gLipstickCompositorStub->setFullscreenSurface(surface);
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

QObject *LipstickCompositor::clipboard() const
{
    return gLipstickCompositorStub->clipboard();
}

bool LipstickCompositor::debug() const
{
    return gLipstickCompositorStub->debug();
}

LipstickCompositorWindow *LipstickCompositor::windowForId(int id) const
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

void LipstickCompositor::surfaceSizeChanged()
{
    gLipstickCompositorStub->surfaceSizeChanged();
}

void LipstickCompositor::surfaceTitleChanged()
{
    gLipstickCompositorStub->surfaceTitleChanged();
}

void LipstickCompositor::surfaceDamaged(const QRegion &rect)
{
    gLipstickCompositorStub->surfaceDamaged(rect);
}

void LipstickCompositor::windowSwapped()
{
    gLipstickCompositorStub->windowSwapped();
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

void LipstickCompositor::initialize()
{
    gLipstickCompositorStub->initialize();
}

bool LipstickCompositor::completed()
{
    return gLipstickCompositorStub->completed();
}

void LipstickCompositor::timerEvent(QTimerEvent *e)
{
    gLipstickCompositorStub->timerEvent(e);
}

QQmlListProperty<QObject> LipstickCompositor::data()
{
    return QQmlListProperty<QObject>();
}

QWaylandKeymap *LipstickCompositor::keymap()
{
    return nullptr;
}

QWaylandCompositor::QWaylandCompositor(QObject *)
{
}

QWaylandQuickCompositor::QWaylandQuickCompositor(QObject *)
{
}

WindowPropertyMap::WindowPropertyMap(
        QtWayland::ExtendedSurface *surface, QWaylandSurface *waylandSurface,  QObject *parent)
    : QQmlPropertyMap(parent)
    , m_surface(surface)
    , m_waylandSurface(waylandSurface)
{
}

WindowPropertyMap::~WindowPropertyMap()
{
}

QVariant WindowPropertyMap::updateValue(const QString &key, const QVariant &value)
{
    return QQmlPropertyMap::updateValue(key, value);
}

#endif
