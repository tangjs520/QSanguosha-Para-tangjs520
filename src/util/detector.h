#ifndef _DETECTOR_H
#define _DETECTOR_H

#include <QObject>

class UdpSocket;

class Detector : public QObject
{
    Q_OBJECT

public:
    explicit Detector(ushort localPort, QObject *parent = 0);

    void detect(ushort remotePort);

private slots:
    void receiveHeartbeat(const QString &message, const QString &address, ushort);

private:
    UdpSocket *m_socket;

    Q_DISABLE_COPY(Detector)

signals:
    void detect_finished(const QString &server_name, const QString &address);
};

#endif
