#ifndef _DAEMON_H
#define _DAEMON_H

#include <QObject>

class UdpSocket;

class Daemon : public QObject
{
    Q_OBJECT

public:
    Daemon(const QString &serverName, ushort port, QObject *parent = 0);

private slots:
    void sendHeartbeat(const QString &, const QString &address, ushort port);

private:
    QString m_serverName;
    UdpSocket *m_socket;

    Q_DISABLE_COPY(Daemon)
};

#endif
