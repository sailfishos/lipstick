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

PROTOCOL_PATH = $$system($$pkgConfigExecutable() --variable=pkgdatadir wayland-protocols)

WAYLANDSERVERSOURCES += $$PROTOCOL_PATH/stable/xdg-shell/xdg-shell.xml \

QT += compositor
