#ifndef _RECORDER_H
#define _RECORDER_H

#include <QObject>
#include <QTime>

class Recorder : public QObject
{
    Q_OBJECT

public:
    explicit Recorder(QObject *parent = 0);

    bool save(const QString &fileName) const;

    QStringList getRecords() const;

public slots:
    void recordLine(const QString &line);

private:
    QTime m_watch;
    QByteArray m_data;
};

#endif
