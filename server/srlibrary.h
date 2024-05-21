#ifndef SRLIBRARY_H
#define SRLIBRARY_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include "mydir.h"
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <QDebug>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <openssl/evp.h>


bool encryptFileInPlace(const std::string &filePath, const std::string &key)
{
    std::ifstream input(filePath, std::ios::binary | std::ios::ate);
    if (!input)
    {
        std::cerr << "Failed to open input file." << std::endl;
        return false;
    }

    std::streamsize fileSize = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<unsigned char> buffer(fileSize);
    input.read(reinterpret_cast<char *>(&buffer[0]), fileSize);
    input.close();

    unsigned char iv[EVP_MAX_IV_LENGTH] = {0}; // Инициализационный вектор

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Failed to create cipher context." << std::endl;
        return false;
    }

    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    int len, cipherTextLen;

    if (!EVP_EncryptInit_ex(ctx, cipher, NULL, reinterpret_cast<const unsigned char *>(key.c_str()), iv))
    {
        std::cerr << "Failed to initialize encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<unsigned char> encryptedBuffer(buffer.size() + EVP_MAX_BLOCK_LENGTH);

    if (!EVP_EncryptUpdate(ctx, &encryptedBuffer[0], &len, &buffer[0], static_cast<int>(buffer.size())))
    {
        std::cerr << "Failed to encrypt data." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    cipherTextLen = len;

    if (!EVP_EncryptFinal_ex(ctx, &encryptedBuffer[0] + len, &len))
    {
        std::cerr << "Failed to finalize encryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    cipherTextLen += len;

    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        std::cerr << "Failed to open output file." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    output.write(reinterpret_cast<char *>(&encryptedBuffer[0]), cipherTextLen);
    output.close();

    EVP_CIPHER_CTX_free(ctx);

    return true;
}

bool decryptFileInPlace(const std::string &filePath, const std::string &key)
{
    std::ifstream input(filePath, std::ios::binary | std::ios::ate);
    if (!input)
    {
        std::cerr << "Failed to open input file." << std::endl;
        return false;
    }

    std::streamsize fileSize = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<unsigned char> encryptedBuffer(fileSize);
    input.read(reinterpret_cast<char *>(&encryptedBuffer[0]), fileSize);
    input.close();

    unsigned char iv[EVP_MAX_IV_LENGTH] = {0}; // Инициализационный вектор

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        std::cerr << "Failed to create cipher context." << std::endl;
        return false;
    }

    const EVP_CIPHER *cipher = EVP_aes_256_cbc();
    int len, plainTextLen;

    if (!EVP_DecryptInit_ex(ctx, cipher, NULL, reinterpret_cast<const unsigned char *>(key.c_str()), iv))
    {
        std::cerr << "Failed to initialize decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<unsigned char> decryptedBuffer(encryptedBuffer.size() + EVP_MAX_BLOCK_LENGTH);

    if (!EVP_DecryptUpdate(ctx, &decryptedBuffer[0], &len, &encryptedBuffer[0], static_cast<int>(encryptedBuffer.size())))
    {
        std::cerr << "Failed to decrypt data." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plainTextLen = len;

    if (!EVP_DecryptFinal_ex(ctx, &decryptedBuffer[0] + len, &len))
    {
        std::cerr << "Failed to finalize decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plainTextLen += len;

    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output)
    {
        std::cerr << "Failed to open output file." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    output.write(reinterpret_cast<char *>(&decryptedBuffer[0]), plainTextLen);
    output.close();

    EVP_CIPHER_CTX_free(ctx);

    return true;
}

struct request{
    int typeOfRequest; /*  -1 - корневой каталог, 0 - прогрузить папку, 1 - скачать файл,
                            2 -  переименовать файл, 3 - удалить файл, 4 - скачать файл с сервера 5 - создать папку 7 - скачать папку*/
    int len; // длина отправляемых данных в первой отправке
    int len2 = 0; // длина отправляемых данных во второй отправке (доп данные)
};

int sendRequest(int *clientSocket, struct sockaddr_in* serverAddress, struct request req, int flags){
    // Создание сокета
    *clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*clientSocket == -1) {
        return -1;
    }

    // Установка соединения с сервером
    if (::connect(*clientSocket, (struct sockaddr*)serverAddress, sizeof(struct sockaddr_in)) == -1){
        ::close(*clientSocket);
        return -2;
    }

    size_t bytesSend  = send(*clientSocket, &req, sizeof(struct request), flags);
    if (bytesSend == -1){
        return -3; // ошибка отправки
    }
    return 1;  // все отправлено
}

int readRequest(int *clientSocket, struct request* buf){
    int bytesRead = read(*clientSocket, buf, sizeof(struct request));
    return bytesRead;
}

void concatenateStrings(const char* str1, const char* str2, char* result, size_t resultSize) {
    strncpy(result, str1, resultSize);          // Копируем первую строку в результат
    size_t str1Len = strlen(str1);
    size_t remainingSize = resultSize - str1Len - 1;  // Вычисляем оставшуюся доступную длину для второй строки
    strncat(result, str2, remainingSize);       // Добавляем вторую строку к результату
}

std::vector<myDir> getDirectoryContents(char *path) {
    std::vector<myDir> contents;
    DIR* dir = opendir(path);

    if (dir == nullptr) {
        // Обработка ошибки открытия директории
        return contents;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            // Пропускаем текущую и родительскую директории
            continue;
        }

        myDir item;
        strncpy(item.name, entry->d_name,strlen(entry->d_name));

        if (entry->d_type == DT_DIR){
            item.type = 0;
            item.isLoaded = 0;
        }
        else {
            item.type = 1;
            item.isLoaded = 1;
        }
        contents.push_back(item);
    }

    closedir(dir);
    // специальный маркер для указания конца передачи данных
    myDir endMarker;
    strncpy(endMarker.name, "END", 4);
    endMarker.type = -1; // Используйте нереальное значение типа
    contents.push_back(endMarker);

    return contents;
}

int sendDirectoryContent(char baseUrl[128], char url[256], int *clientSocket, int flags){

    char fullUrl[384];
    concatenateStrings(baseUrl, url, fullUrl, sizeof(fullUrl));

    std::vector contents = getDirectoryContents(fullUrl);
    for (myDir item : contents){
        int nBytes = send(*clientSocket,&item,sizeof(struct myDir),0);
        if (nBytes == -1){
            return -1;
        }
    }
    return 1;
}

std::vector<myDir> readDirectoryContent(int *clientSocket){

    std::vector<myDir> content;
    int recvBytes;
    myDir buffer;

    do {
        recvBytes = read(*clientSocket, &buffer, sizeof(myDir));
        if (recvBytes > 0) {
            if (strncmp(buffer.name, "END", 3) == 0 && buffer.type == -1) {
                // Получен специальный маркер конца передачи данных
                break;
            }
            content.push_back(buffer);
        }

    } while (recvBytes > 0);

    return content;
}

int deleteDirectory(int * clientSocket, struct request *req, const char* baseUrl){
    char *path = (char*) malloc(sizeof(char) * (req->len+1));
    memset(path, 0, req->len+1);
    // Считываем путь к файлу
    int bytesRead = read(*clientSocket, path, req->len);
    if (bytesRead != req->len) {
        qDebug() << "Failed to read the full path from the client";
        free(path);
        return -1;
    }
    // Создаем полный путь к файлу, объединяя baseUrl и path
    std::string oldFilePath = std::string(baseUrl) + std::string(path);

    try {
        std::filesystem::remove_all(oldFilePath);
    } catch (const std::filesystem::filesystem_error& ex) {
        return -2;
    }
    return 1;
}


int renameDirectory(int *clientSocket, struct request *req, const char *baseUrl) {
    char *path = (char *)malloc(sizeof(char) * (req->len+1));
    char *newName = (char *)malloc(sizeof(char) * (req->len2+1));

    memset(path, 0, req->len+1);
    memset(newName, 0, req->len2+1);

    // Считываем путь к файлу
    int bytesRead = read(*clientSocket, path, req->len);
    if (bytesRead != req->len) {
        qDebug() << "Failed to read the full path from the client";
        free(path);
        free(newName);
        return -1;
    }

    // Считываем новое имя файла
    bytesRead = read(*clientSocket, newName, req->len2);
    if (bytesRead != req->len2) {
        qDebug() << "Failed to read the new name from the client";
        free(path);
        free(newName);
        return -1;
    }

    // Создаем полный путь к файлу, объединяя baseUrl и path
    std::string oldFilePath = std::string(baseUrl) + std::string(path);

    // Получаем путь к директории, в которой находится файл
    std::string directory = oldFilePath.substr(0, oldFilePath.find_last_of("/"));

    // Создаем новый полный путь к файлу, объединяя путь к директории и новое имя файла
    std::string newFilePath = directory + "/" + std::string(newName);

    // Переименовываем файл
    if (std::rename(oldFilePath.c_str(), newFilePath.c_str()) != 0) {
        qDebug() << "Failed to rename file";
        free(path);
        free(newName);
        return -1;
    }

    qDebug() << "Renamed file from:" << oldFilePath.c_str();
    qDebug() << "To:" << newFilePath.c_str();

    free(path);
    free(newName);
    return 0;
}

int createFolder(int* clientSocket, struct request *req, char* baseUrl){
    char *folderName =  (char*)malloc(sizeof(char)* (req->len+1));
    memset(folderName, 0 , req->len+1);
    char *wayToFolder =  (char*)malloc(sizeof(char)* (req->len2+1));
    memset(wayToFolder, 0 , req->len2+1);
    read(*clientSocket,folderName,req->len);
    read(*clientSocket,wayToFolder, req->len2);

    qDebug() << "считано" << folderName << " " <<wayToFolder ;
    qDebug() << "размеры" << req->len << " " << req->len2;
    // Создаем папку с именем folderName по указанному пути
    QString fullPathQSTR = QString(baseUrl) + QString(wayToFolder) +QString("/")+ QString(folderName);
    std::string fullPathSTD = fullPathQSTR.toStdString();
    const char* fullPath = fullPathSTD.c_str();
    int result = mkdir(fullPath, 0777);

    qDebug() << "создать в " << fullPath;
    qDebug() << result;

    free (folderName);
    free(wayToFolder);
    return result;
}

int readFileFromClient(int* clientSocket, struct request* req, char* baseUrl){
    char *pathToSaveFile =  (char*)malloc(sizeof(char)* (req->len+1));
    memset(pathToSaveFile, 0 , req->len+1);

    int bytesRead = read(*clientSocket, pathToSaveFile, req->len);
    if (bytesRead != req->len) {
        qDebug() << "Failed to read the full path from the client";
        free(pathToSaveFile);
        return -2;
    }

    std::string fullPathToSavedFile = std::string(baseUrl) + std::string(pathToSaveFile);
    const char* filePath = fullPathToSavedFile.c_str();

    FILE* f = fopen(filePath,"w");
    if (f==NULL){
        return -1;
    }
    for (int i=0;i<req->len2;i++){
        char buf;
        read(*clientSocket,&buf,sizeof(char));
        fwrite(&buf,sizeof(char),1,f);
    }
    fclose(f);
    return 1;
}

int sendFileToClient(int *clientSocket, struct request* req, char* baseUrl){
    char *fullPathToFile = (char *)malloc(sizeof(char) * (req->len+1));
    memset(fullPathToFile, 0, req->len+1);

    int bytesRead = read(*clientSocket, fullPathToFile, req->len);
    if (bytesRead != req->len) {
        qDebug() << "Failed to read the full path from the client";
        free(fullPathToFile);
        return -2;
    }


    std::string oldFilePath = std::string(baseUrl) + std::string(fullPathToFile);
    const char* filePath = oldFilePath.c_str();

    FILE* file = fopen(filePath, "r");
    if (file == NULL){
        return -1;
    }
    fseek(file, 0, SEEK_END);
    int lenOfFile = ftell(file);
    fseek(file, 0 ,SEEK_SET);

    send(*clientSocket,&lenOfFile,sizeof(int),0);

    for (int i=0;i<lenOfFile;i++){
        char buf = fgetc(file);
        send(*clientSocket,&buf,sizeof(buf),0);
    }


    free(fullPathToFile);
    return 1;
}

void sendFolder(int *clientSocket, struct request *req, char*  baseUrl, const std::string key){
    char *fullPathToFolder = (char *)malloc(sizeof(char) * (req->len+1));
    memset(fullPathToFolder, 0, req->len+1);

    read(*clientSocket,fullPathToFolder,req->len);
    std::string fullPathWithBaseUrlSTD = std::string(baseUrl)+std::string(fullPathToFolder);
    char* fullPathWithBaseUrlC = (char*)fullPathWithBaseUrlSTD.c_str();

    std::vector<myDir> dirContent = getDirectoryContents(fullPathWithBaseUrlC);
    for (auto entry : dirContent){
        send(*clientSocket,&entry.type,sizeof(int),0); //отправили что это файл
        if (entry.type == 0){
            int nameLen = strlen(entry.name);
            send (*clientSocket, &nameLen, sizeof(int),0);
            send(*clientSocket, &entry.name, nameLen,0); // отправили имя
        }
        else if (entry.type == 1){
            int nameLen = strlen(entry.name);
            send (*clientSocket, &nameLen, sizeof(int),0);
            send(*clientSocket, &entry.name, nameLen,0); // отправили имя

            std::string wayToFile = fullPathWithBaseUrlSTD + std::string ("/") + std::string(entry.name);

            encryptFileInPlace(wayToFile, key);

            FILE* file = fopen(wayToFile.c_str(), "r");
            if (file == NULL){
                return;
            }
            fseek(file, 0, SEEK_END);
            int lenOfFile = ftell(file);
            fseek(file, 0 ,SEEK_SET);

            send(*clientSocket, &lenOfFile,sizeof(int),0); // отправили длину файла

            for (int i=0;i<lenOfFile; i++){
                char buf = fgetc(file);
                send(*clientSocket,&buf,sizeof(buf),0);
            }
            fclose(file);

            qDebug() << "fileName " << wayToFile;

            decryptFileInPlace(wayToFile, key);
        }

    }

    free(fullPathToFolder);
}

void getFolder(int *clientSocket, struct request *req, char*  baseUrl, const std::string key){
    char *fullPathToFolder = (char *)malloc(sizeof(char) * (req->len+1));
    memset(fullPathToFolder, 0, req->len+1);

    read(*clientSocket,fullPathToFolder,req->len);
    std::string fullPathWithBaseUrlSTD = std::string(baseUrl)+std::string(fullPathToFolder);


    char type = 0;

    while(type!=-1){
        read(*clientSocket, &type, sizeof(char)); // читаем тип
        if (type == 0){ // папка
            int nameLen;
            read(*clientSocket,&nameLen,sizeof(int));
            char* dirName = (char*)malloc(sizeof(char)*nameLen+1);
            memset(dirName, 0,nameLen+1);
            read(*clientSocket, dirName, nameLen);

            std::string wayToCreateDir = fullPathWithBaseUrlSTD + std::string("/") +std::string(dirName);
            qDebug() << "mkdir " << mkdir(wayToCreateDir.c_str(), 0777);
        }
        else if (type==1){
            int nameLen;
            read(*clientSocket,&nameLen,sizeof(int));

            char* fileName = (char*)malloc(sizeof(char)*nameLen+1);
            memset(fileName, 0,nameLen+1);
            read(*clientSocket, fileName, nameLen); // считали имя

            int fileLen;
            read(*clientSocket, &fileLen, sizeof(int));

            std::string wayToCreateFile = fullPathWithBaseUrlSTD + std::string("/") +std::string(fileName);

            FILE* f = fopen(wayToCreateFile.c_str(),"w");
            if (f==NULL){
                return;
            }
            for (int i = 0;i<fileLen;i++){
                char buf;
                read(*clientSocket, & buf,sizeof(char));
                fwrite(&buf,sizeof(char),1,f);
            }
            fclose(f);
            decryptFileInPlace(wayToCreateFile.c_str(),key);

        }
        else if (type == -1){
            break;
        }
    }


    free(fullPathToFolder);
}

#endif // SRLIBRARY_H
