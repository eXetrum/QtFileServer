#include "qclientthread.h"

QClientThread::QClientThread(QString storageFolder, int ID, QObject *parent) : QThread(parent)
{
    this->storageFolder = storageFolder;
    this->socketDescriptor = ID;

}
// Запуск потока
void QClientThread::run()
{
    qDebug() << socketDescriptor << " Starting thread";
    socket = new QTcpSocket();
    if(!socket->setSocketDescriptor(this->socketDescriptor))
    {
        emit error(socket->error());
        return;
    }

    headerReceived = false;

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnect()), Qt::DirectConnection);

    qDebug() << socketDescriptor << " Client connected";

    exec();
}

void QClientThread::readyRead()
{
    qDebug() << "+= Received";
    // receive header
    if(!headerReceived)
    {
        receiveBuff += socket->readAll();
        if(receiveBuff.contains("\r\n\r\n")) {
            int bytes = receiveBuff.indexOf("\r\n\r\n") + 1;
            QByteArray message = receiveBuff.left(bytes + 1);
            headerReceived = true;
            // Если задано свойство длинна контента, значит есть тело сообщения - принимаем
            int headerPos = message.toLower().indexOf("content-length:");

            if(headerPos != -1)
            {
                //bodyBuff.clear();
                contentLen = 0;
                QString contentLenHeader = message.mid(headerPos, message.indexOf("\r\n", headerPos) - headerPos);
                QRegExp rx("(\\d+)");
                rx.indexIn(contentLenHeader);
                QStringList list = rx.capturedTexts();
                contentLen = list[1].toLong();
                qDebug() << "Content-Length=" << contentLen;
                readyRead();
            }
            else
            {
                ProcessData(message);
            }
        }
    }
    // receive body
    else
    {
        receiveBuff += socket->readAll();
        QByteArray body = receiveBuff.mid(receiveBuff.indexOf("\r\n\r\n"));
        qDebug() << body.size() << " " << contentLen;
        if(receiveBuff.size() >= contentLen) {
            qDebug() << "DONE" << receiveBuff.size() << ", " << body.size();
            ProcessData(receiveBuff);
        }
        else {
            qDebug() << "ReceivedBODY=" << receiveBuff.size();

        }
    }
}

