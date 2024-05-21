#ifndef CLIENT_H
#define CLIENT_H
#include <condition_variable>
#include <QMainWindow>
#include <QFileSystemModel>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <dirent.h>
#include <thread>
#include "mydir.h"
#include <QStandardItem>
#include "customitem.h"
#include "renameform.h"
#include "deleteform.h"
#include "downloadform.h"
#include "createfolderform.h"
#include <QEventLoop>
QT_BEGIN_NAMESPACE
namespace Ui
{
    class client;
}
QT_END_NAMESPACE

class client : public QMainWindow
{
    Q_OBJECT

public:
    std::atomic<bool> connectionFlag;
    client(QWidget *parent = nullptr);
    ~client();
    struct sockaddr_in serverAddress;
    int clientSocket;

private:
    std::atomic<bool> terminatedFlag = 0;
    std::thread thread;
    Ui::client *ui;
    std::atomic<bool> lastButtonClicked = 0; // 0 - create, 1 - disconnect
    const std::string key = "mysecretkey123456789012345678901";

    QEventLoop eventLoop;

    char folderPath[1024] = "root_url";
    char buffer[1024];
    QStandardItemModel *treeModel;
    QFileSystemModel *model;

    renameForm *renameform;
    deleteForm *deleteform;
    downloadForm *downloadform;
    CreateFolderForm *createFolderForm;

    std::condition_variable cv;
    std::mutex mutex;
    std::atomic<bool> flag1 = false;
    std::atomic<bool> flag2 = false;
    std::atomic<bool> flag3 = false;
    std::atomic<bool> flag4 = false;
    std::atomic<bool> flag5 = false;
    std::atomic<bool> flag6 = false;
    std::atomic<bool> flag7 = false;
    std::atomic<bool> flag8 = false;
    std::atomic<bool> flag9 = false;

    QString buffer1 = "";
    QString buffer2 = "";
    CustomItem *bufferItem = nullptr;

    void createClient();
    void sendPathToServer();

    void populateTreeView(const std::vector<myDir> &dirContents, CustomItem *parentItem);
    void renameFile(QString fullPath, QString newName);
    QString getFullPathToClickedItem();
    void removeChildItems(QStandardItemModel *model, CustomItem *parentItem);
    bool checkIfIsFiles(QModelIndex *currentNetworkindex, QModelIndex *currentLocalIndex);
    QString getFullPathToClickedLocalItem();
    bool checkIfIsFiles2(QModelIndex *currentNetworkindex, QModelIndex *currentLocalIndex);
    bool checkIfIsFolderAndFolder();
    QString getFullPathToindexOnLocalList(QModelIndex index);
    bool deleteChildOfParentItem(QModelIndex currentNetworkIndex);
    bool deleteChildOfCurrentIndex(QModelIndex currentNetworkIndex);
    bool checkFlags();
    void downloadFolderOperation(QString, QString);

public slots:
    void connectToServer();
    void clearTextBox();
    void sendRequestToServer();
    void treeItemClicked(const QModelIndex &index);
    void loadDirectoryFromServer(QString, CustomItem *);
    void renameButtonClicked();
    void deleteButtonClicked();
    void deleteFile(QString);
    void uploadFromServerToLocalStorage(QString, QString);
    void downloadButtonClicked();
    void uploadButtonClicked();
    void uploadFromLocalStorageToServer(QString, QString);
    void disconnect();
    void changeRootDir();
    void createDirectoryOnServerClicked();
    void createFolderOnServerOperation(QString folderName, QString wayToCreate);
    void loadDirectoryFromServerSLOT(QString, CustomItem *);
    void deleteFileSLOT(QString fullPath);
    void renameFileSLOT(QString fullPath, QString newName);
    void uploadFromServerToLocalStorageSLOT(QString fullPathToFile, QString fullPathToFolder);
    void uploadFromLocalStorageToServerSLOT(QString fullPathToFile, QString fullPathToDirectory);
    void createFolderOnServerOperationSLOT(QString folderName, QString wayToCreate);
    void sendPathToServerSLOT();
    void reload();
    void createFolderOnLocalDir();
    void downloadFolderButtonClicked();
    void downloadFolderSignalSlot(QString, QString);
    void uploadFolderButtonClicked();
    void uploadFolderSignalSlot(QString, QString);
    void uploadFolderOperation(QString, QString);

signals:
    void sendPathSignal();
    void loadDirectory(QString, CustomItem *);
    void renameOperation(QString, QString);
    void deleteOperation(QString);
    void uploadFromServerToLocalStorageOperation(QString, QString);
    void uploadFromLocalToServerOperation(QString, QString);
    void createFolderOnServer(QString, QString);
    void downloadFolderSignal(QString, QString);
    void uploadFolderSignal(QString, QString);
};
#endif // CLIENT_H
