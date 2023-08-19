
#include "olympusdaemonprocess.h"


QRegularExpression OlympusDaemonProcess::numericRegex("(\\d+)$");

OlympusDaemonProcess::OlympusDaemonProcess(QObject *parent)
    : QObject{parent}
{
    // Assign private member variables
    server = new Server(this);
    olympusPM = nullptr;
    client = nullptr;

    // Default values
    port = 1500;
    exclusive = true;
    time = 5000;

    // Connect signals & slots
    QObject::connect(server, SIGNAL(requestForDaemon(MessageSocket)),
                     this, SLOT(receiveRequest(MessageSocket)), Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(sendResponse(MessageSocket)),
                     server, SLOT(receiveResponse(MessageSocket)), Qt::QueuedConnection);

    // Initialize the handlers map
    handlers = QMap<QString, RequestHandler> ();

    // Alias for quit daemon process
    handlers.insert("quitdaemon", &OlympusDaemonProcess::handleQuitDaemon);
    handlers.insert("quitprogram", &OlympusDaemonProcess::handleQuitDaemon);
    handlers.insert("quitapp", &OlympusDaemonProcess::handleQuitDaemon);
    handlers.insert("exit", &OlympusDaemonProcess::handleQuitDaemon);

    // Alias for quit Olympus port manager
    handlers.insert("quitportmanager", &OlympusDaemonProcess::handleQuitPortManager);
    handlers.insert("quitpm", &OlympusDaemonProcess::handleQuitPortManager);
    handlers.insert("exitportmanager", &OlympusDaemonProcess::handleQuitPortManager);
    handlers.insert("exitpm", &OlympusDaemonProcess::handleQuitPortManager);

    // Alias for show/hide console window
    handlers.insert("showconsole", &OlympusDaemonProcess::handleShowConsole);
    handlers.insert("showwindow", &OlympusDaemonProcess::handleShowConsole);
    handlers.insert("show", &OlympusDaemonProcess::handleShowConsole);
    handlers.insert("hideconsole", &OlympusDaemonProcess::handleHideConsole);
    handlers.insert("hidewindow", &OlympusDaemonProcess::handleHideConsole);
    handlers.insert("hide", &OlympusDaemonProcess::handleHideConsole);

    // Alias for initialize Olympus port manager
    handlers.insert("initializeportmanager", &OlympusDaemonProcess::handleInitializePortManager);
    handlers.insert("initportmanager", &OlympusDaemonProcess::handleInitializePortManager);
    handlers.insert("initpm", &OlympusDaemonProcess::handleInitializePortManager);

    // Alias for enumerate interface
    handlers.insert("enumerateinterface", &OlympusDaemonProcess::handleEnumerateInterface);
    handlers.insert("enuminterface", &OlympusDaemonProcess::handleEnumerateInterface);
    handlers.insert("enumerateport", &OlympusDaemonProcess::handleEnumerateInterface);
    handlers.insert("enumport", &OlympusDaemonProcess::handleEnumerateInterface);

    // Alias for open interface
    handlers.insert("openinterface", &OlympusDaemonProcess::handleOpenInterface);
    handlers.insert("openport", &OlympusDaemonProcess::handleOpenInterface);

    // Alias for close interface
    handlers.insert("closeinterface", &OlympusDaemonProcess::handleCloseInterface);
    handlers.insert("closeport", &OlympusDaemonProcess::handleCloseInterface);

    handlers.insert("isconnected", &OlympusDaemonProcess::handleIsConnected);
    handlers.insert("login", &OlympusDaemonProcess::handleLogIn);
    handlers.insert("logout", &OlympusDaemonProcess::handleLogOut);

}

void OlympusDaemonProcess::setPort(int port)
{
    this->port = port;
}

void OlympusDaemonProcess::setExclusive(bool exclusive)
{
    this->exclusive = exclusive;
}

void OlympusDaemonProcess::setTimer(int time)
{
    this->time = time;
}

void OlympusDaemonProcess::waitWithoutBlocking(int time)
{
    QEventLoop loop;
    QTimer::singleShot(time, &loop, SLOT(quit()));  // time unit: msec
    loop.exec();
}

bool OlympusDaemonProcess::initializeServer()
{
    int attempts = 0;
    while (!server->openServer(port, exclusive)) {
        if (attempts++ > 10) {
            return false;
        }
    }

    if (time > 0) {
        numDaemonStartServerAttempts = 0;
        // Set up a timer to check the server status
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &OlympusDaemonProcess::checkServerStatus);
        timer->start(time*1000); // unit milliseconds
        qDebug() << "[Daemon] Server status check is enabled with a time interval of" << time << "seconds";
    }
    else {
        qDebug() << "[Daemon] Server status check is disabled";
    }

    return true;
}

