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

#include <QtCore/QtEndian>
#include <QtCore/QUrl>
#include <QtCore/QCryptographicHash>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkCookie>

#include "qhttpconnection_p.h"

class QWebSocket::Private : public QObject
{
    Q_OBJECT
public:
    enum ReadState {
        ReadHeaders
        , ReadDone
    };
    Private(QHttpConnection *c, QWebSocket *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders);
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
    QHttpConnection *connection;
    QString remoteAddress;
    ReadState state;
    QUrl url;
    QHash<QByteArray, QByteArray> rawHeaders;
    QList<QNetworkCookie> cookies;
    bool connected;
    QByteArray message;
};

QWebSocket::Private::Private(QHttpConnection *c, QWebSocket *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders)
    : QObject(parent)
    , q(parent)
    , draft(true)
    , version(17)
    , connection(c)
    , state(ReadHeaders)
    , url(url)
    , rawHeaders(rawHeaders)
    , connected(false)
{
//    qDebug() << Q_FUNC_INFO << __LINE__ << url << rawHeaders;
    this->url.setScheme("ws");
    remoteAddress = connection->peerAddress().toString();
    connect(connection, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(disconnected()));
    QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    connect(this, SIGNAL(destroyed()), connection, SLOT(deleteLater()));
}

void QWebSocket::Private::readyRead()
{
    switch (state) {
    case ReadHeaders:
        while (connection->canReadLine()) {
            QByteArray line = connection->readLine();
            line = line.left(line.length() - 2);
//            qDebug() << Q_FUNC_INFO << __LINE__ << line;
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
                        cookies.append(QNetworkCookie::parseCookies(c));
                    }
                } else {
                    rawHeaders.insert(name.toLower(), value);
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
//    connection->write("HTTP/1.1 101 Switching Protocols\r\n");
    connection->write("HTTP/1.1 101 Web Socket Protocol Handshake\r\n");
    connection->write("Upgrade: WebSocket\r\n");
    connection->write("Connection: Upgrade\r\n");

    if (rawHeaders.contains("sec-websocket-key")) {
        QByteArray key = rawHeaders.value("sec-websocket-key");
        key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
        key = key.toBase64();
        connection->write("Sec-WebSocket-Accept: " + key + "\r\n");
    }
    connection->write("Sec-WebSocket-Origin: " + rawHeaders.value("origin") + "\r\n");
    connection->write("Sec-WebSocket-Location: " + url.toString().toUtf8() + "\r\n");
    if (!protocol.isNull()) {
        connection->write("Sec-WebSocket-Protocol: " + protocol + "\r\n");
    }
    connection->write("\r\n");
    if (rawHeaders.contains("sec-websocket-key1") && rawHeaders.contains("sec-websocket-key2")) {
        version = 0;
        QByteArray key1 = rawHeaders.value("sec-websocket-key1");
        QByteArray key2 = rawHeaders.value("sec-websocket-key2");
        QByteArray challenge;

//        qDebug() << decode(QByteArray("3 7 78B 67  5      W%89")).toHex();
        challenge.append(decode(key1));
        challenge.append(decode(key2));
        QByteArray body = connection->read(8);
//        qDebug() << challenge.toHex() << body.toHex() << body.length();
        challenge.append(body);
//        qDebug() << challenge.toHex();
        body = QCryptographicHash::hash(challenge, QCryptographicHash::Md5);
//        qDebug() << body.toHex() << body.length();
//        qDebug() << body;
        connection->write(body);
//        connection->write("\r\n");
        connection->flush();
    }
    connected = true;
}

void QWebSocket::Private::close()
{
    connection->disconnectFromHost();
}

QByteArray QWebSocket::Private::decode(const QByteArray &key) const
{
//    qDebug() << Q_FUNC_INFO << __LINE__ << key;
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
//    qDebug() << Q_FUNC_INFO << __LINE__ << num << spaceCount << value;
    for (int i = 0; i < 4; i++) {
//        qDebug() << Q_FUNC_INFO << __LINE__ << value;
        char ch = value & 0xff;
//        qDebug() << Q_FUNC_INFO << __LINE__ << QString("0x%1").arg(value & 0xff, 0, 16);
        ret.prepend(ch);
        value /= 0x100;
    }
//    qDebug() << Q_FUNC_INFO << __LINE__ << ret;
    return ret;
}

void QWebSocket::Private::readData()
{
//    qDebug() << Q_FUNC_INFO << __LINE__;
    int pos = 0;
    QByteArray data = connection->readAll();
//    qDebug() << Q_FUNC_INFO << __LINE__ << data.toHex() << data.length();

    if (draft && version == 0 && (unsigned char)data.at(0) == 0x00 && (unsigned char)data.at(data.length() - 1) == 0xff) {
        data = data.mid(1, data.length() - 2);
    } else {
        unsigned char firstByte = data.at(pos++);
        qDebug() << Q_FUNC_INFO << __LINE__ << firstByte;
        bool fin = ((firstByte & 0x80) >> 7 == 1);
        qDebug() << Q_FUNC_INFO << __LINE__ << fin;
        if (!fin) {
            message.append(data);
            return;
        }
        char opcode = (firstByte & 0x0f);
        switch (opcode) {
        case 0x0:
            qDebug() << Q_FUNC_INFO << __LINE__ << "continuation";
            break;
        case 0x1:
            qDebug() << Q_FUNC_INFO << __LINE__ << "text";
            break;
        case 0x2:
            qDebug() << Q_FUNC_INFO << __LINE__ << "binary";
            break;
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7:
            qDebug() << Q_FUNC_INFO << __LINE__ << "reserved for further non-control frames";
            break;
        case 0x8:
            qDebug() << Q_FUNC_INFO << __LINE__ << "connection close";
            break;
        case 0x9:
            qDebug() << Q_FUNC_INFO << __LINE__ << "ping";
            break;
        case 0xA:
            qDebug() << Q_FUNC_INFO << __LINE__ << "pong";
            break;
        default:
            qDebug() << Q_FUNC_INFO << __LINE__ << "reserved for further control frames";
            break;
        }
        qDebug() << Q_FUNC_INFO << __LINE__ << opcode;

        unsigned char secondByte = data.at(pos++);
        bool mask = ((secondByte & 0x80) >> 7 == 1);
        if (!mask) {
    //        qWarning() << "browse should always mask the payload data";
    //        return;
        }
        qulonglong payloadLength = (secondByte & 0x7f);
        qDebug() << Q_FUNC_INFO << __LINE__ << payloadLength;
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
//    qDebug() << Q_FUNC_INFO << __LINE__ << data;
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
    connection->write(data);
    connection->flush();
}


void QWebSocket::Private::disconnected()
{
    qDebug() << Q_FUNC_INFO << __LINE__ << q << sender();
    q->deleteLater();
    qDebug() << Q_FUNC_INFO << __LINE__;
}

QWebSocket::QWebSocket(QHttpConnection *parent, const QUrl &url, const QHash<QByteArray, QByteArray> &rawHeaders)
    : QObject(parent)
    , d(new Private(parent, this, url, rawHeaders))
{
}

const QString &QWebSocket::remoteAddress() const
{
    return d->remoteAddress;
}

bool QWebSocket::hasRawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.contains(headerName.toLower());
}

QByteArray QWebSocket::rawHeader(const QByteArray &headerName) const
{
    return d->rawHeaders.value(headerName.toLower());
}

QList<QByteArray> QWebSocket::rawHeaderList() const
{
    return d->rawHeaders.keys();
}

const QList<QNetworkCookie> &QWebSocket::cookies() const
{
    return d->cookies;
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

#include "qwebsocket.moc"
