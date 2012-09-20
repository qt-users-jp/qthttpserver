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

#include "qhttpserver.h"

#include <QtNetwork/QTcpServer>

#include "qhttpconnection_p.h"

class QHttpServer::Private : public QTcpServer
{
    Q_OBJECT
public:
    Private(QHttpServer *parent);

protected:
#if QT_VERSION < 0x050000
    void incomingConnection(int socketDescriptor);
#else
    void incomingConnection(qintptr socketDescriptor);
#endif

private:
    QHttpServer *q;
};

QHttpServer::Private::Private(QHttpServer *parent)
    : QTcpServer(parent)
    , q(parent)
{
    setMaxPendingConnections(1000);
}

#if QT_VERSION < 0x050000
void QHttpServer::Private::incomingConnection(int socketDescriptor)
#else
void QHttpServer::Private::incomingConnection(qintptr socketDescriptor)
#endif
{
    QHttpConnection *connection = new QHttpConnection(socketDescriptor, this);
    connect(connection, SIGNAL(ready(QHttpRequest *, QHttpReply *)), q, SIGNAL(incomingConnection(QHttpRequest *, QHttpReply *)));
}

QHttpServer::QHttpServer(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

bool QHttpServer::listen(const QHostAddress &address, quint16 port)
{
    return d->listen(address, port);
}

void QHttpServer::close()
{
    d->close();
}

bool QHttpServer::isListening() const
{
    return d->isListening();
}

void QHttpServer::setMaxPendingConnections(int numConnections)
{
    d->setMaxPendingConnections(numConnections);
}

int QHttpServer::maxPendingConnections() const
{
    return d->maxPendingConnections();
}

quint16 QHttpServer::serverPort() const
{
    return d->serverPort();
}

QHostAddress QHttpServer::serverAddress() const
{
    return d->serverAddress();
}

QAbstractSocket::SocketError QHttpServer::serverError() const
{
    return d->serverError();
}

QString QHttpServer::errorString() const
{
    return d->errorString();
}

#include "qhttpserver.moc"
