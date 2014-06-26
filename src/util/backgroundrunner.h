#ifndef BACKGROUNDRUNNER_H
#define BACKGROUNDRUNNER_H

#include <QObject>
#include <QApplication>

class BackgroundRunner : public QObject
{
    Q_OBJECT

public:
    enum DeletionPolicy {
        KeepWhenFinished = 0,
        DeleteReceiverWhenFinished,
        DeleteSelfWhenFinished,
        DeleteAllWhenFinished = DeleteReceiverWhenFinished | DeleteSelfWhenFinished
    };

    //参数isUseGUI用于标识后台运行的Slot是否会使用GUI资源，
    //如果使用了GUI资源，则根据Qt的要求(Not GUI Thread Use GUI is not safe)，
    //该Slot将会在主线程中运行，我们内部会采取措施，
    //使得该Slot运行的同时，不会导致主界面停顿。
    explicit BackgroundRunner(QObject *const receiver, bool isUseGUI = false);
    ~BackgroundRunner();

    void addSlot(const char *const slotName);
    void start(BackgroundRunner::DeletionPolicy policy = DeleteAllWhenFinished);

    static void msleep(int msecs);

public slots:
    void slotFinished();

private:
    static QThread *const _getMainThread() {
        return QApplication::instance()->thread();
    }

    QObject *const m_receiver;
    QThread *const m_workerThread;

    int m_slotCount;
    int m_finishedSlotCount;

    Q_DISABLE_COPY(BackgroundRunner)
};

#endif // BACKGROUNDRUNNER_H
