#include "clientsocket.h"

#include <QTcpSocket>
#include <QHostAddress>

ClientSocket::ClientSocket(QObject *parent/* = 0*/)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    init();
}

ClientSocket::ClientSocket(QTcpSocket *socket, QObject *parent/* = 0*/)
    : QObject(parent), m_socket(socket)
{
    m_socket->setParent(this);
    init();
}

void ClientSocket::init()
{
    connect(m_socket, SIGNAL(connected()), this, SIGNAL(connected()));
    connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));

    connect(m_socket, SIGNAL(readyRead()), this, SLOT(receive()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(reportErrorMessage(QAbstractSocket::SocketError)));

    connect(m_socket, SIGNAL(disconnected()), m_socket, SLOT(deleteLater()));
}

void ClientSocket::connectToHost(const QString &address, ushort port)
{
    m_socket->connectToHost(address, port);
}

void ClientSocket::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

void ClientSocket::send(const QString &message)
{
    QByteArray data = message.toAscii();
    if (!data.endsWith("\n")) {
        data.append("\n");
    }

    m_socket->write(data);
    m_socket->flush();
}

bool ClientSocket::isConnected() const
{
    return m_socket->state() == QTcpSocket::ConnectedState;
}

QString ClientSocket::peerName() const
{
    QString peer_name = m_socket->peerName();
    if (peer_name.isEmpty()) {
        peer_name = QString("%1:%2").arg(peerAddress()).arg(m_socket->peerPort());
    }

    return peer_name;
}

QString ClientSocket::peerAddress() const
{
    return m_socket->peerAddress().toString();
}

void ClientSocket::receive()
{
    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine();
        if (!data.isEmpty() && data != "\n") {
            emit message_received(data);
        }
    }
}

void ClientSocket::reportErrorMessage(QAbstractSocket::SocketError socket_error)
{
    QString reason;
    switch (socket_error) {
    case QAbstractSocket::ConnectionRefusedError:
        reason = tr("Connection was refused or timeout");
        break;
    case QAbstractSocket::RemoteHostClosedError:
        reason = tr("Remote host close this connection");
        break;
    case QAbstractSocket::HostNotFoundError:
        reason = tr("Host not found");
        break;
    case QAbstractSocket::SocketAccessError:
        reason = tr("Socket access error");
        break;
    case QAbstractSocket::NetworkError:
        reason = tr("Network error");
        break;
    default:
        reason = tr("Unknow error");
        break;
    }

    emit error_occurred(static_cast<int>(socket_error), reason);
}
