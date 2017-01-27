#ifndef HTTPLISTENERSETTINGS_H
#define HTTPLISTENERSETTINGS_H

#include <QtGlobal>
#include <QString>

struct HttpListenerSettings
{
    QLatin1String host;
    quint16 port;
    quint32 minThreads = 4U;
    quint32 maxThreads = 100U;
    quint32 cleanupInterval = 1000U;
    quint32 readTimeout = 60000U;
    quint64 maxRequestSize = 1600ULL;
    quint64 maxMultiPartSize = 1000000ULL;
    QString sslKeyFile;
    QString sslCertFile;
};

#endif // HTTPLISTENERSETTINGS_H
