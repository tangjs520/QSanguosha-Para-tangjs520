#include "TimedProgressBar.h"
#include "clientstruct.h"
#include "SkinBank.h"

#include <QPainter>

TimedProgressBar::TimedProgressBar()
    : m_hasTimer(false), m_autoHide(false), m_timer(0),
    m_step(0), m_max(0), m_val(0), m_mutex(QMutex::Recursive),
    m_progBg(G_ROOM_SKIN.getProgressBarPixmap(0)),
    m_prog(G_ROOM_SKIN.getProgressBarPixmap(100))
{
    this->setTextVisible(false);
}

void TimedProgressBar::show() {
    m_mutex.lock();
    if (!m_hasTimer || m_max <= 0) {
        m_mutex.unlock();
        return;
    }
    if (m_timer != 0) {
        killTimer(m_timer);
        m_timer = 0;
    }
    m_timer = startTimer(m_step);
    this->setMaximum(m_max);
    this->setValue(m_val);
    QProgressBar::show();
    m_mutex.unlock();
}

void TimedProgressBar::hide() {
    m_mutex.lock();
    if (m_timer != 0) {
        killTimer(m_timer);
        m_timer = 0;
    }
    m_mutex.unlock();
    QProgressBar::hide();
}

void TimedProgressBar::timerEvent(QTimerEvent *) {
    bool emitTimeout = false;
    bool doHide = false;
    int val = 0;
    m_mutex.lock();
    m_val += m_step;
    if (m_val >= m_max) {
        m_val = m_max;
        if (m_autoHide)
            doHide = true;
        else {
            killTimer(m_timer);
            m_timer = 0;
        }
        emitTimeout = true;
    }
    val = m_val;
    m_mutex.unlock();
    this->setValue(val);
    if (doHide) hide();
    update();
    if (emitTimeout) emit timedOut();
}

using namespace QSanProtocol;

QSanCommandProgressBar::QSanCommandProgressBar() {
    m_step = Config.S_PROGRESS_BAR_UPDATE_INTERVAL;
    m_hasTimer = (ServerInfo.OperationTimeout != 0);
    m_instanceType = S_CLIENT_INSTANCE;
}

void QSanCommandProgressBar::setCountdown(CommandType command) {
    m_mutex.lock();
    m_max = ServerInfo.getCommandTimeout(command, m_instanceType);
    m_mutex.unlock();
}

void QSanCommandProgressBar::paintEvent(QPaintEvent *) {
    m_mutex.lock();
    int val = this->m_val;
    int max = this->m_max;
    m_mutex.unlock();
    int width = this->width();
    int height = this->height();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (orientation() == Qt::Vertical) {
        painter.translate(0, height);
        qSwap(width, height);
        painter.rotate(-90);
    }

    painter.drawPixmap(0, 0, width, height, m_progBg);

    double percent = 1 - (double)val / (double)max;
    QRectF rect = QRectF(0, 0, percent * width, height);

    //以rect的右边为中轴，7为半径画一个椭圆
    QRectF ellipseRect;
    ellipseRect.setTopLeft(QPointF(rect.right() - 7, rect.top()));
    ellipseRect.setBottomRight(QPointF(rect.right() + 7, rect.bottom()));

    QPainterPath rectPath;
    QPainterPath ellipsePath;
    rectPath.addRect(rect);
    ellipsePath.addEllipse(ellipseRect);

    QPainterPath polygonPath = rectPath.united(ellipsePath);
    painter.setClipPath(polygonPath);

    painter.drawPixmap(0, 0, width, height, m_prog);
}

void QSanCommandProgressBar::setCountdown(const Countdown &countdown) {
    m_mutex.lock();
    m_hasTimer = (countdown.m_type != Countdown::S_COUNTDOWN_NO_LIMIT);
    m_max = countdown.m_max;
    m_val = countdown.m_current;
    m_mutex.unlock();
}

#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QTimer>

TimerLabel::TimerLabel(QGraphicsScene *const scene, QWidget *const parent)
    : QLabel(parent), m_proxyWidget(NULL), m_timer(NULL), m_paused(false)
{
    if (scene) {
        setObjectName("GameTimer");
        m_proxyWidget = scene->addWidget(this);
        m_proxyWidget->setParent(scene);

        resetTimeSpan();
        showTimeFmtStr();

        m_timer = new QTimer(this);
        connect(m_timer, SIGNAL(timeout()), this, SLOT(recordTiming()));
    }
}

TimerLabel::~TimerLabel()
{
    if (m_timer) {
        m_timer->stop();
    }
}

void TimerLabel::setPos(const QPointF &pos)
{
    if (m_proxyWidget) {
        m_proxyWidget->setPos(pos);
    }
}

void TimerLabel::start()
{
    if (m_timer) {
        resetTimeSpan();
        showTimeFmtStr();

        m_timer->start(1000);
        m_paused = false;
    }
}

void TimerLabel::pause()
{
    if (m_timer) {
        m_timer->stop();
        m_paused = true;
    }
}

void TimerLabel::resume()
{
    if (m_timer && m_paused) {
        m_timer->start(1000);
        m_paused = false;
    }
}

void TimerLabel::stop()
{
    if (m_timer) {
        m_timer->stop();
        m_paused = false;
    }
}

void TimerLabel::showTimeFmtStr()
{
    QString timeFmtStr = m_timeSpan.toString("hh:mm:ss");
    setText(timeFmtStr);
}

void TimerLabel::resetTimeSpan()
{
    m_timeSpan.setHMS(0, 0, 0);
}

void TimerLabel::recordTiming()
{
    m_timeSpan = m_timeSpan.addSecs(1);
    showTimeFmtStr();
}
