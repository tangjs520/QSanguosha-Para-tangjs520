#include "lingcards.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"
#include "settings.h"

AwaitExhausted::AwaitExhausted(Card::Suit suit, int number): TrickCard(suit, number){
    setObjectName("await_exhausted");
}

QString AwaitExhausted::getSubtype() const{
    return "await_exhausted";
}

bool AwaitExhausted::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const {
    return to_select != Self;
}

bool AwaitExhausted::targetsFeasible(const QList<const Player *> &, const Player *) const{
    return true;
}

void AwaitExhausted::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct new_use = card_use;
    if (!card_use.to.contains(card_use.from))
        new_use.to << card_use.from;

    if (getSkillName() == "neo2013duoshi")
        room->addPlayerHistory(card_use.from, "NeoDuoshiAE", 1);

    TrickCard::onUse(room, new_use);
}

void AwaitExhausted::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(2);
    effect.to->getRoom()->askForDiscard(effect.to, objectName(), 2, 2, false, true);
}

BefriendAttacking::BefriendAttacking(Card::Suit suit, int number): SingleTargetTrick(suit, number){
    setObjectName("befriend_attacking");
}

bool BefriendAttacking::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num)
        return false;

    QList<const Player *> siblings = Self->getAliveSiblings();
    int distance = 0;

    foreach(const Player *p, siblings){
        int dist = Self->distanceTo(p);
        if (dist > distance)
            distance = dist;
    }

    return Self->distanceTo(to_select) == distance && to_select->getKingdom() != "god" && to_select->getKingdom() != Self->getKingdom();
}

bool BefriendAttacking::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() > 0;
}

void BefriendAttacking::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(1);
    effect.from->drawCards(3);
}

KnownBoth::KnownBoth(Card::Suit suit, int number): SingleTargetTrick(suit, number){
    setObjectName("known_both");
    can_recast = true;
}

bool KnownBoth::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (Self->isCardLimited(this, Card::MethodUse))
        return false;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if (targets.length() >= total_num || to_select == Self)
        return false;

    return !to_select->isKongcheng() || !to_select->getGeneral();
}

bool KnownBoth::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;

    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
        return targets.length() != 0;

    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() <= total_num;
}

void KnownBoth::onUse(Room *room, const CardUseStruct &card_use) const{
    if (card_use.to.isEmpty()){
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, card_use.from->objectName());
        reason.m_skillName = this->getSkillName();
        room->moveCardTo(this, card_use.from, NULL, Player::DiscardPile, reason);
        card_use.from->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#Card_Recast";
        log.from = card_use.from;
        log.card_str = card_use.card->toString();
        room->sendLog(log);

        card_use.from->drawCards(1);
    }
    else
        SingleTargetTrick::onUse(room, card_use);
}

