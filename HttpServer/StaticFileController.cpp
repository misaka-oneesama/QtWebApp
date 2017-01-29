#include "StaticFileController.hpp"

#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>

QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN

StaticFileController::StaticFileController(StaticFileControllerConfig *settings, QObject *parent)
    : HttpRequestHandler(parent)
{
    this->maxAge = settings->maxAge();
    this->encoding = settings->encoding();
    this->docroot = settings->docRoot();

    if(!(this->docroot.startsWith(":/") || this->docroot.startsWith("qrc://")))
    {
        /// TODO / IMPROVEMENT:
        ///  all platforms: resolve environment variables in path
        ///  unix: resolve home tilde to home path
    }

    qDebug("StaticFileController: docroot=%s, encoding=%s, maxAge=%i", qUtf8Printable(this->docroot), qUtf8Printable(this->encoding), this->maxAge);

    this->maxCachedFileSize = settings->maxCachedFileSize();
    this->cache.setMaxCost(settings->cacheSize());
    this->cacheTimeout = settings->cacheTime();

    qDebug("StaticFileController: cache timeout=%u, size=%i", this->cacheTimeout, this->cache.maxCost());
}

void StaticFileController::service(HttpRequest &request, HttpResponse &response)
{
    QByteArray path = request.getPath();

    // Check if we have the file in cache
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    this->mutex.lock();

    CacheEntry *entry = this->cache.object(path);
    if (entry && (this->cacheTimeout == 0 || entry->created>now - this->cacheTimeout))
    {
        QByteArray document = entry->document; // copy the cached document, because other threads may destroy the cached entry immediately after mutex unlock.
        QByteArray filename = entry->filename;
        this->mutex.unlock();

        qDebug("StaticFileController: Cache hit for %s", path.constData());
        this->setContentType(filename, response);

        response.setHeader("Cache-Control", "max-age=" + QByteArray::number(this->maxAge / 1000));
        response.write(document);
    }

    else
    {
        this->mutex.unlock();

        // The file is not in cache.
        qDebug("StaticFileController: Cache miss for %s", path.constData());

        // Forbid access to files outside the docroot directory
        if (path.contains("/.."))
        {
            qWarning("StaticFileController: detected forbidden characters in path %s", path.constData());

            response.setStatus(403, "Forbidden");
            response.write("403 Forbidden", true);
            return;
        }

        /// FIXME: get rid of this?
        ///
        // If the filename is a directory, append index.html.
        if (QFileInfo(docroot + path).isDir())
        {
            path += "/index.html";
        }

        // Try to open the file
        QFile file(this->docroot + path);
        qDebug("StaticFileController: Open file %s", qUtf8Printable(file.fileName()));

        if (file.open(QIODevice::ReadOnly))
        {
            this->setContentType(path, response);
            response.setHeader("Cache-Control", "max-age=" + QByteArray::number(this->maxAge / 1000));
            if (static_cast<quint64>(file.size()) <= this->maxCachedFileSize)
            {
                // Return the file content and store it also in the cache
                entry = new CacheEntry();
                while (!file.atEnd() && !file.error())
                {
                    QByteArray buffer = file.read(65536);
                    response.write(buffer);
                    entry->document.append(buffer);
                }

                entry->created = now;
                entry->filename = path;

                this->mutex.lock();
                this->cache.insert(request.getPath(), entry, entry->document.size());
                this->mutex.unlock();
            }

            else
            {
                // Return the file content, do not store in cache
                while (!file.atEnd() && !file.error())
                {
                    response.write(file.read(65536));
                }
            }

            file.close();
        }

        else
        {
            if (file.exists())
            {
                qWarning("StaticFileController: Cannot open existing file %s for reading", qUtf8Printable(file.fileName()));
                response.setStatus(403, "Forbidden");
                response.write("403 Forbidden", true);
            }

            else
            {
                response.setStatus(404, "Not Found");
                response.write("404 Not Found", true);
            }
        }
    }
}

void StaticFileController::setContentTypeEncoding(const QString &encoding)
{
    this->encoding = encoding;
}

void StaticFileController::setContentType(const QString &fileName, HttpResponse &response) const
{
    static const QMimeDatabase db;
    QByteArray mimeType = db.mimeTypeForFile(this->docroot + fileName).name().toUtf8();
    qDebug("StaticFileController: MIME type for file '%s' -> %s", qUtf8Printable(this->docroot + fileName), mimeType.constData());

    if (this->encoding.isEmpty())
    {
        response.setHeader("Content-Type", mimeType);
    }

    else
    {
        response.setHeader("Content-Type", mimeType + "; charset=" + this->encoding.toUtf8());
    }

    mimeType.clear();
}

QTWEBAPP_HTTPSERVER_NAMESPACE_END
