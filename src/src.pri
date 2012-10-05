INCLUDEPATH += $$PWD
VPATH += $$PWD

SOURCES += \
    $$PWD/qhttpserver.cpp \
    $$PWD/qhttprequest.cpp \
    $$PWD/qhttpconnection.cpp \
    $$PWD/qhttpreply.cpp \
    $$PWD/qwebsocket.cpp

HEADERS += \
    $$PWD/qthttpserverglobal.h \
    $$PWD/qhttpserver.h \
    $$PWD/qhttprequest.h \
    $$PWD/qhttpreply.h \
    $$PWD/qwebsocket.h

PRIVATE_HEADERS = \
    $$PWD/qhttpconnection_p.h
