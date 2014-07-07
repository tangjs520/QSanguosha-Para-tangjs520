#include "serversocket.h"
#include "clientsocket.h"

#include <QTcpServer>

ServerSocket::ServerSocket(QObject *parent/* = 0*/)
    : QObject(parent)
{
    m_server = new QTcpServer(this);
    connect(m_server, SIGNAL(newConnection()), this, SLOT(acceptNewConnection()));
}

bool ServerSocket::listen(ushort port)
{
    return m_server->listen(QHostAddress::Any, port);
}

void ServerSocket::acceptNewConnection()
{
    QTcpSocket *socket = m_server->nextPendingConnection();
    ClientSocket *connection = new ClientSocket(socket, this);
    emit new_connection(connection);
}
