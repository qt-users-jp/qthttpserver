#ifndef QABSTRACTREQUEST_H
#define QABSTRACTREQUEST_H

#include "qthttpserverglobal.h"
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkCookie>

QT_BEGIN_NAMESPACE

class QHttpConnection;
class QNetworkCookie;

class Q_HTTPSERVER_EXPORT QAbstractRequest
{
public:
    explicit QAbstractRequest(QHttpConnection *parent);
    ~QAbstractRequest();

    QHttpConnection *connection();
    const QUuid &uuid() const;
    const QString &remoteAddress() const;
    bool hasRawHeader(const QByteArray &headerName) const;
    QByteArray rawHeader(const QByteArray &headerName) const;
    QList<QByteArray> rawHeaderList() const;
    const QList<QNetworkCookie> &cookies() const;

protected:
    QHash<QByteArray, QByteArray> rawHeaders() const;
    void insertRawHeader(const QByteArray &key, const QByteArray& value);
    void addCookie(const QList<QNetworkCookie> &cookie);

private:
    class Private;
    Private *d;
    Q_DISABLE_COPY(QAbstractRequest)
};

QT_END_NAMESPACE

#endif // QABSTRACTREQUEST_H
