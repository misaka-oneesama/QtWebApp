INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

QT += network

# Enable very detailed debug messages when compiling the debug version
CONFIG (debug, debug|release) {
    DEFINES += SUPERVERBOSE
}

HEADERS += $$PWD/HttpGlobal.hpp \
           $$PWD/HttpListener.hpp \
           $$PWD/HttpServerSettings.hpp \
           $$PWD/HttpConnectionHandler.hpp \
           $$PWD/HttpConnectionHandlerPool.hpp \
           $$PWD/HttpRequest.hpp \
           $$PWD/HttpResponse.hpp \
           $$PWD/httpcookie.h \
           $$PWD/httprequesthandler.h \
           $$PWD/httpsession.h \
           $$PWD/httpsessionstore.h \
           $$PWD/staticfilecontroller.h

SOURCES += $$PWD/HttpGlobal.cpp \
           $$PWD/HttpListener.cpp \
           $$PWD/HttpConnectionHandler.cpp \
           $$PWD/HttpConnectionHandlerPool.cpp \
           $$PWD/HttpRequest.cpp \
           $$PWD/HttpResponse.cpp \
           $$PWD/httpcookie.cpp \
           $$PWD/httprequesthandler.cpp \
           $$PWD/httpsession.cpp \
           $$PWD/httpsessionstore.cpp \
           $$PWD/staticfilecontroller.cpp
