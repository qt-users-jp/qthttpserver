load(qt_build_config)

TARGET     = QtHttpServer
MODULE     = httpserver
QT         = core network

load(qt_module)

include(src.pri)

PUBLIC_HEADERS = $$HEADERS
HEADERS += $$PRIVATE_HEADERS
