#include "server.h"
#include "./ui_server.h"
#include "srlibrary.h"
#include <QFileDialog>
#include <iostream>
#include <sys/types.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
server::server(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::server)
{
    this->ui->setupUi(this);

    connect(this->ui->pushButton, SIGNAL(clicked()), this, SLOT(startServerThread()));
    this->ui->textEdit->setReadOnly(true);  // Установка режима только для чтения
    this->ui->ipLine->setText("0.0.0.0");
    this->ui->portLine->setText("42100");
    this->ui->localIpLine->setReadOnly(true);
    this->ui->localIpLine->setText(this->getLocalIP());
}

server::~server()
{
    delete ui;
}

void server::createServer(){
    qDebug() << "поток создан";
    std::cout<<pthread_self() << std::endl;
    // Создание сокета
    this->listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        this->ui->label->setText("Failed to create socket");
        return;
    }

    // Настройка адреса и порта
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(this->ui->ipLine->text().toUtf8().constData());
    //serverAddress.sin_addr.s_addr = INADDR_ANY; // Принимать подключения со всех IP-адресов
    serverAddress.sin_port = htons(this->ui->portLine->text().toUShort()); // Порт для прослушивания

    // Привязка сокета к адресу и порту
    if (bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        this->ui->label->setText("Failed to bind socket");
        ::close(listenSocket);
        return;
    }

    // Прослушивание входящих подключений
    if (listen(listenSocket, 5) == -1) { // Максимальная длина очереди ожидающих подключений - 5
        this->ui->label->setText("Failed to listen on socket");
        ::close(listenSocket);
        return;
    }

    // Получение порта и IP-адреса сервера
    struct sockaddr_in boundAddress;
    socklen_t boundAddressLength = sizeof(boundAddress);
    if (getsockname(listenSocket, (struct sockaddr*)&boundAddress, &boundAddressLength) == -1) {
        this->ui->label->setText("Failed to get socket information");
        ::close(listenSocket);
        return;
    }

    // Преобразование порта и IP-адреса в строковый формат
    char ipAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(boundAddress.sin_addr), ipAddress, INET_ADDRSTRLEN);
    int port = ntohs(boundAddress.sin_port);

    // Вывод полученных данных
    QString serverInfo = "Server IP: " + QString(ipAddress) + ", Port: " + QString::number(port);
    this->ui->serverInfoLabel->setText(serverInfo);

    this->ui->label->setText("Listening for incoming connections...");

    while (this->terminatedFlag == 0) {
        qDebug() << "cycle 1";
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);

        // Принятие входящего подключения
        int clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            if (terminatedFlag ==1){
                continue;
            }
            this->ui->label->setText("Failed to accept connection");
            continue;
        }

        char buffer[256] = {0};

        // Успешное подключение
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddress.sin_port);

        QString message = "Accepted connection from " + QString(clientIP) + ", Port: " + QString::number(clientPort);
        this->ui->label->setText(message);


            qDebug() << "cycle 2";
            struct request req;
            int bytesRead = readRequest(&clientSocket, &req);
            if (bytesRead<=0){

                break;
            }
            char* sendedUrl;
            if (req.typeOfRequest==0){
                sendedUrl = (char*)malloc(sizeof(char)*req.len);
                read(clientSocket, sendedUrl, req.len);
            }

            if (req.typeOfRequest == -1){ // корневoй запрос

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрошен список каталогов ./");
                });
                char *buf = (char*)calloc(1024,sizeof(char));
                strncpy(buf,baseUrl,sizeof(baseUrl));
                std::vector<myDir> contents = ::getDirectoryContents(buf);
                free (buf);
                for (myDir item : contents){
                    send(clientSocket,&item,sizeof(struct myDir),0);
                }

                //char zagl[2] = {0};
                //sendDirectoryContent(this->baseUrl, zagl, &clientSocket, 0);


                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: отправлен список каталогов ./");
                });
            }
            else if (req.typeOfRequest==0){

                QMetaObject::invokeMethod(this, [this, &sendedUrl]() {
                    this->ui->textEdit->append("Server: запрошен список каталогов ./" + QString(sendedUrl));
                });

                sendDirectoryContent(baseUrl,sendedUrl,&clientSocket,0);

                QMetaObject::invokeMethod(this, [this, &sendedUrl]() {
                    this->ui->textEdit->append("Server: отправлен список каталогов ./" + QString(sendedUrl));
                });
            }
            else if (req.typeOfRequest == 2){ // переименовать
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запроc на переименование файла");
                });

                renameDirectory(&clientSocket, &req, this->baseUrl);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: переименование файла завершено");
                });
            }
            else if (req.typeOfRequest == 3){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запроc на удаление файла");
                });

                deleteDirectory(&clientSocket, &req, this->baseUrl);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: удаление файла завершено");
                });
            }
            else if (req.typeOfRequest == 4){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрос на загрузку файла");
                });

                sendFileToClient(&clientSocket, &req, this->baseUrl);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: файл успешно отправлен");
                });
            }
            else if (req.typeOfRequest == 5){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрос на сохранение файла");
                });

                readFileFromClient(&clientSocket, &req, this->baseUrl);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: файл успешно принят и сохранен");
                });
            }
            else if (req.typeOfRequest == 6){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрос на создание папки");
                });

                int res = createFolder(&clientSocket, &req, this->baseUrl);

                QMetaObject::invokeMethod(this, [this,res]() {
                    if (res==-1){
                        this->ui->textEdit->append("Server: ошибка создания папки");
                    }
                    else {
                        this->ui->textEdit->append("Server: папка успешно создана");
                    }

                });
            }
            else if (req.typeOfRequest == 7){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрос на отправку папки");
                });

                sendFolder(&clientSocket, &req, this->baseUrl,this->key);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: папка успешно отправлена");
                });
            }
            else if (req.typeOfRequest == 8){
                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: запрос на сохранение папки");
                });

                getFolder(&clientSocket, &req, this->baseUrl,this->key);

                QMetaObject::invokeMethod(this, [this]() {
                    this->ui->textEdit->append("Server: папка успешно сохранена");
                });
            }
            // Закрытие клиентского сокета
            ::close(clientSocket);



    }
    qDebug() << "поток закончился";
    // Закрытие прослушивающего сокета
    ::close(listenSocket);
}
void server::startServerThread() {
    if (this->createFlag == 1){
        return;
    }
    this->createFlag = 1;
    this->terminatedFlag = 0;
    qDebug() << "clicked on";
    std::thread serverThread(&server::createServer, this);
    serverThread.detach();
}

void server::selectDirectory(){
    if (createFlag==1)
        return;
    QString folderPath = QFileDialog::getExistingDirectory(this, "Выберите папку", "/home", QFileDialog::ShowDirsOnly);
    folderPath = folderPath + QString("/");
    if (!folderPath.isEmpty()) {
        std::string str = folderPath.toStdString();
        strncpy(baseUrl, str.c_str(),1024);
        this->ui->portLine_2->setText(baseUrl);
    }
}

void server::abortServer(){
    this->terminatedFlag = 1;
    shutdown(this->listenSocket, SHUT_RDWR);
    this->createFlag = 0;
    this->ui->label->setText("Server aborted");
    this->ui->serverInfoLabel->setText("") ;
    this->ui->textEdit->clear();
    this->createFlag = 0;
}

QString server::getLocalIP(){
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;

    if (getifaddrs(&ifAddrStruct) == -1)
    {
        std::cerr << "Error getting interface addresses" << std::endl;
        return "";
    }

    char host[NI_MAXHOST];

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr)
        {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags & IFF_RUNNING && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0)
            {
                freeifaddrs(ifAddrStruct);
                return host;
            }
        }
    }

    freeifaddrs(ifAddrStruct);
    return "";
}
