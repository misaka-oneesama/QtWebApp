#ifndef HTTPGLOBAL_HPP
#define HTTPGLOBAL_HPP

#include <QtGlobal>

#define QTWEBAPP_NAME QtWebApp

#define QTWEBAPP_NAMESPACE_BEGIN namespace QTWEBAPP_NAME {
#define QTWEBAPP_NAMESPACE_END }

#define QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN QTWEBAPP_NAMESPACE_BEGIN namespace HttpServer {
#define QTWEBAPP_HTTPSERVER_NAMESPACE_END QTWEBAPP_NAMESPACE_END }

// This is specific to Windows dll's
#if defined(Q_OS_WIN)
    #if defined(QTWEBAPPLIB_EXPORT)
        #define DECLSPEC Q_DECL_EXPORT
    #elif defined(QTWEBAPPLIB_IMPORT)
        #define DECLSPEC Q_DECL_IMPORT
    #endif
#endif

#if !defined(DECLSPEC)
    #define DECLSPEC
#endif

QTWEBAPP_NAMESPACE_BEGIN

/** Get the library version number */
DECLSPEC const char *getQtWebAppLibVersion();

QTWEBAPP_NAMESPACE_END

#endif // HTTPGLOBAL_HPP
