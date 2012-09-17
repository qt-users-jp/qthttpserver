include($$PWD/../3rdparty/qtservice/src/qtservice.pri)

INCLUDEPATH += $$PWD
VPATH += $$PWD

SOURCES += \
    $$PWD/qhttpservice.cpp \
    $$PWD/qhttpserver.cpp \
    $$PWD/qhttprequest.cpp \
    $$PWD/qhttpconnection.cpp \
    $$PWD/qhttpreply.cpp

HEADERS += \
    $$PWD/qthttpserverglobal.h \
    $$PWD/qhttpservice.h \
    $$PWD/qhttpserver.h \
    $$PWD/qhttprequest.h \
    $$PWD/qhttpreply.h

PRIVATE_HEADERS = \
    $$PWD/qhttpconnection_p.h
