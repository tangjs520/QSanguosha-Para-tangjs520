#include "companion-table.h"
#include "engine.h"

CompanionTable::Companion CompanionTable::m_companions;

void CompanionTable::init()
{
    QStringList companionList = GetConfigFromLuaState(Sanguosha->getLuaState(), "companion_pairs").toStringList();
    foreach (const QString &companions, companionList) {
        QStringList ones_others = companions.split("+");

        QStringList ones = ones_others.first().split("|");
        QStringList others = ones_others.last().split("|");

        foreach (const QString &one, ones) {
            m_companions[one] += others.toSet();
        }

        foreach (const QString &other, others) {
            m_companions[other] += ones.toSet();
        }
    }
}

bool CompanionTable::isCompanion(const QString &one, const QString &other)
{
    if (one != other) {
        QStringSet companions = getCompanions(one);
        if (!companions.isEmpty()) {
            return companions.contains(other);
        }
    }
    return false;
}
