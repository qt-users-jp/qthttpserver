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

#include <QtCore/QCoreApplication>

#include <QtHttpServer/QHttpServer>
#include <QtHttpServer/QHttpRequest>
#include <QtHttpServer/QHttpReply>

class HttpServer : public QHttpServer
{
    Q_OBJECT
public:
    explicit HttpServer(QObject *parent = 0);

private slots:
    void hello(QHttpRequest *request, QHttpReply *reply);
};

HttpServer::HttpServer(QObject *parent)
    : QHttpServer(parent)
{
    connect(this, SIGNAL(incomingConnection(QHttpRequest *, QHttpReply *)), this, SLOT(hello(QHttpRequest *, QHttpReply *)));

}

void HttpServer::hello(QHttpRequest *request, QHttpReply *reply)
{
    Q_UNUSED(request)
    reply->setStatus(200);
    reply->setRawHeader("Content-Type", "text/html; charset=utf-8;");
    reply->write("<html>\r\n"
                 "    <head>\r\n"
                 "        <title>Hello QtHttpServer</title>\r\n"
                 "    </head>\r\n"
                 "    <body>\r\n"
                 "        <h1>Hello QtHttpServer</h1>\r\n"
                 "    </body>\r\n"
                 "</html>\r\n");
    reply->close();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    HttpServer server;
    if (!server.listen(QHostAddress::Any, 8080)) {
        qWarning() << "failed to listen.";
        return -1;
    }

    return app.exec();
}

#include "main.moc"
