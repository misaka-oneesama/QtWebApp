#include "HttpRequest.hpp"
#include "httpcookie.h"

#include <QList>
#include <QDir>

using namespace stefanfrings;

HttpRequest::HttpRequest(HttpServerSettings *settings)
{
    this->status = WaitForRequest;
    this->currentSize = 0;
    this->expectedBodySize = 0;
    this->maxSize = settings->maxRequestSize;
    this->maxMultiPartSize = settings->maxMultiPartSize;
}

void HttpRequest::readRequest(QTcpSocket *socket)
{
    #ifdef SUPERVERBOSE
        qDebug("HttpRequest: read request");
    #endif

    int toRead = this->maxSize - this->currentSize + 1; // allow one byte more to be able to detect overflow
    this->lineBuffer.append(socket->readLine(toRead));
    this->currentSize += this->lineBuffer.size();

    if (!this->lineBuffer.contains('\r') && !this->lineBuffer.contains('\n'))
    {
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: collecting more parts until line break");
        #endif

        return;
    }

    QByteArray newData = this->lineBuffer.trimmed();
    this->lineBuffer.clear();

    if (!newData.isEmpty())
    {
        QList<QByteArray> list = newData.split(' ');
        if (list.count() != 3 || !list.at(2).contains("HTTP"))
        {
            qWarning("HttpRequest: received broken HTTP request, invalid first line");
            this->status = Abort;
        }

        else
        {
            this->method = list.at(0).trimmed();
            this->path = list.at(1);
            this->version = list.at(2);
            this->peerAddress = socket->peerAddress();
            this->status = WaitForHeader;
        }
    }
}

void HttpRequest::readHeader(QTcpSocket *socket)
{
    #ifdef SUPERVERBOSE
        qDebug("HttpRequest: read header");
    #endif

    int toRead = this->maxSize - this->currentSize + 1; // allow one byte more to be able to detect overflow
    this->lineBuffer.append(socket->readLine(toRead));
    this->currentSize += this->lineBuffer.size();

    if (!this->lineBuffer.contains('\r') && !this->lineBuffer.contains('\n'))
    {
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: collecting more parts until line break");
        #endif

        return;
    }

    QByteArray newData = this->lineBuffer.trimmed();
    this->lineBuffer.clear();

    int colon = newData.indexOf(':');
    if (colon > 0)
    {
        // Received a line with a colon - a header
        this->currentHeader = newData.left(colon).toLower();
        QByteArray value = newData.mid(colon + 1).trimmed();
        this->headers.insert(this->currentHeader, value);

        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: received header %s: %s",currentHeader.data(),value.data());
        #endif
    }

    else if (!newData.isEmpty())
    {
        // received another line - belongs to the previous header
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: read additional line of header");
        #endif

        // Received additional line of previous header
        if (this->headers.contains(currentHeader))
        {
            this->headers.insert(this->currentHeader, this->headers.value(this->currentHeader) + " " + newData);
        }
    }

    else
    {
        // received an empty line - end of headers reached
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: headers completed");
        #endif

        // Empty line received, that means all headers have been received
        // Check for multipart/form-data
        QByteArray contentType = this->headers.value("content-type");
        if (contentType.startsWith("multipart/form-data"))
        {
            int posi = contentType.indexOf("boundary=");
            if (posi >= 0)
            {
                this->boundary = contentType.mid(posi + 9);
                if (this->boundary.startsWith('"') && this->boundary.endsWith('"'))
                {
                   this->boundary = this->boundary.mid(1, this->boundary.length() - 2);
                }
            }
        }

        QByteArray contentLength = headers.value("content-length");
        if (!contentLength.isEmpty())
        {
            this->expectedBodySize = contentLength.toInt();
        }

        if (this->expectedBodySize == 0)
        {
            #ifdef SUPERVERBOSE
                qDebug("HttpRequest: expect no body");
            #endif

            this->status = Complete;
        }

        else if (this->boundary.isEmpty() && this->expectedBodySize + this->currentSize > this->maxSize)
        {
            qWarning("HttpRequest: expected body is too large");
            this->status = Abort;
        }

        else if (!this->boundary.isEmpty() && this->expectedBodySize > this->maxMultiPartSize)
        {
            qWarning("HttpRequest: expected multipart body is too large");
            this->status = Abort;
        }

        else
        {
            #ifdef SUPERVERBOSE
                qDebug("HttpRequest: expect %i bytes body",expectedBodySize);
            #endif

            this->status = WaitForBody;
        }
    }
}

