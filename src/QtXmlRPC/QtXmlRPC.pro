NAME         = QtXmlRPC
TARGET       = $${NAME}

QT           = core
QT          -= gui
QT          += network
QT          += sql
QT          += script
QT          += Essentials

load(qt_build_config)
load(qt_module)

CONFIG(debug, debug|release) {
  DEFINES   += XMLRPC_DEBUG
#  DEFINES   += VERY_DEEP_DEBUG
}

INCLUDEPATH += $${PWD}/../../include/$${NAME}

HEADERS     += $${PWD}/../../include/$${NAME}/qtxmlrpc.h

include ($${PWD}/XmlRPC/XmlRPC.pri)

OTHER_FILES += $${PWD}/../../include/$${NAME}/headers.pri

include ($${PWD}/../../doc/Qt/Qt.pri)

win32 {
LIBS        += -lws2_32
}

TRNAME       = $${NAME}
include ($${PWD}/../../Translations.pri)
