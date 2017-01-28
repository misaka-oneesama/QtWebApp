#include "HttpConnectionHandler.hpp"
#include "HttpResponse.hpp"

using namespace stefanfrings;

HttpConnectionHandler::HttpConnectionHandler(HttpServerSettings *settings, HttpRequestHandler *requestHandler, QSslConfiguration *sslConfiguration)
    : QThread()
{
    Q_ASSERT(requestHandler != nullptr);

    this->settings = settings;
    this->requestHandler = requestHandler;
    this->sslConfiguration = sslConfiguration;

    // Create TCP or SSL socket
    this->createSocket();

    // execute signals in my own thread
    this->moveToThread(this);
    this->socket->moveToThread(this);
    this->readTimer.moveToThread(this);

    // Connect signals
    QObject::connect(this->socket, &QTcpSocket::readyRead, this, &HttpConnectionHandler::read);
    QObject::connect(this->socket, &QTcpSocket::disconnected, this, &HttpConnectionHandler::disconnected);
    QObject::connect(&this->readTimer, &QTimer::timeout, this, &HttpConnectionHandler::readTimeout);
    this->readTimer.setSingleShot(true);

    qDebug("HttpConnectionHandler (%p): constructed", this);
    this->start();
}

HttpConnectionHandler::~HttpConnectionHandler()
{
    this->quit();
    this->wait();
    qDebug("HttpConnectionHandler (%p): destroyed", this);
}

void HttpConnectionHandler::createSocket()
{
    // If SSL is supported and configured, then create an instance of QSslSocket
    #ifndef QT_NO_OPENSSL
        if (this->sslConfiguration)
        {
            QSslSocket *sslSocket = new QSslSocket();
            sslSocket->setSslConfiguration(*sslConfiguration);
            this->socket = sslSocket;
            qDebug("HttpConnectionHandler (%p): SSL is enabled", this);
            return;
        }
    #endif

    // else create an instance of QTcpSocket
    this->socket = new QTcpSocket();
}

void HttpConnectionHandler::run()
{
    qDebug("HttpConnectionHandler (%p): thread started", this);

    try
    {
        this->exec();
    }

    catch (...)
    {
        qCritical("HttpConnectionHandler (%p): an uncatched exception occured in the thread",this);
    }

    this->socket->close();
    delete this->socket;
    this->readTimer.stop();
    qDebug("HttpConnectionHandler (%p): thread stopped", this);
}

void HttpConnectionHandler::handleConnection(tSocketDescriptor socketDescriptor)
{
    qDebug("HttpConnectionHandler (%p): handle new connection", this);
    this->busy = true;
    Q_ASSERT(this->socket->isOpen() == false); // if not, then the handler is already busy

    ///// FIXME: is this still required?
    // UGLY workaround - we need to clear writebuffer before reusing this socket
    // https://bugreports.qt-project.org/browse/QTBUG-28914
    this->socket->connectToHost("", 0);
    this->socket->abort();

    if (!this->socket->setSocketDescriptor(socketDescriptor))
    {
        qCritical("HttpConnectionHandler (%p): cannot initialize socket: %s", this,qPrintable(this->socket->errorString()));
        return;
    }

    #ifndef QT_NO_OPENSSL
        // Switch on encryption, if SSL is configured
        if (this->sslConfiguration)
        {
            qDebug("HttpConnectionHandler (%p): Starting encryption", this);
            static_cast<QSslSocket*>(this->socket)->startServerEncryption();
        }
    #endif

    // Start timer for read timeout
    this->readTimer.start(this->settings->readTimeout);

    // delete previous request
    delete this->currentRequest;
    this->currentRequest = nullptr;
}

bool HttpConnectionHandler::isBusy()
{
    return busy;
}

void HttpConnectionHandler::setBusy()
{
    this->busy = true;
}

void HttpConnectionHandler::readTimeout()
{
    qDebug("HttpConnectionHandler (%p): read timeout occured",this);

    this->socket->flush();
    this->socket->disconnectFromHost();

    delete this->currentRequest;
    this->currentRequest = nullptr;
}

