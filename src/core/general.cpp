#include "general.h"
#include "engine.h"
#include "skill.h"
#include "package.h"
#include "client.h"
#include "settings.h"
#include "SkinBank.h"
#include "companion-table.h"

#include <QDir>
#include <QFile>

General::General(Package *package, const QString &name, const QString &kingdom,
    int max_hp, bool male, bool hidden, bool never_shown) : QObject(package),
    kingdom(kingdom), max_hp(max_hp), gender(male ? Male : Female),
    hidden(hidden), never_shown(never_shown)
{
    static QChar lord_symbol('$');
    if (name.endsWith(lord_symbol)) {
        QString copy = name;
        copy.remove(lord_symbol);
        lord = true;
        setObjectName(copy);
    } else {
        lord = false;
        setObjectName(name);
    }
}

#include <QMessageBox>
void General::addSkill(Skill *skill) {
    if (!skill) {
        QMessageBox::warning(NULL, "", tr("Invalid skill added to general %1").arg(objectName()));
        return;
    }
    if (!skillname_list.contains(skill->objectName())) {
        skill->setParent(this);
        skillname_list << skill->objectName();
    }
}

void General::addSkill(const QString &skill_name) {
    if (!skillname_list.contains(skill_name)) {
        extra_set.insert(skill_name);
        skillname_list << skill_name;
    }
}

bool General::hasSkill(const QString &skill_name) const{
    return skillname_list.contains(skill_name);
}

QList<const Skill *> General::getSkillList() const{
    QList<const Skill *> skills;
    const Skill *skill = NULL;
    foreach (const QString &skill_name, skillname_list) {
        if (skill_name == "mashu" && ServerInfo.DuringGame
            && ServerInfo.GameMode == "02_1v1" && ServerInfo.GameRuleMode != "Classical") {
                skill = Sanguosha->getSkill("xiaoxi");
        }
        else {
            skill = Sanguosha->getSkill(skill_name);
        }

        if (NULL == skill) {
            continue;
        }

        skills << skill;
    }

    return skills;
}

QList<const Skill *> General::getVisibleSkillList() const{
    QList<const Skill *> skills;
    foreach (const Skill *skill, getSkillList()) {
        if (skill->isVisible())
            skills << skill;
    }

    return skills;
}

QSet<const Skill *> General::getVisibleSkills() const{
    return getVisibleSkillList().toSet();
}

QSet<const TriggerSkill *> General::getTriggerSkills() const{
    QSet<const TriggerSkill *> skills;
    foreach (const QString &skill_name, skillname_list) {
        const TriggerSkill *skill = Sanguosha->getTriggerSkill(skill_name);
        if (skill)
            skills << skill;
    }

    return skills;
}

QString General::getPackage() const{
    QObject *p = parent();
    if (p)
        return p->objectName();
    else
        return QString(); // avoid null pointer exception;
}

QString General::getSkillDescription(bool includeMagatamas) const {
    QString description = QString("<img src='image/kingdom/icon/%1.png'/> ").arg(kingdom);

    QString color_str = Sanguosha->getKingdomColor(kingdom).name();
    description.append(QString("<font color=%1 size=4><b>%2</b></font> ")
        .arg(color_str).arg(Sanguosha->translate(objectName())));

    if (includeMagatamas) {
        for (int i = 0; i < max_hp; ++i) {
            description.append("<img src='image/system/magatamas/5.png' height=12/>");
        }
    }
    description.append("<br/>");

    //增加珠联璧合系统
    QStringSet companions = CompanionTable::getCompanions(objectName());
    if (!companions.isEmpty()) {
        description.append(QString("<br/><font color=%1><b>%2</b></font> ").arg(color_str).arg(tr("Companions:")));
        foreach (const QString &general, companions) {
            description.append(QString("<font color=%1>%2</font> ").arg(color_str).arg(Sanguosha->translate(general)));
        }
        description.append("<br/>");
    }

    QList<const Skill *> skills = getVisibleSkillList();
    foreach (const QString &skill_name, getRelatedSkillNames()) {
        const Skill *skill = Sanguosha->getSkill(skill_name);
        if (skill && skill->isVisible()) {
            skills << skill;
        }
    }

    foreach (const Skill *const &skill, skills) {
        description.append("<br/>");

        QString skill_name = Sanguosha->translate(skill->objectName());
        QString desc = skill->getDescription();
        desc.replace("\n", "<br/>");
        description.append(QString("<b>%1</b>: %2<br/>").arg(skill_name).arg(desc));
    }

    return description;
}

void General::lastWord() const{
    QString basePath = "audio/death";
    const QString fileNamePattern = "%1/%2.ogg";

    QString generalName = objectName();
    int generalSkinIndex = Config.value(QString("HeroSkin/%1").arg(generalName), 0).toInt();
    if (0 != generalSkinIndex) {
        QString tmpPath = QString("image/heroskin/audio/%1_%2/death").arg(generalName).arg(generalSkinIndex);
        QString tmpFile = fileNamePattern.arg(tmpPath).arg(generalName);
        if (QFile::exists(tmpFile)) {
            basePath = tmpPath;
        }
        else {
            QString matchGeneralFullName = G_ROOM_SKIN.getMatchGeneralNameFromAudioConfig(
                generalName, generalSkinIndex, getGenderString());
            if (!matchGeneralFullName.isEmpty()) {
                QString tmpPath = QString("image/heroskin/audio/%1/death").arg(matchGeneralFullName);
                if (QDir(tmpPath).exists()) {
                    basePath = tmpPath;
                }
            }
        }
    }

    QString filename = fileNamePattern.arg(basePath).arg(generalName);
    bool fileExists = QFile::exists(filename);
    if (!fileExists) {
        QStringList origin_generals = objectName().split("_");
        if (origin_generals.length() > 1)
            filename = fileNamePattern.arg(basePath).arg(origin_generals.last());
    }

    Sanguosha->playAudioEffect(filename);
}

//增加珠联璧合系统
bool General::isCompanionWith(const General *other) const
{
    if (NULL != other && other->kingdom == kingdom) {
        return CompanionTable::isCompanion(objectName(), other->objectName());
    }
    return false;
}
