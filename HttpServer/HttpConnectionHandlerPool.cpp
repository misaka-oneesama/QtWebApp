#ifndef QT_NO_OPENSSL
    #include <QSslSocket>
    #include <QSslKey>
    #include <QSslCertificate>
    #include <QSslConfiguration>
#endif

#include "HttpConnectionHandlerPool.hpp"

using namespace stefanfrings;

HttpConnectionHandlerPool::HttpConnectionHandlerPool(HttpServerSettings *settings, HttpRequestHandler *requestHandler)
    : QObject()
{
    this->settings = settings;
    this->requestHandler = requestHandler;
    this->loadSslConfig();
    this->cleanupTimer.start(this->settings->cleanupInterval);
    QObject::connect(&this->cleanupTimer, &QTimer::timeout, this, &HttpConnectionHandlerPool::cleanup);
}

HttpConnectionHandlerPool::~HttpConnectionHandlerPool()
{
    // delete all connection handlers and wait until their threads are closed

    for (HttpConnectionHandler *handler : this->pool)
    {
        delete handler;
    }

    delete this->sslConfiguration;
    this->sslConfiguration = nullptr;
    qDebug("HttpConnectionHandlerPool (%p): destroyed", this);
}

HttpConnectionHandler *HttpConnectionHandlerPool::getConnectionHandler()
{
    HttpConnectionHandler *freeHandler = nullptr;
    this->mutex.lock();

    // find a free handler in pool
    for (HttpConnectionHandler *handler : this->pool)
    {
        if (!handler->isBusy())
        {
            freeHandler = handler;
            freeHandler->setBusy();
            break;
        }
    }

    // create a new handler, if necessary
    if (!freeHandler)
    {
        if (static_cast<quint32>(this->pool.count()) < this->settings->maxThreads)
        {
            freeHandler = new HttpConnectionHandler(this->settings, this->requestHandler, this->sslConfiguration);
            freeHandler->setBusy();
            this->pool.append(freeHandler);
        }
    }

    this->mutex.unlock();
    return freeHandler;
}

void HttpConnectionHandlerPool::cleanup()
{
    quint32 idleCounter = 0;

    this->mutex.lock();

    for (HttpConnectionHandler *handler : pool)
    {
        if (!handler->isBusy())
        {
            if (++idleCounter > this->settings->minThreads)
            {
                delete handler;
                this->pool.removeOne(handler);
                qDebug("HttpConnectionHandlerPool: Removed connection handler (%p), pool size is now %i",handler,pool.size());
                break; // remove only one handler in each interval
            }
        }
    }

    this->mutex.unlock();
}

void HttpConnectionHandlerPool::loadSslConfig()
{
    // If certificate and key files are configured, then load them
    QString sslKeyFileName = this->settings->sslKeyFile;
    QString sslCertFileName = this->settings->sslCertFile;

    if (!sslKeyFileName.isEmpty() && !sslCertFileName.isEmpty())
    {
        #ifdef QT_NO_OPENSSL
            qWarning("HttpConnectionHandlerPool: SSL is not supported");
        #else

            /// TODO / IMPROVEMENT:
            ///  all platforms: resolve environment variables in path
            ///  unix: resolve home tilde to home path

            // Load the SSL certificate
            QFile certFile(sslCertFileName);
            if (!certFile.open(QIODevice::ReadOnly))
            {
                qCritical("HttpConnectionHandlerPool: cannot open sslCertFile %s", qUtf8Printable(sslCertFileName));
                return;
            }

            QSslCertificate certificate(&certFile, QSsl::Pem);
            certFile.close();

            // Load the key file
            QFile keyFile(sslKeyFileName);
            if (!keyFile.open(QIODevice::ReadOnly))
            {
                qCritical("HttpConnectionHandlerPool: cannot open sslKeyFile %s", qUtf8Printable(sslKeyFileName));
                return;
            }

            QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
            keyFile.close();

            // Create the SSL configuration
            this->sslConfiguration = new QSslConfiguration();
            this->sslConfiguration->setLocalCertificate(certificate);
            this->sslConfiguration->setPrivateKey(sslKey);
            this->sslConfiguration->setPeerVerifyMode(QSslSocket::VerifyNone);
            this->sslConfiguration->setProtocol(QSsl::TlsV1SslV3);

            qDebug("HttpConnectionHandlerPool: SSL settings loaded");
         #endif
    }
}
