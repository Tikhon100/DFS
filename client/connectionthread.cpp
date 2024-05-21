#include "connectionthread.h"

ConnectionThread::ConnectionThread() {}


void ConnectionThread::run(client* thisClient )
{
    char folderPath[1024] = "/home/tikhon/Документы";
    char buffer[1024];
    // Создание сокета
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        thisClient->ui->label->setText("Failed to create socket" );
        return;
    }

    // Установка адреса сервера
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(this->ui->ipLine->text().toUtf8().constData()); // Здесь указывается IP-адрес сервера
    serverAddress.sin_port = htons(this->ui->portLine->text().toUShort()); // Порт сервера

    while(!terminatedFlag){
        if (sendPathFolderFlag){
            // Установка соединения с сервером
            if (::connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
                this->ui->label->setText("Failed to connect to server");
                ::close(clientSocket);
                return;
            }

            send(clientSocket, folderPath, strlen(folderPath), 0);
            QMetaObject::invokeMethod(this, [this, &folderPath]() {
                this->ui->textEdit->append(QString("Client: folder path sent to server") + QString(folderPath));
            });
            sendPathFolderFlag=0;
        }
    }

    // Закрытие клиентского сокета
    ::close(clientSocket);
    this->connectionFlag = 0;



    while (true)
    {

        connect(this, &ConnectionThread::sendRequest, this, &ConnectionThread::sendRequestToServer);
    }
}

void ConnectionThread::sendRequestToServer(){

}
