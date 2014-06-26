#ifndef _CLIENT_SOCKET_H
#define _CLIENT_SOCKET_H

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;

class ClientSocket : public QObject
{
    Q_OBJECT

public:
    explicit ClientSocket(QObject *parent = 0);
    explicit ClientSocket(QTcpSocket *socket, QObject *parent = 0);

    void connectToHost(const QString &address, ushort port);
    void disconnectFromHost();

    void send(const QString &message);

    bool isConnected() const;

    QString peerName() const;
    QString peerAddress() const;

private:
    void init();

    QTcpSocket *m_socket;

    Q_DISABLE_COPY(ClientSocket)

private slots:
    void receive();
    void reportErrorMessage(QAbstractSocket::SocketError socket_error);

signals:
    void connected();
    void disconnected();

    void message_received(const QString &message);
    void error_occurred(int errorCode, const QString &errorString);
};

#endif
