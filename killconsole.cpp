#include <QCoreApplication>
#include <QThread>
#include <windows.h>
#include <iphlpapi.h>
#include <tlhelp32.h>
#include <QString>
#include <QDebug>
#include <QTcpSocket>
#include <iostream>
#include <conio.h>

#pragma comment(lib, "iphlpapi.lib")

// 查找指定进程的PID
DWORD FindProcessId(const QString& processName)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry = { sizeof(PROCESSENTRY32) };
        if (Process32First(snapshot, &processEntry))
        {
            do
            {
                if (processName == QString::fromWCharArray(processEntry.szExeFile))
                {
                    pid = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

// 查找指定进程的TCP连接信息
int FindTcpConnections(DWORD pid)
{
    MIB_TCPTABLE_OWNER_PID* tcpTable = nullptr;
    ULONG tcpTableSize = 0;
    DWORD result = GetExtendedTcpTable(nullptr, &tcpTableSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (result == ERROR_INSUFFICIENT_BUFFER)
    {
        tcpTable = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(new char[tcpTableSize]);
        result = GetExtendedTcpTable(tcpTable, &tcpTableSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        if (result == NO_ERROR)
        {
            for (DWORD i = 0; i < tcpTable->dwNumEntries; i++)
            {
                if (tcpTable->table[i].dwOwningPid == pid)
                {
//                    QString localAddress = QString("%1:%2").arg(inet_ntoa(*(in_addr*)&tcpTable->table[i].dwLocalAddr)).arg(ntohs(tcpTable->table[i].dwLocalPort));
                    int port = ntohs(tcpTable->table[i].dwLocalPort);
                    qDebug() << "Found an instance of IX83Server has opened the port" << port;
                    std::cout << "Do you want to close this process? (Y/n): ";
                    char c;
                    std::cin >> c;
                    if (c == 'y' || c == 'Y')
                    {
                        QTcpSocket *socket = new QTcpSocket(nullptr);
                        socket->connectToHost(QHostAddress::LocalHost, port);
                        if (!socket->waitForConnected()) {
                            delete[] reinterpret_cast<char*>(tcpTable);
                            return 0;  // failed to connect
                        }
                        socket->write("{Daemon}Exit\r\n");
                        socket->flush();
                        if (socket->waitForReadyRead(1000)) {
                            QString response = QString::fromUtf8(socket->readAll());
                            if (response.contains("quit")) {
                                qDebug() << "IX83Server quitted safely.";
                                delete[] reinterpret_cast<char*>(tcpTable);
                                return 1;  // quit safely
                            }
                            else {
                                delete[] reinterpret_cast<char*>(tcpTable);
                                return 2;  // failed to quit safely
                            }
                        }
                        else {
                            delete[] reinterpret_cast<char*>(tcpTable);
                            return 2;  // failed to quit safely
                        }
                    }
                    else if (c == 'n' || c == 'N')
                    {
                        delete[] reinterpret_cast<char*>(tcpTable);
                        return 3;  // do not quit
                    }
                }
            }
        }
    }
    return 0;
}

//int main(int argc, char *argv[])
int main()
{
    //    QCoreApplication app(argc, argv);

    // 查找IX83Server.exe的PID
    while (1)
    {
        DWORD pid = FindProcessId("IX83Server.exe");
        // 查找IX83Server.exe的TCP连接信息
        if (pid != 0) {
            int result = FindTcpConnections(pid);
            if (result == 0) {
                qDebug() << "Found an instance of IX83Server without opening any port";
                std::cout << "Do you want to terminate this process? (Y/n): ";
                char c;
                std::cin >> c;
                if (c == 'y' || c == 'Y')
                {
                    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                    if (processHandle != nullptr)
                    {
                        TerminateProcess(processHandle, 0);
                        CloseHandle(processHandle);
                        qDebug() << "IX83Server has been terminated.";
                    }
                }
            }
            else if (result == 2) {
                qDebug() << "This instance of IX83Server is connected by another application and the port is exclusive";
                std::cout << "Do you want to terminate this process? (Y/n): ";
                char c;
                std::cin >> c;
                if (c == 'y' || c == 'Y')
                {
                    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                    if (processHandle != nullptr)
                    {
                        TerminateProcess(processHandle, 0);
                        CloseHandle(processHandle);
                        qDebug() << "IX83Server has been terminated.";
                    }
                }
            }
            else if (result == 3) {
                break;
            }
            QThread::msleep(500);
        }
        else {
            qDebug() << "No more instance of IX83Server is found.";
            break;
        }
    }

    std::cout << "Press any key to exit..." << std::endl;
    _getch();

    //    return app.exec();
}
