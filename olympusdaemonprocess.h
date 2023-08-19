
#ifndef OLYMPUSDAEMONPROCESS_H
#define OLYMPUSDAEMONPROCESS_H

#include <QCoreApplication>
#include <QMap>
#include <QRegularExpression>
#include "server.h"
#include "client.h"
#include "olympusportmanager.h"


class OlympusDaemonProcess : public QObject
{
    Q_OBJECT
public:
    explicit OlympusDaemonProcess(QObject *parent = nullptr);
    void setPort(int);
    void setExclusive(bool);
    void setTimer(int);
    bool initializeServer();
    bool initializePortManager();

private:
    Server *server;
    Client *client;
    OlympusPortManager *olympusPM;

    static QRegularExpression numericRegex;

    int port;
    bool exclusive;
    int time;
    int numDaemonStartServerAttempts;

    void waitWithoutBlocking(int);

    void quitPortManager();

    typedef void (OlympusDaemonProcess::*RequestHandler)(MessageSocket);
    QMap<QString, RequestHandler> handlers;

    void handleDefault(MessageSocket);
    void handleQuitDaemon(MessageSocket);
    void handleQuitPortManager(MessageSocket);
    void handleShowConsole(MessageSocket);
    void handleHideConsole(MessageSocket);
    void handleInitializePortManager(MessageSocket);
    void handleEnumerateInterface(MessageSocket);
    void handleOpenInterface(MessageSocket);
    void handleCloseInterface(MessageSocket);
    void handleIsConnected(MessageSocket);
    void handleLogIn(MessageSocket);
    void handleLogOut(MessageSocket);

signals:
    void sendResponse(MessageSocket);

private slots:
    void quitDaemon();
    void checkServerStatus();

    void receiveRequest(MessageSocket);
    void receiveException(MessageSocket);
};


#endif // OLYMPUSDAEMONPROCESS_H
