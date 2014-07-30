#ifndef _ENGINE_H
#define _ENGINE_H

#include "RoomState.h"
#include "card.h"
#include "general.h"
#include "skill.h"
#include "package.h"
#include "exppattern.h"
#include "protocol.h"
#include "util.h"

#include <QHash>
#include <QStringList>
#include <QMetaObject>
#include <QThread>
#include <QList>
#include <QMutex>

#include <QReadWriteLock>

const int HEGEMONY_ROLE_INDEX = 5;

class AI;
class Scenario;
class LuaBasicCard;
class LuaTrickCard;
class LuaWeapon;
class LuaArmor;
class LuaTreasure;

struct lua_State;

class Engine: public QObject {
    Q_OBJECT

public:
    Engine();
    ~Engine();

    void addTranslationEntry(const char *key, const char *value);

    void addHeroSkinTranslationEntry(const char *key, const char *value);
    QString heroSkinTranslate(const QString &to_translate) const;

    QString translate(const QString &to_translate) const;
    lua_State *getLuaState() const;

    int getMiniSceneCounts();

    void addPackage(Package *package);
    void addBanPackage(const QString &package_name);

    void clearBanPackage() {
        ban_package.clear();
    }

    QStringList getBanPackages() const;
    Card *cloneCard(const Card *card) const;
    Card *cloneCard(const QString &name, Card::Suit suit = Card::SuitToBeDecided,
        int number = -1, const QStringList &flags = QStringList()) const;
    SkillCard *cloneSkillCard(const QString &name) const;
    QString getVersionNumber() const;
    QString getVersion() const;
    QString getVersionName() const;
    QString getMODName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QColor getKingdomColor(const QString &kingdom) const;
    QMap<QString, QColor> getSkillTypeColorMap() const;
    QStringList getChattingEasyTexts() const;
    QStringList getSetupString() const;

    //获取机器人名称列表，该列表中的名称将在单机游戏时随机分配给电脑玩家
    const QStringList &getRobotNames() const;
    const QString &getRandomRobotName() const;

    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const QString &mode) const;
    int getPlayerCount(const QString &mode) const;
    QString getRoles(const QString &mode) const;
    QStringList getRoleList(const QString &mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const QString &name) const;
    bool matchExpPattern(const QString &pattern, const Player *player, const Card *card) const;
    Card::HandlingMethod getCardHandlingMethod(const QString &method_name) const;
    QList<const Skill *> getRelatedSkills(const QString &skill_name) const;

    QStringList getModScenarioNames() const;
    void addScenario(Scenario *scenario);
    const Scenario *getScenario(const QString &name) const;
    void addPackage(const QString &name);

    const QList<const General *> &getAllGenerals() const {
        static QList<const General *> allGenerals = findChildren<const General *>();
        return allGenerals;
    }

    const General *getGeneral(const QString &name) const;
    int getGeneralCount(bool include_banned = false, const QString &kingdom = QString()) const;
    const Skill *getSkill(const QString &skill_name) const;
    const Skill *getSkill(const EquipCard *card) const;
    QStringList getSkillNames() const;
    const TriggerSkill *getTriggerSkill(const QString &skill_name) const;
    const ViewAsSkill *getViewAsSkill(const QString &skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    QList<const MaxCardsSkill *> getMaxCardsSkills() const;
    QList<const TargetModSkill *> getTargetModSkills() const;
    QList<const InvaliditySkill *> getInvaliditySkills() const;

    QList<const AttackRangeSkill *> getAttackRangeSkills() const;

    QList<const TriggerSkill *> getGlobalTriggerSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount(bool include_banned = true) const;

    const Card *getEngineCard(int cardId) const;
    // @todo: consider making this const Card *
    Card *getCard(int cardId);
    WrappedCard *getWrappedCard(int cardId);

    QStringList getLords(bool contain_banned = false) const;
    QStringList getRandomLords() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>(), const QString &kingdom = QString()) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames(const QString &kingdom = QString()) const;

    //在虎牢关模式，神吕布变身为“暴怒的战神”后，播放特定的背景音乐
    void playHulaoPassBGM();

    void playSystemAudioEffect(const QString &name, bool continuePlayWhenPlaying = false) const;
    void playAudioEffect(const QString &filename, bool continuePlayWhenPlaying = false) const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to,
        const Card *card, const QList<const Player *> &others = QList<const Player *>()) const;
    int correctDistance(const Player *from, const Player *to) const;

    int correctMaxCards(const Player *target, bool fixed = false,
        const QString &except = QString()) const;

    int correctCardTarget(const TargetModSkill::ModType type,
        const Player *from, const Card *card) const;
    bool correctSkillValidity(const Player *player, const Skill *skill) const;

    int correctAttackRange(const Player *target,
        bool include_weapon = true, bool fixed = false) const;

    void registerRoom(QObject *room);
    void unregisterRoom();

    void clearRooms();

    QObject *currentRoomObject();
    Room *currentRoom();
    RoomState *currentRoomState();

    void blockAllRoomSignals(bool block);

    QString getCurrentCardUsePattern();
    CardUseStruct::CardUseReason getCurrentCardUseReason();

    QString findConvertFrom(const QString &general_name) const;
    bool isGeneralHidden(const QString &general_name) const;