void QClientThread::ProcessData(QByteArray receivedData)
{
    QString response = "HTTP/1.1 200 OK\r\nContent-type: text/html; charset=utf-8\r\n\r\n%1";
    //QByteArray receivedData = socket->readAll();
    qDebug() << socketDescriptor;
    qDebug() << "Headers: ";
    qDebug() << "===================================================================";
    //qDebug() << receivedData;
    qDebug() << "===================================================================";

    QString request = QString::fromLocal8Bit(receivedData);
    std::map<QString, QString> items;
    //QString cmd;
    //qDebug() << "params:" << cmd.indexOf("/?");
    if(request.indexOf("/?") == -1)
    {
        request = "";
    }
    else
    {
        QString params = request.section("\r\n", 0, 0);
        int pos = params.lastIndexOf(' ');
        if(pos != -1)
        {
            params = params.mid(0, pos);
            //qDebug() << request;
        }

        QUrlQuery query(params.section("/?", 1));
        for(int i = 0; i < query.queryItems().size(); ++i)
        {
            QPair<QString, QString> p = query.queryItems()[i];
            items[p.first] = p.second;
            //qDebug() << p.first << " " << p.second;
        }
    }

    //qDebug() << "INDEX=" <<
    /*QString body = receivedData.mid(receivedData.indexOf("\r\n\r\n"));
    QString fname = receivedData.mid(receivedData.indexOf("filename"));

    QStringList arr = body.split("\r\n\r\n");
    qDebug() << "================";
    for(int i = 0; i < arr.size(); ++i)
    {
        qDebug() << "================";
        qDebug() << arr[i];
        qDebug() << "================";
    }
    qDebug() << "================";
    qDebug() << "SIZE :" << receivedData.size();
    qDebug() << "BODY :" << body ;
    qDebug() << "FNAME:" << fname;*/

    //qDebug() << "action=" << items["action"];
    QString responseData = "";

    // Команда вывода списка предметов или тем или шпаргалок
    if(items["action"] == "list")
    {
        qDebug() << "action=" << "LIST";
        if(items["subject"] == "")
        {
            responseData = ListSubjects();
        }
        else
        {
            // Получаем название предмета (совпадает с именем папки)
            QString subject = QUrl::fromPercentEncoding( items["subject"].toUtf8() );
            // Если тема не задана - выводим список тем выбранного предмета
            if(items["theme"] == "")
            {
                // Получаем список тем (список подпапок)
                responseData = ListThemes( subject );
            }
            // Если и предмет и тема заданы - выводим список шпаргалок
            else
            {
                QString theme = QUrl::fromPercentEncoding( items["theme"].toUtf8() );
                // Если шпора не выбрана
                if(items["shpora"] == "")
                {
                    // Получаем список шпаргалок по выбранной теме
                    responseData = ListShpores( subject, theme );
                    //qDebug() << "Subj=" << subject << ", theme=" << theme;
                }
                // Если шпора выбрана - читаем и отправляем файл клиенту
                else
                {
                    QString shpora = QUrl::fromPercentEncoding( items["shpora"].toUtf8() );

                    QByteArray blob = GetShpora( subject, theme, shpora );
                    response = "HTTP/1.1 200 OK\r\nContent-type: text/html; charset=utf-8\r\nContent-Type: image/png\r\nContent-Length: %1\r\n\r\n";
                    QByteArray raw = response.arg(blob.size()).toUtf8();
                    raw.insert(raw.size(), blob);

                    socket->write(raw);
                    socket->disconnectFromHost();
                    return;
                }
            }

        }
    }
    else if(items["action"] == "add")
    {
        // Получаем предмет
        QString subject = QUrl::fromPercentEncoding( items["subject"].toUtf8() ).replace(':', ' ');
        // Получаем тему
        QString theme = QUrl::fromPercentEncoding( items["theme"].toUtf8() ).replace(':', ' ');;
        // Получаем шпору
        QString shpora = QUrl::fromPercentEncoding( items["shpora"].toUtf8() );

        QString subjectFolder = QDir(storageFolder).filePath( subject );
        QString themeFolder = QDir( subjectFolder ).filePath( theme );


        qDebug() << "action=" << "ADD";
        // Если предмет не задан
        if(items["subject"] == "")
        {
            responseData = "-1";
        }
        // Если предмет задан
        else
        {
            qDebug() << "Subject:" << subject;
            qDebug() << "Theme  :" << items["theme"];

            // Если папка с выбранным предметом не создана - создаем
            if(!QDir(subjectFolder).exists())
            {
                qDebug() << "Create subject folder:" << subjectFolder;
                QDir().mkdir(subjectFolder);
                responseData = ListSubjects();
            }
            // Если тема задана
            if(items["theme"] != "")
            {
                if(!QDir(themeFolder).exists())
                {
                    qDebug() << "Create theme folder:" << themeFolder;
                    QDir().mkdir(themeFolder);
                    responseData = ListThemes( subject );
                }
            }
            // Шпора задана
            if(items["shpora"] != "")
            {
                QByteArray header = receivedData.mid(0, receivedData.indexOf("\r\n\r\n"));
                QByteArray body = receivedData.mid(receivedData.indexOf("\r\n\r\n"));
                int boundaryPos = header.indexOf("boundary=");
                QByteArray boundary = header.mid(boundaryPos);
                boundary = boundary.mid(9, boundary.indexOf("\r\n") - 9);//, header.indexOf("\r\n", boundaryPos));
                qDebug() << "HEADER" << header;
                qDebug() << "Boundary=" << boundary;
                //qDebug() << body.indexOf(boundary);
                int startPos = body.indexOf("filename=");
                int endPos = body.indexOf("\r\n", startPos);
                qDebug() << startPos << ", " << endPos;
                QString fname = body.mid(startPos, endPos - startPos);
                fname = fname.mid(9);
                qDebug() << "Filename:" << fname;

                int end = body.indexOf("\r\n--" + boundary + "--\r\n");
                int start = body.lastIndexOf("\r\n--" + boundary + "\r\n", end - 1);
                qDebug() << "Start=" << start << ", end=" << end;
                start = body.indexOf("\r\n\r\n", start) + 4;
                QByteArray blob = body.mid(start, end - start);
                //blob = blob.mid(blob.indexOf("\r\n" + boundary + "\r\n"));
                qDebug() << "PART LAST=" << blob.size();//, pos);
                /*QList<QByteArray> list = body.split(boundary);
                for(int i = 0; i < list.size(); ++i)
                    qDebug() << "PART#1=" << list[i];*/
                //qDebug() << "BODY" << body;
                //qDebug() << "Uploadfile=" << shpora << ", path=" << shporaFile;
                //qDebug() << "SIZE=" << receivedData.size();
                //socket->read(1)
                //QByteArray blob = socket->read(20815);
                //qDebug() << "Received:" << blob.size() << " bytes";


                shpora = fname.remove(0, 1);
                shpora = shpora .remove(fname.size() - 1, 1);
                QString shporaFile = QDir(themeFolder).filePath(shpora);

                qDebug() << "Shpora path:" << shporaFile;
                // Если шпора уже есть в папке
                if(QFile(shporaFile).exists())
                {
                    qDebug() << "Shpora already exist" << shporaFile;
                    responseData = "-1";
                }
                // Иначе пробуем сохранить
                else
                {
                    QFile file(shporaFile);
                    if (!file.open(QIODevice::WriteOnly)) {
                        responseData = "-1";
                    }
                    file.write(blob);

                    responseData = ListShpores(subject, theme);
                }
            }

        }

    }
    else
    {
        qDebug() << "action=" << "DEFAULT LIST";
        responseData = ListSubjects();
    }
    // Отвечаем клиенту

    socket->write(response.arg(responseData).toUtf8());
    // Закрываем соединение
    socket->disconnectFromHost();
    // Очищаем буфер
    receiveBuff.clear();
    // Маркер получения заголовка сбрасываем
    headerReceived = false;
}
// Получить файл стилей
QByteArray QClientThread::GetCSS()
{
    return 0;
}

