
#ifndef OLYMPUSPORTMANAGER_H
#define OLYMPUSPORTMANAGER_H


#include <iostream>
#include <conio.h>
#include <QTimer>
#include <QEventLoop>
#include <olympussdk.h>
#include <server.h>


class OlympusPortManager : public QObject
{
    Q_OBJECT
public:
    explicit OlympusPortManager(QObject *parent = nullptr);
    ~OlympusPortManager();

    MDK_MSL_CMD	m_Cmd;

    bool initialize();
    bool isConnected();
    bool isLogged();
    bool isBusy();

    int enumerateInterface();
    bool openInterface(int);
    void closeInterface();
    void logIn();
    void logOut();

private:
    bool logged = false;
    bool pendingLog = false;
    bool waitingResponse = false;
    int numInterface = 0;

    MessageSocket pendingMessage;

    void *pInterface;
    ptr_Initialize ptr_init;
    ptr_EnumInterface ptr_enumIf;
    ptr_GetInterfaceInfo ptr_getInfo;
    ptr_OpenInterface ptr_openIf;
    ptr_SendCommand ptr_sendCmd;
    ptr_RegisterCallback ptr_reCb;
    ptr_CloseInterface ptr_closeIf;

    bool sendCommand(QString);
    void sendResponse();
    void clearPendingMessage();
    void waitWithoutBlocking(int);

    friend int CALLBACK CommandCallback(ULONG MsgId,
                                        ULONG wParam,
                                        ULONG lParam,
                                        PVOID pv,
                                        PVOID pContext,
                                        PVOID pCaller);
    friend int CALLBACK NotifyCallback(ULONG MsgId,
                                       ULONG wParam,
                                       ULONG lParam,
                                       PVOID pv,
                                       PVOID pContext,
                                       PVOID pCaller);
    friend int CALLBACK ErrorCallback(ULONG MsgId,
                                      ULONG wParam,
                                      ULONG lParam,
                                      PVOID pv,
                                      PVOID pContext,
                                      PVOID pCaller);

signals:
    void sendMessageSocket(MessageSocket);
    void sendNotify(QString);
    void emergencyStop();
    void sendGlobalQuit();

private slots:
    void receiveRequest(MessageSocket);
    void receiveEmergencyStop();
};

#endif // OLYMPUSPORTMANAGER_H
