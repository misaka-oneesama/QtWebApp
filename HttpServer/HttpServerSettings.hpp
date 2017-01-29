#ifndef HTTPLISTENERSETTINGS_HPP
#define HTTPLISTENERSETTINGS_HPP

#include <QtGlobal>
#include <QString>
#include <QByteArray>

struct HttpServerSettings
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

class HttpSessionStoreSettings
{
public:
    HttpSessionStoreSettings()
    {
    }

    ~HttpSessionStoreSettings()
    {
        this->m_cookieName.clear();
        this->m_cookieName.clear();
        this->m_cookieComment.clear();
        this->m_cookieDomain.clear();
        this->m_expirationTime = {};
    }

    void setCookieName(const QByteArray &cookieName)
    {
        this->m_cookieName = cookieName;
    }

    const QByteArray &cookieName() const
    {
        return this->m_cookieName.isEmpty() ? this->m_defaultCookieName : this->m_cookieName;
    }

    void setCookiePath(const QByteArray &cookiePath)
    {
        this->m_cookiePath = cookiePath;
    }

    const QByteArray &cookiePath() const
    {
        return this->m_cookiePath;
    }

    void setCookieComment(const QByteArray &cookieComment)
    {
        this->m_cookieComment = cookieComment;
    }

    const QByteArray &cookieComment() const
    {
        return this->m_cookieComment;
    }

    void setCookieDomain(const QByteArray &cookieDomain)
    {
        this->m_cookieDomain = cookieDomain;
    }

    const QByteArray &cookieDomain() const
    {
        return this->m_cookieDomain;
    }

    void setExpirationTime(const qint64 &expirationTime)
    {
        this->m_expirationTime = expirationTime;
    }

    const qint64 &expirationTime() const
    {
        return this->m_expirationTime;
    }

private:
    QByteArray m_cookieName;
    QByteArray m_cookiePath;
    QByteArray m_cookieComment;
    QByteArray m_cookieDomain;
    qint64 m_expirationTime = 3600000LL;

    static const QByteArray m_defaultCookieName;
};

class StaticFileControllerConfig
{
public:
    StaticFileControllerConfig();
    StaticFileControllerConfig(const QString &docRoot, const QString &encoding = QString("UTF-8"), const quint32 &maxAge = 60000U);
    ~StaticFileControllerConfig();

    void setDocRoot(const QString &path);
    void setEncoding(const QString &encoding);
    void setMaxAge(const quint32 &maxAge);
    void setMaxCachedFileSize(const quint64 &maxCachedFileSize);
    void setCacheTime(const quint32 &cacheTime);
    void setCacheSize(const int &cacheSize);

    const QString &docRoot() const;
    const QString &encoding() const;
    const quint32 &maxAge() const;
    const quint64 &maxCachedFileSize() const;
    const quint32 &cacheTime() const;
    const int &cacheSize() const;

private:
    QString m_docRoot = QString("."); // process working directory
    QString m_encoding = QString("UTF-8");
    quint32 m_maxAge = 60000U;
    quint64 m_maxCachedFileSize = 65536ULL;
    quint32 m_cacheTime = 60000U;
    int m_cacheSize = 1000000;
};

#endif // HTTPLISTENERSETTINGS_HPP
