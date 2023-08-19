
#include "olympusdaemonprocess.h"
#include <QSharedMemory>


int main(int argc, char *argv[])
{
    QCoreApplication coreApp(argc, argv);

    // Check if another instance of the program is already running
    QSharedMemory sharedMemory("IX83Server");
    if (!sharedMemory.create(1)) {
        qCritical() << "Another instance is already running.";
        std::cout << "Press any key to exit..." << std::endl;
        _getch();
        return 0;
    }

    // Default values
    int port = 1500;
    bool exclusive = true;
    int time = 5;
    bool hideWindow = true;

    // Parse command line arguments
    QStringList args = coreApp.arguments();
    for (int i = 1; i < args.size(); i++) {
        if (args[i] == "-p" && i + 1 < args.size()) {            
            port = args[i + 1].toInt();
        }
        else if (args[i] == "-e" && i + 1 < args.size()) {
            exclusive = (args[i + 1].toLower() == "true");
        }
        else if (args[i] == "-t" && i + 1 < args.size()) {
            time = args[i + 1].toInt();
        }
        else if (args[i] == "-show") {
            hideWindow = false;
        }
    }

    qRegisterMetaType<MessageSocket>("MessageSocket");

    OlympusDaemonProcess *daemon = new OlympusDaemonProcess(nullptr);

    daemon->setPort(port);
    daemon->setExclusive(exclusive);
    daemon->setTimer(time);

    if (!daemon->initializeServer()) {
        qCritical() << "[Daemon] Unable to open server on port" << port
                    << "Please check firewall configurations.";
        std::cout << "Press any key to exit..." << std::endl;
        _getch();
        return 0;
    }

    daemon->initializePortManager();

    if (hideWindow) {
        ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    }

    return coreApp.exec();
}
