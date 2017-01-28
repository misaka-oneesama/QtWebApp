#include "HttpListener.hpp"
#include "HttpConnectionHandler.hpp"
#include "HttpConnectionHandlerPool.hpp"

#include <QCoreApplication>

using namespace stefanfrings;

HttpListener::HttpListener(HttpServerSettings *settings, HttpRequestHandler* requestHandler, QObject *parent)
    : QTcpServer(parent)
{
    Q_ASSERT(requestHandler != nullptr);
    this->settings = settings;
    this->requestHandler = requestHandler;

    // Reqister type of socketDescriptor for signal/slot handling
    qRegisterMetaType<tSocketDescriptor>("tSocketDescriptor");

    // Start listening
    this->listen();
}

HttpListener::~HttpListener()
{
    this->close();
    qDebug("HttpListener: destroyed");
}

void HttpListener::listen()
{
    if (!this->pool)
    {
        this->pool = new HttpConnectionHandlerPool(this->settings, this->requestHandler);
    }

    QTcpServer::listen(QString(this->settings->host).isEmpty() ? QHostAddress::Any : QHostAddress(this->settings->host), this->settings->port);

    if (!this->isListening())
    {
        qCritical("HttpListener: Cannot bind on port %i: %s", this->settings->port, qUtf8Printable(this->errorString()));
    }

    else
    {
        qDebug("HttpListener: Listening on port %s:%i", qUtf8Printable(this->settings->host), this->settings->port);
    }
}

void HttpListener::close()
{
    QTcpServer::close();
    qDebug("HttpListener: closed");

    if (this->pool)
    {
        delete this->pool;
        this->pool = nullptr;
    }
}

void HttpListener::incomingConnection(tSocketDescriptor socketDescriptor)
{
#ifdef SUPERVERBOSE
    qDebug("HttpListener: New connection");
#endif

    HttpConnectionHandler *freeHandler = nullptr;

    if (this->pool)
    {
        freeHandler = this->pool->getConnectionHandler();
    }

    // Let the handler process the new connection.
    if (freeHandler)
    {
        // The descriptor is passed via event queue because the handler lives in another thread
        QMetaObject::invokeMethod(freeHandler, "handleConnection", Qt::QueuedConnection, Q_ARG(tSocketDescriptor, socketDescriptor));
    }

    else
    {
        // Reject the connection
        qDebug("HttpListener: Too many incoming connections");
        QTcpSocket *socket = new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
        socket->write("HTTP/1.1 503 Too Many Connections\nConnection: close\n\nToo Many Connections\n");
        socket->disconnectFromHost();
    }
}
