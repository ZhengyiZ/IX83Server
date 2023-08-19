
#include "olympusportmanager.h"


extern OlympusPortManager *g_olympusPM;
OlympusPortManager *g_olympusPM = nullptr;

bool loadLibraries(QLibrary &pm,
                   QLibrary &pd,
                   QLibrary &fsi,
                   QLibrary &gt)
{
    int count = 0;
    if (pm.load()) count++;
    if (pd.load()) count++;
    if (fsi.load()) count++;
    if (gt.load()) count++;

    if (count < 4) {
        qCritical() << "SDK Loading Error:\n"
                    << "Please check that the following files are in the installation directory:\n"
                    << " - msl_pm.dll\n"
                    << " - msl_pd_1394.dll\n"
                    << " - fsi1394.dll\n"
                    << " - gt_log.dll\n"
                    << " - gtlib.config\n"
                    << "If any of these files are missing, please locate them in the IX3 SDK.";
        return false;
    }
    qDebug() << "[PM] Libraries were loaded";
    return true;
}

bool loadFunctionPointers(QLibrary &pm,
                          ptr_Initialize &ptr_init,
                          ptr_EnumInterface &ptr_enumIf,
                          ptr_GetInterfaceInfo &ptr_getInfo,
                          ptr_OpenInterface &ptr_openIf,
                          ptr_SendCommand &ptr_sendCmd,
                          ptr_RegisterCallback &ptr_reCb,
                          ptr_CloseInterface &ptr_closeIf)
{
    ptr_init = (ptr_Initialize)pm.resolve("MSL_PM_Initialize");
    ptr_enumIf = (ptr_EnumInterface)pm.resolve("MSL_PM_EnumInterface");
    ptr_getInfo = (ptr_GetInterfaceInfo)pm.resolve("MSL_PM_GetInterfaceInfo");
    ptr_openIf = (ptr_OpenInterface)pm.resolve("MSL_PM_OpenInterface");
    ptr_sendCmd = (ptr_SendCommand)pm.resolve("MSL_PM_SendCommand");
    ptr_reCb = (ptr_RegisterCallback)pm.resolve("MSL_PM_RegisterCallback");
    ptr_closeIf = (ptr_CloseInterface)pm.resolve("MSL_PM_CloseInterface");

    if (!ptr_init || !ptr_getInfo || !ptr_enumIf
        || !ptr_openIf || !ptr_sendCmd
        || !ptr_reCb || !ptr_closeIf) {
        qCritical() << "SDK Loading Error:\n"
                    << "Please confirm that the following DLLs are intact.\n"
                    << " - msl_pm.dll\n"
                    << " - msl_pd_1394.dll\n"
                    << " - fsi1394.dll\n"
                    << " - gt_log.dll";
        return false;
    }
    qDebug() << "[PM] Function pointers were loaded";
    return true;
}

//	COMMAND: call back entry from SDK port manager
int CALLBACK CommandCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                             PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);

    OlympusPortManager *olympusPM = (OlympusPortManager *)pContext;
    if (olympusPM) {
        // Extract string from struct
        QString response = QString(QLatin1String((char *)olympusPM->m_Cmd.m_Rsp));
        qDebug() << "[PM] Response < " << response;
        if (response.contains("L +")) {
            if (olympusPM->pendingLog) {
                olympusPM->logged = !olympusPM->logged;
                qDebug() << "[PM] Internal Logged Flag: " << olympusPM->isLogged();
                olympusPM->pendingLog = false;
            }
        }
        if (olympusPM->pendingMessage.socket) {
            olympusPM->pendingMessage.response = response;
            olympusPM->sendResponse();
        }
        olympusPM->waitingResponse = false;
    }
    else {
        qCritical() << "[PM] The callback pointer is invalid, program is quiting now!";
        g_olympusPM->pendingMessage.response = "Error:-8071:IX3-TPC is not responding. "
                                               "Please make sure that IX3-TPC is ready.";
        g_olympusPM->sendResponse();
        g_olympusPM->closeInterface();
        emit g_olympusPM->emergencyStop();
    }
    return 0;
}

//	NOTIFICATION: call back entry from SDK port manager
int	CALLBACK NotifyCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                            PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);

    char *notify = (char *)pv;
    OlympusPortManager *olympusPM = (OlympusPortManager *)pContext;

    QString rsp(notify);
    rsp.remove("\r\n", Qt::CaseInsensitive);
    qDebug() << "[PM] Notify < " << rsp;

    emit olympusPM->sendNotify(rsp);
    return 0;
}

//	ERROR NOTIFICATON: call back entry from SDK port manager
int	CALLBACK ErrorCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                           PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);
    return 0;
}