void HttpRequest::readBody(QTcpSocket *socket)
{
    Q_ASSERT(this->expectedBodySize != 0);

    if (this->boundary.isEmpty())
    {
        // normal body, no multipart
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: receive body");
        #endif

        int toRead = this->expectedBodySize - this->bodyData.size();
        QByteArray newData = socket->read(toRead);
        this->currentSize += newData.size();
        this->bodyData.append(newData);

        if (this->bodyData.size() >= this->expectedBodySize)
        {
            this->status = Complete;
        }
    }

    else
    {
        // multipart body, store into temp file
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: receiving multipart body");
        #endif

        // Create an object for the temporary file, if not already present
        if (!this->tempFile)
        {
            this->tempFile = new QTemporaryFile;
        }

        if (!tempFile->isOpen())
        {
            tempFile->open();
        }

        // Transfer data in 64kb blocks
        int fileSize = this->tempFile->size();
        int toRead = this->expectedBodySize - fileSize;

        if (toRead > 65536)
        {
            toRead = 65536;
        }

        fileSize += this->tempFile->write(socket->read(toRead));
        if (fileSize >= this->maxMultiPartSize)
        {
            qWarning("HttpRequest: received too many multipart bytes");
            this->status = Abort;
        }

        else if (fileSize >= this->expectedBodySize)
        {
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: received whole multipart body");
        #endif

            this->tempFile->flush();
            if (this->tempFile->error())
            {
                qCritical("HttpRequest: Error writing temp file for multipart body");
            }

            this->parseMultiPartFile();
            this->tempFile->close();
            this->status = Complete;
        }
    }
}

void HttpRequest::decodeRequestParams()
{
    #ifdef SUPERVERBOSE
        qDebug("HttpRequest: extract and decode request parameters");
    #endif

    // Get URL parameters
    QByteArray rawParameters;
    int questionMark = this->path.indexOf('?');
    if (questionMark >= 0)
    {
        rawParameters = this->path.mid(questionMark + 1);
        this->path = this->path.left(questionMark);
    }

    // Get request body parameters
    QByteArray contentType = this->headers.value("content-type");
    if (!this->bodyData.isEmpty() && (contentType.isEmpty() || contentType.startsWith("application/x-www-form-urlencoded")))
    {
        if (!rawParameters.isEmpty())
        {
            rawParameters.append('&');
            rawParameters.append(this->bodyData);
        }

        else
        {
            rawParameters = this->bodyData;
        }
    }

    // Split the parameters into pairs of value and name
    QList<QByteArray> list = rawParameters.split('&');
    for (auto&& part : list)
    {
        int equalsChar = part.indexOf('=');
        if (equalsChar >= 0)
        {
            QByteArray name = part.left(equalsChar).trimmed();
            QByteArray value = part.mid(equalsChar + 1).trimmed();
            this->parameters.insert(this->urlDecode(name), this->urlDecode(value));
        }

        else if (!part.isEmpty())
        {
            // Name without value
            this->parameters.insert(this->urlDecode(part), "");
        }
    }
}

void HttpRequest::extractCookies()
{
    #ifdef SUPERVERBOSE
        qDebug("HttpRequest: extract cookies");
    #endif

    for (const QByteArray &cookieStr : this->headers.values("cookie"))
    {
        QList<QByteArray> list = HttpCookie::splitCSV(cookieStr);
        for (auto&& part : list)
        {
            #ifdef SUPERVERBOSE
                qDebug("HttpRequest: found cookie %s",part.data());
            #endif

            // Split the part into name and value
            QByteArray name;
            QByteArray value;

            int posi = part.indexOf('=');
            if (posi)
            {
                name = part.left(posi).trimmed();
                value = part.mid(posi + 1).trimmed();
            }

            else
            {
                name = part.trimmed();
                value = "";
            }

            this->cookies.insert(name, value);
        }
    }

    this->headers.remove("cookie");
}

