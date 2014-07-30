#include "ling2013.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "engine.h"

#include "ling.h"
#include "maneuvering.h"
#include "settings.h"
#include "lingcards.h"

Neo2013XinzhanCard::Neo2013XinzhanCard(): XinzhanCard(){
    setObjectName("Neo2013XinzhanCard");
}

class Neo2013Xinzhan: public ZeroCardViewAsSkill{
public:
    Neo2013Xinzhan(): ZeroCardViewAsSkill("neo2013xinzhan"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return (!player->hasUsed("Neo2013XinzhanCard")) && (player->getHandcardNum() > player->getLostHp());
    }

    virtual const Card *viewAs() const{
        return new Neo2013XinzhanCard;
    }
};

class Neo2013Huilei: public TriggerSkill {
public:
    Neo2013Huilei():TriggerSkill("neo2013huilei") {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player)
            return false;
        ServerPlayer *killer = death.damage ? death.damage->from : NULL;
        if (killer) {
            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);
            room->notifySkillInvoked(player, objectName());

            QString killer_name = killer->getGeneralName();
            if (killer_name.contains("zhugeliang") || killer_name == "wolong")
                room->broadcastSkillInvoke(objectName(), 1);
            else
                room->broadcastSkillInvoke(objectName(), 2);

            killer->throwAllHandCards();
            room->setPlayerMark(killer, "@HuileiDecrease", 1);
            room->acquireSkill(killer, "#neo2013huilei-maxcards", false);
        }

        return false;
    }
};

class Neo2013HuileiDecrease: public MaxCardsSkill{
public:
    Neo2013HuileiDecrease(): MaxCardsSkill("#neo2013huilei-maxcards"){
    }

    virtual int getExtra(const Player *target) const{
        if (target->getMark("@HuileiDecrease") > 0)
            return -1;
        return 0;
    }
};

class Neo2013Yishi: public Yishi{
public:
    Neo2013Yishi(): Yishi(){
        setObjectName("neo2013yishi");
    }
};

class Neo2013HaoyinVS: public ZeroCardViewAsSkill{
public:
    Neo2013HaoyinVS(): ZeroCardViewAsSkill("neo2013haoyin"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        Analeptic *a = new Analeptic(Card::NoSuit, 0);
        a->setSkillName(objectName());
        return a;
    }
};

class Neo2013Haoyin: public TriggerSkill{
public:
    Neo2013Haoyin(): TriggerSkill("neo2013haoyin"){
        view_as_skill = new Neo2013HaoyinVS;
        events << PreCardUsed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Analeptic") && use.from == player && use.card->getSkillName() == objectName())
            room->loseHp(player);

        return false;
    }
};

class Neo2013Zhulou: public Zhulou{
public:
    Neo2013Zhulou(): Zhulou(){
        setObjectName("neo2013zhulou");
    }

    virtual QString getAskForCardPattern() const{
        return "^BasicCard";
    }
};

Neo2013FanjianCard::Neo2013FanjianCard(): NeoFanjianCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
    setObjectName("Neo2013FanjianCard");
}

void Neo2013FanjianCard::DifferentEffect(Room *room, ServerPlayer *from, ServerPlayer *to) const{
    from->setFlags("Neo2013Fanjian_InTempMoving");
    int card_id1 = room->askForCardChosen(from, to, "he", "neo2013fanjian");
    Player::Place place1 = room->getCardPlace(card_id1);
    to->addToPile("#fanjian", card_id1);
    int card_id2 = -1;
    if (!to->isNude()){
        card_id2 = room->askForCardChosen(from, to, "he", "neo2013fanjian");
    }
    room->moveCardTo(Sanguosha->getCard(card_id1), to, place1, CardMoveReason(CardMoveReason::S_REASON_GOTCARD, from->objectName()));
    from->setFlags("-Neo2013Fanjian_InTempMoving");
    DummyCard dummy;
    dummy.addSubcard(card_id1);
    if (card_id2 != -1)
        dummy.addSubcard(card_id2);

    room->obtainCard(from, &dummy);
}

class Neo2013Fanjian: public OneCardViewAsSkill{
public:
    Neo2013Fanjian(): OneCardViewAsSkill("neo2013fanjian"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return (!player->isKongcheng() && !player->hasUsed("Neo2013FanjianCard"));
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013FanjianCard *c = new Neo2013FanjianCard;
        c->addSubcard(originalCard);
        return c;
    }
};

class Neo2013Fankui: public MasochismSkill{
public:
    Neo2013Fankui(): MasochismSkill("neo2013fankui"){
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 0; i < damage.damage; ++i){
            QList<ServerPlayer *> players;
            foreach (ServerPlayer *p, room->getOtherPlayers(target)){
                if (!p->isNude())
                    players << p;
            }

            if (players.isEmpty())
                return;

            ServerPlayer *victim;
            if ((victim = room->askForPlayerChosen(target, players, objectName(), "@neo2013fankui", true, true)) != NULL){
                int card_id = room->askForCardChosen(target, victim, "he", objectName());
                room->obtainCard(target, Sanguosha->getCard(card_id), room->getCardPlace(card_id) != Player::PlaceHand);

                room->broadcastSkillInvoke(objectName());
            }
        }
    }
};

class Neo2013Renwang: public TriggerSkill{
public:
    Neo2013Renwang(): TriggerSkill("neo2013renwang"){
        events << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()){
            foreach(ServerPlayer *p, use.to){
                if (TriggerSkill::triggerable(p)){
                    if (!player->hasFlag(objectName()))
                        room->setPlayerFlag(player, objectName());
                    else if (p->canDiscard(player, "he") && p->askForSkillInvoke(objectName(), QVariant::fromValue(player))){
                        int card_id = room->askForCardChosen(p, player, "he", objectName(), false, Card::MethodDiscard);

                        room->throwCard(card_id, player, p);
                    }
                }
            }
        }
        return false;
    }
};

Neo2013YongyiCard::Neo2013YongyiCard(): SkillCard(){
    mute = true;
}

bool Neo2013YongyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    QList<int> arrows = Self->getPile("neoarrow");
    QList<const Card *> arrowcards;

    foreach(int id, arrows)
        arrowcards << Sanguosha->getCard(id);

    QList<const Player *> players = Self->getAliveSiblings();

    QList<const Player *> tars;

    foreach(const Card *c, arrowcards){
        Slash *slash = new Slash(c->getSuit(), c->getNumber());
        slash->addSubcard(c);
        slash->setSkillName("neo2013yongyi");
        QList<const Player *> oldplayers = players;
        foreach(const Player *p, oldplayers)
            if (slash->targetFilter(targets, p, Self)){
                players.removeOne(p);
                tars << p;
            }
            delete slash;
            slash = NULL;
            if (players.isEmpty())
                break;
    }

    return tars.contains(to_select);
}

const Card *Neo2013YongyiCard::validate(CardUseStruct &cardUse) const{
    QList<int> arrows = cardUse.from->getPile("neoarrow");
    QList<int> arrowsdisabled;
    QList<const Card *> arrowcards;

    foreach(int id, arrows)
        arrowcards << Sanguosha->getCard(id);

    foreach(const Card *c, arrowcards){
        Slash *slash = new Slash(c->getSuit(), c->getNumber());
        slash->addSubcard(c);
        slash->setSkillName("neo2013yongyi");
        foreach(ServerPlayer *to, cardUse.to)
            if (cardUse.from->isProhibited(to, slash)){
                arrows.removeOne(slash->getSubcards()[0]);
                arrowsdisabled << slash->getSubcards()[0];
                break;
            }
            delete slash;
            slash = NULL;
    }

    if (arrows.isEmpty())
        return NULL;

    Room *room = cardUse.from->getRoom();

    int or_aidelay = Config.AIDelay;
    Config.AIDelay = 0;

    room->fillAG(arrows + arrowsdisabled, cardUse.from, arrowsdisabled);

    int slashcard = room->askForAG(cardUse.from, arrows, false, "neo2013yongyi");

    room->clearAG(cardUse.from);
    Config.AIDelay = or_aidelay;

    const Card *c = Sanguosha->getCard(slashcard);
    Slash *realslash = new Slash(c->getSuit(), c->getNumber());
    realslash->addSubcard(c);
    realslash->setSkillName("neo2013yongyi");
    return realslash;
}

