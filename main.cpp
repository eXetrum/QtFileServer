#include <QtCore/QCoreApplication>
#include "qserver.h"
#include <locale>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "Russian");
    QCoreApplication a(argc, argv);
    QString folder = "DataStorage";
    QString fullPath = QDir::cleanPath(a.applicationDirPath() + QDir::separator() + folder);
    int port = 80;
    QServer Server(fullPath, port);

    Server.StartServer();

    return a.exec();
}
