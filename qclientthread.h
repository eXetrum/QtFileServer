#ifndef QCLIENTTHREAD_H
#define QCLIENTTHREAD_H

#include <QThread>
#include <QTcpSocket>
#include <QDir>
#include <QDirIterator>
#include <QUrl>
#include <QUrlQuery>
#include <QRegExp>
#include <QDebug>

#include <map>

class QClientThread : public QThread
{
    Q_OBJECT
public:
    explicit QClientThread(QString fodler, int ID, QObject *parent = 0);
    void run();

signals:
    void error(QTcpSocket::SocketError socketError);

public slots:
    void readyRead();
    void disconnect();

private:
    // Буфер приема запроса от клиента
    QByteArray receiveBuff;
    // Маркер характеризующий окончание приема заголовков
    bool headerReceived;
    // Размер тела запроса
    long contentLen;
    // Этот метод позволяет вернуть ответ клиенту
    void ProcessData(QByteArray receivedData);
    // Получить файл стилей
    QByteArray GetCSS();
    // Получить конкретную шпаргалку
    QByteArray GetShpora(QString subject, QString theme, QString shpora);
    // Список шпаргалок выбранного предмета и темы
    QString ListShpores(QString subject, QString theme);
    // Список тем указанонго предмета
    QString ListThemes(QString subject);
    // Получить список предметов
    QString ListSubjects();
    // Путь к каталогу-хранилищу
    QString storageFolder;
    // Дескриптор клиентского сокета
    int socketDescriptor;
    // Клиентский сокет
    QTcpSocket *socket;
};

#endif // QCLIENTTHREAD_H