void KnownBoth::onEffect(const CardEffectStruct &effect) const {
    Room *room = effect.from->getRoom();
    ServerPlayer *player = effect.to;

    QStringList choicelist;
    if (!effect.to->isKongcheng())
        choicelist.append("handcards");
    if (room->getMode() == "04_1v3" || room->getMode() == "06_3v3") {
        ;
    } else if (room->getMode() == "06_XMode") {
        QStringList backup = player->tag["XModeBackup"].toStringList();
        if (backup.length() > 0)
            choicelist.append("remainedgenerals");
    } else if (room->getMode() == "02_1v1") {
        QStringList list = player->tag["1v1Arrange"].toStringList();
        if (list.length() > 0)
            choicelist.append("remainedgenerals");
    } else if (Config.EnableBasara) {
        if (player->getGeneralName() == "anjiang" || player->getGeneral2Name() == "anjiang")
            choicelist.append("generals");
    } else if (!player->isLord()) {
        choicelist.append("role");
    }
    if (choicelist.isEmpty()) return;
    QString choice = room->askForChoice(effect.from, "known_both", choicelist.join("+"), QVariant::fromValue(player));

    LogMessage log;
    log.type = "$known_bothView";
    log.from = effect.from;
    log.to << effect.to;
    log.arg = "known_both:" + choice;
    room->doBroadcastNotify(room->getOtherPlayers(effect.from), QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());

    if (choice == "handcards") {
        QList<int> ids;
        foreach (const Card *card, player->getHandcards()) {
            if (card->isBlack())
                ids << card->getEffectiveId();
        }

        int card_id = room->doGongxin(effect.from, player, ids, "known_both");
        if (card_id == -1) return;
        effect.from->tag.remove("known_both");
        CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.from->objectName(), QString(), "known_both", QString());

    } else if (choice == "remainedgenerals") {
        QStringList list;
        if (room->getMode() == "02_1v1")
            list = player->tag["1v1Arrange"].toStringList();
        else if (room->getMode() == "06_XMode")
            list = player->tag["XModeBackup"].toStringList();
        foreach (QString name, list) {
            LogMessage log;
            log.type = "$known_both";
            log.from = effect.from;
            log.to << player;
            log.arg = name;
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }
        Json::Value arr(Json::arrayValue);
        arr[0] = QSanProtocol::Utils::toJsonString("known_both");
        arr[1] = QSanProtocol::Utils::toJsonArray(list);
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arr);
    } else if (choice == "generals") {
        QStringList list = player->getBasaraGeneralNames();
        foreach (const QString &name, list) {
            LogMessage log;
            log.type = "$KnownBothViewGeneral";
            log.from = effect.from;
            log.to << player;
            log.arg = name;
            room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
        }
        Json::Value arg(Json::arrayValue);
        arg[0] = QSanProtocol::Utils::toJsonString("known_both");
        arg[1] = QSanProtocol::Utils::toJsonArray(list);
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_VIEW_GENERALS, arg);
    } else if (choice == "role") {
        Json::Value arg(Json::arrayValue);
        arg[0] = QSanProtocol::Utils::toJsonString(player->objectName());
        arg[1] = QSanProtocol::Utils::toJsonString(player->getRole());
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_SET_EMOTION, arg);

        LogMessage log;
        log.type = "$ViewRole";
        log.from = effect.from;
        log.to << player;
        log.arg = player->getRole();
        room->doNotify(effect.from, QSanProtocol::S_COMMAND_LOG_SKILL, log.toJsonValue());
    }
}

NeoDrowning::NeoDrowning(Card::Suit suit, int number): AOE(suit, number){
    setObjectName("neo_drowning");
}

void NeoDrowning::onEffect(const CardEffectStruct &effect) const{
    QVariant data = QVariant::fromValue(effect);
    Room *room = effect.to->getRoom();
    QString choice = "";
    if (!effect.to->getEquips().isEmpty() && (choice = room->askForChoice(effect.to, objectName(), "throw+damage", data)) == "throw")
        effect.to->throwAllEquips();
    else{
        ServerPlayer *source = NULL;
        if (effect.from->isAlive())
            source = effect.from;
        if (choice == "")
            room->getThread()->delay();
        room->damage(DamageStruct(this, source, effect.to));
    }
}

SixSwords::SixSwords(Card::Suit suit, int number): Weapon(suit, number, 2){
    setObjectName("SixSwords");
}

SixSwordsCard::SixSwordsCard(): SkillCard(){
}

bool SixSwordsCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const{
    return to_select != Self;
}

void SixSwordsCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@SixSwordsBuff");
}

class SixSwordsSkillVS: public ZeroCardViewAsSkill{
public:
    SixSwordsSkillVS(): ZeroCardViewAsSkill("SixSwords"){
        response_pattern = "@@SixSwords";
    }

    virtual const Card *viewAs() const{
        return new SixSwordsCard;
    }
};

class SixSwordsSkill: public WeaponSkill{
public:
    SixSwordsSkill(): WeaponSkill("SixSwords"){
        events << EventPhaseStart << BeforeCardsMove;
        view_as_skill = new SixSwordsSkillVS;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<ServerPlayer *> players = room->getOtherPlayers(player);
        if (triggerEvent == BeforeCardsMove){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip))
                foreach(int id, move.card_ids){
                    const Card *card = Sanguosha->getCard(id);
                    if (card->getClassName() == "SixSwords"){
                        foreach(ServerPlayer *p, players)
                            if (p->getMark("@SixSwordsBuff") > 0)
                                p->loseMark("@SixSwordsBuff");
                        break;
                    }
            }
        }
        else if (player->getPhase() == Player::NotActive){
            foreach(ServerPlayer *p, players)
                if (p->getMark("@SixSwordsBuff") > 0)
                    p->loseMark("@SixSwordsBuff");
            room->askForUseCard(player, "@@SixSwords", "@six_swords");
        }
        return false;
    }
};

class SixSwordsSkillRange: public AttackRangeSkill{
public:
    SixSwordsSkillRange(): AttackRangeSkill("#SixSwords"){
    }

    virtual int getExtra(const Player *target, bool) const{
        if (target->getMark("@SixSwordsBuff") > 0)
            return 1;
        return 0;
    }
};

