#include "qabstractrequest.h"
#include "qhttpconnection_p.h"

#include <QtCore/QHash>
#include <QtNetwork/QHostAddress>

class QAbstractRequest::Private
{
public:
    Private(QHttpConnection *connection);

    QHttpConnection *connection;
    QUuid uuid;
    QString remoteAddress;
    QHash<QByteArray, QByteArray> rawHeaders;
    QList<QNetworkCookie> cookies;
};

QAbstractRequest::Private::Private(QHttpConnection *connection)
    : connection(connection)
    , uuid(QUuid::createUuid())
    , remoteAddress(connection->peerAddress().toString())
{

}

QAbstractRequest::QAbstractRequest(QHttpConnection *parent)
    : d(new Private(parent))
{

}

QAbstractRequest::~QAbstractRequest()
{
    delete d;
}

QHttpConnection *QAbstractRequest::connection()
{
    return d->connection;
}

const QUuid &QAbstractRequest::uuid() const
{
    return d->uuid;
}

const QString &QAbstractRequest::remoteAddress() const
{
    return d->remoteAddress;
}

bool QAbstractRequest::hasRawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.contains(headerName.toLower());
}

QByteArray QAbstractRequest::rawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.value(headerName.toLower());
}

QList<QByteArray> QAbstractRequest::rawHeaderList() const
{
    return d->rawHeaders.keys();
}

const QList<QNetworkCookie> &QAbstractRequest::cookies() const
{
    return d->cookies;
}

QHash<QByteArray, QByteArray> QAbstractRequest::rawHeaders() const
{
    return d->rawHeaders;
}

void QAbstractRequest::insertRawHeader(const QByteArray &key, const QByteArray& value)
{
    d->rawHeaders.insert(key, value);
}

void QAbstractRequest::addCookie(const QList<QNetworkCookie> &cookie)
{
    d->cookies.append(cookie);
}