bool OlympusDaemonProcess::initializePortManager()
{
    if (!olympusPM) {
        QObject::disconnect(server, SIGNAL(requestForOlympus(MessageSocket)),
                            this, SLOT(receiveException(MessageSocket)));

        olympusPM = new OlympusPortManager(this);

        if (olympusPM->initialize()) {
            QObject::connect(server, SIGNAL(requestForOlympus(MessageSocket)),
                             olympusPM, SLOT(receiveRequest(MessageSocket)), Qt::QueuedConnection);
            QObject::connect(olympusPM, SIGNAL(sendMessageSocket(MessageSocket)),
                             server, SLOT(receiveResponse(MessageSocket)), Qt::QueuedConnection);
            QObject::connect(olympusPM, SIGNAL(sendGlobalQuit()), this, SLOT(quitDaemon()));
            return true;
        }
        else {
            quitPortManager();
            QObject::connect(server, SIGNAL(requestForOlympus(MessageSocket)),
                             this, SLOT(receiveException(MessageSocket)), Qt::QueuedConnection);
            return false;
        }
    }
    return true;
}

void OlympusDaemonProcess::quitPortManager()
{
    if (olympusPM) {
        if (olympusPM->isConnected()) {
            olympusPM->closeInterface();
            while (olympusPM->isConnected())
                waitWithoutBlocking(50);
        }
        delete olympusPM;
        olympusPM = nullptr;
    }
}

void OlympusDaemonProcess::quitDaemon()
{
    quitPortManager();
    delete server;
    qApp->quit();
}

void OlympusDaemonProcess::checkServerStatus()
{
    bool serverStatus = server->isServerOpen();

    if (!serverStatus) {
        // Server is not running, try to start it
        if (!server->openServer(port, exclusive)) {
            // Server failed to start
            if (++numDaemonStartServerAttempts > 10) {
                qCritical() << "[Daemon] Unable to open server on port" << port
                            << "Please check firewall configurations.";
                ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
                std::cout << "Press any key to exit..." << std::endl;
                _getch();
                quitDaemon();
            }
        } else {
            // Server started successfully
            numDaemonStartServerAttempts = 0;
        }
    } else {
        // Server is running
        numDaemonStartServerAttempts = 0;
    }
}

void OlympusDaemonProcess::receiveRequest(MessageSocket ms)
{
    // Get the request string and remove any leading or trailing whitespace
    QString request = ms.request.trimmed().toLower();

    // Use a regular expression to extract the numeric part of the request
    QRegularExpressionMatch match = numericRegex.match(request);
    if (match.hasMatch()) {
        QString numericPart = match.captured(1);
        request.chop(numericPart.length());
    }

    // Look up the handler function using the modified request string
    RequestHandler handler = handlers.value(request, &OlympusDaemonProcess::handleDefault);

    // Call the handler function with the message socket
    (this->*handler)(ms);
}

