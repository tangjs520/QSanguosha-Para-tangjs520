#include "daemon.h"
#include "udpsocket.h"

Daemon::Daemon(const QString &serverName, ushort port, QObject *parent/*= 0*/)
    : QObject(parent), m_serverName(serverName)
{
    m_socket = new UdpSocket(port, this);
    connect(m_socket,
        SIGNAL(message_received(const QString &, const QString &, ushort)),
        this, SLOT(sendHeartbeat(const QString &, const QString &, ushort)));
}

void Daemon::sendHeartbeat(const QString &, const QString &address, ushort port)
{
    m_socket->send(m_serverName, address, port);
}
