#include "replayfile.h"
#include "protocol.h"

#include <QTextStream>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QImage>
#include <qmath.h>

using namespace QSanProtocol;

const int COMPRESSION_LEVEL = 9;
const int BYTES_PER_PIXEL = 4;
const QImage::Format PNG_FORMAT = QImage::Format_ARGB32;

class DataImageWriter
{
public:
    DataImageWriter(const QByteArray &data) : m_data(data) {
        int mod = m_data.size() % BYTES_PER_PIXEL;
        if (mod != 0) {
            int paddingBytes = BYTES_PER_PIXEL - mod;
            m_data.append(QByteArray(paddingBytes, '\0'));
        }

        int pixels = m_data.size() / BYTES_PER_PIXEL;
        m_pixelsPerSide = qCeil(qSqrt(static_cast<double>(pixels)));
        int paddingPixels = m_pixelsPerSide * m_pixelsPerSide - pixels;
        if (paddingPixels > 0) {
            m_data.append(QByteArray(paddingPixels * BYTES_PER_PIXEL, '\0'));
        }
    }

    bool save(const QString &fileName) const {
        QImage image(reinterpret_cast<const uchar *>(m_data.constData()),
            m_pixelsPerSide, m_pixelsPerSide, PNG_FORMAT);
        return image.save(fileName);
    }

private:
    QByteArray m_data;
    int m_pixelsPerSide;
};

DataImageWriter TxtToPng(const QByteArray &txtData)
{
    QByteArray data = qCompress(txtData, COMPRESSION_LEVEL);

    qint32 actual_size = data.size();
    data.prepend(reinterpret_cast<const char *>(&actual_size), sizeof(qint32));

    //此处不能直接返回通过data.constData()来构建的QImage对象
    //由于QByteArray的implicit sharing(copy-on-write)特性，data在离开该函数后将会被释放掉(因为其引用计数仅为1)
    //从而导致QImage对象的数据出现随机值。为了避免data的释放，需要增加一个间接层来接受data，以便增加其引用计数
    return DataImageWriter(data);
}

QByteArray PngToTxt(const QString &fileName)
{
    QImage image(fileName);
    if (image.format() != PNG_FORMAT) {
        image = image.convertToFormat(PNG_FORMAT);
    }

    const uchar *imageData = image.bits();
    qint32 actual_size = *(reinterpret_cast<const qint32 *>(imageData));

    QByteArray data(reinterpret_cast<const char *>(imageData + sizeof(qint32)), actual_size);
    data = qUncompress(data);

    return data;
}

bool isTxtFile(const QString &fileName)
{
    return fileName.endsWith(".txt", Qt::CaseInsensitive);
}

bool isPngFile(const QString &fileName)
{
    return fileName.endsWith(".png", Qt::CaseInsensitive);
}

ReplayFile::ReplayFile(const QString &fileName)
    : m_fileName(fileName), m_isValid(false)
{
    if (isTxtFile(fileName)) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            m_data = file.readAll();
        }
    }
    else if (isPngFile(fileName)) {
        m_data = PngToTxt(fileName);
    }

    if (!m_data.isEmpty()) {
        QBuffer buffer(&m_data);
        parse(&buffer);
    }

    m_current = m_records.constBegin();
    m_end = m_records.constEnd();
}

ReplayFile::ReplayFile(const ReplayFile &other)
    : m_fileName(other.m_fileName), m_isValid(other.m_isValid),
    m_data(other.m_data), m_records(other.m_records),
    m_current(m_records.constBegin()), m_end(m_records.constEnd())
{
}

bool ReplayFile::readRecord(int &elapsedMsecs, QString &command)
{
    if (m_current != m_end) {
        elapsedMsecs = m_current->m_line.first;
        command = m_current->m_line.second;

        ++m_current;
        return true;
    }

    return false;
}

int ReplayFile::getLastElapsedMsecs() const
{
    if (!m_records.isEmpty()) {
        return m_records.last().m_line.first;
    }

    return -1;
}

QStringList ReplayFile::getRecords() const
{
    QString recordStr(m_data);
    QStringList records = recordStr.split("\n");
    return records;
}

bool ReplayFile::saveAsFormatConverted()
{
    QFileInfo fileInfo(m_fileName);
    QString saveAsFileName = fileInfo.absoluteDir().absoluteFilePath(fileInfo.baseName());

    if (isTxtFile(m_fileName)) {
        saveAsFileName.append(".png");
    }
    else if (isPngFile(m_fileName)) {
        saveAsFileName.append(".txt");
    }

    return saveAs(saveAsFileName);
}

ReplayFile &ReplayFile::operator=(const ReplayFile &other)
{
    if (this != &other) {
        m_fileName = other.m_fileName;
        m_isValid = other.m_isValid;
        m_data = other.m_data;
        m_records = other.m_records;
        m_current = m_records.constBegin();
        m_end = m_records.constEnd();
    }

    return *this;
}

bool ReplayFile::save(const QString &fileName, const QByteArray &data)
{
    if (isTxtFile(fileName)) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return file.write(data) != -1;
        }
    }
    else if (isPngFile(fileName)) {
        DataImageWriter dataImageWriter = TxtToPng(data);
        return dataImageWriter.save(fileName);
    }

    return false;
}

void ReplayFile::parse(QIODevice *IODevicePtr)
{
    if (IODevicePtr->open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(IODevicePtr);

        Record checkVersionRecord;
        Record setupRecord;
        stream >> checkVersionRecord >> setupRecord;
        if (checkVersionRecord.getCommandType() == S_COMMAND_CHECK_VERSION
            && setupRecord.getCommandType() == S_COMMAND_SETUP) {
            m_records.append(checkVersionRecord);
            m_records.append(setupRecord);

            Record record;
            while (!stream.atEnd()) {
                stream >> record;
                m_records.append(record);
            }

            m_isValid = true;
        }

        IODevicePtr->close();
    }
}

CommandType ReplayFile::Record::getCommandType() const
{
    QSanGeneralPacket packet;
    packet.parse(m_line.second.toStdString());
    return packet.getCommandType();
}

QTextStream &operator>>(QTextStream &stream, ReplayFile::Record &record)
{
    QString line = stream.readLine();

    QString elapsedSection = line.section(' ', 0, 0);
    int elapsedMsecs = elapsedSection.toInt();
    QString command = line.section(' ', 1);

    record.m_line.first = elapsedMsecs;
    record.m_line.second = command;

    return stream;
}
