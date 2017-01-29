#include "HttpCookie.hpp"

QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN

HttpCookie::HttpCookie()
{
    version=1;
    maxAge=0;
    secure=false;
}

HttpCookie::HttpCookie(const QByteArray &name,
                       const QByteArray &value,
                       const int &maxAge,
                       const QByteArray &path,
                       const QByteArray &comment,
                       const QByteArray &domain,
                       const bool &secure,
                       const bool &httpOnly)
{
    this->name = name;
    this->value = value;
    this->maxAge = maxAge;
    this->path = path;
    this->comment = comment;
    this->domain = domain;
    this->secure = secure;
    this->httpOnly = httpOnly;
    this->version = 1;
}

HttpCookie::HttpCookie(const QByteArray &source)
{
    this->version = 1;
    this->maxAge = 0;
    this->secure = false;
    QList<QByteArray> list = this->splitCSV(source);
    for (auto&& part : list)
    {

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

        // Set fields
        if (name == "Comment")
        {
            this->comment = value;
        }

        else if (name == "Domain")
        {
            this->domain = value;
        }

        else if (name == "Max-Age")
        {
            this->maxAge = value.toInt();
        }

        else if (name == "Path")
        {
            this->path = value;
        }

        else if (name == "Secure")
        {
            this->secure = true;
        }

        else if (name == "HttpOnly")
        {
            this->httpOnly = true;
        }

        else if (name == "Version")
        {
            this->version = value.toInt();
        }

        else
        {
            if (this->name.isEmpty())
            {
                this->name = name;
                this->value = value;
            }

            else
            {
                qWarning("HttpCookie: Ignoring unknown %s=%s", name.constData(), value.constData());
            }
        }
    }
}

QByteArray HttpCookie::toByteArray() const
{
    QByteArray buffer(this->name);
    buffer.append('=');
    buffer.append(this->value);

    if (!this->comment.isEmpty())
    {
        buffer.append("; Comment=");
        buffer.append(this->comment);
    }

    if (!this->domain.isEmpty())
    {
        buffer.append("; Domain=");
        buffer.append(this->domain);
    }

    if (this->maxAge != 0)
    {
        buffer.append("; Max-Age=");
        buffer.append(QByteArray::number(this->maxAge));
    }

    if (!this->path.isEmpty())
    {
        buffer.append("; Path=");
        buffer.append(this->path);
    }

    if (this->secure)
    {
        buffer.append("; Secure");
    }

    if (this->httpOnly)
    {
        buffer.append("; HttpOnly");
    }

    buffer.append("; Version=");
    buffer.append(QByteArray::number(this->version));

    return buffer;
}

void HttpCookie::setName(const QByteArray &name)
{
    this->name = name;
}

void HttpCookie::setValue(const QByteArray &value)
{
    this->value = value;
}

void HttpCookie::setComment(const QByteArray &comment)
{
    this->comment = comment;
}

void HttpCookie::setDomain(const QByteArray &domain)
{
    this->domain = domain;
}

void HttpCookie::setMaxAge(const int &maxAge)
{
    this->maxAge = maxAge;
}

void HttpCookie::setPath(const QByteArray &path)
{
    this->path = path;
}

void HttpCookie::setSecure(const bool &secure)
{
    this->secure = secure;
}

void HttpCookie::setHttpOnly(const bool &httpOnly)
{
    this->httpOnly = httpOnly;
}

QByteArray HttpCookie::getName() const
{
    return this->name;
}

QByteArray HttpCookie::getValue() const
{
    return this->value;
}

QByteArray HttpCookie::getComment() const
{
    return this->comment;
}

QByteArray HttpCookie::getDomain() const
{
    return this->domain;
}

int HttpCookie::getMaxAge() const
{
    return this->maxAge;
}

QByteArray HttpCookie::getPath() const
{
    return this->path;
}

bool HttpCookie::getSecure() const
{
    return this->secure;
}

bool HttpCookie::getHttpOnly() const
{
    return this->httpOnly;
}

int HttpCookie::getVersion() const
{
    return this->version;
}

QList<QByteArray> HttpCookie::splitCSV(const QByteArray &source)
{
    bool inString = false;
    QList<QByteArray> list;
    QByteArray buffer;

    for (int i = 0; i < source.size(); ++i)
    {
        char c = source.at(i);

        if (!inString)
        {
            if (c == '\"')
            {
                inString = true;
            }

            else if (c == ';')
            {
                QByteArray trimmed = buffer.trimmed();
                if (!trimmed.isEmpty())
                {
                    list.append(trimmed);
                }
                buffer.clear();
            }

            else
            {
                buffer.append(c);
            }
        }

        else
        {
            if (c == '\"')
            {
                inString = false;
            }

            else
            {
                buffer.append(c);
            }
        }
    }

    QByteArray trimmed = buffer.trimmed();
    if (!trimmed.isEmpty())
    {
        list.append(trimmed);
    }

    return list;
}

QTWEBAPP_HTTPSERVER_NAMESPACE_END
