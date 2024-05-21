#include "client.h"
#include "./ui_client.h"
#include <thread>
#include "mydir.h"
#include "srlibrary.h"
#include <QShortcut>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QMimeType>
#include <iostream>
#include <QThread>
#include <sys/stat.h>
client::client(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::client)
{
    std::cout << "HOLA MAIN MAIN TOCHNO " << std::this_thread::get_id() << std::endl;
    this->ui->setupUi(this);

    this->connectionFlag=0;
    this->model = new QFileSystemModel;
    model->setRootPath(QDir::rootPath());
    // Скрываем ненужные колонки

    this->ui->localStorageList->setModel(this->model);
    this->ui->localStorageList->setRootIndex(this->model->index(QDir::rootPath()));
    for (int i = 1; i < 4; ++i){
        this->ui->localStorageList->hideColumn(i);
    }

    this->ui->textEdit->setReadOnly(true);  // Установка режима только для чтения

    this->ui->ipLine->setText("127.0.0.1");
    this->ui->portLine->setText("4100");

    treeModel = new QStandardItemModel(this);
    this->ui->networkStorageList->setModel(treeModel);

    connect(ui->networkStorageList, &QTreeView::clicked, this, &client::treeItemClicked);
    ui->networkStorageList->setEditTriggers(QTreeView::NoEditTriggers);

    QShortcut *shortcutF3 = new QShortcut(QKeySequence(Qt::Key_F3), this);
    QShortcut *shortcutF4 = new QShortcut(QKeySequence(Qt::Key_F4), this);
    QShortcut *shortcutF5 = new QShortcut(QKeySequence(Qt::Key_F5), this);
    QShortcut *shortcutF6 = new QShortcut(QKeySequence(Qt::Key_F6), this);

    connect(shortcutF3, &QShortcut::activated, this, &client::uploadButtonClicked);
    connect(shortcutF4, &QShortcut::activated, this, &client::downloadButtonClicked);
    connect(shortcutF5, &QShortcut::activated, this, &client::renameButtonClicked);
    connect(shortcutF6, &QShortcut::activated, this, &client::deleteButtonClicked);
    this->lastButtonClicked = 1;
}

client::~client()
{
    delete ui;
}

void client::connectToServer(){
    if (this->lastButtonClicked==1){
        if (connectionFlag==0){
            this->terminatedFlag=0;
            thread=std::thread(&client::createClient, this);

            connectionFlag=1;
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = inet_addr(this->ui->ipLine->text().toUtf8().constData());
            serverAddress.sin_port = htons(this->ui->portLine->text().toUShort());
        }
        else {
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = inet_addr(this->ui->ipLine->text().toUtf8().constData());
            serverAddress.sin_port = htons(this->ui->portLine->text().toUShort());
            this->ui->label->setText("");
            emit sendPathSignal();
        }
        lastButtonClicked = 0;
    }
}

void client::createClient()
{
    qDebug() << "поток создан";
    // Подключаем сигналы к слотам с помощью Qt::QueuedConnection
    connect(this, &client::sendPathSignal, this, &client::sendPathToServerSLOT, Qt::QueuedConnection);
    connect(this, &client::loadDirectory, this, &client::loadDirectoryFromServerSLOT, Qt::QueuedConnection);
    connect(this, &client::renameOperation, this, &client::renameFileSLOT, Qt::QueuedConnection);
    connect(this, &client::deleteOperation, this, &client::deleteFileSLOT, Qt::QueuedConnection);
    connect(this, &client::uploadFromServerToLocalStorageOperation, this, &client::uploadFromServerToLocalStorageSLOT, Qt::QueuedConnection);
    connect(this, &client::uploadFromLocalToServerOperation, this, &client::uploadFromLocalStorageToServerSLOT, Qt::QueuedConnection);
    connect(this, &client::createFolderOnServer, this, &client::createFolderOnServerOperationSLOT, Qt::QueuedConnection);
    connect(this, &client::downloadFolderSignal, this, &client::downloadFolderSignalSlot, Qt::QueuedConnection);
    connect (this, &client::uploadFolderSignal, this, &client::uploadFolderSignalSlot,Qt::QueuedConnection);

    sendPathToServer();
    while (!terminatedFlag) {
        std::unique_lock<std::mutex> lock(mutex);
        // Ожидаем изменения значений флагов
        cv.wait_for(lock, std::chrono::milliseconds(100), [this]{return this->flag1.load() || this->flag2.load() ||
                                                                        this->flag3.load() || this->flag4.load() ||
                                                                        this->flag5.load() || this->flag6.load() ||
                                                                        this->flag7.load();});

        // Проверяем значения флагов и выполняем соответствующие действия
        if (this->flag1.load() == 1){
            sendPathToServer();
            this->flag1.store(false);
        }
        else if (this->flag2.load()==1){
            loadDirectoryFromServer(this->buffer1, this->bufferItem);
            this->flag2.store(false);
        }
        else if (this->flag3.load()==1){
            renameFile(this->buffer1, this->buffer2);
            this->flag3.store(false);
        }
        else if (this->flag4.load()==1){
            deleteFile(this->buffer1);
            this->flag4.store(false);
        }
        else if (this->flag5.load()==1){
            uploadFromServerToLocalStorage(this->buffer1, this->buffer2);
            this->flag5.store(false);
        }
        else if (this->flag6.load()==1){
            uploadFromLocalStorageToServer(this->buffer1, this->buffer2);
            this->flag6.store(false);
        }
        else if (this->flag7.load()==1){
            createFolderOnServerOperation(buffer1, buffer2);
            this->flag7.store(false);
        }
        else if (this->flag8.load()==1){
            downloadFolderOperation(buffer1, buffer2);
            this->flag8.store(false);
        }
        else if (this->flag9.load() == 1){
            uploadFolderOperation(buffer1, buffer2);
            this->flag9.store(false);
        }
    }

    // Закрытие клиентского сокета
    ::close(this->clientSocket);
    connectionFlag = 0;
    qDebug() << "поток закончился";
}