class Neo2013YongyiVS: public ZeroCardViewAsSkill{
public:
    Neo2013YongyiVS(): ZeroCardViewAsSkill("neo2013yongyi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }

    virtual const Card *viewAs() const{
        return new Neo2013YongyiCard;
    }
};

class Neo2013Yongyi: public TriggerSkill{
public:
    Neo2013Yongyi(): TriggerSkill("neo2013yongyi"){
        events << TargetConfirmed << Damage;
        view_as_skill = new Neo2013YongyiVS;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirmed){
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card->isKindOf("Slash") || use.card->isNDTrick()) && use.to.length() == 1 && use.to.contains(player)){
                const Card *c = room->askForExchange(player, objectName(), 1, 1, false, "@neo2013yongyiput", true);
                if (c != NULL)
                    player->addToPile("neoarrow", c, false);
            }
        }
        else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName()
                && !damage.chain && !damage.transfer && damage.by_user)
                if (damage.card->isRed()){
                    if (player->askForSkillInvoke(objectName(), data)){
                        room->broadcastSkillInvoke(objectName(), 3);
                        player->drawCards(1);
                    }
                }
                else if (damage.card->isBlack()){
                    if (!damage.to->isNude() && player->askForSkillInvoke(objectName(), data)){
                        int card_id = room->askForCardChosen(player, damage.to, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(card_id, damage.to, player);
                        room->broadcastSkillInvoke(objectName(), 4);
                    }
                }
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const{
        if (card->isKindOf("Slash"))
            return qrand() % 2 + 1;
        return -1;
    }
};

class Neo2013Duoyi: public TriggerSkill{
public:
    Neo2013Duoyi(): TriggerSkill("neo2013duoyi"){
        events << EventPhaseStart << CardUsed << CardResponded << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive() && !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL || selfplayer->isKongcheng())
            return false;
        static QStringList types;
        if (types.isEmpty())
            types << "BasicCard" << "EquipCard" << "TrickCard";

        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() != Player::RoundStart)
                return false;

            if (room->askForDiscard(selfplayer, objectName(), 1, 1, true, true, "@neo2013duoyi")){
                QString choice = room->askForChoice(selfplayer, objectName(), types.join("+"), data);
                room->setPlayerMark(player, "YiDuoyiType", types.indexOf(choice) + 1);
            }
        }
        else if (triggerEvent == CardUsed || triggerEvent == CardResponded){
            if (player->getMark("YiDuoyiType") == 0)
                return false;

            std::string t = types[player->getMark("YiDuoyiType") - 1].toStdString();   //QString 2 char * is TOO complicated!

            const Card *c = NULL;

            if (triggerEvent == CardUsed)
                c = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }
            if (c == NULL)
                return false;

            if (c->isKindOf(t.c_str()))
                selfplayer->drawCards(1);
        }
        else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "YiDuoyiType", 0);
        }
        return false;
    }
};

Neo2013PujiCard::Neo2013PujiCard(): SkillCard(){
}

bool Neo2013PujiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0 && !to_select->isNude() && to_select != Self;
}

void Neo2013PujiCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets[0];
    const Card *card = Sanguosha->getCard(room->askForCardChosen(source, target, "he", "neo2013puji", false, Card::MethodDiscard));

    QList<ServerPlayer *> beneficiary;
    if (Sanguosha->getCard(getSubcards()[0])->isBlack())
        beneficiary << source;

    if (card->isBlack())
        beneficiary << target;

    room->throwCard(card, target, source);

    if (beneficiary.length() != 0)
        foreach(ServerPlayer *p, beneficiary){
            QStringList choicelist;
            choicelist << "draw";
            if (p->isWounded())
                choicelist << "recover";

            QString choice = room->askForChoice(p, "neo2013puji", choicelist.join("+"));

            if (choice == "draw")
                p->drawCards(1);
            else {
                RecoverStruct r;
                r.who = p;
                room->recover(p, r);
            }
    }
}

class Neo2013Puji: public OneCardViewAsSkill{
public:
    Neo2013Puji(): OneCardViewAsSkill("neo2013puji"){
    }

    virtual bool viewFilter(const Card *to_select) const{
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013PujiCard *c = new Neo2013PujiCard;
        c->addSubcard(originalCard);
        return c;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->canDiscard(player, "he") && !player->hasUsed("Neo2013PujiCard");
    }
};

class Neo2013Shushen: public TriggerSkill{
public:
    Neo2013Shushen(): TriggerSkill("neo2013shushen"){
        events << HpRecover;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        RecoverStruct recover_struct = data.value<RecoverStruct>();
        int recover = recover_struct.recover;
        for (int i = 1; i <= recover; ++i){
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@neo2013shushen", true, true);
            if (target != NULL){
                room->broadcastSkillInvoke(objectName());
                room->drawCards(target, 1);
            }
            else
                break;
        }
        return false;
    }
};

class Neo2013Shenzhi: public PhaseChangeSkill{
public:
    Neo2013Shenzhi(): PhaseChangeSkill("neo2013shenzhi"){
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        if (player->getPhase() != Player::Start || player->isKongcheng())
            return false;

        foreach (const Card *card, player->getHandcards()){
            if (player->isJilei(card))
                return false;
        }

        Room *room = player->getRoom();
        if (room->askForSkillInvoke(player, objectName())){
            room->broadcastSkillInvoke(objectName());
            player->throwAllHandCards();
            RecoverStruct recover;
            recover.who = player;
            recover.recover = 1;
            room->recover(player, recover);
        }
        return false;
    }
};

class Neo2013Longyin: public TriggerSkill{
public:
    Neo2013Longyin(): TriggerSkill("neo2013longyin"){
        events << CardUsed;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPhase() == Player::Play;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")){
            ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
            if (selfplayer != NULL && selfplayer->canDiscard(selfplayer, "he")){
                const Card *c = NULL;
                if (use.card->isRed())
                    c = room->askForCard(selfplayer, "..red", "@neo2013longyin", data, objectName());
                else if (use.card->isBlack())
                    c = room->askForCard(selfplayer, "..black", "@neo2013longyin", data, objectName());
                if (c != NULL){
                    if (use.m_addHistory)
                        room->addPlayerHistory(player, use.card->getClassName(), -1);
                    selfplayer->drawCards(1);
                }
            }
        }
        return false;
    }
};

class Neo2013Duoshi: public OneCardViewAsSkill{
public:
    Neo2013Duoshi(): OneCardViewAsSkill("neo2013duoshi"){
        filter_pattern = ".|red|.|hand!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->usedTimes("NeoDuoshiAE") <= 4;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        AwaitExhausted *ae = new AwaitExhausted(originalCard->getSuit(), originalCard->getNumber());
        ae->addSubcard(originalCard);
        ae->setSkillName(objectName());
        return ae;
    }
};

