#ifndef _PLAYER_H
#define _PLAYER_H

#include "general.h"
#include "WrappedCard.h"

#include <QObject>
#include <QTcpSocket>

class EquipCard;
class Weapon;
class Armor;
class Horse;
class DelayedTrick;
class DistanceSkill;
class TriggerSkill;

class Player : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString screenname READ screenName WRITE setScreenName)
    Q_PROPERTY(QString nickname READ nickName WRITE setNickName)
    Q_PROPERTY(int hp READ getHp WRITE setHp)
    Q_PROPERTY(int maxhp READ getMaxHp WRITE setMaxHp)
    Q_PROPERTY(QString kingdom READ getKingdom WRITE setKingdom)
    Q_PROPERTY(QString role READ getRole WRITE setRole)
    Q_PROPERTY(QString general READ getGeneralName WRITE setGeneralName)
    Q_PROPERTY(QString general2 READ getGeneral2Name WRITE setGeneral2Name)
    Q_PROPERTY(QString state READ getState WRITE setState)
    Q_PROPERTY(int handcard_num READ getHandcardNum)
    Q_PROPERTY(int seat READ getSeat WRITE setSeat)
    Q_PROPERTY(QString phase READ getPhaseString WRITE setPhaseString)
    Q_PROPERTY(bool faceup READ faceUp WRITE setFaceUp)
    Q_PROPERTY(bool alive READ isAlive WRITE setAlive)
    Q_PROPERTY(QString flags READ getFlags WRITE setFlags)
    Q_PROPERTY(bool chained READ isChained WRITE setChained)
    Q_PROPERTY(bool owner READ isOwner WRITE setOwner)
    Q_PROPERTY(bool ready READ isReady WRITE setReady)
    Q_PROPERTY(bool role_shown READ hasShownRole WRITE setShownRole)
    Q_PROPERTY(General::Gender gender READ getGender WRITE setGender)
    Q_PROPERTY(bool careerist READ isCareerist WRITE setCareerist)
    Q_PROPERTY(bool kicked READ isKicked WRITE setKicked)

    Q_ENUMS(Phase)
    Q_ENUMS(Place)
    Q_ENUMS(Role)

