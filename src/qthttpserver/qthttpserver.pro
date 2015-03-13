TARGET     = QtHttpServer
MODULE     = httpserver
QT         = core network

load(qt_module)

include(qthttpserver.pri)

PUBLIC_HEADERS = $$HEADERS
HEADERS += $$PRIVATE_HEADERS
CONFIG-=create_cmake
