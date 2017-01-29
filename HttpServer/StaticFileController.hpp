#ifndef STATICFILECONTROLLER_HPP
#define STATICFILECONTROLLER_HPP

#include <QCache>
#include <QMutex>

#include "HttpGlobal.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpRequestHandler.hpp"
#include "HttpServerSettings.hpp"

namespace stefanfrings {

/**
  Delivers static files. It is usually called by the applications main request handler when
  the caller requests a path that is mapped to static files.
  <p>
  The following settings are required in the config file:
  <code><pre>
  path=../docroot
  encoding=UTF-8
  maxAge=60000
  cacheTime=60000
  cacheSize=1000000
  maxCachedFileSize=65536
  </pre></code>
  The path is relative to the directory of the config file. In case of windows, if the
  settings are in the registry, the path is relative to the current working directory.
  <p>
  The encoding is sent to the web browser in case of text and html files.
  <p>
  The cache improves performance of small files when loaded from a network
  drive. Large files are not cached. Files are cached as long as possible,
  when cacheTime=0. The maxAge value (in msec!) controls the remote browsers cache.
  <p>
  Do not instantiate this class in each request, because this would make the file cache
  useless. Better create one instance during start-up and call it when the application
  received a related HTTP request.
*/

class DECLSPEC StaticFileController : public HttpRequestHandler
{
    Q_OBJECT
    Q_DISABLE_COPY(StaticFileController)

public:

    /** Constructor */
    StaticFileController(StaticFileControllerConfig *settings, QObject *parent = nullptr);

    /** Generates the response */
    void service(HttpRequest &request, HttpResponse &response);

    /** Sets the content type encoding header */
    void setContentTypeEncoding(const QString &encoding);

private:

    /** Encoding of text files */
    QString encoding;

    /** Root directory of documents */
    QString docroot;

    /** Maximum age of files in the browser cache */
    quint32 maxAge;

    struct CacheEntry {
        QByteArray document;
        qint64 created;
        QByteArray filename;
    };

    /** Timeout for each cached file */
    quint32 cacheTimeout;

    /** Maximum size of files in cache, larger files are not cached */
    quint64 maxCachedFileSize;

    /** Cache storage */
    QCache<QString, CacheEntry> cache;

    /** Used to synchronize cache access for threads */
    QMutex mutex;

    /** Set a content-type header in the response depending on the mime type of the file */
    void setContentType(const QString &file, HttpResponse &response) const;
};

} // end of namespace

#endif // STATICFILECONTROLLER_HPP
