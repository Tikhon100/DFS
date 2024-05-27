#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <thread>
#include "mydir.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class server;
}
QT_END_NAMESPACE

class server : public QMainWindow
{
    Q_OBJECT

public:
    server(QWidget *parent = nullptr);
    ~server();


private:
    Ui::server *ui;
    std::atomic<bool> createFlag = 0;
    std::atomic<bool> terminatedFlag = 0;
    std::thread serverThread;
    void createServer();
    char baseUrl[1024] =  "/home/tikhon/Документы/Kursach/server/storage/";
    std::vector<myDir> getDirectoryContents(char path[256]);
    int listenSocket;
    const std::string key = "mysecretkey123456789012345678901";
    QString getLocalIP();

public slots:
    void startServerThread();
    void selectDirectory();
    void abortServer();
};


#endif // SERVER_H
