#include "h-momentumex.h"
#include "general.h"
#include "standard.h"
#include "client.h"
#include "engine.h"

WuxinCard::WuxinCard(){
    mute = true;
}

bool WuxinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    foreach(int id, Self->getPile("skysoldier")){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(Self) && theslash->targetFilter(targets, to_select, Self) && !Sanguosha->isProhibited(Self, to_select, theslash);
            if (can_slash)
                return true;
        }
    }
    return false;
}

bool WuxinCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    foreach(int id, Self->getPile("skysoldier")){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(Self) && theslash->targetsFeasible(targets, Self);
            if (can_slash)
                return true;
        }
    }
    return false;
}

QList<const Player *> WuxinCard::ServerPlayerList2PlayerList(QList<ServerPlayer *> thelist){
    QList<const Player *> targetlist;
    foreach (ServerPlayer *p, thelist){
        targetlist << (const Player *)p;
    }
    return targetlist;
}

const Card *WuxinCard::validate(CardUseStruct &cardUse) const{
    QList<int> skysoldier = cardUse.from->getPile("skysoldier");
    QList<int> black_skysoldier, disabled_skysoldier;
    foreach(int id, skysoldier){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("wuxin");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(cardUse.from) &&
                theslash->targetsFeasible(ServerPlayerList2PlayerList(cardUse.to), cardUse.from);
            if (can_slash)
                black_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        else
            disabled_skysoldier << id;
    }

    if (black_skysoldier.isEmpty())
        return NULL;

    Room *room = cardUse.from->getRoom();
    room->fillAG(skysoldier, cardUse.from, disabled_skysoldier);
    int slash_id = room->askForAG(cardUse.from, black_skysoldier, false, "wuxin");
    room->clearAG(cardUse.from);

    Slash *slash = new Slash(Card::SuitToBeDecided, -1);
    slash->addSubcard(slash_id);
    slash->setSkillName("wuxin");
    return slash;
}

class WuxinVS : public ZeroCardViewAsSkill {
public:
    WuxinVS(): ZeroCardViewAsSkill("wuxin"){

    }

    virtual const Card *viewAs() const{
        return new WuxinCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }
};

class Wuxin : public PhaseChangeSkill {
public:
    Wuxin(): PhaseChangeSkill("wuxin"){
        view_as_skill = new WuxinVS;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if (target->getPhase() == Player::Draw){
            Room *room = target->getRoom();

            int qunplayers = 0;
            foreach(ServerPlayer *p, room->getAlivePlayers())
                if (p->getKingdom() == "qun")
                    ++qunplayers;

            if (qunplayers <= 1)
                return false;

            if (qunplayers > 0 && target->askForSkillInvoke(objectName())){
                QList<int> guanxing_cards = room->getNCards(qunplayers);
                room->askForGuanxing(target, guanxing_cards, Room::GuanxingUpOnly);
            }

            if (target->getPile("skysoldier").length() == 0){
                Room *room = target->getRoom();

                int qunplayers = 0;
                foreach(ServerPlayer *p, room->getAlivePlayers())
                    if (p->getKingdom() == "qun")
                        ++qunplayers;

                if (qunplayers == 0)
                    return false;

                QList<int> skill2cards = room->getNCards(qunplayers);
                CardMoveReason reason(CardMoveReason::S_REASON_TURNOVER, target->objectName(), objectName(), QString());
                CardsMoveStruct move(skill2cards, NULL, Player::PlaceTable, reason);
                room->moveCardsAtomic(move, true);
                room->getThread()->delay();
                room->getThread()->delay();

                target->addToPile("skysoldier", skill2cards, true);

            }

        }
        return false;
    }
};

class WuxinPreventLoseHp : public TriggerSkill {
public:
    WuxinPreventLoseHp(): TriggerSkill("#wuxin-prevent"){
        events << PreDamageDone << PreHpLost;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QList<int> skysoldier = player->getPile("skysoldier");
        QList<int> red_skysoldier, disabled_skysoldier;
        foreach(int id, skysoldier){
            if (Sanguosha->getCard(id)->isRed())
                red_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        //todo:data
        if (!red_skysoldier.isEmpty() && player->askForSkillInvoke("wuxin", data)){
            room->fillAG(skysoldier, player, disabled_skysoldier);
            int id = room->askForAG(player, red_skysoldier, false, "wuxin");
            room->clearAG(player);

            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, player->objectName(), "wuxin", QString());
            room->moveCardTo(Sanguosha->getCard(id), NULL, Player::DiscardPile, reason, true);
            if (triggerEvent == PreDamageDone){
                DamageStruct damage = data.value<DamageStruct>();
                damage.damage = 0;
                data = QVariant::fromValue(damage);
            }
            else
                data = 0;
        }
        return false;
    }
};