class Neo2013Danji: public PhaseChangeSkill{
public:
    Neo2013Danji(): PhaseChangeSkill("neo2013danji"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::RoundStart
            && target->getMark(objectName()) == 0
            && target->getHandcardNum() > target->getHp();
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());
        LogMessage l;
        l.type = "#NeoDanjiWake";
        l.from = player;
        l.arg = QString::number(player->getHandcardNum());
        l.arg2 = QString::number(player->getHp());
        room->sendLog(l);
        room->broadcastSkillInvoke(objectName());
        room->doLightbox("$DanjiAnimate", 5000);
        if (room->changeMaxHpForAwakenSkill(player)){
            room->setPlayerProperty(player, "kingdom", "shu");
            room->setPlayerMark(player, objectName(), 1);
            room->handleAcquireDetachSkills(player, "mashu|zhongyi|neo2013huwei");
        }

        return false;
    }
};

class Neo2013Huwei: public PhaseChangeSkill{
public:
    Neo2013Huwei(): PhaseChangeSkill("neo2013huwei"){
        frequency = Limited;
        limit_mark = "@yihuwei";
    }

    virtual bool onPhaseChange(ServerPlayer *player) const{
        Room *room = player->getRoom();
        if (player->getPhase() != Player::RoundStart)
            return false;
        if (player->getMark("@yihuwei") == 0)
            return false;

        NeoDrowning *dr = new NeoDrowning(Card::NoSuit, 0);
        dr->setSkillName(objectName());

        if (!dr->isAvailable(player)){
            delete dr;
            return false;
        }
        if (player->askForSkillInvoke(objectName())){
            room->doLightbox("$HuweiAnimate", 4000);
            room->setPlayerMark(player, "@yihuwei", 0);
            room->useCard(CardUseStruct(dr, player, room->getOtherPlayers(player)), false);
        }
        return false;
    }
};

class Neo2013Huoshui: public TriggerSkill{
public:
    Neo2013Huoshui(): TriggerSkill("neo2013huoshui") {
        events << EventPhaseStart << Death
            << EventLoseSkill << EventAcquireSkill
            << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual int getPriority() const{
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == EventPhaseStart) {
            if (!TriggerSkill::triggerable(player)
                || (player->getPhase() != Player::RoundStart || player->getPhase() != Player::NotActive)) return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player || !player->hasSkill(objectName())) return false;
        } else if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName() || player->getPhase() == Player::NotActive) return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName() || !player->hasSkill(objectName()) || player->getPhase() == Player::NotActive)
                return false;
        } else if (triggerEvent == CardsMoveOneTime) {  //this can fix filter skill?
            if (!room->getCurrent() || !room->getCurrent()->hasSkill(objectName())) return false;
        }

        if (player->getPhase() == Player::RoundStart || triggerEvent == EventAcquireSkill)
            room->broadcastSkillInvoke(objectName(), 1);
        else if (player->getPhase() == Player::NotActive || triggerEvent == EventLoseSkill)
            room->broadcastSkillInvoke(objectName(), 2);

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);
        Json::Value args;
        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

class Neo2013Qingcheng: public TriggerSkill{
public:
    Neo2013Qingcheng(): TriggerSkill("neo2013qingcheng"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual int getPriority() const{
        return 6;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() == Player::RoundStart){
            QStringList Qingchenglist = player->tag["neo2013qingcheng"].toStringList();
            if (!Qingchenglist.isEmpty()){
                foreach (QString skill_name, Qingchenglist) {
                    room->setPlayerMark(player, "Qingcheng" + skill_name, 0);
                    if (player->hasSkill(skill_name)) {
                        LogMessage log;
                        log.type = "$QingchengReset";
                        log.from = player;
                        log.arg = skill_name;
                        room->sendLog(log);
                    }
                }
                player->tag.remove("neo2013qingcheng");
                room->broadcastSkillInvoke(objectName(), 2);

                foreach (ServerPlayer *p, room->getAllPlayers())
                    room->filterCards(p, p->getCards("he"), true);

                Json::Value args;
                args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

            }
            ServerPlayer *zou = room->findPlayerBySkillName(objectName());
            if (zou == NULL || zou->isNude() || zou == player)
                return false;

            if (room->askForDiscard(zou, "neo2013qingcheng", 1, 1, true, true, "@neo2013qingcheng-discard")){
                QStringList skill_list;
                foreach (const Skill *skill, player->getVisibleSkillList()){
                    if (!skill_list.contains(skill->objectName()) && !skill->inherits("SPConvertSkill") && !skill->isAttachedLordSkill()) {
                        skill_list << skill->objectName();
                    }
                }
                QString skill_qc;
                if (!skill_list.isEmpty()) {
                    QVariant data_for_ai = QVariant::fromValue(player);
                    skill_qc = room->askForChoice(zou, "neo2013qingcheng", skill_list.join("+"), data_for_ai);
                }

                if (!skill_qc.isEmpty()) {
                    LogMessage log;
                    log.type = "$QingchengNullify";
                    log.from = zou;
                    log.to << player;
                    log.arg = skill_qc;
                    room->sendLog(log);

                    QStringList Qingchenglist = player->tag["neo2013qingcheng"].toStringList();
                    Qingchenglist << skill_qc;
                    player->tag["neo2013qingcheng"] = QVariant::fromValue(Qingchenglist);
                    room->addPlayerMark(player, "Qingcheng" + skill_qc);
                    room->broadcastSkillInvoke(objectName(), 1);
                }
            }
        }

        foreach (ServerPlayer *p, room->getAllPlayers())
            room->filterCards(p, p->getCards("he"), true);

        Json::Value args;
        args[0] = QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);

        return false;
    }
};

Neo2013XiechanCard::Neo2013XiechanCard(){
}

void Neo2013XiechanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    room->setPlayerMark(effect.from, "@neo2013xiechan", 0);
    bool success = effect.from->pindian(effect.to, "neo2013xiechan", NULL);
    Card *card = NULL;
    if (success)
        card = new Slash(Card::NoSuit, 0);
    else
        card = new Duel(Card::NoSuit, 0);

    card->setSkillName("_neo2013xiechan");
    room->useCard(CardUseStruct(card, effect.from, effect.to), false);
}

class Neo2013Xiechan: public ZeroCardViewAsSkill{
public:
    Neo2013Xiechan(): ZeroCardViewAsSkill("neo2013xiechan"){
        frequency = Limited;
        limit_mark = "@neo2013xiechan";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@neo2013xiechan") > 0;
    }

    virtual const Card *viewAs() const{
        return new Neo2013XiechanCard;
    }
};

class Neo2013Chengxiang: public MasochismSkill {
public:
    Neo2013Chengxiang(): MasochismSkill("neo2013chengxiang") {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        Room *room = target->getRoom();
        for (int i = 1; i <= damage.damage; ++i) {
            if (!target->askForSkillInvoke(objectName(), QVariant::fromValue(damage)))
                return;
            room->broadcastSkillInvoke(objectName());

            QList<int> card_ids = room->getNCards(4);
            room->fillAG(card_ids);

            QList<int> to_get, to_throw;
            while (true) {
                int sum = 0;
                foreach (int id, to_get)
                    sum += Sanguosha->getCard(id)->getNumber();
                foreach (int id, card_ids) {
                    if (sum + Sanguosha->getCard(id)->getNumber() > 13) {
                        room->takeAG(NULL, id, false);
                        card_ids.removeOne(id);
                        to_throw << id;
                    }
                }
                if (card_ids.isEmpty()) break;

                int card_id = room->askForAG(target, card_ids, card_ids.length() < 4, objectName());
                if (card_id == -1) break;
                card_ids.removeOne(card_id);
                to_get << card_id;
                room->takeAG(target, card_id, false);
                if (card_ids.isEmpty()) break;
            }
            DummyCard *dummy = new DummyCard;
            if (!to_get.isEmpty()) {
                dummy->addSubcards(to_get);
                target->obtainCard(dummy);
            }
            dummy->clearSubcards();
            if (!to_throw.isEmpty() || !card_ids.isEmpty()) {
                dummy->addSubcards(to_throw + card_ids);
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, target->objectName(), objectName(), QString());
                room->throwCard(dummy, reason, NULL);
            }
            delete dummy;

            room->clearAG();
        }
    }
};