void client::sendPathToServerSLOT(){
    this->flag1.store(true);
    cv.notify_one();
}

void client::sendPathToServer()
{
    std::cout << "HOLA " << std::this_thread::get_id() << std::endl;
    struct request req;
    req.typeOfRequest = -1;
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        this->lastButtonClicked=1;

        this->terminatedFlag=0;
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        this->lastButtonClicked=1;

        this->terminatedFlag=0;
        return;
    }

    QMetaObject::invokeMethod(this, [this]() {
        this->ui->textEdit->append(QString("Client: извлечение списка каталогов ") + QString(folderPath));
    });

    std::vector<myDir> content = readDirectoryContent(&clientSocket);

    QMetaObject::invokeMethod(this, [this, content]{
        populateTreeView(content,nullptr);
    });


    QMetaObject::invokeMethod(this, [this]() {
        this->ui->textEdit->append(QString("Client: список каталогов извлечен ") + QString(folderPath));
    });

    ::close (clientSocket);

}

void client::loadDirectoryFromServerSLOT(QString str, CustomItem* item){
    this->buffer1 = QString(str);
    this->bufferItem = item;
    this->flag2 = true;
    cv.notify_one();
}

void client::loadDirectoryFromServer(QString str , CustomItem* item){std::cout << "HOLA " << std::this_thread::get_id() << std::endl;
    // создание строки типа char* из str
    QByteArray byteArray = str.toUtf8();
    const char* charArray = byteArray.constData();
    char result[255];
    qstrncpy(result, charArray, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';

    // отправляем запрос на сервер
    struct request req;
    req.typeOfRequest = 0;
    req.len = strlen(result)+1;
    int res = sendRequest(&clientSocket, &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
    }
    // отправка пути на сервер
    send(clientSocket, result, strlen(result)+1, 0);
    QMetaObject::invokeMethod(this, [this, result]() {
        this->ui->textEdit->append(QString("Client: извлечение списка каталогов ") + QString(result));
    });

    // прием данных директории
    std::vector<myDir> content = readDirectoryContent(&clientSocket);
    QMetaObject::invokeMethod(this, [this, content, item]() {
        populateTreeView(content, item);
    });

    QMetaObject::invokeMethod(this, [this, result]() {
        this->ui->textEdit->append(QString("Client: список каталогов извлечен ") + QString(result));
    });

    ::close (clientSocket);
}

void client::deleteFileSLOT(QString fullPath){
    this->buffer1 = fullPath;
    this->flag4.store(true);
    cv.notify_one();
}

void client::deleteFile(QString fullPath){
    QByteArray byteArray = fullPath.toUtf8();
    const char* charArray = byteArray.constData();
    char result[255];
    qstrncpy(result, charArray, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';


    // отправка запроса на сервер
    struct request req;
    req.typeOfRequest = 3; // удаление файла
    req.len = strlen(result);
    req.len2 = 0;
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }
    send(clientSocket, result, req.len, 0);
    QMetaObject::invokeMethod(this, [this, result]() {
        this->ui->textEdit->append(QString("Client: удаление файла ") + QString(result));
    });

    ::close(clientSocket);
    QMetaObject::invokeMethod(this, [this]{
        QModelIndex currentIndex = this->ui->networkStorageList->currentIndex();
        QModelIndex parentIndex = currentIndex.parent(); // Получаем индекс родительского элемента
        if (parentIndex.isValid()) {
            CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(parentIndex));
            parentItem->setIsLoaded(0);

            // Получаем индекс родительского элемента
            QModelIndex parentIndex = treeModel->indexFromItem(parentItem);
            // Удаляем все дочерние элементы и их вложенные дочерние элементы
            int rowCount = treeModel->rowCount(parentIndex);
            for (int row = rowCount - 1; row >= 0; --row) {
                treeModel->removeRow(row, parentIndex);
            }

        }
        else {
            treeModel->clear();
            delete treeModel;
            treeModel = new QStandardItemModel(this);
            this->ui->networkStorageList->setModel(treeModel);
            emit sendPathSignal();
        }
    });

}