void HttpRequest::readFromSocket(QTcpSocket *socket)
{
    Q_ASSERT(this->status != Complete);

    switch (this->status)
    {
        case WaitForRequest:
            this->readRequest(socket);
            break;

        case WaitForHeader:
            this->readHeader(socket);
            break;

        case WaitForBody:
            this->readBody(socket);
            break;

        default: break;
    }

    if ((this->boundary.isEmpty() && this->currentSize > this->maxSize) ||
        (!this->boundary.isEmpty() && this->currentSize > this->maxMultiPartSize))
    {
        qWarning("HttpRequest: received too many bytes");
        this->status = Abort;
    }

    if (this->status == Complete)
    {
        // Extract and decode request parameters from url and body
        this->decodeRequestParams();

        // Extract cookies from headers
        this->extractCookies();
    }
}

HttpRequest::RequestStatus HttpRequest::getStatus() const
{
    return this->status;
}

QByteArray HttpRequest::getMethod() const
{
    return this->method;
}

QByteArray HttpRequest::getPath() const
{
    return this->urlDecode(this->path);
}

const QByteArray &HttpRequest::getRawPath() const
{
    return this->path;
}

QByteArray HttpRequest::getVersion() const
{
    return version;
}

QByteArray HttpRequest::getHeader(const QByteArray &name) const
{
    return this->headers.value(name.toLower());
}

QList<QByteArray> HttpRequest::getHeaders(const QByteArray &name) const
{
    return this->headers.values(name.toLower());
}

QMultiMap<QByteArray, QByteArray> HttpRequest::getHeaderMap() const
{
    return this->headers;
}

QByteArray HttpRequest::getParameter(const QByteArray &name) const
{
    return this->parameters.value(name);
}

QList<QByteArray> HttpRequest::getParameters(const QByteArray &name) const
{
    return this->parameters.values(name);
}

QMultiMap<QByteArray, QByteArray> HttpRequest::getParameterMap() const
{
    return this->parameters;
}

QByteArray HttpRequest::getBody() const
{
    return this->bodyData;
}

QByteArray HttpRequest::urlDecode(const QByteArray &source)
{
    QByteArray buffer(source);
    buffer.replace('+', ' ');
    int percentChar = buffer.indexOf('%');

    while (percentChar >= 0)
    {
        bool ok;
        char byte = buffer.mid(percentChar + 1, 2).toInt(&ok, 16);
        if (ok)
        {
            buffer.replace(percentChar, 3, static_cast<char*>(&byte), 1);
        }
        percentChar = buffer.indexOf('%', percentChar + 1);
    }

    return buffer;
}

