#ifndef _UDP_SOCKET_H
#define _UDP_SOCKET_H

#include <QObject>

class QUdpSocket;
class QHostAddress;

class UdpSocket : public QObject
{
    Q_OBJECT

public:
    explicit UdpSocket(ushort localPort, QObject *parent = 0);

    void broadcast(const QString &message, ushort port);
    void send(const QString &message, const QString &address, ushort port);

private slots:
    void receive();

private:
    void sendImpl(const QByteArray &message, const QHostAddress &address, ushort port);

    QUdpSocket *m_socket;

    Q_DISABLE_COPY(UdpSocket)

signals:
    void message_received(const QString &message, const QString &address, ushort port);
};

#endif