class Neo2013Xiangxue: public PhaseChangeSkill{
public:
    Neo2013Xiangxue(): PhaseChangeSkill("neo2013xiangxue"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Start)
            return false;

        Room *room = target->getRoom();
        bool invoke = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(target))
            if (p->getHandcardNum() >= target->getHandcardNum()){
                invoke = false;
                break;
            }

            if (!invoke)
                return false;

            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$neo2013xiangxue");

            if (room->changeMaxHpForAwakenSkill(target)){
                room->setPlayerMark(target, objectName(), 1);
                room->drawCards(target, 2);
                QStringList l;
                l << "neo2013tongwu" << "neo2013bingyin";
                room->handleAcquireDetachSkills(target, l);
            }
            return false;
    }
};

class Neo2013TongwuVS: public ViewAsSkill{
public:
    Neo2013TongwuVS(): ViewAsSkill("neo2013tongwu"){
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
            || (Sanguosha->getCurrentCardUsePattern() == "slash"
            && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE))
            return selected.isEmpty() && to_select->isNDTrick();
        else if (Sanguosha->getCurrentCardUsePattern() == "@@neo2013tongwu")
            return false;

        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY
            || (Sanguosha->getCurrentCardUsePattern() == "slash"
            && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)){
                if (cards.length() == 0)
                    return NULL;
                Slash *slash = new Slash(cards[0]->getSuit(), cards[0]->getNumber());
                slash->setSkillName(objectName());
                slash->addSubcards(cards);
                return slash;
        }
        else if (Sanguosha->getCurrentCardUsePattern() == "@@neo2013tongwu"){
            const Card *card = Sanguosha->getCard(Self->property("tongwucard").toInt());
            Card *card_to_use = Sanguosha->cloneCard(card->objectName(), Card::NoSuit, 0);
            card_to_use->setSkillName(objectName());
            return card_to_use;
        }

        return NULL;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return (pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            || (pattern == "@@neo2013tongwu");
    }
};

class Neo2013Tongwu: public TriggerSkill{
public:
    Neo2013Tongwu(): TriggerSkill("neo2013tongwu"){
        view_as_skill = new Neo2013TongwuVS;
        events << DamageDone << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == DamageDone){
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash") && damage.card->getSkillName() == objectName())
                room->setCardFlag(damage.card, "tongwucaninvoke");
        }
        else if (TriggerSkill::triggerable(player)){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->isKindOf("Slash") && use.card->hasFlag("tongwucaninvoke")){
                room->setCardFlag(use.card, "-tongwucaninvoke");
                const Card *card_to_use = Sanguosha->getCard(use.card->getSubcards().first());
                if (!card_to_use->isAvailable(player))
                    return false;
                else if (card_to_use->targetFixed()){
                    if (!player->askForSkillInvoke(objectName(), QVariant::fromValue(card_to_use)))
                        return false;
                    Card *real_use = Sanguosha->cloneCard(card_to_use->objectName(), Card::NoSuit, 0);
                    real_use->setSkillName(objectName());
                    room->useCard(CardUseStruct(real_use, player, QList<ServerPlayer *>()));
                }
                else {
                    room->setPlayerProperty(player, "tongwucard", card_to_use->getId());
                    room->askForUseCard(player, "@@neo2013tongwu", "@neo2013tongwu");
                    room->setPlayerProperty(player, "tongwucard", QVariant());
                }
            }
        }
        return false;
    }
};

class Neo2013Bingyin: public PhaseChangeSkill{
public:
    Neo2013Bingyin(): PhaseChangeSkill("neo2013bingyin"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target) && target->getMark(objectName()) == 0;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() != Player::Finish)
            return false;

        Room *room = target->getRoom();
        bool invoke = true;
        foreach (ServerPlayer *p, room->getOtherPlayers(target))
            if (p->getHp() < target->getHp()){
                invoke = false;
                break;
            }

            if (!invoke)
                return false;

            room->broadcastSkillInvoke(objectName());
            room->doLightbox("$neo2013bingyin");

            room->setPlayerMark(target, objectName(), 1);
            QStringList l;
            l << "neo2013touxi" << "neo2013muhui";
            room->handleAcquireDetachSkills(target, l);

            return false;
    }
};

class Neo2013Touxi: public TriggerSkill{
public:
    Neo2013Touxi(): TriggerSkill("neo2013touxi"){
        frequency = Compulsory;
        events << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == NULL || use.card == NULL || !use.card->isKindOf("Slash"))
            return false;

        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
            room->askForUseSlashTo(p, use.from, "@neo2013touxi-slash:" + use.from->objectName(), false);

        return false;
    }
};

class Neo2013TouxiRange: public AttackRangeSkill{
public:
    Neo2013TouxiRange(): AttackRangeSkill("#neo2013touxi"){
    }

    virtual int getFixed(const Player *target, bool ) const{
        if (target->hasSkill("neo2013touxi")) {
            const Player *current = NULL;
            foreach (const Player *p, target->getAliveSiblings()){
                if (p->getPhase() != Player::NotActive){
                    current = p;
                    break;
                }
            }

            if (current != NULL)
                return current->getHp() > 0 ? current->getHp(): 0;
        }
        return -1;
    }
};

class Neo2013Muhui: public ProhibitSkill{
public:
    Neo2013Muhui(): ProhibitSkill("neo2013muhui"){
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *> */) const{
        return to->hasSkill(objectName()) && (!card->isKindOf("SkillCard") || card->isKindOf("GuhuoCard") || card->isKindOf("QiceCard") || card->isKindOf("GudanCard"));
    }
};

class Neo2013MuhuiDis: public PhaseChangeSkill{
public:
    Neo2013MuhuiDis(): PhaseChangeSkill("#neo2013muhui"){
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::NotActive)
            target->getRoom()->loseMaxHp(target);
        return false;
    }
};

Neo2013XiongyiCard::Neo2013XiongyiCard(): XiongyiCard(){
    setObjectName("Neo2013XiongyiCard");
}

int Neo2013XiongyiCard::getDrawNum() const{
    return 1;
}

class Neo2013Xiongyi: public ZeroCardViewAsSkill{
public:
    Neo2013Xiongyi(): ZeroCardViewAsSkill("neo2013xiongyi"){
        frequency = Limited;
        limit_mark = "@arise";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("@arise") >= 1;
    }

    virtual const Card *viewAs() const{
        return new Neo2013XiongyiCard;
    }
};

class Neo2013Qijun: public TriggerSkill{
public:
    Neo2013Qijun(): TriggerSkill("neo2013qijun"){
        events << EventPhaseStart << Damage;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        ServerPlayer *mateng = room->findPlayerBySkillName(objectName());

        if (triggerEvent == EventPhaseStart){
            if (player->getPhase() == Player::Play){
                if (mateng == NULL || mateng->isDead() || mateng == player)
                    return false;

                if (!mateng->canDiscard(mateng, "he"))
                    return false;

                if (!room->askForDiscard(mateng, objectName(), 1, 1, true, true, "@qijun-discard"))
                    return false;

                room->setPlayerMark(player, "qijun", 1);
                room->setPlayerFlag(player, "qijun");
                room->handleAcquireDetachSkills(player, "mashu");
            }
            else if (player->getPhase() == Player::NotActive){
                if (player->getMark("qijun") < 1)
                    return false;

                room->handleAcquireDetachSkills(player, "-mashu");
                room->setPlayerMark(player, "qijun", 0);
            }
        }
        else if (player->hasFlag("qijun")){
            room->setPlayerFlag(player, "-qijun");

            if (mateng == NULL || mateng->isDead())
                return false;

            mateng->drawCards(1);

        }
        return false;
    }
};

