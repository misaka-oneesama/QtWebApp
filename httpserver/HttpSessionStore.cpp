#include "HttpSessionStore.hpp"

#include <QDateTime>
#include <QUuid>

using namespace stefanfrings;

HttpSessionStore::HttpSessionStore(HttpSessionStoreSettings *settings, QObject *parent)
    : QObject(parent)
{
    this->settings = settings;

    QObject::connect(&this->cleanupTimer, &QTimer::timeout, this, &HttpSessionStore::sessionTimerEvent);
    this->cleanupTimer.start(60000);

    this->cookieName = this->settings->cookieName();
    this->expirationTime = this->settings->expirationTime();

    qDebug("HttpSessionStore: Sessions expire after %llu milliseconds", this->expirationTime);
}

HttpSessionStore::~HttpSessionStore()
{
    this->cleanupTimer.stop();
}

QByteArray HttpSessionStore::getSessionId(HttpRequest &request, HttpResponse &response)
{
    // The session ID in the response has priority because this one will be used in the next request.
    this->mutex.lock();

    // Get the session ID from the response cookie
    QByteArray sessionId = response.getCookies().value(this->cookieName).getValue();

    if (sessionId.isEmpty())
    {
        // Get the session ID from the request cookie
        sessionId = request.getCookie(this->cookieName);
    }

    // Clear the session ID if there is no such session in the storage.
    if (!sessionId.isEmpty())
    {
        if (!this->sessions.contains(sessionId))
        {
            qDebug("HttpSessionStore: received invalid session cookie with ID %s", sessionId.constData());
            sessionId.clear();
        }
    }

    this->mutex.unlock();
    return sessionId;
}

HttpSession HttpSessionStore::getSession(HttpRequest &request, HttpResponse &response, bool allowCreate)
{
    QByteArray sessionId = getSessionId(request, response);
    this->mutex.lock();

    if (!sessionId.isEmpty())
    {
        HttpSession session = this->sessions.value(sessionId);

        if (!session.isNull())
        {
            this->mutex.unlock();

            // Refresh the session cookie
            response.setCookie(HttpCookie(this->settings->cookieName(),
                                          session.getId(),
                                          this->expirationTime / 1000,
                                          this->settings->cookiePath(),
                                          this->settings->cookieComment(),
                                          this->settings->cookieDomain()));
            session.setLastAccess();
            return session;
        }
    }

    // Need to create a new session
    if (allowCreate)
    {
        HttpSession session(true);
        qDebug("HttpSessionStore: create new session with ID %s", session.getId().constData());
        this->sessions.insert(session.getId(), session);
        response.setCookie(HttpCookie(this->settings->cookieName(),
                                      session.getId(),
                                      this->expirationTime / 1000,
                                      this->settings->cookiePath(),
                                      this->settings->cookieComment(),
                                      this->settings->cookieDomain()));
        this->mutex.unlock();
        return session;
    }

    // Return a null session
    this->mutex.unlock();
    return HttpSession();
}

HttpSession HttpSessionStore::getSession(const QByteArray &id)
{
    this->mutex.lock();
    HttpSession session = this->sessions.value(id);
    this->mutex.unlock();
    session.setLastAccess();
    return session;
}

void HttpSessionStore::sessionTimerEvent()
{
    this->mutex.lock();
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    QMap<QByteArray, HttpSession>::iterator i = this->sessions.begin();
    while (i != this->sessions.end())
    {
        QMap<QByteArray, HttpSession>::iterator prev = i;
        ++i;
        HttpSession session = prev.value();
        qint64 lastAccess = session.getLastAccess();
        if (now - lastAccess > this->expirationTime)
        {
            qDebug("HttpSessionStore: session %s expired", session.getId().constData());
            this->sessions.erase(prev);
        }
    }

    this->mutex.unlock();
}

/** Delete a session */
void HttpSessionStore::removeSession(const HttpSession &session)
{
    this->mutex.lock();
    this->sessions.remove(session.getId());
    this->mutex.unlock();
}
