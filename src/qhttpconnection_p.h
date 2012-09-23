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

#ifndef QHTTPCONNECTION_H
#define QHTTPCONNECTION_H

#include <QtNetwork/QTcpSocket>

class QHttpRequest;
class QHttpReply;

class QHttpConnection : public QTcpSocket
{
    Q_OBJECT
public:
#if QT_VERSION < 0x050000
    explicit QHttpConnection(int socketDescriptor, QObject *parent = 0);
#else
    explicit QHttpConnection(qintptr socketDescriptor, QObject *parent = 0);
#endif
    ~QHttpConnection();

    const QHttpRequest *requestFor(QHttpReply *reply);

signals:
    void ready(QHttpRequest *request, QHttpReply *reply);

private:
    class Private;
    Private *d;
};

#endif // QHTTPCONNECTION_H