Neo2013ZhoufuCard::Neo2013ZhoufuCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool Neo2013ZhoufuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const{
    return targets.isEmpty() &&  to_select->getPile("incantationn").isEmpty();
}

void Neo2013ZhoufuCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    target->tag["Neo2013ZhoufuSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    target->addToPile("incantationn", this);
}

class Neo2013ZhoufuViewAsSkill: public OneCardViewAsSkill {
public:
    Neo2013ZhoufuViewAsSkill(): OneCardViewAsSkill("neo2013zhoufu") {
        filter_pattern = ".|.|.|hand";
    }

    /*
    virtual bool isEnabledAtPlay(const Player *player) const{
    return !player->hasUsed("Neo2013ZhoufuCard");
    }
    */

    virtual const Card *viewAs(const Card *originalcard) const{
        Card *card = new Neo2013ZhoufuCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Neo2013Zhoufu: public TriggerSkill{
public:
    Neo2013Zhoufu(): TriggerSkill("neo2013zhoufu") {
        events << StartJudge << Death;
        view_as_skill = new Neo2013ZhoufuViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->getPile("incantationn").length() > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("incantationn").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$ZhoufuJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                judge->who->objectName(),
                QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            int id = player->getPile("incantationn").first();
            ServerPlayer *zhangbao = player->tag["Neo2013ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
            if (zhangbao && zhangbao->isAlive()){
                RecoverStruct recover;
                recover.who = zhangbao;
                room->recover(zhangbao, recover);
            }
        }
        return false;
    }
};

class Neo2013Yingbing: public TriggerSkill {
public:
    Neo2013Yingbing(): TriggerSkill("neo2013yingbing") {
        events << StartJudge;
        frequency = Frequent;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *, ServerPlayer *player, QVariant &data) const{
        JudgeStruct *judge = data.value<JudgeStruct *>();
        int id = judge->card->getEffectiveId();
        ServerPlayer *zhangbao = player->tag["Neo2013ZhoufuSource" + QString::number(id)].value<ServerPlayer *>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(objectName(), data))
            zhangbao->drawCards(2);
        return false;
    }
};

class Neo2013Yizhong: public TriggerSkill{
public:
    Neo2013Yizhong(): TriggerSkill("neo2013yizhong"){
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (player->faceUp() && effect.slash->isBlack()){
            LogMessage l;
            l.type = "#DanlaoAvoid";
            l.from = player;
            l.arg = effect.slash->objectName();
            l.arg2 = objectName();
            room->sendLog(l);

            return true;
        }
        return false;
    }
};

class Neo2013Canhui: public ProhibitSkill{ //Room::askForCardChosen()
public:
    Neo2013Canhui(): ProhibitSkill("neo2013canhui"){
    }

    virtual bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> &) const{
        return (to->hasSkill(objectName()) && !to->faceUp() && card->isRed() && card->getSkillName() != "nosguhuo");
    }
};

class Neo2013CanhuiTr: public TriggerSkill{
public:
    Neo2013CanhuiTr(): TriggerSkill("#neo2013canhui"){
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (!player->faceUp()){
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if ((move.from == player || move.to == player) && (move.from_places.contains(Player::PlaceHand) || move.to_place == Player::PlaceHand)){
                room->showAllCards(player);
            }
        }
        return false;
    }
};

class Neo2013Kunxiang: public TriggerSkill{
public:
    Neo2013Kunxiang(): TriggerSkill("neo2013kunxiang"){
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const{
        if (player->getPhase() != Player::RoundStart)
            return false;

        if (TriggerSkill::triggerable(player)){
            if (!player->askForSkillInvoke(objectName()))
                return false;

            player->turnOver();

            DummyCard dummy;
            dummy.addSubcards(player->getEquips());
            dummy.addSubcards(player->getJudgingArea());

            if (dummy.subcardsLength() > 0)
                room->moveCardTo(&dummy, player, Player::PlaceHand);

            if (player->getHandcardNum() < 7)
                room->drawCards(player, 7 - player->getHandcardNum());

            throw TurnBroken;
        }
        else {
            ServerPlayer *yujin = room->findPlayerBySkillName(objectName());
            if (yujin == NULL || yujin->isDead() || yujin->faceUp() || yujin->isNude())
                return false;

            if (!player->askForSkillInvoke(objectName()))
                return false;

            int card_id = room->askForCardChosen(player, yujin, "he", objectName());

            player->obtainCard(Sanguosha->getCard(card_id));

            if (room->askForChoice(player, objectName(), "draw+dismiss") == "draw")
                yujin->drawCards(1);
        }
        return false;
    }
};

class Neo2013Zongxuan: public TriggerSkill{
public:
    Neo2013Zongxuan(): TriggerSkill("neo2013zongxuan"){
        events << BeforeCardsMove;
    }

public:
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
            && move.to_place == Player::DiscardPile){
                QList<int> ids = move.card_ids;
                if (ids.isEmpty() || player->isNude() || !player->askForSkillInvoke(objectName(), data))
                    return false;

                while (!ids.isEmpty()){
                    room->fillAG(ids, player);
                    int id = room->askForAG(player, ids, true, objectName());
                    room->clearAG(player);
                    if (id == -1)
                        break;

                    ServerPlayer *orig_owner = room->getCardOwner(id);
                    Player::Place orig_place = room->getCardPlace(id);

                    const Card *c = room->askForExchange(player, objectName(), 1, 1, true, "@zongxuan-card", true);
                    if (c == NULL)
                        break;

                    bool isDrawPileTop = (room->askForChoice(player, objectName(), "zongxuanup+zongxuandown", QVariant::fromValue(c)) == "zongxuanup");
                    if (isDrawPileTop){
                        room->moveCardTo(c, NULL, Player::DrawPile);
                    }
                    else { // temp method for move cards to the bottom of drawpile, bugs exist probably
                        QList<int> to_move;
                        to_move << c->getEffectiveId();
                        room->moveCardsToEndOfDrawpile(to_move);
                    }

                    ids.removeOne(id);
                    move.from_places.removeAt(move.card_ids.at(id));
                    move.card_ids.removeOne(id);
                    if (room->getCardOwner(id) == orig_owner && room->getCardPlace(id) == orig_place)
                        room->obtainCard(player, id);
                }
                data = QVariant::fromValue(move);
        }
        return false;
    }
};

Neo2013JiejiCard::Neo2013JiejiCard(){
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void Neo2013JiejiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->addToPile("robbery", this, false);
}