void client::renameFileSLOT(QString fullPath, QString newName){
    this->buffer1 = fullPath;
    this->buffer2 = newName;
    this->flag3.store(true);
    cv.notify_one();
}

void client::renameFile(QString fullPath, QString newName){
    // создание строки типа char* из fullPath
    QByteArray byteArray = fullPath.toUtf8();
    const char* charArray = byteArray.constData();
    char result[255];
    qstrncpy(result, charArray, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';

    // создание строки типа char* из newName
    QByteArray byteArray2 = newName.toUtf8();
    const char* charArray2 = byteArray2.constData();
    char result2[255];
    qstrncpy(result2, charArray2, sizeof(result2) - 1);
    result2[sizeof(result2) - 1] = '\0';

    // отправка запроса на сервер
    struct request req;
    req.typeOfRequest = 2;
    req.len = strlen(result);
    req.len2 = strlen(result2);
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }

    send(clientSocket, result, req.len, 0);
    send(clientSocket, result2, req.len2, 0);
    QMetaObject::invokeMethod(this, [this, result, result2]() {
        this->ui->textEdit->append(QString("Client: переименование файла ") + QString(result) + QString(" в файл ") + QString(result2));
    });

    ::close(clientSocket);

    QMetaObject::invokeMethod(this, [this]{
        QModelIndex currentIndex = this->ui->networkStorageList->currentIndex();
        QModelIndex parentIndex = currentIndex.parent(); // Получаем индекс родительского элемента
        if (parentIndex.isValid()) {
            CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(parentIndex));
            parentItem->setIsLoaded(0);

            // Получаем индекс родительского элемента
            QModelIndex parentIndex = treeModel->indexFromItem(parentItem);
            // Удаляем все дочерние элементы и их вложенные дочерние элементы
            int rowCount = treeModel->rowCount(parentIndex);
            for (int row = rowCount - 1; row >= 0; --row) {
                treeModel->removeRow(row, parentIndex);
            }
        }
        else {
            treeModel->clear();
            delete treeModel;
            treeModel = new QStandardItemModel(this);
            this->ui->networkStorageList->setModel(treeModel);
            emit sendPathSignal();
        }
    });

}

void client::uploadFromServerToLocalStorageSLOT(QString fullPathToFile, QString fullPathToFolder){
    this->buffer1 = fullPathToFile;
    this->buffer2 = fullPathToFolder;
    this->flag5.store(true);
    cv.notify_one();
}

void client::uploadFromServerToLocalStorage(QString fullPathToFile, QString fullPathToFolder){
    // создание строки типа char* из fullPath
    QByteArray byteArray = fullPathToFile.toUtf8();
    const char* charArray = byteArray.constData();
    char result[255];
    qstrncpy(result, charArray, sizeof(result) - 1);
    result[sizeof(result) - 1] = '\0';


    struct request req;
    req.typeOfRequest = 4;
    req.len = strlen(result);
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }

    send(clientSocket, result, req.len, 0);
    int lenOfFile;
    read(clientSocket,&lenOfFile,sizeof(int));


    // Получаем только имя файла из первой строки
    std::filesystem::path filePath(fullPathToFile.toStdString());
    std::string fileName = filePath.filename();

    // Объединяем пути
    std::filesystem::path combinedPath = std::filesystem::path(fullPathToFolder.toStdString()) / fileName;

    // Получаем итоговый путь в виде строки
    std::string finalPath = combinedPath.string();
    const char* forOpenPath = combinedPath.c_str();



    FILE* file = fopen(forOpenPath, "w");
    if (file == NULL) {
        return ;
    }
    char buf;
    for (int i=0;i<lenOfFile;i++){
        int n = read(clientSocket, &buf, sizeof(char));
        if (n<=0){
            break;
        }
        fwrite(&buf,1,sizeof(char),file);
    }
    fclose(file);

    ::close(clientSocket);
}

void client::uploadFromLocalStorageToServerSLOT(QString fullPathToFile, QString fullPathToDirectory){
    this->buffer1 = fullPathToFile;
    this->buffer2 = fullPathToDirectory;
    this->flag6.store(true);
    cv.notify_one();
}

