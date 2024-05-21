#ifndef NETWORKSCANNER_H
#define NETWORKSCANNER_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
class NetworkScanner : public QObject
{
    Q_OBJECT
public:
    explicit NetworkScanner(QObject *parent = nullptr) : QObject(parent) {}

public slots:
    void scanNetwork()
    {
        const QString ipAddressStart = "192.168.1.0";
        const QString ipAddressEnd = "192.168.1.255";
        int port = 0;
        QHostAddress startAddress(ipAddressStart);
        QHostAddress endAddress(ipAddressEnd);

        QList<QHostAddress> addresses;

        // Собираем все IP-адреса в список
        for (quint32 ip = startAddress.toIPv4Address(); ip <= endAddress.toIPv4Address(); ++ip) {
            QHostAddress currentAddress(ip);
            addresses.append(currentAddress);
        }

        // Параллельное сканирование с использованием Qt Concurrent
        QFuture<void> future = QtConcurrent::map(addresses, [=](const QHostAddress& currentAddress) {
            QTcpSocket socket;
            socket.connectToHost(currentAddress, port);

            if (socket.waitForConnected(10)) {
                qDebug() << "Client found at" << currentAddress.toString();
                // Можно выполнять дополнительные действия с найденным клиентом
            }

            socket.close();
        });

        future.waitForFinished();
    }
};
#endif // NETWORKSCANNER_H
