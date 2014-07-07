#ifndef COMPANIONTABLE_H
#define COMPANIONTABLE_H

#include <QSet>
#include <QMap>

class QString;

typedef QSet<QString> QStringSet;

class CompanionTable
{
    typedef QMap<QString, QStringSet> Companion;

public:
    static void init();
    static bool isCompanion(const QString &one, const QString &other);
    static QStringSet getCompanions(const QString &name) { return m_companions.value(name); }

private:
    static Companion m_companions;
};

#endif // COMPANIONTABLE_H
