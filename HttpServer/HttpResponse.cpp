#include "HttpResponse.hpp"

QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN

HttpResponse::HttpResponse(QTcpSocket *socket)
{
    this->socket = socket;
    this->statusCode = 200;
    this->statusText = "OK";
    this->sentHeaders = false;
    this->sentLastPart = false;
    this->chunkedMode = false;
}

void HttpResponse::setHeader(const QByteArray &name, const QByteArray &value)
{
    Q_ASSERT(!this->sentHeaders);
    this->headers.insert(name, value);
}

void HttpResponse::setHeader(const QByteArray &name, int value)
{
    Q_ASSERT(!this->sentHeaders);
    this->headers.insert(name, QByteArray::number(value));
}

QMap<QByteArray, QByteArray> &HttpResponse::getHeaders()
{
    return this->headers;
}

void HttpResponse::setStatus(int statusCode, const QByteArray &description)
{
    this->statusCode = statusCode;
    this->statusText = description;
}

int HttpResponse::getStatusCode() const
{
   return this->statusCode;
}

void HttpResponse::writeHeaders()
{
    Q_ASSERT(!this->sentHeaders);
    QByteArray buffer;
    buffer.append("HTTP/1.1 ");
    buffer.append(QByteArray::number(this->statusCode));
    buffer.append(' ');
    buffer.append(this->statusText);
    buffer.append("\n");

    for (auto&& name : headers.keys())
    {
        buffer.append(name);
        buffer.append(": ");
        buffer.append(this->headers.value(name));
        buffer.append("\n");
    }

    for (auto&& cookie : this->cookies.values())
    {
        buffer.append("Set-Cookie: ");
        buffer.append(cookie.toByteArray());
        buffer.append("\n");
    }

    buffer.append("\n");
    this->writeToSocket(buffer);
    this->sentHeaders = true;
}

bool HttpResponse::writeToSocket(QByteArray data)
{
    int remaining = data.size();
    char* ptr = data.data();

    while (this->socket->isOpen() && remaining > 0)
    {
        // If the output buffer has become large, then wait until it has been sent.
        if (this->socket->bytesToWrite() > 16384)
        {
            this->socket->waitForBytesWritten(-1);
        }

        int written = this->socket->write(ptr, remaining);
        if (written == -1)
        {
          return false;
        }

        ptr += written;
        remaining -= written;
    }

    return true;
}

void HttpResponse::write(const QByteArray &data, bool lastPart)
{
    Q_ASSERT(!this->sentLastPart);

    // Send HTTP headers, if not already done (that happens only on the first call to write())
    if (!this->sentHeaders)
    {
        // If the whole response is generated with a single call to write(), then we know the total
        // size of the response and therefore can set the Content-Length header automatically.
        if (lastPart)
        {
           // Automatically set the Content-Length header
           this->headers.insert("Content-Length", QByteArray::number(data.size()));
        }

        // else if we will not close the connection at the end, them we must use the chunked mode.
        else
        {
            QByteArray connectionValue = this->headers.value("Connection", this->headers.value("connection"));

            if (QString::compare(connectionValue, "close", Qt::CaseInsensitive) != 0)
            {
                this->headers.insert("Transfer-Encoding", "chunked");
                this->chunkedMode = true;
            }
        }

        this->writeHeaders();
    }

    // Send data
    if (data.size() > 0)
    {
        if (this->chunkedMode)
        {
            if (data.size() > 0)
            {
                QByteArray size = QByteArray::number(data.size(), 16);
                this->writeToSocket(size);
                this->writeToSocket("\n");
                this->writeToSocket(data);
                this->writeToSocket("\n");
            }
        }

        else
        {
            this->writeToSocket(data);
        }
    }

    // Only for the last chunk, send the terminating marker and flush the buffer.
    if (lastPart)
    {
        if (this->chunkedMode)
        {
            this->writeToSocket("0\n\n");
        }

        this->socket->flush();
        this->sentLastPart = true;
    }
}

bool HttpResponse::hasSentLastPart() const
{
    return this->sentLastPart;
}

void HttpResponse::setCookie(const HttpCookie &cookie)
{
    Q_ASSERT(!this->sentHeaders);

    if (!cookie.getName().isEmpty())
    {
        this->cookies.insert(cookie.getName(), cookie);
    }
}

QMap<QByteArray, HttpCookie> &HttpResponse::getCookies()
{
    return this->cookies;
}

void HttpResponse::redirect(const QByteArray &url)
{
    this->setStatus(303, "See Other");
    this->setHeader("Location", url);
    this->write("Redirect", true);
}

void HttpResponse::flush()
{
    this->socket->flush();
}

bool HttpResponse::isConnected() const
{
    return this->socket->isOpen();
}

QTWEBAPP_HTTPSERVER_NAMESPACE_END