bool OlympusPortManager::sendCommand(QString command)
{
    if (waitingResponse) {
        QTimer::singleShot(50, this, [this, command]() {
            this->sendCommand(command);
        });
        return false;
    }

    qDebug() << "[PM] Command > " << command;

    waitingResponse = true;

    if (command.contains("L ")) {
        pendingLog = true;
    }

    // command initiate
    int	len;
    memset(&m_Cmd, 0x00, sizeof(MDK_MSL_CMD));

    // add a new line to command string
    command.append("\r\n");
    len = command.length();

    // QString to BYTE( unsigned char* )
    QByteArray tmp = command.toLatin1();

    // write to memory
    memcpy(m_Cmd.m_Cmd, tmp.data(), len);

    // set parameters for command
    // copy from sample source code
    m_Cmd.m_CmdSize		= 0;
    m_Cmd.m_Callback	= (void *)CommandCallback;
    m_Cmd.m_Context		= NULL;		// this pointer passed by pv
    m_Cmd.m_Timeout		= 10000;	// (ms)
    m_Cmd.m_Sync		= FALSE;
    m_Cmd.m_Command		= TRUE;		// TRUE: Command , FALSE: it means QUERY form ('?').

    // send command
    return this->ptr_sendCmd(this->pInterface, &m_Cmd);
}

OlympusPortManager::OlympusPortManager(QObject *parent)
    : QObject{parent}
{
    g_olympusPM = this;
    pInterface = nullptr;
    logged = false;
    clearPendingMessage();
    QObject::connect(this, SIGNAL(emergencyStop()), this, SLOT(receiveEmergencyStop()));
}

OlympusPortManager::~OlympusPortManager()
{
    g_olympusPM = nullptr;
    qDebug() << "[PM] The destructor is called.";
}

bool OlympusPortManager::initialize()
{
    QLibrary pm("msl_pm.dll");
    QLibrary pd("msl_pd_1394.dll");
    QLibrary fsi("fsi1394.dll");
    QLibrary gt("gt_log.dll");

    // If failed to load libraries
    if (!loadLibraries(pm, pd, fsi, gt)) return false;

    // If failed to load function pointers
    if (!loadFunctionPointers(pm, ptr_init,
                              ptr_enumIf,
                              ptr_getInfo,
                              ptr_openIf,
                              ptr_sendCmd,
                              ptr_reCb,
                              ptr_closeIf)) return false;

    ptr_init();

    return true;
}

bool OlympusPortManager::isConnected()
{
    return pInterface != nullptr;
}

bool OlympusPortManager::isLogged()
{
    return logged;
}

bool OlympusPortManager::isBusy()
{
    return waitingResponse;
}

void OlympusPortManager::waitWithoutBlocking(int time)
{
    QEventLoop loop;
    QTimer::singleShot(time, &loop, SLOT(quit()));  // time unit: msec
    loop.exec();
}

int OlympusPortManager::enumerateInterface()
{
    if (pInterface) return 0;
    numInterface = ptr_enumIf();
    return numInterface;
}

bool OlympusPortManager::openInterface(int interfaceIndex)
{
    if (interfaceIndex+1 > numInterface) {
        // if connected
        qWarning() << "The interface" << interfaceIndex
                   << "is not within the range" << numInterface-1;
        return false;
    }
    else {
        // Get the interface pointer
        if (pInterface) {
            qWarning() << "Request to open a new interface before close the older";
            return false;
        }
        else {
            ptr_getInfo(interfaceIndex, &pInterface);

            // Open interface
            if (ptr_openIf(pInterface)) {

                // Register callback functions
                if (ptr_reCb(this->pInterface, CommandCallback,
                             NotifyCallback, ErrorCallback, this)) {
                    return true;
                }
                else {
                    closeInterface();
                    qWarning() << "Could not register callbacks on interface" << interfaceIndex;
                    return false;
                }
            }
            else {
                pInterface = nullptr;
                qWarning() << "Could not open interface" << interfaceIndex;
                return false;
            }
        }
    }
}

void OlympusPortManager::closeInterface()
{
    if (pInterface) {
        if (logged && !pendingLog) {
            pendingLog = true;
            sendCommand("L 0,0");
            waitWithoutBlocking(100);
            closeInterface();
        }
        else if (pendingLog) {
            waitWithoutBlocking(100);
            closeInterface();
        }
        else {
            ptr_closeIf(pInterface);
            pInterface = nullptr;
        }
    }
}

void OlympusPortManager::receiveRequest(MessageSocket ms)
{
    if (waitingResponse) {
        QTimer::singleShot(50, this, [this, ms]() {
            this->receiveRequest(ms);
        });
    }

    QString command = ms.request.trimmed();
    pendingMessage = ms;

    if (pInterface != nullptr) {
        sendCommand(command);
    }
    else {
        pendingMessage.response = "Error:-8070:No interface is opened.";
        sendResponse();
    }

}

void OlympusPortManager::sendResponse()
{
    if (pendingMessage.socket) {
        emit sendMessageSocket(pendingMessage);
    }
    else {
        qWarning() << "PM Warning: MessageSocket do not have a correct socket";
    }
    clearPendingMessage();
}

void OlympusPortManager::clearPendingMessage()
{
    pendingMessage.request = QString();
    pendingMessage.response = QString();
    pendingMessage.socket = nullptr;
}

void OlympusPortManager::logIn()
{
    if (!logged) {
        pendingLog = true;
        sendCommand("L 1,0");
    }
}

void OlympusPortManager::logOut()
{
    if (logged) {
        pendingLog = true;
        sendCommand("L 0,0");
    }
}

void OlympusPortManager::receiveEmergencyStop()
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    std::cout << "Press any key to exit..." << std::endl;
    _getch();
    emit sendGlobalQuit();
}
