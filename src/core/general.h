#ifndef _GENERAL_H
#define _GENERAL_H

class Skill;
class TriggerSkill;
class Package;

#include <QObject>
#include <QSet>
#include <QStringList>

class General: public QObject {
    Q_OBJECT
    Q_ENUMS(Gender)
    Q_PROPERTY(QString kingdom READ getKingdom CONSTANT)
    Q_PROPERTY(int maxhp READ getMaxHp CONSTANT)
    Q_PROPERTY(bool male READ isMale STORED false CONSTANT)
    Q_PROPERTY(bool female READ isFemale STORED false CONSTANT)
    Q_PROPERTY(Gender gender READ getGender CONSTANT)
    Q_PROPERTY(bool lord READ isLord CONSTANT)
    Q_PROPERTY(bool hidden READ isHidden CONSTANT)

public:
    General(Package *package, const QString &name, const QString &kingdom,
        int max_hp = 4, bool male = true, bool hidden = false, bool never_shown = false);

    enum Gender { Sexless, Male, Female, Neuter };

    // property getters/setters
    int getMaxHp() const { return max_hp; }
    const QString &getKingdom() const { return kingdom; }
    bool isMale() const { return Male == gender; }
    bool isFemale() const { return Female == gender; }
    bool isNeuter() const { return Neuter == gender; }
    bool isLord() const { return lord; }
    bool isHidden() const { return hidden; }
    bool isTotallyHidden() const { return never_shown; }
    Gender getGender() const { return gender; }
    void setGender(Gender gender) { this->gender = gender; }
    QString getGenderString() const { return (Male == gender ? "male" : "female"); }

    void addSkill(Skill *skill);
    void addSkill(const QString &skill_name);
    bool hasSkill(const QString &skill_name) const;

    QList<const Skill *> getSkillList() const;
    QList<const Skill *> getVisibleSkillList() const;
    QSet<const Skill *> getVisibleSkills() const;
    QSet<const TriggerSkill *> getTriggerSkills() const;

    void addRelateSkill(const QString &skill_name) { related_skills << skill_name; }
    const QStringList &getRelatedSkillNames() const { return related_skills; }

    QString getPackage() const;

    QString getSkillDescription(bool includeMagatamas = false) const;

    const QSet<QString> &getExtraSkillSet() const { return extra_set; }

    //增加珠联璧合系统
    bool isCompanionWith(const General *other) const;

public slots:
    void lastWord() const;

private:
    QString kingdom;
    int max_hp;
    Gender gender;
    bool lord;
    QSet<QString> extra_set;
    QStringList skillname_list;
    QStringList related_skills;
    bool hidden;
    bool never_shown;
};

#endif
