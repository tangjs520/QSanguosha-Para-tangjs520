#ifndef _TIMED_PROGRESS_BAR_H
#define _TIMED_PROGRESS_BAR_H

#include <QProgressBar>
#include <QTimerEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QMutex>

class TimedProgressBar: public QProgressBar {
Q_OBJECT
public:
    TimedProgressBar();

    inline void setTimerEnabled(bool enabled) {
        m_mutex.lock();
        m_hasTimer = enabled;
        m_mutex.unlock();
    }
    inline void setCountdown(time_t maximum, time_t startVal = 0) {
        m_mutex.lock();
        m_max = maximum;
        m_val = startVal;
        m_mutex.unlock();
    }
    inline void setAutoHide(bool enabled) { m_autoHide = enabled; }
    inline void setUpdateInterval(time_t step) { m_step = step; }
    virtual void show();
    virtual void hide();

signals:
    void timedOut();

protected:
    virtual void timerEvent(QTimerEvent *);
    bool m_hasTimer;
    bool m_autoHide;
    int m_timer;
    time_t m_step, m_max, m_val;
    QMutex m_mutex;

    const QPixmap m_progBg;
    const QPixmap m_prog;
};

#include "protocol.h"
#include "settings.h"

class QSanCommandProgressBar: public TimedProgressBar {
    Q_OBJECT

public:
    QSanCommandProgressBar();
    inline void setInstanceType(QSanProtocol::ProcessInstanceType type) { m_instanceType = type; }
    void setCountdown(QSanProtocol::CommandType command);
    void setCountdown(const QSanProtocol::Countdown &countdown);

protected:
    virtual void paintEvent(QPaintEvent *);
    QSanProtocol::ProcessInstanceType m_instanceType;
};

//新增一个计时器，用于游戏计时
#include <QLabel>
#include <QTime>
class QTimer;
class QGraphicsProxyWidget;

class TimerLabel : public QLabel
{
    Q_OBJECT

public:
    explicit TimerLabel(QGraphicsScene *const scene, QWidget *const parent = 0);
    ~TimerLabel();

    void setPos(const QPointF &pos);

    void start();
    void pause();
    bool isPaused() const { return m_paused; }
    void resume();
    void stop();

private:
    void showTimeFmtStr();
    void resetTimeSpan();

    QGraphicsProxyWidget *m_proxyWidget;
    QTimer *m_timer;
    QTime m_timeSpan;
    bool m_paused;

private slots:
    void recordTiming();
};

#endif
