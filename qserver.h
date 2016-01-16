#ifndef QSERVER_H
#define QSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QDir>

#include <QDateTime>

#include "qclientthread.h"


class QServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit QServer(QString storageFolder, int port, QObject *parent = 0);
    void StartServer();
signals:

public slots:

protected:
    void incomingConnection(int socketDescriptor);
private:
    QString storageFolder;
    int port;
};

#endif // QSERVER_H
