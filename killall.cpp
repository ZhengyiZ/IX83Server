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

// Find the specified process ID
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

// Find the TCP connection information of the specified process ID
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
                            qDebug() << "IX83Server on port" << port << "has been closed safely." << Qt::endl;
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
            }
        }
    }
    return 0;
}

//int main(int argc, char *argv[])
int main()
{
    //    QCoreApplication app(argc, argv);

    // find all instances of IX83Server
    while (1)
    {
        DWORD pid = FindProcessId("IX83Server.exe");
        if (pid != 0) {
            int result = FindTcpConnections(pid);
            if (result == 0) {
                qDebug() << "Found an instance of IX83Server without opening any port";
                HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (processHandle != nullptr)
                {
                    TerminateProcess(processHandle, 0);
                    CloseHandle(processHandle);
                    qDebug() << "IX83Server has been forced to terminate." << Qt::endl;
                }
            }
            else if (result == 2) {
                qDebug() << "This instance of IX83Server is connected by another application and the port is exclusive";

                HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (processHandle != nullptr)
                {
                    TerminateProcess(processHandle, 0);
                    CloseHandle(processHandle);
                    qDebug() << "IX83Server has been forced to terminate." << Qt::endl;
                }
            }
            QThread::msleep(500);
        }
        else {
            break;
        }
    }

    qDebug() << "No more instance of IX83Server has been found.";

    std::cout << "Press any key to exit..." << std::endl;
    _getch();

    //    return app.exec();
}