void HttpConnectionHandler::disconnected()
{
    qDebug("HttpConnectionHandler (%p): disconnected", this);

    this->socket->close();
    this->readTimer.stop();
    this->busy = false;
}

void HttpConnectionHandler::read()
{
    // The loop adds support for HTTP pipelinig
    while (this->socket->bytesAvailable())
    {
        #ifdef SUPERVERBOSE
            qDebug("HttpConnectionHandler (%p): read input",this);
        #endif

        // Create new HttpRequest object if necessary
        if (!this->currentRequest)
        {
            this->currentRequest = new HttpRequest(this->settings);
        }

        // Collect data for the request object
        while (this->socket->bytesAvailable() &&
               this->currentRequest->getStatus() != HttpRequest::Complete &&
               this->currentRequest->getStatus() != HttpRequest::Abort)
        {
            this->currentRequest->readFromSocket(this->socket);
            if (this->currentRequest->getStatus() == HttpRequest::WaitForBody)
            {
                // Restart timer for read timeout, otherwise it would
                // expire during large file uploads.
                this->readTimer.start(this->settings->readTimeout);
            }
        }

        // If the request is aborted, return error message and close the connection
        if (this->currentRequest->getStatus() == HttpRequest::Abort)
        {
            this->socket->write("HTTP/1.1 413 Entity Too Large\nConnection: close\n\n413 Entity Too Large\n");
            this->socket->flush();
            this->socket->disconnectFromHost();
            delete this->currentRequest;
            this->currentRequest = nullptr;
            return;
        }

        // If the request is complete, let the request mapper dispatch it
        if (this->currentRequest->getStatus() == HttpRequest::Complete)
        {
            this->readTimer.stop();
            qDebug("HttpConnectionHandler (%p): received request",this);

            // Copy the Connection:close header to the response
            HttpResponse response(this->socket);
            bool closeConnection = QString::compare(this->currentRequest->getHeader("Connection"), "close", Qt::CaseInsensitive) == 0;
            if (closeConnection)
            {
                response.setHeader("Connection", "close");
            }

            // In case of HTTP 1.0 protocol add the Connection:close header.
            // This ensures that the HttpResponse does not activate chunked mode, which is not spported by HTTP 1.0.
            else
            {
                if (QString::compare(this->currentRequest->getVersion(), "HTTP/1.0", Qt::CaseInsensitive) == 0)
                {
                    closeConnection = true;
                    response.setHeader("Connection", "close");
                }
            }

            // Call the request mapper
            try
            {
                this->requestHandler->service(*this->currentRequest, response);
            }

            catch (...)
            {
                qCritical("HttpConnectionHandler (%p): An uncatched exception occured in the request handler",this);
            }

            // Finalize sending the response if not already done
            if (!response.hasSentLastPart())
            {
                response.write(QByteArray(), true);
            }

            qDebug("HttpConnectionHandler (%p): finished request",this);

            // Find out whether the connection must be closed
            if (!closeConnection)
            {
                // Maybe the request handler or mapper added a Connection:close header in the meantime
                if (QString::compare(response.getHeaders().value("Connection"), "close", Qt::CaseInsensitive) == 0)
                {
                    closeConnection = true;
                }

                else
                {
                    // If we have no Content-Length header and did not use chunked mode, then we have to close the
                    // connection to tell the HTTP client that the end of the response has been reached.
                    if (!response.getHeaders().contains("Content-Length"))
                    {
                        if (QString::compare(response.getHeaders().value("Transfer-Encoding"), "chunked", Qt::CaseInsensitive) != 0)
                        {
                            closeConnection = true;
                        }
                    }
                }
            }

            // Close the connection or prepare for the next request on the same connection.
            if (closeConnection)
            {
                this->socket->flush();
                this->socket->disconnectFromHost();
            }

            else
            {
                // Start timer for next request
                readTimer.start(this->settings->readTimeout);
            }

            delete this->currentRequest;
            this->currentRequest = nullptr;
        }
    }
}
