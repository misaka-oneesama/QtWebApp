INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD/HttpServer

QT += network

# Enable very detailed debug messages when compiling the debug version
CONFIG (debug, debug|release) {
    DEFINES += QTWEBAPP_SUPERVERBOSE
}

HEADERS += $$PWD/HttpServer/HttpGlobal.hpp \
           $$PWD/HttpServer/HttpListener.hpp \
           $$PWD/HttpServer/HttpServerSettings.hpp \
           $$PWD/HttpServer/HttpConnectionHandler.hpp \
           $$PWD/HttpServer/HttpConnectionHandlerPool.hpp \
           $$PWD/HttpServer/HttpRequest.hpp \
           $$PWD/HttpServer/HttpResponse.hpp \
           $$PWD/HttpServer/HttpCookie.hpp \
           $$PWD/HttpServer/HttpRequestHandler.hpp \
           $$PWD/HttpServer/HttpSession.hpp \
           $$PWD/HttpServer/HttpSessionStore.hpp \
           $$PWD/HttpServer/StaticFileController.hpp

SOURCES += $$PWD/HttpServer/HttpGlobal.cpp \
           $$PWD/HttpServer/HttpListener.cpp \
           $$PWD/HttpServer/HttpServerSettings.cpp \
           $$PWD/HttpServer/HttpConnectionHandler.cpp \
           $$PWD/HttpServer/HttpConnectionHandlerPool.cpp \
           $$PWD/HttpServer/HttpRequest.cpp \
           $$PWD/HttpServer/HttpResponse.cpp \
           $$PWD/HttpServer/HttpCookie.cpp \
           $$PWD/HttpServer/HttpRequestHandler.cpp \
           $$PWD/HttpServer/HttpSession.cpp \
           $$PWD/HttpServer/HttpSessionStore.cpp \
           $$PWD/HttpServer/StaticFileController.cpp