class Neo2013JiejiVS: public OneCardViewAsSkill{
public:
    Neo2013JiejiVS(): OneCardViewAsSkill("neo2013jieji"){
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("Neo2013JiejiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013JiejiCard *jieji = new Neo2013JiejiCard;
        jieji->addSubcard(originalCard);
        return jieji;
    }
};

class Neo2013Jieji: public TriggerSkill{
public:
    Neo2013Jieji(): TriggerSkill("neo2013jieji"){
        events << CardUsed << CardResponded << JinkEffect << CardsMoveOneTime;
        view_as_skill = new Neo2013JiejiVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *selfplayer = room->findPlayerBySkillName(objectName());
        if (selfplayer == NULL || selfplayer->getPile("robbery").length() == 0)
            return false;

        QList<int> robbery = selfplayer->getPile("robbery");
        QList<int> disable, able;

        if (triggerEvent == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != NULL && use.from->hasSkill(objectName()))
                return false;

            if (use.card == NULL || (use.card->isKindOf("SkillCard") || use.card->isKindOf("EquipCard")))
                return false;
            foreach(int id, robbery){
                if (Sanguosha->getCard(id)->sameColorWith(use.card))
                    able.append(id);
                else
                    disable.append(id);
            }
            if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                return false;

            room->fillAG(robbery, selfplayer, disable);
            int card_id = room->askForAG(selfplayer, able, true, objectName());
            room->clearAG(selfplayer);
            if (card_id != -1){
                room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                room->obtainCard(selfplayer, use.card);
                use.to.clear();
                data = QVariant::fromValue(use);
            }
        }
        else if (triggerEvent == CardResponded){
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_card->isKindOf("Jink") && resp.m_isUse){
                foreach(int id, robbery){
                    if (Sanguosha->getCard(id)->sameColorWith(resp.m_card))
                        able.append(id);
                    else
                        disable.append(id);
                }
                if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                    return false;

                room->fillAG(robbery, selfplayer, disable);
                int card_id = room->askForAG(selfplayer, able, true, objectName());
                room->clearAG(selfplayer);
                if (card_id != -1){
                    room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                    player->tag["jiejijink"] = QVariant::fromValue(resp.m_card);
                    return true;
                }
            }
        }
        else if (triggerEvent == JinkEffect){
            if (player->tag["jiejijink"].value<const Card *>() == data.value<const Card *>()){
                player->tag.remove("jiejijink");
                return true;
            }
        }
        else if (triggerEvent == CardsMoveOneTime){
            if (!player->hasSkill(objectName()))
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == NULL || move.to == NULL)
                return false;

            if (move.card_ids.length() != 1 || !move.from_places.contains(Player::PlaceHand) || move.to_place != Player::PlaceEquip)
                return false;

            const Card *card = Sanguosha->getCard(move.card_ids[0]);
            if (!card->isKindOf("EquipCard"))
                return false;

            foreach(int id, robbery){
                if (Sanguosha->getCard(id)->sameColorWith(card))
                    able.append(id);
                else
                    disable.append(id);
            }
            if (able.isEmpty() || !selfplayer->askForSkillInvoke(objectName(), data))
                return false;

            room->fillAG(robbery, selfplayer, disable);
            int card_id = room->askForAG(selfplayer, able, true, objectName());
            room->clearAG(selfplayer);
            if (card_id != -1){
                room->throwCard(Sanguosha->getCard(card_id), CardMoveReason(CardMoveReason::S_REASON_PUT, QString()), NULL);
                room->obtainCard(selfplayer, card);
            }
        }

        return false;
    }
};

class Neo2013Yanyu: public TriggerSkill{
public:
    Neo2013Yanyu(): TriggerSkill("neo2013yanyu"){
        events << BeforeCardsMove << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)){
            if (room->getCurrent() == NULL || room->getCurrent() == player || room->getCurrent()->getPhase() != Player::Play)
                return false;

            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile){

                QList<int> ids = move.card_ids;

                if (player->getMark("neo2013yanyu") >= 3)
                    return false;

                while (!ids.isEmpty() && player->getMark("neo2013yanyu") < 3){
                    room->fillAG(ids, player);
                    int selected = room->askForAG(player, ids, false, objectName());
                    room->clearAG(player);

                    ids.removeOne(selected);

                    QStringList choices;
                    choices << "cancel";
                    if (!player->isNude()){
                        choices << "gain";
                        QList<int> cards = player->handCards();
                        foreach (const Card *c, player->getEquips())
                            cards << c->getEffectiveId();

                        foreach (int card_id, cards)
                            if (Sanguosha->getCard(card_id)->getType() == Sanguosha->getCard(selected)->getType()){
                                choices << "give";
                                break;
                            }
                    }
                    if (choices.length() == 1)
                        continue;

                    QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(selected));

                    if (choice != "cancel"){
                        room->setPlayerMark(player, "neo2013yanyu", player->getMark("neo2013yanyu") + 1);
                    }

                    if (choice == "cancel")
                        continue;
                    else if (choice == "gain"){
                        const Card *card = room->askForExchange(player, objectName() + "-gain", 1, true, "@neo2013yanyu-gain", false);
                        QList<int> to_move;
                        to_move << card->getEffectiveId();
                        room->moveCardsToEndOfDrawpile(to_move);
                        move.from_places.removeAt(move.card_ids.at(selected));
                        move.card_ids.removeOne(selected);
                        room->obtainCard(player, selected);
                    }
                    else if (choice == "give"){
                        QString pattern;
                        switch (Sanguosha->getCard(selected)->getTypeId()){
                        case Card::TypeBasic:
                            pattern = ".Basic";
                            break;
                        case Card::TypeEquip:
                            pattern = ".Equip";
                            break;
                        case Card::TypeTrick:
                            pattern = ".Trick";
                            break;
                        default:
                            Q_ASSERT(false);
                        }
                        const Card *card = room->askForCard(player, pattern, "@neo2013yanyu-give", QVariant(selected), Card::MethodNone, NULL, false, objectName());
                        if (card == NULL)
                            continue;
                        QString choice2 = room->askForChoice(player, objectName() + "-moveplace", "up+down", QVariant(QVariantList() << selected << card->getEffectiveId()));
                        ServerPlayer *to_give = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName() + "-give", "@neo2013yanyu-giveplayer");
                        if (choice2 == "up")
                            room->moveCardTo(card, NULL, Player::DrawPile);
                        else {
                            QList<int> to_move;
                            to_move << card->getEffectiveId();
                            room->moveCardsToEndOfDrawpile(to_move);
                        }
                        move.from_places.removeAt(move.card_ids.at(selected));
                        move.card_ids.removeOne(selected);
                        room->obtainCard(to_give, selected);
                    }
                }
                data = QVariant::fromValue(move);
            }
        }
        else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart){
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName()))
                room->setPlayerMark(p, "neo2013yanyu", 0);
        }
        return false;
    }
};

class Neo2013Jingce: public TriggerSkill{
public:
    Neo2013Jingce(): TriggerSkill("neo2013jingce"){
        frequency = Frequent;
        events << EventPhaseEnd << CardUsed << CardResponded;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if ((triggerEvent == CardUsed || triggerEvent == CardResponded) /*&& TriggerSkill::triggerable(player)*/){
            ServerPlayer *current = room->getCurrent();
            if (current == NULL || current->getPhase() != Player::Play)
                return false;

            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else
                card = data.value<CardResponseStruct>().m_card;

            if (card != NULL && !card->isKindOf("SkillCard"))
                room->setPlayerMark(player, objectName(), player->getMark(objectName()) + 1);
        }
        else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Play){
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())){
                if (p->getMark(objectName()) >= p->getHp() && p->askForSkillInvoke(objectName()))
                    p->drawCards(2);
            }
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->setPlayerMark(p, objectName(), 0);
        }
        return false;
    }
};

Neo2013JinanCard::Neo2013JinanCard(){
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool Neo2013JinanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.length() == 0 && to_select->objectName() == Self->property("neo2013jinan").toString();
}

void Neo2013JinanCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    int id = getEffectiveId();
    Player::Place cardplace = room->getCardPlace(id);
    room->moveCardTo(Sanguosha->getCard(id), effect.to, cardplace);
}

class Neo2013JinanVS: public OneCardViewAsSkill{
public:
    Neo2013JinanVS(): OneCardViewAsSkill("neo2013jinan"){
        response_pattern = "@@neo2013jinan";
    }

