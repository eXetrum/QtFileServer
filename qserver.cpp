#include "qserver.h"

QServer::QServer(QString storageFolder, int port, QObject *parent) :
    QTcpServer(parent)
{
    // Запоминаем рабочий каталог
    this->storageFolder = storageFolder;
    // Сохраняем номер порта, который слушает сервер
    this->port = port;
    // Проверяем есть ли рабочая директория указанная как хранилище
    if(!QDir(storageFolder).exists())
    {
        // Если нету - создаем пустую
        QDir().mkdir(storageFolder);
    }
}
// Запуск сервера
void QServer::StartServer()
{
    // Слушаем указанный порт
    if(!this->listen(QHostAddress::Any, port))
    {
        qDebug() << "Could not start server";
    }
    else
    {
        qDebug() << "Listening...";
    }
}
// Новый клиент
void QServer::incomingConnection(int socketDescriptor)
{
    // Отладочное сообщение в консоль
    qDebug() << socketDescriptor << " Connecting...";
    // Создаем рабочий поток
    QClientThread *thread = new QClientThread( storageFolder, socketDescriptor, this );
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    // Запускаем поток
    thread->start();
}
