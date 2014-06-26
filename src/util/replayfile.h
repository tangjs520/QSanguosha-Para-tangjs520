#ifndef _REPLAY_FILE_H
#define _REPLAY_FILE_H

#include <QPair>
#include <QList>

namespace QSanProtocol {
    enum CommandType;
}
class QTextStream;

class ReplayFile
{
public:
    explicit ReplayFile(const QString &fileName);
    ReplayFile(const ReplayFile &other);

    bool isValid() const {
        return m_isValid;
    }

    bool readRecord(int &elapsedMsecs, QString &command);

    int getLastElapsedMsecs() const;
    QStringList getRecords() const;

    bool saveAs(const QString &fileName) {
        return save(fileName, m_data);
    }

    bool saveAsFormatConverted();

    ReplayFile &operator=(const ReplayFile &other);

    static bool save(const QString &fileName, const QByteArray &data);

private:
    void parse(QIODevice *IODevicePtr);

    QString m_fileName;
    bool m_isValid;
    QByteArray m_data;

    typedef struct tagRecord {
        QSanProtocol::CommandType getCommandType() const;
        QPair<int, QString> m_line;
    } Record;

    typedef QList<Record> Records;
    typedef Records::const_iterator RecordsIter;

    Records m_records;
    RecordsIter m_current;
    RecordsIter m_end;

    friend QTextStream &operator>>(QTextStream &stream, Record &record);
};

#endif
