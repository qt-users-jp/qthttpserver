/* Copyright (c) 2012 QtHttpServer Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the QtHttpServer nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL QTHTTPSERVER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "qhttprequest.h"

#include <QtCore/QUrl>
#include <QtCore/QHash>

#include "qhttpconnection_p.h"

class QHttpRequest::Private : public QObject
{
    Q_OBJECT
public:
    enum ReadState {
        ReadUrl
        , ReadHeaders
        , ReadBody
        , ReadDone
    };

    Private(QHttpConnection *c, QHttpRequest *parent);

private slots:
    void readyRead();
    void disconnected();

private:
    QHttpRequest *q;

public:
    QHttpConnection *connection;
    ReadState state;
    Method method;
    QUrl url;
    QHash<QByteArray, QByteArray> rawHeaders;
};

QHttpRequest::Private::Private(QHttpConnection *c, QHttpRequest *parent)
    : QObject(parent)
    , q(parent)
    , connection(c)
    , state(ReadUrl)
    , method(Unknown)
{
    connect(connection, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(disconnected()));
    q->open(QIODevice::ReadOnly);
}

void QHttpRequest::Private::readyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    switch (state) {
    case ReadUrl:
        if (socket->canReadLine()) {
            QByteArray line = socket->readLine();
            line = line.left(line.length() - 2);
            QList<QByteArray> array = line.split(' ');

            QByteArray method = array.takeFirst();
            if (method == "GET") {
                this->method = Get;
            } else if (method == "POST") {
                this->method = Post;
            } else {
                qWarning() << method << "is not supported.";
                socket->disconnectFromHost();
                return;
            }

            QByteArray path = array.takeFirst();
            url = QUrl(QString::fromUtf8(path));
            url.setScheme(QLatin1String("http"));

            QByteArray http = array.takeFirst();
            if (http != "HTTP/1.1" && http != "HTTP/1.0") {
                qWarning() << http << "is not supported.";
                socket->disconnectFromHost();
                return;
            }
            state = ReadHeaders;
        }
//        break;
    case ReadHeaders:
        while (socket->canReadLine()) {
            QByteArray line = socket->readLine();
            line = line.left(line.length() - 2);

            if (line.isEmpty()) {
                if (!q->hasRawHeader("Content-Length")) {
                    state = ReadDone;
                    disconnect(connection, SIGNAL(readyRead()), this, SLOT(readyRead()));
                    emit q->ready();
                } else {
                    state = ReadBody;
                    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
                }
                break;
            }

            int space = line.indexOf(' ');
            if (space > 0) {
                QByteArray name = line.left(space - 1);
                QByteArray value = line.mid(space + 1);
                rawHeaders.insert(name.toLower(), value.toLower());
                if (name == "Host") {
                    int colon = value.indexOf(':');
                    if (colon > -1) {
                        url.setHost(QString::fromUtf8(value.left(colon)));
                        url.setPort(value.mid(colon + 1).toUInt());
                    } else {
                        url.setHost(QString::fromUtf8(value));
                        url.setPort(80);
                    }
                }
            }
        }
        break;
    case ReadBody:
        if (q->hasRawHeader("Content-Length")) {
            if (connection->bytesAvailable() >= q->rawHeader("Content-Length").toLongLong()) {
                state = ReadDone;
                emit q->ready();
            }
        }
        break;
    default:
        break;
    }
}

void QHttpRequest::Private::disconnected()
{
    q->close();
    q->deleteLater();
}

QHttpRequest::QHttpRequest(QHttpConnection *parent)
    : QIODevice(parent)
    , d(new Private(parent, this))
{
}

QHttpRequest::~QHttpRequest()
{
    delete d;
}

QHttpRequest::Method QHttpRequest::method() const
{
    return d->method;
}

bool QHttpRequest::hasRawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.contains(headerName.toLower());
}

QByteArray QHttpRequest::rawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.value(headerName.toLower());
}

QList<QByteArray> QHttpRequest::rawHeaderList() const
{
    return d->rawHeaders.keys();
}

QUrl QHttpRequest::url() const
{
    return d->url;
}

bool QHttpRequest::atEnd() const
{
    if (d->state != Private::ReadDone) return true;
    return d->connection->atEnd();
}

qint64 QHttpRequest::bytesAvailable() const
{
    if (d->state != Private::ReadDone) return 0;
    return d->connection->bytesAvailable();
}

qint64 QHttpRequest::bytesToWrite() const
{
    return 0;
}

bool QHttpRequest::canReadLine() const
{
    if (d->state != Private::ReadDone) return false;
    return d->connection->canReadLine();
}

void QHttpRequest::close()
{
}

bool QHttpRequest::isSequential() const
{
    return true;
}

qint64 QHttpRequest::pos() const
{
    return 0;
}

bool QHttpRequest::reset()
{
    return true;
}

bool QHttpRequest::seek(qint64 pos)
{
    Q_UNUSED(pos)
    return false;
}

qint64 QHttpRequest::size() const
{
    if (d->state != Private::ReadDone) return 0;
    return bytesAvailable();
}

bool QHttpRequest::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs)
    return false;
}

bool QHttpRequest::waitForReadyRead(int msecs)
{
    if (d->state != Private::ReadDone) return false;
    return d->connection->waitForReadyRead(msecs);
}

qint64 QHttpRequest::readData(char *data, qint64 maxlen)
{
    if (d->state != Private::ReadDone) return -1;
    return d->connection->read(data, maxlen);
}

qint64 QHttpRequest::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)
    return -1;
}

#include "qhttprequest.moc"