void client::uploadFromLocalStorageToServer(QString fullPathToFile, QString fullPathToDirectory){
    std::string str1buf = fullPathToFile.toStdString();
    const char* pathToFile = str1buf.c_str();

    std::string str2buf = fullPathToDirectory.toStdString();


    // Получаем только имя файла из первой строки
    std::filesystem::path filePath(fullPathToFile.toStdString());
    std::string fileName = filePath.filename();

    // Объединяем пути
    std::filesystem::path combinedPath = std::filesystem::path(fullPathToDirectory.toStdString()) / fileName;

    // Получаем итоговый путь в виде строки
    std::string finalPath = combinedPath.string();
    const char* forOpenPath = combinedPath.c_str();
    int lenOfFile;

    FILE* f = fopen(pathToFile, "r");
    if (f==NULL){
        return;
    }
    fseek(f,0,SEEK_END);
    lenOfFile = ftell(f);
    fseek(f,0,SEEK_SET);

    QMetaObject::invokeMethod(this, [this]() {
        this->ui->textEdit->append("Client: отправка файла на сервер... ");
    });

    struct request req;
    req.typeOfRequest = 5; // загрузить на сервер файл
    req.len = strlen(forOpenPath);
    req.len2 = lenOfFile;
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }
    send(clientSocket,forOpenPath,req.len,0);
    for (int i=0;i<lenOfFile;i++){
        char c = fgetc(f);
        send(clientSocket, &c, sizeof(char),0);
    }

    QMetaObject::invokeMethod(this, [this]() {
        this->ui->textEdit->append("Client: отправка файла завершена ");
    });



    fclose(f);
    ::close (clientSocket);
}

void client::createFolderOnServerOperationSLOT(QString folderName, QString wayToCreate){
    this->buffer1 = folderName;
    this->buffer2 = wayToCreate;
    this->flag7.store(true);
    cv.notify_one();
}

void client::createFolderOnServerOperation(QString folderName, QString wayToCreate){
    std::string folderNameSTD = folderName.toStdString();
    std::string wayToCreateSTD = wayToCreate.toStdString();
    const char* folderNameC = folderNameSTD.c_str();
    const char* wayToCreateC = wayToCreateSTD.c_str();

    struct request req;
    req.typeOfRequest = 6;
    req.len = strlen(folderNameC);
    req.len2 = strlen(wayToCreateC);
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }
    send(clientSocket, folderNameC, req.len, 0);
    send(clientSocket, wayToCreateC, req.len2, 0);

    QMetaObject::invokeMethod(this, [this]{
        std::cout << "Create operation invoke (should be main thread)" << std::this_thread::get_id() << std::endl;
        QModelIndex currentNetworkIndex = this->ui->networkStorageList->currentIndex();
        if (currentNetworkIndex.isValid()){
            CustomItem* networkitem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(currentNetworkIndex));
            if(networkitem->getType() == 1){// файл
                if (!deleteChildOfParentItem(currentNetworkIndex)){//файл в корне
                    treeModel = new QStandardItemModel(this);
                    this->ui->networkStorageList->setModel(treeModel);
                    emit sendPathSignal();
                }
            }
            else {
                deleteChildOfCurrentIndex(currentNetworkIndex); // папка
            }
        }
        else {//ничего не выбрано
            treeModel = new QStandardItemModel(this);
            this->ui->networkStorageList->setModel(treeModel);
            emit sendPathSignal();
        }
    });
}


void client::clearTextBox(){
    this->ui->textEdit->clear();
    this->ui->textEdit->append("История очищена");
}

void client::sendRequestToServer(){
    emit sendPathSignal();
}


void client::populateTreeView(const std::vector<myDir>& dirContents, CustomItem* parentItem)
{

    if (parentItem) {
        int rowCount = parentItem->rowCount();
        for (int i=0;i<rowCount;i++){
            parentItem->removeRow(i);
        }
    }
    if (dirContents.empty()){
        CustomItem* emptyItem = new CustomItem("Empty...", -1, -1);
        if (parentItem) {
            parentItem->appendRow(emptyItem);
            parentItem->setIsLoaded(1);
        } else {
            treeModel->appendRow(emptyItem);
        }
    }
    for (const auto& entry : dirContents) {
        CustomItem* item = new CustomItem(QString::fromUtf8(entry.name), entry.type, entry.isLoaded);

        // Установка иконки в зависимости от типа (файл или директория)
        if (entry.type == 1) { // Файл
            QString fileName = QString::fromUtf8(entry.name);
            QMimeDatabase db;
            QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);
            QIcon fileIcon = QIcon::fromTheme(mime.iconName());
            item->setIcon(fileIcon);
        } else { // Директория
            item->setIcon(QIcon("../../icons/folder.png"));

            // Создаем специальный элемент-заглушку для ленивой загрузки содержимого
            CustomItem* lazyLoadItem = new CustomItem("Not loaded...", -1, -1);
            item->appendRow(lazyLoadItem);
        }

        if (parentItem) {
            parentItem->appendRow(item);
            parentItem->setIsLoaded(1);
        } else {
            treeModel->appendRow(item);
        }
    }
}