    virtual bool viewFilter(const Card *to_select) const{
        const Player *target = NULL;
        foreach (const Player *p, Self->getAliveSiblings()) {
            if (p->objectName() == Self->property("neo2013jinan").toString()){
                target = p;
                break;
            }
        }

        if (target == NULL)
            return false;

        if (!to_select->isEquipped())
            return true;
        else {
            const EquipCard *equip = qobject_cast<const EquipCard *>(to_select->getRealCard());
            if (target->getEquip((int)(equip->location())))
                return false;
            return true;
        }
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Neo2013JinanCard *card = new Neo2013JinanCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Neo2013Jinan: public TriggerSkill{
public:
    Neo2013Jinan(): TriggerSkill("neo2013jinan"){
        events << TargetConfirmed;
        view_as_skill = new Neo2013JinanVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, use.to){
            if (p == player)
                continue;
            if (use.card != NULL && (use.card->isKindOf("Slash") || use.card->isNDTrick())){
                room->setPlayerProperty(player, "neo2013jinan", p->objectName());
                try {
                    room->askForUseCard(player, "@@neo2013jinan", "@neo2013jinan", -1, Card::MethodNone);
                    room->setPlayerProperty(player, "neo2013jinan", QVariant());
                }
                catch (TriggerEvent errorevent){
                    if (errorevent == StageChange || errorevent == TurnBroken)
                        room->setPlayerProperty(player, "neo2013jinan", QVariant());

                    throw errorevent;
                }
            }
        }
        return false;
    }
};

class Neo2013Enyuan: public TriggerSkill{
public:
    Neo2013Enyuan(): TriggerSkill("neo2013enyuan"){
        events << HpRecover << Damaged << CardsMoveOneTime;
    }

private:
    const static int NotInvoke = 0;
    const static int En = 1;
    const static int Yuan = 2;

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *target = NULL;
        int enyuaninvoke = NotInvoke;
        int triggertimes = 0;
        switch (triggerEvent){
            case (HpRecover):{
                RecoverStruct recover = data.value<RecoverStruct>();
                if (recover.who != NULL && recover.who != player){
                    target = recover.who;
                    enyuaninvoke = En;
                    triggertimes = recover.recover;
                }
                break;
            }
            case (Damaged):{
                DamageStruct damage = data.value<DamageStruct>();
                if (damage.from != NULL && damage.from != player){
                    target = damage.from;
                    enyuaninvoke = Yuan;
                    triggertimes = damage.damage;
                }
                break;
            }
            case (CardsMoveOneTime):{
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip){
                    enyuaninvoke = NotInvoke;
                    break;
                }
                if (move.from == player && move.to != NULL && move.to != player && player->getPhase() == Player::NotActive){
                    target = (ServerPlayer *)(move.to);
                    enyuaninvoke = Yuan;
                    triggertimes = 1;
                }
                else if (move.to == player && move.from != NULL && move.from != player){
                    target = (ServerPlayer *)(move.from);
                    enyuaninvoke = En;
                    triggertimes = 1;
                }
                break;
            }
            default:
                Q_ASSERT(false);
        }
        room->setPlayerMark(player, objectName(), enyuaninvoke);
        switch (enyuaninvoke){
        case (NotInvoke):{
            break;
        }
        case (En):{
            for (int i = 0; i < triggertimes; ++i){
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target)))
                    target->drawCards(1);
            }
            break;
        }
        case (Yuan):{
            for (int i = 0; i < triggertimes; ++i){
                if (player->askForSkillInvoke(objectName(), QVariant::fromValue(target))){
                    const Card *red = room->askForCard(target, ".H", "@enyuanheart", QVariant::fromValue(player), Card::MethodNone);
                    if (red == NULL)
                        room->loseHp(target);
                    else
                        room->obtainCard(player, red);
                }
            }
            break;
        }
        default:
            Q_ASSERT(false);
        }
        room->setPlayerMark(player, objectName(), 0);
        return false;
    }
};

class Neo2013Ganglie: public MasochismSkill{
public:
    Neo2013Ganglie(): MasochismSkill("neo2013ganglie"){
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &) const{
        Room *room = player->getRoom();
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@neo2013ganglie", true, true);
        if (target != NULL){
            JudgeStruct judge;
            judge.who = player;
            judge.pattern = ".|heart";
            judge.good = false;
            room->judge(judge);

            if (judge.isGood()){
                QStringList choicelist;
                choicelist << "damage";
                if (target->getHandcardNum() > 1)
                    choicelist << "throw";
                QString choice = room->askForChoice(player, objectName(), choicelist.join("+"));
                if (choice == "damage")
                    room->damage(DamageStruct(objectName(), player, target));
                else
                    room->askForDiscard(target, objectName(), 2, 2);
            }
        }
    }
};

class Neo2013Xiezun: public MaxCardsSkill{
public:
    Neo2013Xiezun(): MaxCardsSkill("neo2013xiezun"){
    }

    virtual int getFixed(const Player *target) const{
        if (target->hasSkill(objectName())) {
            int maxcards = 0;
            QList<const Player *> players = target->getAliveSiblings();
            players << target;
            foreach(const Player *p, players){
                int maxcard = p->getMaxCards(objectName());
                if (maxcards < maxcard)
                    maxcards = maxcard;
            }

            return maxcards;
        }
        return -1;
    }
};

class Neo2013Kuangfu: public TriggerSkill{
public:
    Neo2013Kuangfu(): TriggerSkill("neo2013kuangfu"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *panfeng, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (target->getHp() <= panfeng->getHp() && target->hasEquip()) {
            QStringList equiplist;
            for (int i = 0; i <= 3; ++i) {
                if (!target->getEquip(i)) continue;
                if (panfeng->canDiscard(target, target->getEquip(i)->getEffectiveId()) || panfeng->getEquip(i) == NULL)
                    equiplist << QString::number(i);
            }
            if (equiplist.isEmpty() || !panfeng->askForSkillInvoke(objectName(), data))
                return false;
            int equip_index = room->askForChoice(panfeng, "kuangfu_equip", equiplist.join("+"), QVariant::fromValue(target)).toInt();
            const Card *card = target->getEquip(equip_index);
            int card_id = card->getEffectiveId();

            QStringList choicelist;
            if (panfeng->canDiscard(target, card_id))
                choicelist << "throw";
            if (equip_index > -1 && panfeng->getEquip(equip_index) == NULL)
                choicelist << "move";

            QString choice = room->askForChoice(panfeng, "kuangfu", choicelist.join("+"));

            if (choice == "move") {
                room->broadcastSkillInvoke(objectName(), 1);
                room->moveCardTo(card, panfeng, Player::PlaceEquip);
            } else {
                room->broadcastSkillInvoke(objectName(), 2);
                room->throwCard(card, target, panfeng);
            }
        }

        return false;
    }
};