public:
    enum Phase { RoundStart, Start, Judge, Draw, Play, Discard, Finish, NotActive, PhaseNone };
    enum Place { PlaceHand, PlaceEquip, PlaceDelayedTrick, PlaceJudge,
                 PlaceSpecial, DiscardPile, DrawPile, PlaceTable, PlaceUnknown,
                 PlaceWoodenOx,
                 PlaceWuGu };
    enum Role { Lord, Loyalist, Rebel, Renegade };

    explicit Player(QObject *parent);

    void setScreenName(const QString &screen_name) { this->screen_name = screen_name; }
    const QString &screenName() const { return screen_name; }

    void setNickName(const QString &nickName);
    const QString &nickName() const { return screen_name; }

    // property setters/getters
    int getHp() const { return hp; }
    void setHp(int hp);

    int getMaxHp() const { return max_hp; }
    void setMaxHp(int max_hp);

    int getLostHp() const;
    bool isWounded() const;

    General::Gender getGender() const { return m_gender; }
    virtual void setGender(General::Gender gender) { m_gender = gender; }

    bool isMale() const { return m_gender == General::Male; }
    bool isFemale() const { return m_gender == General::Female; }
    bool isNeuter() const { return m_gender == General::Neuter; }

    bool isOwner() const { return owner; }
    void setOwner(bool owner);

    bool isReady() const { return ready; }
    void setReady(bool ready);

    bool hasShownRole() const { return role_shown; }
    void setShownRole(bool shown) { this->role_shown = shown; }

    bool isCareerist() const {
        return careerist;
    }
    void setCareerist(bool careerist) {
        this->careerist = careerist;
    }

    bool isKicked() const {
        return kicked;
    }
    void setKicked(bool kicked) {
        this->kicked = kicked;
    }

    int getMaxCards(const QString &except = QString()) const;

    const QString &getKingdom() const;
    void setKingdom(const QString &kingdom);

    void setRole(const QString &role);
    const QString &getRole() const { return role; }

    Role getRoleEnum() const;

    void setGeneral(const General *general);
    void setGeneralName(const QString &general_name);
    QString getGeneralName() const;

    void setGeneral2Name(const QString &general_name);
    QString getGeneral2Name() const;
    const General *getGeneral2() const { return general2; }

    QStringList getBasaraGeneralNames() const;

    void setState(const QString &state);
    const QString &getState() const { return state; }

    int getSeat() const { return seat; }
    void setSeat(int seat) { this->seat = seat; }

    bool isAdjacentTo(const Player *another) const;

    QString getPhaseString() const;
    void setPhaseString(const QString &phase_str);

    Phase getPhase() const { return phase; }
    void setPhase(Phase phase);

    int getAttackRange(bool include_weapon = true) const;
    bool inMyAttackRange(const Player *other, int distance_fix = 0) const;

    bool isAlive() const { return alive; }
    bool isDead() const { return !isAlive(); }
    void setAlive(bool alive) { this->alive = alive; }

    QString getFlags() const;
    QStringList getFlagList() const;
    virtual void setFlags(const QString &flag);
    bool hasFlag(const QString &flag) const;
    void clearFlags();

    bool faceUp() const { return face_up; }
    void setFaceUp(bool face_up);

    virtual int aliveCount() const = 0;

    void setFixedDistance(const Player *player, int distance);
    void removeFixedDistance(const Player *player, int distance);
    void insertAttackRangePair(const Player *player);
    void removeAttackRangePair(const Player *player);
    int distanceTo(const Player *other, int distance_fix = 0) const;

    const General *getAvatarGeneral() const;
    const General *getGeneral() const { return general; }

    bool isLord() const { return getRole() == "lord"; }

    void acquireSkill(const QString &skill_name);
    void detachSkill(const QString &skill_name);
    void detachAllSkills();
    virtual void addSkill(const QString &skill_name);
    virtual void loseSkill(const QString &skill_name);
    bool hasSkill(const QString &skill_name, bool include_lose = false) const;
    bool hasSkills(const QString &skill_name, bool include_lose = false) const;
    bool hasInnateSkill(const QString &skill_name) const;
    bool hasLordSkill(const QString &skill_name, bool include_lose = false) const;

    virtual QString getGameMode() const = 0;

    void setEquip(WrappedCard *equip);
    void removeEquip(WrappedCard *equip);
    bool hasEquip(const Card *card) const;
    bool hasEquip() const;

    QList<const Card *> getJudgingArea() const;
    QList<int> getJudgingAreaID() const { return judging_area; }

    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    bool containsTrick(const QString &trick_name) const;

    virtual int getHandcardNum() const = 0;
    virtual void removeCard(const Card *card, Place place) = 0;
    virtual void addCard(const Card *card, Place place) = 0;
    virtual QList<const Card *> getHandcards() const = 0;

    WrappedCard *getWeapon() const { return weapon; }
    WrappedCard *getArmor() const { return armor; }
    WrappedCard *getDefensiveHorse() const { return defensive_horse; }
    WrappedCard *getOffensiveHorse() const { return offensive_horse; }
    WrappedCard *getTreasure() const { return treasure; }
    QList<const Card *> getEquips() const;
    const EquipCard *getEquip(int index) const;

    bool hasWeapon(const QString &weapon_name) const;
    bool hasArmorEffect(const QString &armor_name) const;
    bool hasTreasure(const QString &treasure_name) const;

    bool isKongcheng() const;
    bool isNude() const;
    bool isAllNude() const;

    bool canDiscard(const Player *to, const QString &flags) const;
    bool canDiscard(const Player *to, int card_id) const;

    void addMark(const QString &mark, int add_num = 1);
    void removeMark(const QString &mark, int remove_num = 1);
    virtual void setMark(const QString &mark, int value);
    int getMark(const QString &mark) const;
    QStringList getMarkNames() const;

    void setChained(bool chained);
    bool isChained() const { return chained; }

    bool canSlash(const Player *other, const Card *slash, bool distance_limit = true,
                  int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlash(const Player *other, bool distance_limit = true,
                  int rangefix = 0, const QList<const Player *> &others = QList<const Player *>()) const;
    int getCardCount(bool include_equip = true, bool include_judging = false) const;

    QList<int> getPile(const QString &pile_name) const;
    QStringList getPileNames() const;
    QString getPileName(int card_id) const;
    bool pileOpen(const QString &pile_name, const QString &player) const;
    void setPileOpen(const QString &pile_name, const QString &player);

    void addHistory(const QString &name, int times = 1);
    void clearHistory(const QString &name = QString());
    bool hasUsed(const QString &card_class) const;
    int usedTimes(const QString &card_class) const;
    int getSlashCount() const;

    bool hasEquipSkill(const QString &skill_name) const;
    QSet<const TriggerSkill *> getTriggerSkills() const;
    QSet<const Skill *> getSkills(bool include_equip = false, bool visible_only = true) const;
    QList<const Skill *> getSkillList(bool include_equip = false, bool visible_only = true) const;
    QSet<const Skill *> getVisibleSkills(bool include_equip = false) const;
    QList<const Skill *> getVisibleSkillList(bool include_equip = false) const;
    QStringList getAcquiredSkills() const { return acquired_skills; }

    QString getSkillDescription(bool primaryGeneral = true) const;

    virtual bool isProhibited(const Player *to, const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    bool canSlashWithoutCrossbow(const Card *slash = NULL) const;
    virtual bool isLastHandCard(const Card *card, bool contain = false) const = 0;

    inline bool isJilei(const Card *card, bool isHandcard = false) const{ return isCardLimited(card, Card::MethodDiscard, isHandcard); }
    inline bool isLocked(const Card *card, bool isHandcard = false) const{ return isCardLimited(card, Card::MethodUse, isHandcard); }

    void setCardLimitation(const QString &limit_list, const QString &pattern, bool single_turn = false);
    void removeCardLimitation(const QString &limit_list, const QString &pattern);
    void clearCardLimitation(bool single_turn = false);
    bool isCardLimited(const Card *card, Card::HandlingMethod method, bool isHandcard = false) const;

    // just for convenience
    void addQinggangTag(const Card *card);
    void removeQinggangTag(const Card *card);

    QList<const Player *> getSiblings() const;
    QList<const Player *> getAliveSiblings() const;

    QVariantMap tag;

    static bool isNostalGeneral(const Player *p, const QString &general_name);

protected:
    QMap<QString, int> marks;
    QMap<QString, QList<int> > piles;
    QMap<QString, QStringList> pile_open;
    QStringList acquired_skills;
    QStringList skills;
    QHash<QString, int> history;
    QSet<QString> flags;

private:
    QString screen_name;
    bool owner;
    bool ready;
    const General *general, *general2;
    General::Gender m_gender;
    int hp, max_hp;
    QString kingdom;
    QString role;

    //由于新增了野心家势力，但目前野心家势力没有专门的AI支持，
    //所以实际内部仍将野心家势力视为renegade角色，但为了在界面上能显示“野”图标，
    //而不再是“群”图标，需要临时新增一个标识值
    bool careerist;

    bool kicked;

    bool role_shown;
    QString state;
    int seat;
    bool alive;

    Phase phase;
    WrappedCard *weapon, *armor, *defensive_horse, *offensive_horse, *treasure;
    bool face_up;
    bool chained;
    QList<int> judging_area;
    QMultiHash<const Player *, int> fixed_distance;
    QList<const Player *> attack_range_pair;

    QMap<Card::HandlingMethod, QStringList> card_limitation;

signals:
    void general_changed();
    void general2_changed();
    void role_changed(const QString &new_role);
    void state_changed();
    void hp_changed();
    void kingdom_changed();
    void phase_changed();
    void screenname_changed(const QString &newScreenName);
    void owner_changed(bool owner);
    void ready_changed(bool ready);
    void save_me_changed(bool save);
};

#endif