void client::treeItemClicked(const QModelIndex &index)
{
    std::cout << "HOLA MAIN THREAD" << std::this_thread::get_id() << std::endl;
    CustomItem* item = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(index));
    if (item) {
        if (item->getIsLoaded()==0){
            QString fullPath = item->text(); // Инициализируем полный путь и добавляем имя текущего элемента

            QModelIndex parentIndex = index.parent(); // Получаем индекс родительского элемента
            while (parentIndex.isValid()) {
                CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(parentIndex));
                if (parentItem) {
                    fullPath = parentItem->text() + "/" + fullPath; // Добавляем имя родительского элемента к полному пути
                    parentIndex = parentIndex.parent(); // Переходим к следующему родительскому элементу
                } else {
                    break; // Если не удалось получить родительский элемент, выходим из цикла
                }
            }

            emit loadDirectory(fullPath,item);
        }

    }
}

void client::deleteButtonClicked(){
    QModelIndex currentIndex = this->ui->networkStorageList->currentIndex();
    if (currentIndex.isValid()){
        QVariant selectedData = currentIndex.data(Qt::DisplayRole);
        this->deleteform = new deleteForm(this, selectedData.toString());
        this->deleteform->exec();
        if (this->deleteform->getDeleteFlag()){
            QString fullPath = getFullPathToClickedItem();
            emit deleteOperation(fullPath);
        }
    }
}

void client::renameButtonClicked()
{
    QModelIndex currentIndex = this->ui->networkStorageList->currentIndex();
    if (currentIndex.isValid()) {
        QVariant selectedData = currentIndex.data(Qt::DisplayRole);
        this->renameform = new renameForm(this, selectedData.toString());
        this->renameform->exec();
        if (this->renameform->getNameEntered()) {
            QString fullPath = getFullPathToClickedItem();
            emit renameOperation(fullPath, this->renameform->getNewName());
        }
    }
}

void client::downloadButtonClicked(){
    QModelIndex currentNetworkIndex = this->ui->networkStorageList->currentIndex();
    QModelIndex currentLocalIndex = this->ui->localStorageList->currentIndex();
    if (checkIfIsFiles(&currentNetworkIndex,&currentLocalIndex)){
        QVariant networkSelectedData = currentNetworkIndex.data(Qt::DisplayRole);
        QVariant localSelectedData = currentLocalIndex.data(Qt::DisplayRole);
        this->downloadform = new downloadForm(this, networkSelectedData.toString(),localSelectedData.toString());
        this->downloadform->exec();
        if (this->downloadform->getUploadFlag() == 1){
            QString fullPathToFile = getFullPathToClickedItem();
            QString fullPathToFolder = getFullPathToClickedLocalItem();
            emit uploadFromServerToLocalStorageOperation(fullPathToFile,fullPathToFolder);
        }
    }

}

void client::uploadButtonClicked(){
    QModelIndex currentNetworkIndex = this->ui->networkStorageList->currentIndex();
    QModelIndex currentLocalIndex = this->ui->localStorageList->currentIndex();
    if (checkIfIsFiles2(&currentNetworkIndex,&currentLocalIndex)){
        QVariant networkSelectedData = currentNetworkIndex.data(Qt::DisplayRole);
        QVariant localSelectedData = currentLocalIndex.data(Qt::DisplayRole);
        this->downloadform = new downloadForm(this,  localSelectedData.toString(), networkSelectedData.toString());
        this->downloadform->exec();
        if (this->downloadform->getUploadFlag() == 1){
            QString fullPathToFolder = getFullPathToClickedItem();
            QString fullPathToFile = getFullPathToClickedLocalItem();
            emit uploadFromLocalToServerOperation(fullPathToFile,fullPathToFolder);

            QModelIndex parentIndex = currentNetworkIndex.parent(); // Получаем индекс родительского элемента
            if (parentIndex.isValid()) {
                CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(parentIndex));
                parentItem->setIsLoaded(0);

                // Получаем индекс родительского элемента
                QModelIndex parentIndex = treeModel->indexFromItem(parentItem);
                // Удаляем все дочерние элементы и их вложенные дочерние элементы
                int rowCount = treeModel->rowCount(parentIndex);
                for (int row = rowCount - 1; row >= 0; --row) {
                    treeModel->removeRow(row, parentIndex);
                }

            }
            else {
                treeModel->clear();
                delete treeModel;
                treeModel = new QStandardItemModel(this);
                this->ui->networkStorageList->setModel(treeModel);
                emit sendPathSignal();
            }
        }
    }

}



bool client::checkIfIsFiles2(QModelIndex* currentNetworkindex, QModelIndex* currentLocalIndex){
    if(currentNetworkindex->isValid() && currentLocalIndex->isValid()){
        CustomItem* networkitem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(*currentNetworkindex));
        if (networkitem->getType() == 0){
            // Получаем модель из QTreeView
            QFileSystemModel *model = qobject_cast<QFileSystemModel*>(this->ui->localStorageList->model());
            // Получаем индекс выделенного элемента
            QModelIndex currentIndex = this->ui->localStorageList->selectionModel()->currentIndex();
            // Проверяем, является ли элемент папкой
            bool isDir = model->isDir(currentIndex);
            if (!isDir){
                return 1;
            }
        }
    }
    return false;
}