void OlympusDaemonProcess::receiveException(MessageSocket ms)
{
    ms.response = "Error:-8081:The port manager is not running. "
                  "Please use '{Daemon}InitializePortManager' to initialize port manager first.";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleDefault(MessageSocket ms)
{
    ms.response = "Successfully reached the daemon!\n"
                  "The daemon accepts the following requests:\n"
                  " - {Daemon}InitializePortManager\n"
                  " - {Daemon}IsConnected\n"
                  " - {Daemon}EnumerateInterface\n"
                  " - {Daemon}OpenInterfaceX\n"
                  " - {Daemon}CloseInterface\n"
                  " - {Daemon}LogIn\n"
                  " - {Daemon}LogOut\n"
                  " - {Daemon}QuitPortManager\n"
                  " - {Daemon}QuitDaemon\n"
                  "These commands are not case-sensitive, "
                  "but please note that the 'OpenInterfaceX' "
                  "command must include the interface index X, "
                  "such as {Daemon}OpenInterface0.";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleQuitDaemon(MessageSocket ms)
{
    ms.response = "Daemon is quitting now.";
    emit sendResponse(ms);
    QTimer::singleShot(100, this, SLOT(quitDaemon()));
}

void OlympusDaemonProcess::handleQuitPortManager(MessageSocket ms)
{
    quitPortManager();
    ms.response = "1";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleShowConsole(MessageSocket ms)
{
    ::ShowWindow(::GetConsoleWindow(), SW_SHOW);
    ms.response = "1";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleHideConsole(MessageSocket ms)
{
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    ms.response = "1";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleInitializePortManager(MessageSocket ms)
{
    if (olympusPM) {
        ms.response = "1";
    }
    else
    {
        if (initializePortManager())
            ms.response = "1";
        else
            ms.response = "Error:-8080:Port Manager initialization failed. "
                          "Please check if the DLL files are intact. "
                          "For more details, please refer to the error message "
                          "on the server console.";
    }
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleEnumerateInterface(MessageSocket ms)
{
    if (olympusPM) {
        int numInterface = olympusPM->enumerateInterface();
        ms.response = QString::number(numInterface);
    }
    else {
        ms.response = "Error:-8081:The port manager is not running. "
                      "Please use '{Daemon}InitializePortManager' to "
                      "initialize port manager first.";
    }
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleOpenInterface(MessageSocket ms)
{
    if (olympusPM) {
        QString request = ms.request.trimmed().toLower();
        QRegularExpressionMatch match = numericRegex.match(request);

        if (match.hasMatch()) {
            QStringList capturedTexts = match.capturedTexts();
            int interfaceIndex = capturedTexts.at(1).toInt();
            qDebug() << "[Server] Receive request to open interface" << interfaceIndex;

            if (olympusPM->isConnected()) {
                ms.response = "Error:-8084:An interface is opened. "
                              "Before opening a new interface, "
                              "please use {Daemon}CloseInterface to close the older first.";
            }
            else {
                if (olympusPM->openInterface(interfaceIndex)) {
                    client = new Client(this);
                    client->setPort(port+1);
                    client->initialize();
                    QObject::connect(olympusPM, SIGNAL(sendNotify(QString)),
                                     client, SLOT(receiveNotify(QString)), Qt::QueuedConnection);
                    ms.response = "1";
                }
                else {
                    ms.response = "Error:-8082:Failed to open interface "
                                  + QString::number(interfaceIndex)
                                  + ". Please refer to the error message on the server console for more details.";
                }
            }
        }
        else {
            qWarning() << "Request to open interface, but without a index";
            ms.response = "Error:-8083:The open interface message should "
                          "specify the interface index number.";
        }
    }
    else {
        ms.response = "Error:-8081:The port manager is not running. "
                      "Please use '{Daemon}InitializePortManager' to initialize port manager first.";
    }
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleCloseInterface(MessageSocket ms)
{
    if (olympusPM) {
        olympusPM->closeInterface();
    }
    if (client) {
        delete client;
        client = nullptr;
    }
    ms.response = "1";
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleIsConnected(MessageSocket ms)
{
    if (olympusPM) {
        if (olympusPM->isConnected())
            ms.response = "1";
        else
            ms.response = "0";
    }
    else {
        ms.response = "Error:-8081:The port manager is not running. "
                      "Please use '{Daemon}InitializePortManager' to "
                      "initialize port manager first.";
    }
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleLogIn(MessageSocket ms)
{
    if (olympusPM) {
        if (olympusPM->isConnected()) {
            olympusPM->logIn();
            while (!olympusPM->isLogged()) {
                waitWithoutBlocking(50);
            }
            ms.response = "1";
        }
        else {
            ms.response = "Error:-8085:No interface was opened. "
                          "Please use '{Daemon}OpenInterfaceX' to "
                          "open an interface first.";
        }
    }
    else {
        ms.response = "Error:-8081:The port manager is not running. "
                      "Please use '{Daemon}InitializePortManager' to "
                      "initialize port manager first.";
    }
    emit sendResponse(ms);
}

void OlympusDaemonProcess::handleLogOut(MessageSocket ms)
{
    if (olympusPM) {
        if (olympusPM->isConnected()) {
            olympusPM->logOut();
            while (olympusPM->isLogged()) {
                waitWithoutBlocking(50);
            }
            ms.response = "1";
        }
        else {
            ms.response = "Error:-8085:No interface was opened. "
                          "Please use '{Daemon}OpenInterfaceX' to "
                          "open an interface first.";
        }
    }
    else {
        ms.response = "Error:-8081:The port manager is not running. "
                      "Please use '{Daemon}InitializePortManager' to "
                      "initialize port manager first.";
    }
    emit sendResponse(ms);
}
