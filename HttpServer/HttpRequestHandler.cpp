#include "HttpRequestHandler.hpp"

QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN

HttpRequestHandler::HttpRequestHandler(QObject *parent)
    : QObject(parent)
{
}

HttpRequestHandler::~HttpRequestHandler()
{
}

void HttpRequestHandler::service(HttpRequest &request, HttpResponse &response)
{
    qCritical("HttpRequestHandler: you need to override the service() function");
    qDebug("HttpRequestHandler: request=%s %s %s", request.getMethod().constData(), request.getPath().constData(), request.getVersion().constData());
    response.setHeader("Content-Type", "text/plain; charset=UTF-8");
    response.setStatus(501, "Not Implemented");
    response.write("501 Not Implemented", true);
}

QTWEBAPP_HTTPSERVER_NAMESPACE_END