Triblade::Triblade(Card::Suit suit, int number): Weapon(suit, number, 3){
    setObjectName("Triblade");
}

TribladeCard::TribladeCard(): SkillCard(){
}

bool TribladeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.length() == 0 && to_select->hasFlag("TribladeFilter");
}

void TribladeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    room->damage(DamageStruct("Triblade", source, targets[0]));
}

class TribladeSkillVS: public OneCardViewAsSkill{
public:
    TribladeSkillVS(): OneCardViewAsSkill("Triblade"){
        response_pattern = "@@Triblade";
        filter_pattern = ".|.|.|hand!";
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        TribladeCard *c = new TribladeCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class TribladeSkill: public WeaponSkill{
public:
    TribladeSkill(): WeaponSkill("Triblade"){
        events << Damage;
        view_as_skill = new TribladeSkillVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.to && damage.to->isAlive() && damage.card && damage.card->isKindOf("Slash")
            && damage.by_user && !damage.chain && !damage.transfer){
                QList<ServerPlayer *> players;
                foreach(ServerPlayer *p, room->getOtherPlayers(damage.to))
                    if (damage.to->distanceTo(p) == 1){
                        players << p;
                        room->setPlayerFlag(p, "TribladeFilter");
                    }
                    if (players.isEmpty())
                        return false;
                    room->askForUseCard(player, "@@Triblade", "@triblade");
        }

        foreach(ServerPlayer *p, room->getAllPlayers())
            if (p->hasFlag("TribladeFilter"))
                room->setPlayerFlag(p, "TribladeFilter");

        return false;
    }
};

DragonPhoenix::DragonPhoenix(Card::Suit suit, int number): Weapon(suit, number, 2){
    setObjectName("DragonPhoenix");
}

class DragonPhoenixSkill: public WeaponSkill{
public:
    DragonPhoenixSkill(): WeaponSkill("DragonPhoenix"){
        events << TargetConfirmed << Death;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != player)
                return false;

            if (use.card->isKindOf("Slash"))
                foreach(ServerPlayer *to, use.to){
                    if (!to->canDiscard(to, "he"))
                        return false;
                    else if (use.from->askForSkillInvoke(objectName(), data)){
                        QString prompt = "dragon-phoenix-card:" + use.from->objectName();
                        room->askForDiscard(to, objectName(), 1, 1, false, true, prompt);
                    }
            }
        }
        else {
            DeathStruct death = data.value<DeathStruct>();
            if (death.damage != NULL && death.damage->card != NULL && death.damage->card->isKindOf("Slash")
                && death.damage->from == player && player->askForSkillInvoke(objectName(), data)){
                    QString general1 = death.who->getGeneralName(), general2 = death.who->getGeneral2Name();
                    QString general3 = player->getGeneralName(), general4 = player->getGeneral2Name();
                    int maxhp1 = death.who->getMaxHp(), maxhp2 = player->getMaxHp();
                    QList<const Skill *> skills1 = death.who->getVisibleSkillList(), skills2 = player->getVisibleSkillList();

                    QStringList detachlist;
                    foreach(const Skill *skill, skills1)
                        if (player->hasSkill(skill->objectName()))
                            detachlist.append(QString("-") + skill->objectName());

                    if (!detachlist.isEmpty())
                        room->handleAcquireDetachSkills(player, detachlist);

                    detachlist.clear();

                    foreach(const Skill *skill, skills2)
                        if (death.who->hasSkill(skill->objectName()))
                            detachlist.append(QString("-") + skill->objectName());

                    if (!detachlist.isEmpty())
                        room->handleAcquireDetachSkills(player, detachlist);

                    room->changeHero(player, general1, false, false, false, true);
                    if (general2.length() > 0)
                        room->changeHero(player, general2, false, false, true, true);

                    room->changeHero(death.who, general3, false, false, false, true);
                    if (general4.length() > 0)
                        room->changeHero(death.who, general4, false, false, true, true);

                    if (player->getMaxHp() != maxhp1)
                        room->setPlayerProperty(player, "maxhp", (player->isLord()) ? maxhp1 + 1 : maxhp1);

                    if (death.who->getMaxHp() != maxhp2)
                        room->setPlayerProperty(death.who, "maxhp", (death.who->isLord()) ? maxhp2 + 1: maxhp2);
            }
        }
        return false;
    }
};

PeaceSpell::PeaceSpell(Card::Suit suit, int number): Armor(suit, number){
    setObjectName("PeaceSpell");
}

