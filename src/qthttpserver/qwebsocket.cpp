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

#include "qwebsocket.h"
#include "qhttpconnection_p.h"
#include "qhttpserver_logging.h"

#include <QtCore/QtEndian>
#include <QtCore/QUrl>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkCookie>

class QWebSocket::Private : public QObject
{
    Q_OBJECT
public:
    enum ReadState {
        ReadHeaders
        , ReadDone
    };
    Private(QWebSocket *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders);
    void accept(const QByteArray &protocol);
    void close();
    void send(const QByteArray &message);

private slots:
    void readyRead();
    void disconnected();
    void readData();

private:
    QByteArray decode(const QByteArray &key) const;

private:
    QWebSocket *q;
    bool draft;
    int version;

public:
    QUrl url;
    ReadState state;
    bool connected;
    QByteArray message;
};

QWebSocket::Private::Private(QWebSocket *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders)
    : QObject(parent)
    , q(parent)
    , draft(true)
    , version(17)
    , url(url)
    , state(ReadHeaders)
    , connected(false)
{
    this->url.setScheme(QLatin1String("ws"));
    connect(q->connection(), SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(q->connection(), SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(this, SIGNAL(destroyed()), q->connection(), SLOT(deleteLater()));
}

void QWebSocket::Private::readyRead()
{
    QHttpConnection *connection = q->connection();
    switch (state) {
    case ReadHeaders:
        while (connection->canReadLine()) {
            QByteArray line = connection->readLine();
            line = line.left(line.length() - 2);
            if (line.isEmpty()) {
                state = ReadDone;
                emit q->ready();
                break;
            }

            int space = line.indexOf(' ');
            if (space > 0) {
                QByteArray name = line.left(space - 1);
                QByteArray value = line.mid(space + 1);
                if (name == "Host") {
                    int colon = value.indexOf(':');
                    if (colon > -1) {
                        url.setHost(QString::fromUtf8(value.left(colon)));
                        url.setPort(value.mid(colon + 1).toUInt());
                    } else {
                        url.setHost(QString::fromUtf8(value));
                        url.setPort(80);
                    }
                } else if (name == "Cookie") {
                    foreach (const QByteArray &c, value.split(';')) {
                        q->addCookie(QNetworkCookie::parseCookies(c));
                    }
                } else {
                    q->insertRawHeader(name.toLower(), value);
                }
            }
        }
        break;
    case ReadDone:
        if (connected)
            readData();
        break;
    default:
        break;
    }
}

void QWebSocket::Private::accept(const QByteArray &protocol)
{
    QHttpConnection *connection = q->connection();
//    connection->write("HTTP/1.1 101 Switching Protocols\r\n");
    connection->write("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    connection->write("Upgrade: WebSocket\r\n");
    connection->write("Connection: Upgrade\r\n");

    if (q->hasRawHeader("sec-websocket-key")) {
        QByteArray key = q->rawHeader("sec-websocket-key");
        key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
        key = key.toBase64();
        connection->write("Sec-WebSocket-Accept: " + key + "\r\n");
    }
    connection->write("Sec-WebSocket-Origin: " + q->rawHeader("origin") + "\r\n");
    connection->write("Sec-WebSocket-Location: " + url.toString().toUtf8() + "\r\n");
    if (!protocol.isNull()) {
        connection->write("Sec-WebSocket-Protocol: " + protocol + "\r\n");
    }
    connection->write("\r\n");
    if (q->hasRawHeader("sec-websocket-key1") && q->hasRawHeader("sec-websocket-key2")) {
        version = 0;
        QByteArray key1 = q->rawHeader("sec-websocket-key1");
        QByteArray key2 = q->rawHeader("sec-websocket-key2");
        QByteArray challenge;

        challenge.append(decode(key1));
        challenge.append(decode(key2));
        QByteArray body = connection->read(8);
        challenge.append(body);
        body = QCryptographicHash::hash(challenge, QCryptographicHash::Md5);
        connection->write(body);
//        connection->write("\r\n");
        connection->flush();
    }
    connected = true;
}

void QWebSocket::Private::close()
{
    q->connection()->disconnectFromHost();
}

QByteArray QWebSocket::Private::decode(const QByteArray &key) const
{
    QByteArray ret;
    int spaceCount = 0;
    QByteArray num;
    for (int i = 0; i < key.length(); i++) {
        switch (key.at(i)) {
        case ' ':
            spaceCount++;
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            num.append(key.at(i));
            break;
        }
    }

    if (spaceCount == 0) return ret;
    qulonglong value = num.toULongLong() / spaceCount;
    for (int i = 0; i < 4; i++) {
        char ch = value & 0xff;
        ret.prepend(ch);
        value /= 0x100;
    }
    return ret;
}

void QWebSocket::Private::readData()
{
    int pos = 0;
    QByteArray data = q->connection()->readAll();

    if (draft && version == 0 && (unsigned char)data.at(0) == 0x00 && (unsigned char)data.at(data.length() - 1) == 0xff) {
        data = data.mid(1, data.length() - 2);
    } else {
        unsigned char firstByte = data.at(pos++);
        bool fin = ((firstByte & 0x80) >> 7 == 1);
        if (!fin) {
            message.append(data);
            return;
        }

#if 0
        char opcode = (firstByte & 0x0f);
        switch (opcode) {
        case 0x0:
            qhsDebug() << "continuation";
            break;
        case 0x1:
            qhsDebug() << "text";
            break;
        case 0x2:
            qhsDebug() << "binary";
            break;
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            qhsDebug() << "reserved for further non-control frames";
            break;
        case 0x8:
            qhsDebug() << "connection close";
            break;
        case 0x9:
            qhsDebug() << "ping";
            break;
        case 0xA:
            qhsDebug() << "pong";
            break;
        default:
            qhsDebug() << "reserved for further control frames";
            break;
        }
#endif

        unsigned char secondByte = data.at(pos++);
        bool mask = ((secondByte & 0x80) >> 7 == 1);
        if (!mask) {
    //        qhsWarning() << "browse should always mask the payload data";
    //        return;
        }
        qulonglong payloadLength = (secondByte & 0x7f);
        if (payloadLength == 0x7e) {
            payloadLength = 0;
            for (int j = 0; j < 2; j++) {
                payloadLength += ((unsigned char)data.at(pos++) << ((1-j) * 8));
            }
        } else if (payloadLength == 0x7f) {
            payloadLength = 0;
            for (int j = 0; j < 8; j++) {
                payloadLength += ((unsigned char)data.at(pos++) << ((7-j) * 8));
            }
        }
        QByteArray key = data.mid(pos, 4);
        pos += 4;
        data = data.mid(pos, payloadLength);
        int len = data.size();
        if (mask) {
            for (int i = 0; i < len; i++) {
                data[i] = (unsigned char)data.at(i) ^ (unsigned char)key.at(i % key.length());
            }
        }
    }
    emit q->message(data);
}

void QWebSocket::Private::send(const QByteArray &message)
{
    QByteArray data;
    if (draft && version == 0) {
        data.append((char)0x00);
        data.append(message);
        data.append((char)0xff);
    } else {
        unsigned char byte = 0;
        unsigned char fin = (1 << 7);
        byte |= fin;
        unsigned char opcode = (1 << 0);
        byte |= opcode;
        data.append(byte);
        byte = 0;
        if (message.length() < 0x7e) {
            byte |= message.length();
            data.append(byte);
        } else if (message.length() < 0xffff) {
            byte |= 0x7e;
            data.append(byte);
            for (int j = 0; j < 2; j++) {
                byte = (message.length() >> (1 - j) * 8) & 0xff;
                data.append(byte);
            }
        } else {
            byte |= 0x7f;
            data.append(byte);
            for (int j = 0; j < 8; j++) {
                if (j < 4)
                    byte = 0;
                else
                    byte = (message.length() >> (7 - j) * 8) & 0xff;
                data.append(byte);
            }
        }
        data.append(message);
    }
    q->connection()->write(data);
    q->connection()->flush();
}


void QWebSocket::Private::disconnected()
{
    q->deleteLater();
}

QWebSocket::QWebSocket(QHttpConnection *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders)
    : QObject(parent)
    , QAbstractRequest(parent)
    , d(new Private(this, url, rawHeaders))
{
}

const QUrl &QWebSocket::url() const
{
    return d->url;
}

void QWebSocket::accept(const QByteArray &protocol)
{
    d->accept(protocol);
}

void QWebSocket::close()
{
    d->close();
}

void QWebSocket::send(const QByteArray &message)
{
    return d->send(message);
}

void QWebSocket::setUrl(const QUrl &url)
{
    if (d->url == url) return;
    d->url = url;
    emit urlChanged(url);
}

QDebug operator<<(QDebug dbg, const QWebSocket *socket)
{
    if (!socket) {
        return dbg << "QWebSocket {}";
    }
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    dbg.nospace();
    dbg << "QWebSocket ";
    dbg << "{ uuid: " << socket->uuid().toString();
    dbg << "; url: " << socket->url().toString();
    dbg << "; ip: " << socket->remoteAddress();
    if (socket->hasRawHeader("User-Agent")) {
        dbg << "; user_agent: " << socket->rawHeader("User-Agent");
    }
    if (socket->hasRawHeader("Referer")) {
        dbg << "; referer: " << socket->rawHeader("Referer");
    }
    dbg << " }";
    return dbg;
}

#include "qwebsocket.moc"
