#include "lingex.h"
#include "general.h"
#include "standard.h"
#include "client.h"
#include "engine.h"

LuoyiCard::LuoyiCard() {
    target_fixed = true;
}

void LuoyiCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &) const{
    source->setFlags("neoluoyi");
}

NeoFanjianCard::NeoFanjianCard() {
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NeoFanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    const Card *card = Sanguosha->getCard(getSubcards().first());
    int card_id = card->getEffectiveId();
    Card::Suit suit = room->askForSuit(target, objectName() == "NeoFanjianCard" ? "neofanjian" : "neo2013fanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->getThread()->delay();
    target->obtainCard(this);
    room->showCard(target, card_id);

    if (card->getSuit() != suit)
        DifferentEffect(room, zhouyu, target);
}

void NeoFanjianCard::DifferentEffect(Room *room, ServerPlayer *from, ServerPlayer *to) const{
    room->damage(DamageStruct(objectName(), from, to));
}

class NeoGanglie: public MasochismSkill {
public:
    NeoGanglie(): MasochismSkill("neoganglie") {
    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant data = QVariant::fromValue(damage);

        if (room->askForSkillInvoke(xiahou, "neoganglie", data)) {
            room->broadcastSkillInvoke("ganglie");

            JudgeStruct judge;
            judge.pattern = ".|heart";
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if (!from || from->isDead()) return;
            if (judge.isGood()) {
                QStringList choicelist;
                choicelist << "damage";
                if (from->getHandcardNum() > 1)
                    choicelist << "throw";
                QString choice = room->askForChoice(xiahou, objectName(), choicelist.join("+"));
                if (choice == "damage")
                    room->damage(DamageStruct(objectName(), xiahou, from));
                else
                    room->askForDiscard(from, objectName(), 2, 2);
            }
        }
    }
};

class NeoLuoyi: public OneCardViewAsSkill {
public:
    NeoLuoyi(): OneCardViewAsSkill("neoluoyi") {
        filter_pattern = "EquipCard!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("LuoyiCard") && player->canDiscard(player, "he");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        LuoyiCard *card = new LuoyiCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class NeoLuoyiBuff: public TriggerSkill {
public:
    NeoLuoyiBuff(): TriggerSkill("#neoluoyi") {
        events << DamageCaused;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasFlag("neoluoyi") && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.chain || damage.transfer || !damage.by_user) return false;
        const Card *reason = damage.card;
        if (reason && (reason->isKindOf("Slash") || reason->isKindOf("Duel"))) {
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

#include "wind.h"
class NeoJushou: public Jushou {
public:
    NeoJushou(): Jushou() {
        setObjectName("neojushou");
    }

    virtual int getJushouDrawNum(ServerPlayer *caoren) const{
        return 2 + caoren->getLostHp();
    }
};

class NeoFanjian: public OneCardViewAsSkill {
public:
    NeoFanjian(): OneCardViewAsSkill("neofanjian") {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && !player->hasUsed("NeoFanjianCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Card *card = new NeoFanjianCard;
        card->addSubcard(originalCard);
        return card;
    }
};

Zhulou::Zhulou() : PhaseChangeSkill("zhulou")
{
}

QString Zhulou::getAskForCardPattern() const
{
    return ".Weapon";
}

bool Zhulou::onPhaseChange(ServerPlayer *gongsun) const
{
    Room *room = gongsun->getRoom();
    if (gongsun->getPhase() == Player::Finish && gongsun->askForSkillInvoke(objectName())) {
        gongsun->drawCards(2);
        room->broadcastSkillInvoke("zhulou");
        if (!room->askForCard(gongsun, getAskForCardPattern(), "@zhulou-discard"))
            room->loseHp(gongsun);
    }
    return false;
}

LingExPackage::LingExPackage()
    : Package("LingEx")
{
    General *neo_xiahoudun = new General(this, "neo_xiahoudun", "wei");
    neo_xiahoudun->addSkill(new NeoGanglie);

    General *neo_xuchu = new General(this, "neo_xuchu", "wei");
    neo_xuchu->addSkill(new NeoLuoyi);
    neo_xuchu->addSkill(new NeoLuoyiBuff);
    related_skills.insertMulti("neoluoyi", "#neoluoyi");

    General *neo_caoren = new General(this, "neo_caoren", "wei");
    neo_caoren->addSkill(new NeoJushou);

    General *neo_zhouyu = new General(this, "neo_zhouyu", "wu", 3);
    neo_zhouyu->addSkill("yingzi");
    neo_zhouyu->addSkill(new NeoFanjian);

    General *neo_gongsunzan = new General(this, "neo_gongsunzan", "qun");
    neo_gongsunzan->addSkill(new Zhulou);
    neo_gongsunzan->addSkill("yicong");

    addMetaObject<LuoyiCard>();
    addMetaObject<NeoFanjianCard>();
}

ADD_PACKAGE(LingEx)
