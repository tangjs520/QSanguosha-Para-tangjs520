#ifndef _JSON_UTILS_H
#define _JSON_UTILS_H

#include <json/json.h>

#include <QString>
#include <QStringList>
#include <QList>
#include <QRect>
#include <QColor>

namespace QSanProtocol
{
    namespace Utils
    {
        inline QString toQString(const Json::Value &value) {
            if (value.isString()) {
                return QString::fromLocal8Bit(value.asCString());
            }
            else {
                return QString();
            }
        }

        inline Json::Value toJsonString(const QString &s) {
            if (!s.isEmpty()) {
                return Json::Value(s.toAscii().constData());
            }
            else {
                return Json::Value(QString().toStdString());
            }
        }

        Json::Value toJsonArray(const QString &s1, const QString &s2);
        Json::Value toJsonArray(const QString &s1, const Json::Value &s2);
        Json::Value toJsonArray(const QString &s1, const QString &s2, const QString &s3);
        Json::Value toJsonArray(const QList<int> &);
        Json::Value toJsonArray(const QList<QString> &);
        Json::Value toJsonArray(const QStringList &);
        bool tryParse(const Json::Value &, int &);
        bool tryParse(const Json::Value &, double &);
        bool tryParse(const Json::Value &, bool &);
        bool tryParse(const Json::Value &, QList<int> &);
        bool tryParse(const Json::Value &, QString &);
        bool tryParse(const Json::Value &, QStringList &);
        bool tryParse(const Json::Value &, QRect &);
        bool tryParse(const Json::Value &arg, QSize &result);
        bool tryParse(const Json::Value &arg, QPoint &result);
        bool tryParse(const Json::Value &arg, QColor &result);
        bool tryParse(const Json::Value &arg, Qt::Alignment &align);
    }
}

#endif
