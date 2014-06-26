#ifndef _REPLAYER_H
#define _REPLAYER_H

#include "replayfile.h"

#include <QThread>
#include <QWaitCondition>
#include <QMutex>

class Replayer : public QThread
{
    Q_OBJECT

public:
    explicit Replayer(const ReplayFile &file, QObject *parent = 0);

    int getDuration() const {
        int lastElapsedMsecs = m_file.getLastElapsedMsecs();
        return (lastElapsedMsecs == -1) ? 0 : (lastElapsedMsecs / 1000.0);
    }

    qreal getSpeed() const {
        QMutexLocker locker(&m_mutex);
        return m_speed;
    }

    void close();

public slots:
    void uniform();
    void toggle();
    void speedUp();
    void slowDown();

protected:
    virtual void run();

private:
    bool isRequestClose() const {
        QMutexLocker locker(&m_mutex);
        return m_requestClose;
    }

    void tryPause();

    ReplayFile m_file;

    qreal m_speed;
    bool m_paused;
    bool m_requestClose;
    //增加m_quit数据成员，是为了防止在close函数中出现spurious wakeup的情况
    bool m_quitted;

    QWaitCondition m_waitCond;
    mutable QMutex m_mutex;

signals:
    void command_parsed(const QString &cmd);
    void elasped(int secs);
    void speed_changed(qreal speed);
};

#endif
