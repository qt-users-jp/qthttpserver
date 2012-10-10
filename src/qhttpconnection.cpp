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

#include "qhttpconnection_p.h"

#include <QtCore/QTime>
#include <QtCore/QUrl>

#include "qhttprequest.h"
#include "qhttpreply.h"
#include "qwebsocket.h"

class QHttpConnection::Private : public QObject
{
    Q_OBJECT
public:
    Private(QHttpConnection *parent);

private slots:
    void upgrade(const QByteArray &to, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders);
    void requestReady();
    void replyDone(QObject *);
    void websocketReady();

private:
    QHttpConnection *q;
    int keepAlive;

public:
    QMap<QObject*, QHttpRequest*> requestMap;
    QTime timer;
};

QHttpConnection::Private::Private(QHttpConnection *parent)
    : QObject(parent)
    , q(parent)
    , keepAlive(100)
{
    QHttpRequest *request = new QHttpRequest(q);
    connect(request, SIGNAL(ready()), this, SLOT(requestReady()));
    connect(request, SIGNAL(upgrade(QByteArray, QUrl, QHash<QByteArray, QByteArray>)), this, SLOT(upgrade(QByteArray, QUrl, QHash<QByteArray, QByteArray>)));

    timer.start();
    connect(q, SIGNAL(disconnected()), q, SLOT(deleteLater()));
}

void QHttpConnection::Private::upgrade(const QByteArray &to, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders)
{
    QHttpRequest *request = qobject_cast<QHttpRequest *>(sender());
    disconnect(request, 0, this, 0);
    request->deleteLater();
    if (to == "websocket") {
        QWebSocket *socket = new QWebSocket(q, url, rawHeaders);
        connect(socket, SIGNAL(ready()), this, SLOT(websocketReady()));
    }
}

void QHttpConnection::Private::requestReady()
{
    QHttpRequest *request = qobject_cast<QHttpRequest *>(sender());
    disconnect(request, SIGNAL(ready()), this, SLOT(requestReady()));
    QHttpReply *reply = new QHttpReply(q);
    connect(reply, SIGNAL(destroyed(QObject *)), this, SLOT(replyDone(QObject*)));
    requestMap.insert(reply, request);
    emit q->ready(request, reply);

    if (request->hasRawHeader("Connection")) {
//        qDebug() << request->rawHeader("Connection") << keepAlive;
        if (request->rawHeader("Connection") == QByteArray("Keep-Alive").toLower()) {
            if (keepAlive > 0) {
                reply->setRawHeader("Keep-Alive", QString::fromUtf8("timeout=1, max=%1").arg(keepAlive--).toUtf8());
                reply->setRawHeader("Connection", "Keep-Alive");
                request = new QHttpRequest(q);
                connect(request, SIGNAL(ready()), this, SLOT(requestReady()));
            } else {
                reply->setRawHeader("Connection", "Close");
                keepAlive = 0;
            }
        } else {
            reply->setRawHeader("Connection", "Close");
            keepAlive = 0;
        }
    } else {
        reply->setRawHeader("Connection", "Close");
        keepAlive = 0;
    }
}

void QHttpConnection::Private::replyDone(QObject *reply)
{
    if (requestMap.contains(reply)) {
        QHttpRequest *request = requestMap.take(reply);
        request->deleteLater();
    }
    if (keepAlive == 0 && requestMap.isEmpty()) {
        q->disconnectFromHost();
    }
}

void QHttpConnection::Private::websocketReady()
{
    QWebSocket *socket = qobject_cast<QWebSocket *>(sender());
    disconnect(socket, SIGNAL(ready()), this, SLOT(websocketReady()));
    emit q->ready(socket);
}

#if QT_VERSION < 0x050000
QHttpConnection::QHttpConnection(int socketDescriptor, QObject *parent)
#else
QHttpConnection::QHttpConnection(qintptr socketDescriptor, QObject *parent)
#endif
    : QTcpSocket(parent)
    , d(new Private(this))
{
    setSocketOption(KeepAliveOption, 1);
    setSocketDescriptor(socketDescriptor);
}

QHttpConnection::~QHttpConnection()
{
//    qDebug() << d << socketDescriptor() << d->timer.elapsed();
//    qDebug() << d->timer.elapsed();
}

const QHttpRequest *QHttpConnection::requestFor(QHttpReply *reply)
{
    return d->requestMap.value(reply);
}

#include "qhttpconnection.moc"
