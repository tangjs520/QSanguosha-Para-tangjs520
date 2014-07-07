#include "replayer.h"
#include "protocol.h"

using namespace QSanProtocol;

Replayer::Replayer(const ReplayFile &file, QObject *parent/* = 0*/)
    : QThread(parent), m_file(file), m_speed(1.0), m_paused(false),
    m_requestClose(false), m_quitted(false)
{
}

void Replayer::close()
{
    QMutexLocker locker(&m_mutex);
    if (m_quitted) {
        return;
    }

    if (m_paused) {
        m_paused = false;
        m_waitCond.wakeAll();
    }

    m_requestClose = true;

    //必须用while循环来等待条件变量，而不能使用if语句，原因是spurious wakeup。
    while (!m_quitted) {
        m_waitCond.wait(locker.mutex());
    }
}

void Replayer::uniform()
{
    QMutexLocker locker(&m_mutex);

    if (m_speed != 1.0) {
        m_speed = 1.0;
        emit speed_changed(1.0);
    }
}

void Replayer::speedUp()
{
    QMutexLocker locker(&m_mutex);

    if (m_speed < 6.0) {
        qreal inc = m_speed >= 2.0 ? 1.0 : 0.5;
        m_speed += inc;
        emit speed_changed(m_speed);
    }
}

void Replayer::slowDown()
{
    QMutexLocker locker(&m_mutex);

    if (m_speed >= 1.0) {
        qreal dec = m_speed > 2.0 ? 1.0 : 0.5;
        m_speed -= dec;
        emit speed_changed(m_speed);
    }
}

void Replayer::toggle()
{
    QMutexLocker locker(&m_mutex);

    m_paused = !m_paused;
    if (!m_paused) {
        m_waitCond.wakeAll();
    }
}

void Replayer::run()
{
    int lastMsecs = 0;

    QList<CommandType> noDelayCommands;
    noDelayCommands << S_COMMAND_ADD_PLAYER
        << S_COMMAND_REMOVE_PLAYER
        << S_COMMAND_SPEAK;

    int elapsedMsecs;
    QString command;
    while (m_file.readRecord(elapsedMsecs, command)) {
        int delayMsecs = qMin(elapsedMsecs - lastMsecs, 2500);
        lastMsecs = elapsedMsecs;

        bool needDelay = true;
        QSanGeneralPacket packet;
        if (packet.parse(command.toStdString())) {
            if (noDelayCommands.contains(packet.getCommandType())) {
                needDelay = false;
            }
        }

        if (needDelay) {
            delayMsecs /= getSpeed();

            msleep(delayMsecs);
            emit elasped(elapsedMsecs / 1000.0);

            tryPause();
        }

        if (isRequestClose()) {
            break;
        }

        emit command_parsed(command);
    }

    QMutexLocker locker(&m_mutex);
    m_quitted = true;
    if (m_requestClose) {
        m_waitCond.wakeAll();
    }
}

void Replayer::tryPause()
{
    QMutexLocker locker(&m_mutex);

    //必须用while循环来等待条件变量，而不能使用if语句，原因是spurious wakeup。
    while (m_paused) {
        m_waitCond.wait(locker.mutex());
    }
}
