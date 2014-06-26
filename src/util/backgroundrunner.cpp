#include "backgroundrunner.h"

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QTime>

BackgroundRunner::BackgroundRunner(QObject *const receiver, bool isUseGUI)
    : m_receiver(receiver), m_workerThread(new QThread),
    m_slotCount(0), m_finishedSlotCount(0)
{
    //根据参数isUseGUI的值来决定槽运行在哪个线程中
    //若receiver使用主线程，m_workerThread将仅用于关联槽和启动槽
    QThread *const targetThread = isUseGUI ? _getMainThread() : m_workerThread;
    receiver->moveToThread(targetThread);

    connect(receiver, SIGNAL(slot_finished()), this, SLOT(slotFinished()));
}

BackgroundRunner::~BackgroundRunner()
{
    m_workerThread->deleteLater();
}

void BackgroundRunner::addSlot(const char *const slotName)
{
    if (!m_workerThread->isRunning()) {
        connect(m_workerThread, SIGNAL(started()), m_receiver, slotName);
        ++m_slotCount;
    }
}

void BackgroundRunner::start(DeletionPolicy policy/* = DeleteAllWhenFinished*/)
{
    if (policy & DeleteReceiverWhenFinished) {
        connect(m_workerThread, SIGNAL(finished()), m_receiver, SLOT(deleteLater()));
    }
    if (policy & DeleteSelfWhenFinished) {
        connect(m_workerThread, SIGNAL(finished()), this, SLOT(deleteLater()));
    }

    //调用m_workerThread的start后，会立即执行与started信号关联的所有槽
    m_workerThread->start();

    //@todo: 如果槽在m_workerThread中运行，以下结论成立。
    //@todo: 为了统一处理及安全起见，不采用此方案。
    //在这些槽未执行完毕前，即使调用了m_workerThread的quit()，
    //也不会立即退出线程，而是要等到所有槽都执行完毕了，才会发出finished信号，
    //从而确保m_workerThread，m_receiver和BackgroundRunner对象自己可以正常释放。
    //m_workerThread->quit();
}

//由于以下三点原因，所以需要自己实现msleep：
//1、QThread::msleep函数的访问级别是protected，在非QThread子类中无法调用；
//2、QThread::yieldCurrentThread起不到暂停线程的作用(跟踪源码发现其内部只是::Sleep(0))；
//3、为保持代码的多平台可移植性，也不建议直接使用Windows API函数：Sleep。
void BackgroundRunner::msleep(int msecs)
{
    if (QThread::currentThread() != _getMainThread()) {
        QMutex mutex;
        QMutexLocker locker(&mutex);

        QWaitCondition waitCondition;
        waitCondition.wait(locker.mutex(), msecs);
    }
    else {
        //如果m_receiver对象的槽是在主线程中执行，
        //则不能采用QWaitCondition机制，否则主界面会停顿。
        QTime idleTime = QTime::currentTime().addMSecs(msecs);
        while (QTime::currentTime() < idleTime) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
        }
    }
}

void BackgroundRunner::slotFinished()
{
    ++m_finishedSlotCount;
    if (m_finishedSlotCount == m_slotCount) {
        m_workerThread->quit();
    }
}
