#include "HttpSession.hpp"

#include <QDateTime>
#include <QUuid>

QTWEBAPP_HTTPSERVER_NAMESPACE_BEGIN

HttpSession::HttpSession(bool canStore)
{
    if (canStore)
    {
        this->dataPtr = new HttpSessionData();
        this->dataPtr->refCount = 1;
        this->dataPtr->lastAccess = QDateTime::currentMSecsSinceEpoch();
        this->dataPtr->id = QUuid::createUuid().toString().toLocal8Bit();

#ifdef QTWEBAPP_SUPERVERBOSE
        qDebug("HttpSession: created new session data with id %s", dataPtr->id.constData());
#endif
    }
}

HttpSession::HttpSession(const HttpSession &other)
{
    this->dataPtr = other.dataPtr;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForWrite();
        this->dataPtr->refCount++;

#ifdef QTWEBAPP_SUPERVERBOSE
        qDebug("HttpSession: refCount of %s is %i", this->dataPtr->id.constData(), this->dataPtr->refCount);
#endif

        this->dataPtr->lock.unlock();
    }
}

HttpSession &HttpSession::operator= (const HttpSession &other)
{
    HttpSessionData *oldPtr = this->dataPtr;
    this->dataPtr = other.dataPtr;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForWrite();
        this->dataPtr->refCount++;

#ifdef QTWEBAPP_SUPERVERBOSE
        qDebug("HttpSession: refCount of %s is %i", this->dataPtr->id.constData(), this->dataPtr->refCount);
#endif

        this->dataPtr->lastAccess = QDateTime::currentMSecsSinceEpoch();
        this->dataPtr->lock.unlock();
    }

    if (oldPtr)
    {
        int refCount;
        oldPtr->lock.lockForRead();
        refCount = oldPtr->refCount--;

#ifdef QTWEBAPP_SUPERVERBOSE
        qDebug("HttpSession: refCount of %s is %i", oldPtr->id.constData(), oldPtr->refCount);
#endif

        oldPtr->lock.unlock();

        if (refCount == 0)
        {
            delete oldPtr;
            oldPtr = nullptr;
        }
    }

    return *this;
}

HttpSession::~HttpSession()
{
    if (this->dataPtr)
    {
        int refCount;
        this->dataPtr->lock.lockForRead();
        refCount = --this->dataPtr->refCount;

#ifdef QTWEBAPP_SUPERVERBOSE
        qDebug("HttpSession: refCount of %s is %i", dataPtr->id.constData(), dataPtr->refCount);
#endif

        this->dataPtr->lock.unlock();

        if (refCount == 0)
        {
            qDebug("HttpSession: deleting data");
            delete this->dataPtr;
            this->dataPtr = nullptr;
        }
    }
}

QByteArray HttpSession::getId() const
{
    return this->dataPtr ? this->dataPtr->id : QByteArray();
}

bool HttpSession::isNull() const
{
    return this->dataPtr == nullptr;
}

void HttpSession::set(const QByteArray &key, const QVariant &value)
{
    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForWrite();
        this->dataPtr->values.insert(key, value);
        this->dataPtr->lock.unlock();
    }
}

void HttpSession::remove(const QByteArray &key)
{
    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForWrite();
        this->dataPtr->values.remove(key);
        this->dataPtr->lock.unlock();
    }
}

QVariant HttpSession::get(const QByteArray &key) const
{
    QVariant value;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForRead();
        value = this->dataPtr->values.value(key);
        this->dataPtr->lock.unlock();
    }

    return value;
}

bool HttpSession::contains(const QByteArray &key) const
{
    bool found = false;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForRead();
        found = this->dataPtr->values.contains(key);
        this->dataPtr->lock.unlock();
    }

    return found;
}

QMap<QByteArray, QVariant> HttpSession::getAll() const
{
    QMap<QByteArray, QVariant> values;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForRead();
        values = this->dataPtr->values;
        this->dataPtr->lock.unlock();
    }

    return values;
}

qint64 HttpSession::getLastAccess() const
{
    qint64 value = 0LL;

    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForRead();
        value = this->dataPtr->lastAccess;
        this->dataPtr->lock.unlock();
    }

    return value;
}


void HttpSession::setLastAccess()
{
    if (this->dataPtr)
    {
        this->dataPtr->lock.lockForRead();
        this->dataPtr->lastAccess = QDateTime::currentMSecsSinceEpoch();
        this->dataPtr->lock.unlock();
    }
}

QTWEBAPP_HTTPSERVER_NAMESPACE_END
