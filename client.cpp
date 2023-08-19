
#include "client.h"

Client::Client(QObject *parent)
    : QObject{parent}
{
    socket = new QTcpSocket(this);
}

Client::~Client()
{
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    delete socket;
}

void Client::setPort(int port)
{
    this->port = port;
}

void Client::initialize()
{
    socket->connectToHost(QHostAddress::LocalHost, port);
    if (!socket->waitForConnected())
    {
        qWarning() << "[Client] Failed to connect to the server on port" << port;
    }
    qDebug() << "[Clinet] Connected to the server on port" << port;
}

void Client::receiveNotify(QString notify)
{
    if (!notify.endsWith("\r\n")) {
        notify.append("\r\n");
    }

    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(notify.toUtf8());
        socket->flush();
    }
    else {
        qWarning() << "[Clinet] The connection has been blocked somehow, trying to reconnect...";
        initialize();
        receiveNotify(notify);
//        QTimer::singleShot(50, this, [this, notify]() {
//            this->receiveNotify(notify);
//        });
    }
}