bool client::checkIfIsFiles(QModelIndex* currentNetworkindex, QModelIndex* currentLocalIndex){
    if(currentNetworkindex->isValid() && currentLocalIndex->isValid()){
        CustomItem* networkitem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(*currentNetworkindex));
        if (networkitem->getType() == 1){
            // Получаем модель из QTreeView
            QFileSystemModel *model = qobject_cast<QFileSystemModel*>(this->ui->localStorageList->model());
            // Получаем индекс выделенного элемента
            QModelIndex currentIndex = this->ui->localStorageList->selectionModel()->currentIndex();
            // Проверяем, является ли элемент папкой
            bool isDir = model->isDir(currentIndex);
            if (isDir){
                QDir dir(model->filePath(currentIndex));
                QStringList files = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                if (!files.isEmpty()) {
                    return 1;
                }
            }
        }
    }
    return false;
}

QString client::getFullPathToClickedItem(){
    QModelIndex currentIndex = this->ui->networkStorageList->currentIndex(); // Получаем текущий индекс
    QStringList pathList; // Список для хранения пути
    while (currentIndex.isValid()) {
        QString itemText = currentIndex.data(Qt::DisplayRole).toString(); // Получаем текст элемента
        pathList.prepend(itemText); // Добавляем текст элемента в начало списка пути
        currentIndex = currentIndex.parent(); // Переходим к родительскому элементу
    }
    QString path = pathList.join("/"); // Объединяем элементы пути с разделителем
    return path;
}

QString client::getFullPathToClickedLocalItem(){
    QModelIndex currentIndex = this->ui->localStorageList->currentIndex(); // Получаем текущий индекс
    QStringList pathList; // Список для хранения пути
    while (currentIndex.isValid()) {
        QString itemText = currentIndex.data(Qt::DisplayRole).toString(); // Получаем текст элемента
        pathList.prepend(itemText); // Добавляем текст элемента в начало списка пути
        currentIndex = currentIndex.parent(); // Переходим к родительскому элементу
    }
    QString path = pathList.join("/"); // Объединяем элементы пути с разделителем
    return path;
}

void client::disconnect(){
    if (this->lastButtonClicked == 0){
        treeModel = new QStandardItemModel(this);
        this->ui->networkStorageList->setModel(treeModel);
        this->lastButtonClicked = 1;
    }
}

void client::changeRootDir()
{
    QString folderPath = QFileDialog::getExistingDirectory(this, "Выберите папку", "/home", QFileDialog::ShowDirsOnly);
    if (!folderPath.isEmpty()) {
        this->model->setRootPath(QDir::cleanPath(folderPath));
        this->ui->localStorageList->setRootIndex(this->model->index(this->model->rootPath()));
    }
}

QString client::getFullPathToindexOnLocalList(QModelIndex index){
    QStringList pathList; // Список для хранения пути
    while (index.isValid()) {
        QString itemText = index.data(Qt::DisplayRole).toString(); // Получаем текст элемента
        pathList.prepend(itemText); // Добавляем текст элемента в начало списка пути
        index = index.parent(); // Переходим к родительскому элементу
    }
    QString path = pathList.join("/"); // Объединяем элементы пути с разделителем
    return path;
}

void client::createDirectoryOnServerClicked(){
    std::cout << "HOLA " << std::this_thread::get_id() << std::endl;
    QModelIndex currentNetworkIndex = this->ui->networkStorageList->currentIndex();
    if (currentNetworkIndex.isValid()){
        CustomItem* networkitem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(currentNetworkIndex));
        if (networkitem->getType() == 0){ // случай когда выбрана папка
            this->createFolderForm = new CreateFolderForm(this,currentNetworkIndex.data().toString());
            this->createFolderForm->exec();

            if (createFolderForm ->getCreateFlag() == 1){
                emit createFolderOnServer(createFolderForm->getNameOfCreatedDir(),getFullPathToindexOnLocalList(currentNetworkIndex));
                //return ;
            }

        }
        else{ // случай когда выбран файл
            QModelIndex currentParentNetworkIndex = currentNetworkIndex.parent();
            if (currentParentNetworkIndex.isValid()){ // если есть родительская папка
                this->createFolderForm = new CreateFolderForm(this,currentParentNetworkIndex.data().toString());
                this->createFolderForm->exec();

                if (createFolderForm ->getCreateFlag() == 1){

                    emit createFolderOnServer(createFolderForm->getNameOfCreatedDir(),getFullPathToindexOnLocalList(currentParentNetworkIndex));
                    //return ;
                }
            }
            else { // если файл в корне
                this->createFolderForm = new CreateFolderForm(this,"корневая папка");
                this->createFolderForm->exec();

                if (createFolderForm ->getCreateFlag() == 1){

                    emit createFolderOnServer(createFolderForm->getNameOfCreatedDir(), "");
                    //return ;
                }
            }
        }
    }
    else { // случай когда ничего не выбрано - создаем в корне
        this->createFolderForm = new CreateFolderForm(this,"корневая папка");
        this->createFolderForm->exec();
        if (createFolderForm ->getCreateFlag() == 1){
            emit createFolderOnServer(createFolderForm->getNameOfCreatedDir(), "");
            //return ;
        }
    }

}

