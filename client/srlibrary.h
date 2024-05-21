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

#include <iostream>
#include <fstream>
#include <vector>
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
                            2 -  переименовать файл*/
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





#endif // SRLIBRARY_H