Ling2013Package::Ling2013Package(): Package("Ling2013"){
    General *neo2013_masu = new General(this, "neo2013_masu", "shu", 3);
    neo2013_masu->addSkill(new Neo2013Xinzhan);
    neo2013_masu->addSkill(new Neo2013Huilei);

    General *neo2013_guanyu = new General(this, "neo2013_guanyu", "shu");
    neo2013_guanyu->addSkill(new Neo2013Yishi);
    neo2013_guanyu->addSkill("wusheng");

    General *neo2013_zhangfei = new General(this, "neo2013_zhangfei", "shu");
    neo2013_zhangfei->addSkill(new Neo2013Haoyin);
    neo2013_zhangfei->addSkill("paoxiao");
    neo2013_zhangfei->addSkill("tannang");

    General *neo2013_gongsun = new General(this, "neo2013_gongsunzan", "qun");
    neo2013_gongsun->addSkill(new Neo2013Zhulou);
    neo2013_gongsun->addSkill("yicong");

    General *neo2013_zhouyu = new General(this, "neo2013_zhouyu", "wu", 3);
    neo2013_zhouyu->addSkill(new Neo2013Fanjian);
    neo2013_zhouyu->addSkill(new FakeMoveSkill("Neo2013Fanjian"));
    neo2013_zhouyu->addSkill("yingzi");
    related_skills.insertMulti("neo2013fanjian", "#Neo2013Fanjian-fake-move");

    General *neo2013_sima = new General(this, "neo2013_simayi", "wei", 3);
    neo2013_sima->addSkill(new Neo2013Fankui);
    neo2013_sima->addSkill("guicai");

    General *neo2013_caocao = new General(this, "neo2013_caocao$", "wei");

    neo2013_caocao->addSkill("jianxiong");
    neo2013_caocao->addSkill("hujia");
    neo2013_caocao->addSkill(new Neo2013Xiezun);

    General *neo2013_liubei = new General(this, "neo2013_liubei$", "shu");
    neo2013_liubei->addSkill(new Neo2013Renwang);
    neo2013_liubei->addSkill("jijiang");
    neo2013_liubei->addSkill("rende");

    General *neo2013_huangzhong = new General(this, "neo2013_huangzhong", "shu", 4);
    neo2013_huangzhong->addSkill(new Neo2013Yongyi);
    neo2013_huangzhong->addSkill(new SlashNoDistanceLimitSkill("neo2013yongyi"));
    neo2013_huangzhong->addSkill("liegong");
    related_skills.insertMulti("neo2013yongyi", "#neo2013yongyi-slash-ndl");

    General *neo2013_yangxiu = new General(this, "neo2013_yangxiu", "wei", 3);
    neo2013_yangxiu->addSkill(new Neo2013Duoyi);
    neo2013_yangxiu->addSkill("jilei");
    neo2013_yangxiu->addSkill("danlao");

    General *neo2013_huatuo = new General(this, "neo2013_huatuo", "qun", 3);
    neo2013_huatuo->addSkill(new Neo2013Puji);
    neo2013_huatuo->addSkill("jijiu");

    General *neo2013_xuchu = new General(this, "neo2013_xuchu", "wei", 4);
    neo2013_xuchu->addSkill("neoluoyi");
    neo2013_xuchu->addSkill(new Neo2013Xiechan);

    General *neo2013_zhaoyun = new General(this, "neo2013_zhaoyun", "shu", 4);
    neo2013_zhaoyun->addSkill("longdan");
    neo2013_zhaoyun->addSkill("yicong");
    neo2013_zhaoyun->addSkill("jiuzhu");

    General *neo2013_gan = new General(this, "neo2013_ganfuren", "shu", 3, false);
    neo2013_gan->addSkill(new Neo2013Shushen);
    neo2013_gan->addSkill(new Neo2013Shenzhi);

    General *neo2013_guanping = new General(this, "neo2013_guanping", "shu", 4);
    neo2013_guanping->addSkill(new Neo2013Longyin);

    General *neo2013_luxun = new General(this, "neo2013_luxun", "wu", 3);
    neo2013_luxun->addSkill(new Neo2013Duoshi);
    neo2013_luxun->addSkill("qianxun");

    General *neo2013_spguanyu = new General(this, "neo2013_sp_guanyu", "wei", 4);
    neo2013_spguanyu->addSkill(new Neo2013Danji);
    neo2013_spguanyu->addSkill("wusheng");
    neo2013_spguanyu->addRelateSkill("neo2013huwei");

    General *neo2013_zoushi = new General(this, "neo2013_zoushi", "qun", 3, false);
    neo2013_zoushi->addSkill(new Neo2013Huoshui);
    neo2013_zoushi->addSkill(new Neo2013Qingcheng);

    General *neo2013_caochong = new General(this, "neo2013_caochong", "wei", 3);
    neo2013_caochong->addSkill(new Neo2013Chengxiang);
    neo2013_caochong->addSkill("renxin");

    General *neo2013_lvmeng = new General(this, "neo2013_lvmeng", "wu", 5);
    neo2013_lvmeng->addSkill("keji");
    neo2013_lvmeng->addSkill(new Neo2013Xiangxue);
    neo2013_lvmeng->addRelateSkill("neo2013tongwu");
    neo2013_lvmeng->addRelateSkill("neo2013bingyin");
    neo2013_lvmeng->addRelateSkill("neo2013touxi");
    neo2013_lvmeng->addRelateSkill("#neo2013touxi");
    neo2013_lvmeng->addRelateSkill("neo2013muhui");
    neo2013_lvmeng->addRelateSkill("#neo2013muhui");
    related_skills.insertMulti("neo2013touxi", "#neo2013touxi");
    related_skills.insertMulti("neo2013muhui", "#neo2013muhui");

    General *neo2013_mateng = new General(this, "neo2013_mateng", "qun", 4);
    neo2013_mateng->addSkill("mashu");
    neo2013_mateng->addSkill(new Neo2013Xiongyi);
    neo2013_mateng->addSkill(new Neo2013Qijun);

    General *neo2013_zhangbao = new General(this, "neo2013_zhangbao", "qun", 3);
    neo2013_zhangbao->addSkill(new Neo2013Zhoufu);
    neo2013_zhangbao->addSkill(new Neo2013Yingbing);

    General *neo2013_yujin = new General(this, "neo2013_yujin", "wei", 4);
    neo2013_yujin->addSkill(new Neo2013Yizhong);
    neo2013_yujin->addSkill(new Neo2013Canhui);
    neo2013_yujin->addSkill(new Neo2013CanhuiTr);
    neo2013_yujin->addSkill(new Neo2013Kunxiang);
    related_skills.insertMulti("neo2013canhui", "#neo2013canhui");

    General *neo2013_yufan = new General(this, "neo2013_yufan", "wu", 3);
    neo2013_yufan->addSkill(new Neo2013Zongxuan);
    neo2013_yufan->addSkill("zhiyan");

    General *neo2013_panzmaz = new General(this, "neo2013_panzhangmazhong", "wu", 4);
    neo2013_panzmaz->addSkill(new Neo2013Jieji);
    neo2013_panzmaz->addSkill("anjian");

    General *neo2013_xiahoushi = new General(this, "neo2013_xiahoushi", "shu", 3, false);
    neo2013_xiahoushi->addSkill(new Neo2013Yanyu);
    neo2013_xiahoushi->addSkill("xiaode");

    General *neo2013_guohuai = new General(this, "neo2013_guohuai", "wei", 3);
    neo2013_guohuai->addSkill(new Neo2013Jingce);
    neo2013_guohuai->addSkill(new Neo2013Jinan);

    General *neo2013_fazheng = new General(this, "neo2013_fazheng", "shu", 3);
    neo2013_fazheng->addSkill("nosxuanhuo");
    neo2013_fazheng->addSkill(new Neo2013Enyuan);

    General *neo2013_xiahou = new General(this, "neo2013_xiahoudun", "wei", 4);
    neo2013_xiahou->addSkill(new Neo2013Ganglie);

    General *neo2013_panfeng = new General(this, "neo2013_panfeng", "qun", 4);
    neo2013_panfeng->addSkill(new Neo2013Kuangfu);

    addMetaObject<Neo2013XinzhanCard>();
    addMetaObject<Neo2013FanjianCard>();
    addMetaObject<Neo2013YongyiCard>();
    addMetaObject<Neo2013XiongyiCard>();
    addMetaObject<Neo2013ZhoufuCard>();
    addMetaObject<Neo2013JiejiCard>();
    addMetaObject<Neo2013JinanCard>();
    addMetaObject<Neo2013PujiCard>();
    addMetaObject<Neo2013XiechanCard>();

    skills << new Neo2013HuileiDecrease << new Neo2013Huwei
        << new Neo2013Tongwu << new Neo2013Bingyin << new Neo2013Touxi
        << new Neo2013Muhui << new Neo2013MuhuiDis;
}

ADD_PACKAGE(Ling2013)
