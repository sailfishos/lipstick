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
    $$PWD/lipstickrecorder.cpp \
    $$PWD/lipstickviewporter.cpp \
    $$PWD/lipstickfractionalscale.cpp \

DEFINES += QT_COMPOSITOR_QUICK

QT += compositor

# needed for hardware compositor
QT += quick-private gui-private core-private compositor-private qml-private

PROTOCOL_PATH = $$system($$pkgConfigExecutable() --variable=pkgdatadir wayland-protocols)

WAYLANDSERVERSOURCES += \
    ../protocol/lipstick-recorder.xml \
    $$PROTOCOL_PATH/stable/viewporter/viewporter.xml \
    $$PROTOCOL_PATH/staging/fractional-scale/fractional-scale-v1.xml \

OTHER_FILES += $$PWD/compositor.xml $$PWD/fileservice.xml
