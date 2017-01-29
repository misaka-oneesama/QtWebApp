#include "HttpServerSettings.hpp"

QTWEBAPP_NAMESPACE_BEGIN

const QByteArray HttpSessionStoreSettings::m_defaultCookieName = QByteArray("sessionid");

StaticFileControllerConfig::StaticFileControllerConfig()
{
}

StaticFileControllerConfig::StaticFileControllerConfig(const QString &docRoot, const QString &encoding, const quint32 &maxAge)
{
    this->m_docRoot = docRoot;
    this->m_encoding = encoding;
    this->m_maxAge = maxAge;
}

StaticFileControllerConfig::~StaticFileControllerConfig()
{
    this->m_docRoot.clear();
    this->m_encoding.clear();
    this->m_maxAge = {};
}

void StaticFileControllerConfig::setDocRoot(const QString &path)
{
    this->m_docRoot = path;
}

void StaticFileControllerConfig::setEncoding(const QString &encoding)
{
    this->m_encoding = encoding;
}

void StaticFileControllerConfig::setMaxAge(const quint32 &maxAge)
{
    this->m_maxAge = maxAge;
}

void StaticFileControllerConfig::setMaxCachedFileSize(const quint64 &maxCachedFileSize)
{
    this->m_maxCachedFileSize = maxCachedFileSize;
}

void StaticFileControllerConfig::setCacheTime(const quint32 &cacheTime)
{
    this->m_cacheTime = cacheTime;
}

void StaticFileControllerConfig::setCacheSize(const int &cacheSize)
{
    this->m_cacheSize = cacheSize;
}

const QString &StaticFileControllerConfig::docRoot() const
{
    return this->m_docRoot;
}

const QString &StaticFileControllerConfig::encoding() const
{
    return this->m_encoding;
}

const quint32 &StaticFileControllerConfig::maxAge() const
{
    return this->m_maxAge;
}

const quint64 &StaticFileControllerConfig::maxCachedFileSize() const
{
    return this->m_maxCachedFileSize;
}

const quint32 &StaticFileControllerConfig::cacheTime() const
{
    return this->m_cacheTime;
}

const int &StaticFileControllerConfig::cacheSize() const
{
    return this->m_cacheSize;
}

QTWEBAPP_NAMESPACE_END
