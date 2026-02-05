INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/xdgshell.h \
    $$PWD/xdgpositioner.h \
    $$PWD/xdgsurface.h \
    $$PWD/xdgtoplevel.h \
    $$PWD/xdgpopup.h \

SOURCES += \
    $$PWD/xdgshell.cpp \
    $$PWD/xdgpositioner.cpp \
    $$PWD/xdgsurface.cpp \
    $$PWD/xdgtoplevel.cpp \
    $$PWD/xdgpopup.cpp \

WAYLANDSERVERSOURCES += ../protocol/xdg-shell.xml \

QT += compositor
