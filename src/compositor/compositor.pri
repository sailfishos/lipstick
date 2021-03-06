system(qdbusxml2cpp compositor.xml -a lipstickcompositoradaptor -c LipstickCompositorAdaptor -l LipstickCompositor -i lipstickcompositor.h)
system(qdbusxml2cpp fileservice.xml -a fileserviceadaptor -c FileServiceAdaptor)

INCLUDEPATH += $$PWD

PUBLICHEADERS += \
    $$PWD/lipstickcompositor.h \
    $$PWD/lipstickcompositorwindow.h \
    $$PWD/lipstickcompositorprocwindow.h \
    $$PWD/lipstickcompositoradaptor.h \
    $$PWD/fileserviceadaptor.h \
    $$PWD/lipstickkeymap.h \
    $$PWD/windowmodel.h \
    $$PWD/lipsticksurfaceinterface.h \

HEADERS += \
    $$PWD/windowpixmapitem.h \
    $$PWD/windowproperty.h

SOURCES += \
    $$PWD/lipstickcompositor.cpp \
    $$PWD/lipstickcompositorwindow.cpp \
    $$PWD/lipstickcompositorprocwindow.cpp \
    $$PWD/lipstickcompositoradaptor.cpp \
    $$PWD/fileserviceadaptor.cpp \
    $$PWD/lipstickkeymap.cpp \
    $$PWD/windowmodel.cpp \
    $$PWD/windowpixmapitem.cpp \
    $$PWD/windowproperty.cpp \
    $$PWD/lipsticksurfaceinterface.cpp \
    $$PWD/lipstickrecorder.cpp

DEFINES += QT_COMPOSITOR_QUICK

QT += compositor

# needed for hardware compositor
QT += quick-private gui-private core-private compositor-private qml-private

WAYLANDSERVERSOURCES += ../protocol/lipstick-recorder.xml \

OTHER_FILES += $$PWD/compositor.xml $$PWD/fileservice.xml
