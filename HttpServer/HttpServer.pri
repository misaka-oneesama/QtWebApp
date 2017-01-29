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
           $$PWD/HttpCookie.hpp \
           $$PWD/HttpRequestHandler.hpp \
           $$PWD/HttpSession.hpp \
           $$PWD/HttpSessionStore.hpp \
           $$PWD/StaticFileController.hpp

SOURCES += $$PWD/HttpGlobal.cpp \
           $$PWD/HttpListener.cpp \
           $$PWD/HttpServerSettings.cpp \
           $$PWD/HttpConnectionHandler.cpp \
           $$PWD/HttpConnectionHandlerPool.cpp \
           $$PWD/HttpRequest.cpp \
           $$PWD/HttpResponse.cpp \
           $$PWD/HttpCookie.cpp \
           $$PWD/HttpRequestHandler.cpp \
           $$PWD/HttpSession.cpp \
           $$PWD/HttpSessionStore.cpp \
           $$PWD/StaticFileController.cpp
