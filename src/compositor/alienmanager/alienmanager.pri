INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/alienmanager.h \
    $$PWD/aliensurface.h \

SOURCES += \
    $$PWD/alienmanager.cpp \
    $$PWD/aliensurface.cpp \

WAYLANDSERVERSOURCES += $$PWD/../../../protocol/alien-manager.xml
