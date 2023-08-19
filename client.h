
#ifndef CLIENT_H
#define CLIENT_H


#include <QTimer>
#include <QTcpSocket>


class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);
    ~Client();

    void setPort(int);
    void initialize();

private:
    int port;

    QTcpSocket *socket;

signals:

private slots:
    void receiveNotify(QString);

};

#endif // CLIENT_H
