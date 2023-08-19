
#include "server.h"

Server::Server(QObject *parent)
    : QObject{parent}
{
    // assigning private member variables
    server = new QTcpServer;

    // connecting signals & slots
    connect(server, SIGNAL(newConnection()), this, SLOT(handleNewConnection()));
}

Server::~Server() {
    server->close();
    qDebug() << "[Server] Server is closed and destroyed";
}

bool Server::openServer(int port, bool exclusive) {

    this->exclusive = exclusive;

    bool status;

    if (!server->listen(QHostAddress::Any, port)) {
        status = false;
        qDebug() << "[Server] Failed to start server:" << server->errorString();
    }
    else {
        status = true;
        qDebug() << "[Server] Server started on port" << server->serverPort();
    }

    return status;
}

void Server::handleNewConnection() {

    // If exclusive is true and there is already a connection, reject new connections
    if (exclusive) {
        if (!socketList.isEmpty()) {
            return;
        }
    }

    QTcpSocket *socket = server->nextPendingConnection();

    connect(socket, &QTcpSocket::readyRead, this, &Server::receiveDataFromSocket);
    connect(socket, &QTcpSocket::disconnected, this, &Server::socketDisconnected);

    socketList.append(socket);
    socketBufferMap.insert(socket, QByteArray());

    qDebug() << "[Server] A new socket is connected";

}

void Server::receiveDataFromSocket() {

    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());

    if (socket) {

        QByteArray &buffer = socketBufferMap[socket];
        buffer += socket->readAll();

        if (buffer.contains("\r\n")) {
            MessageSocket ms;
            ms.request = QString::fromUtf8(buffer.left(
                buffer.indexOf("\r\n")));
            ms.socket = socket;
            buffer = QByteArray();
            processRequest(ms);
        }
        else {
            qWarning() << "Current buffer:" << buffer;
        }

    }
}

void Server::processRequest(MessageSocket ms) {

    if (ms.request.contains("{Daemon}", Qt::CaseInsensitive)) {
        ms.request.remove("{Daemon}", Qt::CaseInsensitive);
        emit requestForDaemon(ms);
    }
    else if (ms.request.contains("[Olympus]", Qt::CaseInsensitive)) {
        ms.request.remove("[Olympus]", Qt::CaseInsensitive);
        emit requestForOlympus(ms);
    }
    else {
        ms.response = "The server does not accept this type of request.\n"
                      "To reach the Daemon, please use '{Daemon}' as the header.\n"
                      "To reach the Olympus Port Manager, "
                      "please use '[Olympus]' as the header, and follow the Olympus IX83 IF specification.";
        sendResponseToClient(ms);
    }

}

void Server::socketDisconnected() {

    // Cast the pointer returned by sender() to a QTcpSocket* pointer
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());

    if (socket) {
        socketBufferMap.remove(socket);
        socketList.removeOne(socket);
        socket->deleteLater();
        qDebug() << "[Server] A socket is disconnected";
    }

}

void Server::sendResponseToClient(MessageSocket ms) {
    if (socketList.contains(ms.socket)) {
//        qDebug() << "[Server Reply]" << Qt::endl
//                 << "socket:" << ms.socket << Qt::endl
//                 << "request:" << ms.request << Qt::endl
//                 << "response:" << ms.response;
        if (!ms.response.endsWith("\r\n")) {
            ms.response.append("\r\n");
        }
        ms.socket->write(ms.response.toUtf8());
        ms.socket->flush();
    }
}

void Server::receiveResponse(MessageSocket ms) {
    sendResponseToClient(ms);
}

bool Server::isServerOpen() const {
    return server->isListening();
}