class WuxinSlashResponse : public TriggerSkill {
public:
    WuxinSlashResponse(): TriggerSkill("#wuxin-slashresponse"){
        events << CardAsked;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QStringList ask = data.toStringList();
        if (ask.first() == "slash"){
            QList<int> skysoldier = player->getPile("skysoldier");
            QList<int> black_skysoldier, disabled_skysoldier;
            foreach(int id, skysoldier){
                if (Sanguosha->getCard(id)->isBlack())
                    black_skysoldier << id;
                else
                    disabled_skysoldier << id;
            }

            if (black_skysoldier.isEmpty())
                return false;
            //data
            if (player->askForSkillInvoke("wuxin", data)){
                room->fillAG(skysoldier, player, disabled_skysoldier);
                int slash_id = room->askForAG(player, black_skysoldier, false, "wuxin");
                room->clearAG(player);

                Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                slash->addSubcard(slash_id);
                slash->setSkillName("wuxin");
                room->provide(slash);
                return true;
            }
        }
        return false;
    }
};

WendaoCard::WendaoCard(){
    target_fixed = true;
}

void WendaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const{
    const Card *tpys = NULL;
    foreach(ServerPlayer *p, room->getAlivePlayers()){
        foreach(const Card *card, p->getEquips()){
            if (Sanguosha->getEngineCard(card->getEffectiveId())->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(card->getEffectiveId());
                break;
            }
        }
        if (tpys != NULL)
            break;
        foreach(const Card *card, p->getJudgingArea()){
            if (Sanguosha->getEngineCard(card->getEffectiveId())->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(card->getEffectiveId());
                break;
            }
        }
        if (tpys != NULL)
            break;
    }
    if (tpys == NULL)
        foreach(int id, room->getDiscardPile()){
            if (Sanguosha->getEngineCard(id)->isKindOf("PeaceSpell")){
                tpys = Sanguosha->getCard(id);
                break;
            }
    }

    if (tpys == NULL)
        return;

    source->obtainCard(tpys, true);
}

class Wendao : public OneCardViewAsSkill {
public:
    Wendao(): OneCardViewAsSkill("wendao"){
        filter_pattern = ".|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("WendaoCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        WendaoCard *c = new WendaoCard;
        c->addSubcard(originalCard);
        return c;
    }
};

HongfaCard::HongfaCard(){
    mute = true;
    m_skillName = "hongfa_slash";
}

bool HongfaCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if (targets.length() == 0)
        return to_select->hasLordSkill("hongfa") && !to_select->getPile("skysoldier").isEmpty() && to_select != Self;
    else if (targets.length() >= 1){
        if (!targets[0]->hasLordSkill("hongfa") || targets[0]->getPile("skysoldier").isEmpty())
            return false;

        QList<int> skysoldiers = targets[0]->getPile("skysoldier");

        QList<const Player *> fixed_targets = targets;
        fixed_targets.removeOne(targets[0]);

        foreach (int id, skysoldiers){
            if (Sanguosha->getCard(id)->isBlack()){
                Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
                theslash->setSkillName("hongfa");
                theslash->addSubcard(id);
                theslash->deleteLater();
                bool can_slash = theslash->isAvailable(Self) && theslash->targetFilter(fixed_targets, to_select, Self) && !Sanguosha->isProhibited(Self, to_select, theslash);
                if (can_slash)
                    return true;
            }
        }
        return false;
    }
    return false;
}

bool HongfaCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    if (targets.length() >= 2 && targets[0]->hasLordSkill("hongfa") && !targets[0]->getPile("skysoldier").isEmpty()){

        QList<int> skysoldiers = targets[0]->getPile("skysoldier");

        QList<const Player *> fixed_targets = targets;
        fixed_targets.removeOne(targets[0]);

        foreach (int id, skysoldiers){
            if (Sanguosha->getCard(id)->isBlack()){
                Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
                theslash->setSkillName("hongfa");
                theslash->addSubcard(id);
                theslash->deleteLater();
                bool can_slash = theslash->isAvailable(Self) && theslash->targetsFeasible(fixed_targets, Self);
                if (can_slash)
                    return true;
            }
        }
        return false;
    }
    return false;
}

