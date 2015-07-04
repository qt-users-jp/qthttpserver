INCLUDEPATH += $$PWD
VPATH += $$PWD
QT += network

SOURCES += \
    $$PWD/qhttpserver.cpp \
    $$PWD/qhttprequest.cpp \
    $$PWD/qhttpconnection.cpp \
    $$PWD/qhttpreply.cpp \
    $$PWD/qwebsocket.cpp \
    $$PWD/qhttpserver_logging.cpp

HEADERS += \
    $$PWD/qthttpserverglobal.h \
    $$PWD/qhttpserver.h \
    $$PWD/qhttprequest.h \
    $$PWD/qhttpreply.h \
    $$PWD/qwebsocket.h \
    $$PWD/qhttpserver_logging.h

PRIVATE_HEADERS = \
    $$PWD/qhttpconnection_p.h

LIBS += -lz