void PeaceSpell::onUninstall(ServerPlayer *player) const{
    if (player->isAlive() && player->hasArmorEffect(objectName()))
        player->setFlags("peacespell_throwing");
}

PeaceSpellCard::PeaceSpellCard(){
}

bool PeaceSpellCard::targetFilter(const QList<const Player *> &, const Player *, const Player *) const{
    return true;
}

void PeaceSpellCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->gainMark("@PeaceSpellBuff");
}

class PeaceSpellVS: public ZeroCardViewAsSkill{
public:
    PeaceSpellVS(): ZeroCardViewAsSkill("PeaceSpell"){
        response_pattern = "@@PeaceSpell";
    }

    virtual const Card *viewAs() const{
        return new PeaceSpellCard;
    }
};

class PeaceSpellSkill: public ArmorSkill{
public:
    PeaceSpellSkill(): ArmorSkill("PeaceSpell"){
        events << EventPhaseStart << DamageInflicted << BeforeCardsMove << CardsMoveOneTime;
        view_as_skill = new PeaceSpellVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent != CardsMoveOneTime && !ArmorSkill::triggerable(player))
            return false;

        switch(triggerEvent){
        case (EventPhaseStart):{
            if (player->getPhase() == Player::Discard){
                room->askForUseCard(player, "@@PeaceSpell", "@peacespell");
            }
            else if (player->getPhase() == Player::RoundStart){
                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if (p->getMark("@PeaceSpellBuff") > 0)
                        p->loseAllMarks("@PeaceSpellBuff");
                }
            }
            break;
        }
        case (DamageInflicted):{
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature != DamageStruct::Normal)
                return true;
            break;
        }
        case (BeforeCardsMove):{
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip))
                foreach(int id, move.card_ids){
                    const Card *card = Sanguosha->getEngineCard(id);
                    if (card->getClassName() == "PeaceSpell"){
                        foreach(ServerPlayer *p, room->getAlivePlayers())
                            if (p->getMark("@PeaceSpellBuff") > 0)
                                p->loseAllMarks("@PeaceSpellBuff");
                        break;
                    }
            }
            break;
        }
        case (CardsMoveOneTime):{
            if (player->hasFlag("peacespell_throwing")){
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from != NULL && move.from == player && move.from_places.contains(Player::PlaceEquip))
                    foreach(int id, move.card_ids){
                        const Card *card = Sanguosha->getEngineCard(id);
                        if (card->getClassName() == "PeaceSpell"){
                            room->loseHp(player);
                            if (player->isAlive())
                                player->drawCards(2);
                            break;
                        }
                }
            }
            break;
        }
        default:
            Q_ASSERT(false);
        }
        return false;
    }
};

class PeaceSpellMaxCards: public MaxCardsSkill{
public:
    PeaceSpellMaxCards(): MaxCardsSkill("#PeaceSpell"){
    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@PeaceSpellBuff") > 0){
            QList<const Player *> players = target->getAliveSiblings();
            players << target;
            int ext = 0;
            foreach(const Player *p, players){
                if (p->getKingdom() == target->getKingdom())
                    ++ext;
            }
            return ext;
        }
        return 0;
    }
};

LingCardsPackage::LingCardsPackage(): Package("LingCards", Package::CardPack){
    QList<Card *> cards;

    cards << new AwaitExhausted(Card::Diamond, 4);
    cards << new AwaitExhausted(Card::Heart, 11);
    cards << new BefriendAttacking(Card::Heart, 9);
    cards << new KnownBoth(Card::Club, 3);
    cards << new KnownBoth(Card::Club, 4);
    cards << new NeoDrowning(Card::Club, 7);
    cards << new SixSwords(Card::Diamond, 6);
    cards << new Triblade(Card::Diamond, 12);
    cards << new DragonPhoenix(Card::Spade, 2);
    cards << new PeaceSpell(Card::Heart, 3);

    foreach(Card *c, cards)
        c->setParent(this);

    skills << new SixSwordsSkill << new SixSwordsSkillRange
        << new TribladeSkill << new DragonPhoenixSkill
        << new PeaceSpellSkill << new PeaceSpellMaxCards;

    related_skills.insertMulti("SixSwords", "#SixSwords");
    related_skills.insertMulti("PeaceSpell", "#PeaceSpell");

    addMetaObject<SixSwordsCard>();
    addMetaObject<TribladeCard>();
    addMetaObject<PeaceSpellCard>();
}

ADD_PACKAGE(LingCards)