void HttpRequest::parseMultiPartFile()
{
    qDebug("HttpRequest: parsing multipart temp file");
    this->tempFile->seek(0);

    bool finished = false;
    while (!this->tempFile->atEnd() && !finished && !this->tempFile->error())
    {
        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: reading multpart headers");
        #endif

        QByteArray fieldName;
        QByteArray fileName;

        while (!this->tempFile->atEnd() && !finished && !this->tempFile->error())
        {
            QByteArray line = this->tempFile->readLine(65536).trimmed();
            if (line.startsWith("Content-Disposition:"))
            {
                if (line.contains("form-data"))
                {
                    int start = line.indexOf(" name=\"");
                    int end = line.indexOf("\"", start + 7);

                    if (start >= 0 && end >= start)
                    {
                        fieldName = line.mid(start + 7, end - start - 7);
                    }

                    start = line.indexOf(" filename=\"");
                    end = line.indexOf("\"", start + 11);

                    if (start >= 0 && end >= start)
                    {
                        fileName = line.mid(start + 11, end - start - 11);
                    }

                    #ifdef SUPERVERBOSE
                        qDebug("HttpRequest: multipart field=%s, filename=%s", fieldName.constData(), fileName.constData());
                    #endif
                }

                else
                {
                    qDebug("HttpRequest: ignoring unsupported content part %s", line.constData());
                }
            }

            else if (line.isEmpty())
            {
                break;
            }
        }

        #ifdef SUPERVERBOSE
            qDebug("HttpRequest: reading multpart data");
        #endif

        QTemporaryFile *uploadedFile = nullptr;
        QByteArray fieldValue;

        while (!this->tempFile->atEnd() && !finished && !this->tempFile->error())
        {
            QByteArray line = this->tempFile->readLine(65536);
            if (line.startsWith("--" + this->boundary))
            {
                // Boundary found. Until now we have collected 2 bytes too much,
                // so remove them from the last result
                if (fileName.isEmpty() && !fieldName.isEmpty())
                {
                    // last field was a form field
                    fieldValue.remove(fieldValue.size() - 2, 2);
                    this->parameters.insert(fieldName, fieldValue);

                    qDebug("HttpRequest: set parameter %s=%s", fieldName.constData(), fieldValue.constData());
                }

                else if (!fileName.isEmpty() && !fieldName.isEmpty())
                {
                    // last field was a file
                    #ifdef SUPERVERBOSE
                        qDebug("HttpRequest: finishing writing to uploaded file");
                    #endif

                    uploadedFile->resize(uploadedFile->size() - 2);
                    uploadedFile->flush();
                    uploadedFile->seek(0);
                    this->parameters.insert(fieldName, fileName);
                    qDebug("HttpRequest: set parameter %s=%s", fieldName.constData(), fileName.constData());
                    this->uploadedFiles.insert(fieldName, uploadedFile);
                    qDebug("HttpRequest: uploaded file size is %lli", uploadedFile->size());
                }

                if (line.contains(this->boundary + "--"))
                {
                    finished = true;
                }

                break;
            }

            else
            {
                if (fileName.isEmpty() && !fieldName.isEmpty())
                {
                    // this is a form field.
                    this->currentSize += line.size();
                    fieldValue.append(line);
                }

                else if (!fileName.isEmpty() && !fieldName.isEmpty())
                {
                    // this is a file
                    if (!uploadedFile)
                    {
                        uploadedFile = new QTemporaryFile();
                        uploadedFile->open();
                    }

                    uploadedFile->write(line);

                    if (uploadedFile->error())
                    {
                        qCritical("HttpRequest: error writing temp file, %s", qUtf8Printable(uploadedFile->errorString()));
                    }
                }
            }
        }
    }

    if (this->tempFile->error())
    {
        qCritical("HttpRequest: cannot read temp file, %s", qUtf8Printable(this->tempFile->errorString()));
    }

    #ifdef SUPERVERBOSE
        qDebug("HttpRequest: finished parsing multipart temp file");
    #endif
}

HttpRequest::~HttpRequest()
{
    for (auto&& key : this->uploadedFiles.keys())
    {
        QTemporaryFile *file = this->uploadedFiles.value(key);

        if (file->isOpen())
        {
            file->close();
        }

        delete file;
    }

    if (this->tempFile)
    {
        if (this->tempFile->isOpen())
        {
            this->tempFile->close();
        }

        delete tempFile;
    }
}

QTemporaryFile *HttpRequest::getUploadedFile(const QByteArray &fieldName) const
{
    return this->uploadedFiles.value(fieldName);
}

QByteArray HttpRequest::getCookie(const QByteArray &name) const
{
    return this->cookies.value(name);
}

/** Get the map of cookies */
QMap<QByteArray, QByteArray> &HttpRequest::getCookieMap()
{
    return this->cookies;
}

/**
  Get the address of the connected client.
  Note that multiple clients may have the same IP address, if they
  share an internet connection (which is very common).
 */
QHostAddress HttpRequest::getPeerAddress() const
{
    return this->peerAddress;
}
