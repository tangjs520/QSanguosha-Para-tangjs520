#include "detector.h"
#include "udpsocket.h"

Detector::Detector(ushort localPort, QObject *parent/* = 0*/)
    : QObject(parent)
{
    m_socket = new UdpSocket(localPort, this);
    connect(m_socket,
        SIGNAL(message_received(const QString &, const QString &, ushort)),
        this, SLOT(receiveHeartbeat(const QString &, const QString &, ushort)));
}

void Detector::detect(ushort remotePort)
{
    m_socket->broadcast("whoIsServer", remotePort);
}

void Detector::receiveHeartbeat(const QString &message, const QString &address, ushort)
{
    emit detect_finished(message, address);
}
