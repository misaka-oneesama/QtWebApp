INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

QT += network

# Enable very detailed debug messages when compiling the debug version
CONFIG (debug, debug|release) {
    DEFINES += SUPERVERBOSE
}

HEADERS += $$PWD/HttpGlobal.hpp \
           $$PWD/httplistener.h \
           $$PWD/HttpServerSettings.hpp \
           $$PWD/HttpConnectionHandler.hpp \
           $$PWD/HttpConnectionHandlerPool.hpp \
           $$PWD/httprequest.h \
           $$PWD/httpresponse.h \
           $$PWD/httpcookie.h \
           $$PWD/httprequesthandler.h \
           $$PWD/httpsession.h \
           $$PWD/httpsessionstore.h \
           $$PWD/staticfilecontroller.h

SOURCES += $$PWD/HttpGlobal.cpp \
           $$PWD/httplistener.cpp \
           $$PWD/HttpConnectionHandler.cpp \
           $$PWD/HttpConnectionHandlerPool.cpp \
           $$PWD/httprequest.cpp \
           $$PWD/httpresponse.cpp \
           $$PWD/httpcookie.cpp \
           $$PWD/httprequesthandler.cpp \
           $$PWD/httpsession.cpp \
           $$PWD/httpsessionstore.cpp \
           $$PWD/staticfilecontroller.cpp