// Метод получения шпаргалки
QByteArray QClientThread::GetShpora(QString subject, QString theme, QString shpora)
{
    // Путь к каталогу содержащему выбранный предмет
    QString subjectFolder = QDir(storageFolder).filePath(subject);
    // Путь к каталогу содержащему тему
    QString themeFolder = QDir(subjectFolder).filePath(theme);
    // Путь к файлу шпаргалке
    QString shporaFile = QDir(themeFolder).filePath(shpora);
    // Проверяем существует ли каталог с выбранным предметом
    if(!QDir(subjectFolder).exists())
    {
        qDebug() << "Subject: [" << subjectFolder << ", " << subject << "] not found";
        return 0;
    }
    // Проверяем существует ли каталог с выбранной темой
    if(!QDir(themeFolder).exists())
    {
        qDebug() << "Theme: [" << themeFolder << ", " << theme << "] not found";
        return 0;
    }
    // И напоследок проверим существует ли выбранный файл в системе
    if(!QFile(shporaFile).exists())
    {
        qDebug() << "Shpora: [" << shporaFile << ", " << shpora << "] not found";
        return 0;
    }
    // Открываем файл шпаргалку
    QFile file(shporaFile);
    // Проверяем чтобы было открытие
    if (!file.open(QIODevice::ReadOnly)) return 0;
    // Читаем все содержимое и возвращаем результат
    return file.readAll();
}
// Метод получения списка шпаргалок
QString QClientThread::ListShpores(QString subject, QString theme)
{
    // Отладочная запись в консоль
    qDebug() << "[LIST_SHPORA], Subject=" << subject << ", Theme=" << theme;
    // Собираем путь к каталогу содержащему выбранный предмет
    QString subjectFolder = QDir(storageFolder).filePath(subject);
    // Путь к каталогу содержащему тему
    QString themeFolder = QDir(subjectFolder).filePath(theme);

    if(!QDir(subjectFolder).exists())
    {
        qDebug() << "Subject: [" << subjectFolder << ", " << subject << "] not found";
        return "-1";
    }

    if(!QDir(themeFolder).exists())
    {
        qDebug() << "Theme: [" << themeFolder << ", " << theme << "] not found";
        return "-1";
    }
    // Список для шпаргалок
    QStringList filelist;
    // Файловый итератор
    QDirIterator files(themeFolder, QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    // Перечисляем все файлы в указанной папке
    while(files.hasNext())
    {
        // Двигаем к след.
        files.next();
        // Заносим в список файл
        filelist << files.fileName();
    }
    // Собираем строку-ответ сервера
    QString responseData;
    // Помещаем все в блок
    responseData.append("<div id='outer' style='width: 100%'><div id='inner' style='display: table; margin: 0 auto'><div style='display: block; border: solid 1px #ccc'>");
    responseData.append("<ul style='list-style: none; margin: 0; padding: 0; text-align:center;' >");
    // Заносим в блок ссылки с найденными файлами шпарлагками
    for(int i = 0; i < filelist.size(); ++i) {

        responseData.append("<li><a style='display: block; padding: 5px; text-decoration: none; color: #666; border: 1px solid #ccc; background-color: #f0f0f0; border-bottom: none;'");
        responseData.append("onmouseover=\"this.style.color='#ffe'; this.style.backgroundColor='#5488af'\"");
        responseData.append("onmouseout=\"this.style.color='#666'; this.style.backgroundColor='#f0f0f0'\"");
        responseData.append("href='/?action=list&subject=" + QUrl::toPercentEncoding( subject )
                            + "&theme=" + QUrl::toPercentEncoding( theme )
                            + "&shpora=" + QUrl::toPercentEncoding( filelist[i] )
                            + "'>" + filelist[i] + "</a></li>");

    responseData.append("<img src='/?action=list&subject=" + QUrl::toPercentEncoding( subject )
                        + "&theme=" + QUrl::toPercentEncoding( theme )
                        + "&shpora=" + QUrl::toPercentEncoding( filelist[i] )
                        + "'/></br>");
    }
    // Добавим кнопку загрузки шпаргалки
    responseData.append("</br>");
    responseData.append("<form name='addform' action='/?action=add&subject=" + QUrl::toPercentEncoding( subject.toUtf8() )
                        + "&theme=" + QUrl::toPercentEncoding( theme.toUtf8() )
                        + "&shpora=-1"
                        + "' enctype='multipart/form-data' method='post'>");
    responseData.append("<input type='hidden' name='action' value='add'>");
    responseData.append("<input type='hidden' name='subject' value='" + QUrl::toPercentEncoding( subject.toUtf8() ) + "'>");
    responseData.append("<input type='hidden' name='theme' value='" + QUrl::toPercentEncoding( theme.toUtf8() ) + "'>");
    responseData.append("<input type='file' name='shpora' >");
    responseData.append("<input type='submit' value='Добавить шпору'>");
    responseData.append("</form>");
    responseData.append("<a href='/?action=list&subject="
                        + QUrl::toPercentEncoding( subject.toUtf8() ) + "' style='display: block; border:solid 1px #ccc; text-decoration: none; '>[Назад к списку тем]</a></br>");
    responseData.append("</div>");
    // Возвращаем построенный ответ
    return responseData;
}
// Получить список тем
QString QClientThread::ListThemes(QString subject)
{
    qDebug() << "[LIST_THEME], Subject:" << subject;
    // Собираем путь к каталогу содержащему выбранный предмет
    QString subjectFolder = QDir(storageFolder).filePath(subject);

    if(!QDir(subjectFolder).exists())
    {
        qDebug() << "Subject: [" << subjectFolder << ", " << subject << "] not found";
        return "-1";
    }
    // Список каталогов
    QStringList folders;
    // Итератор директорий
    QDirIterator directories(subjectFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    // Перечисляем все директории темы для выбранного предмета
    while(directories.hasNext())
    {
        // Двигаемся дальше
        directories.next();
        // Заносим в список
        folders << directories.fileName();
    }
    // Построим строку-ответ сервера
    QString responseData;
    responseData.append("<div id='outer' style='width: 100%'><div id='inner' style='display: table; margin: 0 auto'><div style='display: block; border: solid 1px #ccc'>");
    responseData.append("<ul style='list-style: none; margin: 0; padding: 0; text-align:center;' >");

    for(int i = 0; i < folders.size(); ++i) {
        responseData.append("<li><a style='display: block; padding: 5px; text-decoration: none; color: #666; border: 1px solid #ccc; background-color: #f0f0f0; border-bottom: none;'");
        responseData.append("onmouseover=\"this.style.color='#ffe'; this.style.backgroundColor='#5488af'\"");
        responseData.append("onmouseout=\"this.style.color='#666'; this.style.backgroundColor='#f0f0f0'\"");
        responseData.append("href='/?action=list&subject=" + QUrl::toPercentEncoding( subject )
                            + "&theme=" + QUrl::toPercentEncoding( folders[i] ) + "'>" + folders[i] + "</a></li>");
    }

    responseData.append("</br>");
    responseData.append("<form name='addform' action='/?' method='get'>");
    responseData.append("<input type='hidden' name='action' value='add'>");
    responseData.append("<input type='hidden' name='subject' value='" + QUrl::toPercentEncoding( subject.toUtf8() ) + "'>");
    responseData.append("<input type='text' name='theme'>");
    responseData.append("<input type='submit' value='Добавить тему'>");
    responseData.append("</form>");
    responseData.append("<a href='/?action=list' style='display: block; border:solid 1px #ccc; text-decoration: none;'>[Назад к списку предметов]</a></br>");
    responseData.append("</div>");

    return responseData;
}
// Метод получения списка предметов
QString QClientThread::ListSubjects()
{

    QStringList folders;
    QDirIterator directories(storageFolder, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);//, QDirIterator::Subdirectories);

    while(directories.hasNext())
    {
        directories.next();
        folders << directories.fileName();// filePath();
    }
    // Строка ответ сервера
    QString responseData;
    responseData.append("<div id='outer' style='width: 100%'><div id='inner' style='display: table; margin: 0 auto'><div style='display: block; border: solid 1px #ccc'>");
    responseData.append("<ul style='list-style: none; margin: 0; padding: 0; text-align:center;' >");

    for(int i = 0; i < folders.size(); ++i) {
        responseData.append("<li><a style='display: block; padding: 5px; text-decoration: none; color: #666; border: 1px solid #ccc; background-color: #f0f0f0; border-bottom: none;'");
        responseData.append("onmouseover=\"this.style.color='#ffe'; this.style.backgroundColor='#5488af'\"");
        responseData.append("onmouseout=\"this.style.color='#666'; this.style.backgroundColor='#f0f0f0'\"");
        responseData.append("href='/?action=list&subject=" + QUrl::toPercentEncoding( folders[i] ) + "'>" + folders[i] + "</a></li>");
    }

    responseData.append("</ul>");

    responseData.append("</div></br>");
    responseData.append("<form name='addform' action='/?' method='get' onsubmit='replacespace();'>");
    responseData.append("<input type='hidden' name='action' value='add'>");
    responseData.append("<input id='p' type='text' name='subject'>");
    responseData.append("<input type='submit' value='Добавить предмет'>");
    responseData.append("</form>");
    responseData.append("<script>function replacespace() { var p = document.getElementById('p'); p.value = p.value.replace(/\\s+/g, ':');}</script>");
    responseData.append("</div></div>");

    return responseData;
}

void QClientThread::disconnect()
{
    qDebug() << socketDescriptor << " Disconnected";

    socket->deleteLater();
    exit(0);
}