private:
    void _loadMiniScenarios();
    void _loadModScenarios();

    QMutex m_mutex;
    QHash<QString, QString> translations;

    QHash<QString, QString> heroSkinTranslations;

    QHash<QString, const General *> generals;
    QHash<QString, const QMetaObject *> metaobjects;
    QHash<QString, QString> className2objectName;
    QHash<QString, const Skill *> skills;
    QHash<QThread *, QObject *> m_rooms;
    QMap<QString, QString> modes;
    QMultiMap<QString, QString> related_skills;
    mutable QMap<QString, const CardPattern *> patterns;

    // special skills
    QList<const ProhibitSkill *> prohibit_skills;
    QList<const DistanceSkill *> distance_skills;
    QList<const MaxCardsSkill *> maxcards_skills;
    QList<const TargetModSkill *> targetmod_skills;
    QList<const InvaliditySkill *> invalidity_skills;

    QList<const AttackRangeSkill *> attackrange_skills;

    QList<const TriggerSkill *> global_trigger_skills;

    QList<Card *> cards;
    QStringList lord_list;
    QSet<QString> ban_package;
    QHash<QString, Scenario *> m_scenarios;
    QHash<QString, Scenario *> m_miniScenes;
    Scenario *m_customScene;

    lua_State *lua;

    QHash<QString, QString> luaBasicCard_className2objectName;
    QHash<QString, const LuaBasicCard *> luaBasicCards;
    QHash<QString, QString> luaTrickCard_className2objectName;
    QHash<QString, const LuaTrickCard *> luaTrickCards;
    QHash<QString, QString> luaWeapon_className2objectName;
    QHash<QString, const LuaWeapon*> luaWeapons;
    QHash<QString, QString> luaArmor_className2objectName;
    QHash<QString, const LuaArmor *> luaArmors;
    QHash<QString, QString> luaTreasure_className2objectName;
    QHash<QString, const LuaTreasure *> luaTreasures;

    QHash<QString, int> card_packages;

    QMultiMap<QString, QString> sp_convert_pairs;
    QStringList extra_hidden_generals;
    QStringList removed_hidden_generals;
    QStringList extra_default_lords;
    QStringList removed_default_lords;
};

//由于存在多线程读写Engine::skills的情况，
//所以需要增加对它的同步保护
class ReadWriteLockHelper
{
protected:
    static QReadWriteLock m_readWriteLock;
};
class ReadLock : public ReadWriteLockHelper
{
public:
    ReadLock() { m_readWriteLock.lockForRead(); }
    ~ReadLock() { m_readWriteLock.unlock(); }
};
class WriteLock : public ReadWriteLockHelper
{
public:
    WriteLock() { m_readWriteLock.lockForWrite(); }
    ~WriteLock() { m_readWriteLock.unlock(); }
};

static inline QVariant GetConfigFromLuaState(lua_State *L, const char *key) {
    return GetValueFromLuaState(L, "config", key);
}

extern Engine *Sanguosha;

#endif