const Card *HongfaCard::validate(CardUseStruct &cardUse) const{
    if (!cardUse.to.first()->hasLordSkill("hongfa") || cardUse.to.first()->getPile("skysoldier").isEmpty())
        return NULL;
    Room *room = cardUse.from->getRoom();
    ServerPlayer *slasher = cardUse.to.first();
    cardUse.to.removeOne(slasher);
    QList<int> skysoldier = slasher->getPile("skysoldier");
    QList<int> black_skysoldier, disabled_skysoldier;
    foreach(int id, skysoldier){
        if (Sanguosha->getCard(id)->isBlack()){
            Slash *theslash = new Slash(Card::SuitToBeDecided, -1);
            theslash->setSkillName("hongfa");
            theslash->addSubcard(id);
            theslash->deleteLater();
            bool can_slash = theslash->isAvailable(cardUse.from) &&
                theslash->targetsFeasible(WuxinCard::ServerPlayerList2PlayerList(cardUse.to), cardUse.from);
            if (can_slash)
                black_skysoldier << id;
            else
                disabled_skysoldier << id;
        }
        else
            disabled_skysoldier << id;
    }

    if (black_skysoldier.isEmpty())
        return NULL;

    room->fillAG(skysoldier, cardUse.from, disabled_skysoldier);
    int slash_id = room->askForAG(cardUse.from, black_skysoldier, false, "hongfa");
    room->clearAG(cardUse.from);

    room->showCard(slasher, slash_id);
    bool can_use = room->askForSkillInvoke(slasher, "hongfa_slash", "letslash:" + cardUse.from->objectName());
    if (can_use){
        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
        slash->addSubcard(slash_id);
        slash->setSkillName("hongfa");
        return slash;
    }
    return NULL;
}

class HongfaSlashVS : public ZeroCardViewAsSkill {
public:
    HongfaSlashVS(): ZeroCardViewAsSkill("hongfa_slash"){
        attached_lord_skill = true;
    }

    virtual const Card *viewAs() const{
        return new HongfaCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "slash" && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
    }
};

class HongfaSlash : public TriggerSkill {
public:
    HongfaSlash(): TriggerSkill("hongfa_slash"){
        view_as_skill = new HongfaSlashVS;
        events << CardAsked;
        attached_lord_skill = true;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        QStringList ask = data.toStringList();
        if (ask.first() == "slash"){

            QList<ServerPlayer *> skysoldiers;
            foreach(ServerPlayer *p, room->getOtherPlayers(player)){
                QList<int> skysoldier = p->getPile("skysoldier");
                QList<int> black_skysoldier, disabled_skysoldier;
                foreach(int id, skysoldier){
                    if (Sanguosha->getCard(id)->isBlack())
                        black_skysoldier << id;
                    else
                        disabled_skysoldier << id;
                }

                if (!black_skysoldier.isEmpty())
                    skysoldiers << p;
            }
            //data
            if (!skysoldiers.isEmpty()){
                ServerPlayer *zhangjiao = room->askForPlayerChosen(player, skysoldiers, objectName(), "@hongfa-response", true);
                if (zhangjiao != NULL){
                    QList<int> skysoldier = zhangjiao->getPile("skysoldier");
                    QList<int> black_skysoldier, disabled_skysoldier;
                    foreach(int id, skysoldier){
                        if (Sanguosha->getCard(id)->isBlack())
                            black_skysoldier << id;
                        else
                            disabled_skysoldier << id;
                    }

                    room->fillAG(skysoldier, player, disabled_skysoldier);
                    int slash_id = room->askForAG(player, black_skysoldier, false, "hongfa");
                    room->clearAG(player);

                    room->showCard(zhangjiao, slash_id);
                    bool can_use = room->askForSkillInvoke(zhangjiao, "hongfa_slash", "letslash:" + player->objectName());
                    if (can_use){
                        Slash *slash = new Slash(Card::SuitToBeDecided, -1);
                        slash->addSubcard(slash_id);
                        slash->setSkillName("hongfa");
                        room->provide(slash);
                        return true;
                    }
                }
            }
        }
        return false;
    }
};

class Hongfa : public TriggerSkill {
public:
    Hongfa(): TriggerSkill("hongfa$"){
        events << GameStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill(objectName()) && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const{
        foreach(ServerPlayer *p, room->getAllPlayers()){
            room->attachSkillToPlayer(p, "hongfa_slash");
        }
        return false;
    }
};

HMomentumExPackage::HMomentumExPackage()
    : Package("h_momentumex")
{
    General *zhangjiao = new General(this, "heg_zhangjiao$", "qun", 3);
    zhangjiao->addSkill(new Wuxin);
    zhangjiao->addSkill(new WuxinPreventLoseHp);
    zhangjiao->addSkill(new WuxinSlashResponse);
    related_skills.insertMulti("wuxin", "#wuxin-prevent");
    related_skills.insertMulti("wuxin", "#wuxin-slashresponse");
    zhangjiao->addSkill(new Wendao);
    zhangjiao->addSkill(new Hongfa);
    skills << new HongfaSlash;

    addMetaObject<WuxinCard>();
    addMetaObject<WendaoCard>();
}

ADD_PACKAGE(HMomentumEx)
