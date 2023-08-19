
#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>

struct MessageSocket {
    QString request;
    QString response;
    QTcpSocket* socket;
};

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();
    bool openServer(int, bool);
    bool isServerOpen() const;

private:
    QTcpServer *server;
    QList<QTcpSocket*> socketList;
    QHash<QTcpSocket*, QByteArray> socketBufferMap;

    bool exclusive;

    void processRequest(MessageSocket);
    void sendResponseToClient(MessageSocket);

signals:
    void requestForDaemon(MessageSocket);
    void requestForOlympus(MessageSocket);

private slots:
    // Server slots
    void handleNewConnection();
    void receiveDataFromSocket();
    void socketDisconnected();

    void receiveResponse(MessageSocket);
};

#endif // SERVER_H
