#ifndef _SERVER_SOCKET_H
#define _SERVER_SOCKET_H

#include <QObject>

class QTcpServer;
class ClientSocket;

class ServerSocket : public QObject
{
    Q_OBJECT

public:
    explicit ServerSocket(QObject *parent = 0);

    bool listen(ushort port);

private slots:
    void acceptNewConnection();

private:
    QTcpServer *m_server;

    Q_DISABLE_COPY(ServerSocket)

signals:
    //The socket is created as a child of the server,
    //which means that it is automatically deleted when the ServerSocket object is destroyed.
    //It is still a good idea to delete the object explicitly when you are done with it, to avoid wasting memory.
    void new_connection(ClientSocket *connection);
};

#endif