bool client::deleteChildOfParentItem(QModelIndex currentNetworkIndex){
    QModelIndex parentIndex = currentNetworkIndex.parent(); // Получаем индекс родительского элемента
    if (parentIndex.isValid()) {
        CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(parentIndex));
        parentItem->setIsLoaded(0);

        // Получаем индекс родительского элемента
        QModelIndex parentIndex = treeModel->indexFromItem(parentItem);
        // Удаляем все дочерние элементы и их вложенные дочерние элементы
        int rowCount = treeModel->rowCount(parentIndex);
        for (int row = rowCount - 1; row >= 0; --row) {
            treeModel->removeRow(row, parentIndex);
        }
        return true;
    }
    return false;
}

bool client::deleteChildOfCurrentIndex(QModelIndex currentNetworkIndex){
    if (currentNetworkIndex.isValid()) {
        CustomItem* parentItem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(currentNetworkIndex));
        parentItem->setIsLoaded(0);

        // Получаем индекс родительского элемента
        QModelIndex parentIndex = treeModel->indexFromItem(parentItem);
        // Удаляем все дочерние элементы и их вложенные дочерние элементы
        int rowCount = treeModel->rowCount(parentIndex);
        for (int row = rowCount - 1; row >= 0; --row) {
            treeModel->removeRow(row, parentIndex);
        }
        return true;
    }
    return false;
}

void client::reload(){
    emit disconnect();
    emit connectToServer();
}

void client::createFolderOnLocalDir(){
    // Получаем выбранный индекс в QListView
    QModelIndex selectedIndex = this->ui->localStorageList->currentIndex();

    if (!selectedIndex.isValid()){
        return;
    }
    // Получаем путь к выбранному объекту
    QString path = this->model->fileInfo(selectedIndex).absoluteFilePath();
    this->createFolderForm = new CreateFolderForm(this,selectedIndex.data().toString());
    this->createFolderForm->exec();
    if (createFolderForm ->getCreateFlag() == 1){
        // Создаем новую папку в выбранном пути с именем
        QString newFolderPath = path + "/" + this->createFolderForm->getNameOfCreatedDir();
        QDir().mkdir(newFolderPath);
    }

}

bool client::checkIfIsFolderAndFolder(){
    QModelIndex currentNetworkIndex = this->ui->networkStorageList->currentIndex();
    QModelIndex currentLocalIndex = this->ui->localStorageList->currentIndex();
    if (currentNetworkIndex.isValid() && currentLocalIndex.isValid()){
        CustomItem* networkitem = dynamic_cast<CustomItem*>(treeModel->itemFromIndex(currentNetworkIndex));
        if (networkitem->getType() == 0){
            // Получаем модель из QTreeView
            QFileSystemModel *model = qobject_cast<QFileSystemModel*>(this->ui->localStorageList->model());
            // Получаем индекс выделенного элемента
            QModelIndex currentIndex = this->ui->localStorageList->selectionModel()->currentIndex();
            // Проверяем, является ли элемент папкой
            bool isDir = model->isDir(currentIndex);
            if (isDir){
                return true;
            }
        }
    }
    return false;
}

void client::uploadFolderButtonClicked(){
    if(checkIfIsFolderAndFolder()){
        emit uploadFolderSignal(getFullPathToClickedLocalItem(),getFullPathToClickedItem());
    }
}

void client::downloadFolderButtonClicked(){
    if(checkIfIsFolderAndFolder()){
        emit downloadFolderSignal(getFullPathToClickedLocalItem(),getFullPathToClickedItem());
    }
}

void client::downloadFolderSignalSlot(QString fullPathToFolderLocal, QString fullPathToFolderNetwork){
    this->buffer1 = QString(fullPathToFolderLocal);
    this->buffer2 = QString(fullPathToFolderNetwork);
    this->flag8 = true;
    cv.notify_one();
}

void client::uploadFolderSignalSlot(QString fullPathToFolderLocal, QString fullPathToFolderNetwork){
    this->buffer1 = QString(fullPathToFolderLocal);
    this->buffer2 = QString(fullPathToFolderNetwork);
    this->flag9 = true;
    cv.notify_one();
}

