#ifndef CONNECTIONTHREAD_H
#define CONNECTIONTHREAD_H

#include <QObject>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include "client.h"
class ConnectionThread : public QObject
{
    Q_OBJECT

public:
    ConnectionThread();

signals:
    void sendRequest(); // Сигнал для отправки запроса к серверу

public slots:
    void run(client*);
    void sendRequestToServer();
};

#endif // CONNECTIONTHREAD_H
