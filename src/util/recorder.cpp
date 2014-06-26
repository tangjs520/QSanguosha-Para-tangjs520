#include "recorder.h"
#include "replayfile.h"

#include <QStringList>

Recorder::Recorder(QObject *parent/* = 0*/)
    : QObject(parent)
{
    m_watch.start();
}

void Recorder::recordLine(const QString &line)
{
    int elapsedMsecs = m_watch.elapsed();
    m_data.append(QString("%1 %2").arg(elapsedMsecs).arg(line));
    if (!m_data.endsWith("\n")) {
        m_data.append("\n");
    }
}

bool Recorder::save(const QString &fileName) const
{
    return ReplayFile::save(fileName, m_data);
}

QStringList Recorder::getRecords() const
{
    QString recordData(m_data);
    QStringList records = recordData.split("\n");
    return records;
}