void client::downloadFolderOperation(QString fullPathToFolderLocal, QString fullPathToFolderNetwork){
    QMetaObject::invokeMethod(this, [this]{
        this->ui->textEdit->append("Client: загрузка папки с сервера...");
    });


    std::string fullPathToFolderNetworkSTD = fullPathToFolderNetwork.toStdString();
    const char* fullPathToFolderNetworkC = fullPathToFolderNetworkSTD.c_str();

    struct request req;
    req.typeOfRequest = 7; // скачать всю папку
    req.len = strlen(fullPathToFolderNetworkC);
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }

    send(clientSocket, fullPathToFolderNetworkC, req.len, 0);

    int currentType = 0;
    while(currentType!=-1){
        read(this->clientSocket,&currentType,sizeof(int));
        if (currentType == 0){
            int nameLen;
            read(this->clientSocket,&nameLen,sizeof(int));

            char* dirName = (char*)malloc(sizeof(char)*nameLen+1);
            memset(dirName, 0,nameLen+1);
            read(this->clientSocket, dirName, nameLen);

            std::string wayToCreateDir = fullPathToFolderLocal.toStdString() + std::string("/") +std::string(dirName);
            qDebug() << "mkdir " << mkdir(wayToCreateDir.c_str(), 0777);
        }
        else if(currentType == 1){
            int nameLen;
            read(this->clientSocket,&nameLen,sizeof(int));

            char* fileName = (char*)malloc(sizeof(char)*nameLen+1);
            memset(fileName, 0,nameLen+1);
            read(this->clientSocket, fileName, nameLen);

            int fileLen;
            read(this->clientSocket,&fileLen,sizeof(int));

            std::string wayToCreateFile = fullPathToFolderLocal.toStdString() + std::string("/") +std::string(fileName);

            FILE* f = fopen(wayToCreateFile.c_str(),"w");
            if (f==NULL){
                return;
            }
            for (int i=0;i<fileLen;i++){
                char buf;
                read(this->clientSocket,&buf,sizeof(char));
                fwrite(&buf,sizeof(char),1,f);
            }
            fclose(f);

            decryptFileInPlace(wayToCreateFile.c_str(),key);
        }
    }

    QMetaObject::invokeMethod(this, [this]{
        this->ui->textEdit->append("Client: загрузка папки с сервера завершена");
    });
}

void client::uploadFolderOperation(QString fullPathToFolderLocal, QString fullPathToFolderNetwork){
    QMetaObject::invokeMethod(this, [this]{
        this->ui->textEdit->append("Client: загрузка папки на сервер...");
    });

    std::string fullPathToFolderNetworkSTD = fullPathToFolderNetwork.toStdString();
    const char* fullPathToFolderNetworkC = fullPathToFolderNetworkSTD.c_str();

    struct request req;
    req.typeOfRequest = 8; // скачать всю папку
    req.len = strlen(fullPathToFolderNetworkC);
    int res = sendRequest(&(this->clientSocket), &serverAddress, req, 0);
    if (res==-1){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to create socket");
        });
        return;
    }
    else if (res == -2){
        QMetaObject::invokeMethod(this, [this]() {
            this->ui->label->setText("Failed to connect to server");
        });
        return;
    }

    send(clientSocket, fullPathToFolderNetworkC, req.len, 0);


    QDir localFolder(fullPathToFolderLocal);
    QFileInfoList entries = localFolder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            // Отправка типа "папка"
            char dirType = 0;
            send(clientSocket, &dirType, sizeof(dirType), 0); // тип

            QByteArray dirNameBytes = entry.fileName().toUtf8();
            int lenOfName = dirNameBytes.size();

            send(clientSocket,&lenOfName,sizeof(int),0); // отправили длину имени папки
            send(clientSocket, dirNameBytes.data(), dirNameBytes.size(), 0); // отправили имя папки
        } else {
            // Отправка типа "файл"
            char fileType = 1;
            send(clientSocket, &fileType, sizeof(fileType), 0); // тип

            // Отправка имени файла
            QByteArray fileNameBytes = entry.fileName().toUtf8();

            int lenOfName = fileNameBytes.size();
            send(clientSocket,&lenOfName,sizeof(int),0); // отправили длину имени файла
            send(clientSocket, fileNameBytes.data(), fileNameBytes.size(), 0); // отправили имя файла

            std::string str = entry.filePath().toStdString();
            const char* wayToFile = str.c_str();
            // Отправка содержимого файла
            encryptFileInPlace(wayToFile, key);

            FILE* file = fopen(wayToFile, "r");
            if (file == NULL){
                return;
            }
            fseek(file, 0, SEEK_END);
            int lenOfFile = ftell(file);
            fseek(file, 0 ,SEEK_SET);

            send(clientSocket, &lenOfFile,sizeof(int),0); // отправили длину файла

            for (int i=0;i<lenOfFile; i++){
                char buf = fgetc(file);
                send(clientSocket,&buf,sizeof(buf),0);
            }
            fclose(file);

            decryptFileInPlace(wayToFile, key);
        }
    }
    char endMarker = -1;
    send(clientSocket, &endMarker,sizeof(char),0); // конец отправки

    QMetaObject::invokeMethod(this, [this]{
        deleteChildOfCurrentIndex(this->ui->networkStorageList->currentIndex());
    });
    QMetaObject::invokeMethod(this, [this]{
        this->ui->textEdit->append("Client: загрузка папки на сервер завершена");
    });
}
