#include "udpsocket.h"

#include <QUdpSocket>
#include <QHostAddress>

UdpSocket::UdpSocket(ushort localPort, QObject *parent/* = 0*/)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
    m_socket->bind(localPort, QUdpSocket::ShareAddress);
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(receive()));
}

void UdpSocket::broadcast(const QString &message, ushort port)
{
    sendImpl(message.toUtf8(), QHostAddress(QHostAddress::Broadcast), port);
}

void UdpSocket::send(const QString &message, const QString &address, ushort port)
{
    sendImpl(message.toUtf8(), QHostAddress(address), port);
}

void UdpSocket::sendImpl(const QByteArray &message, const QHostAddress &host, ushort port)
{
    m_socket->writeDatagram(message, host, port);
    m_socket->flush();
}

void UdpSocket::receive()
{
    while (m_socket->hasPendingDatagrams()) {
        QHostAddress from;
        quint16 port;
        QByteArray message;
        message.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(message.data(), message.size(), &from, &port);

        emit message_received(QString::fromUtf8(message), from.toString(), port);
    }
}
