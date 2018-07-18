TEMPLATE = aux

lipstick_doc.config       += no_check_exist no_link explicit_dependencies
lipstick_doc.commands      = doxygen $$PWD/doxygen.cfg
lipstick_doc.input         = $$PWD/doxygen.cfg
lipstick_doc.output        = $$OUT_PWD/html/index.html

notification_doc.config       += no_check_exist no_link explicit_dependencies
notification_doc.commands      = doxygen $$PWD/notifications/doxygen.cfg
notification_doc.input         = $$PWD/notifications/doxygen.cfg
notification_doc.output        = $$OUT_PWD/notifications/html/index.html

# Install rules
htmldocs.files = \
    lipstick \
    lipstick-notification
htmldocs.path = /usr/share/doc
htmldocs.CONFIG += no_check_exist

QMAKE_EXTRA_COMPILERS += \
    lipstick_doc \
    notification_doc

QMAKE_EXTRA_TARGETS += \
    lipstick_doc \
    notification_doc

INSTALLS += \
    htmldocs \

OTHER_FILES = src/*.dox
