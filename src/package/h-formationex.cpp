#include "h-formationex.h"
#include "general.h"
#include "standard.h"
#include "client.h"
#include "engine.h"

class Zhangwu : public TriggerSkill {
public:
    Zhangwu(): TriggerSkill("zhangwu"){
        events << CardsMoveOneTime << BeforeCardsMove;
    }

public:
    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place == Player::DrawPile)
            return false;
        int fldfid = -1;
        foreach (int id, move.card_ids)
            if (Sanguosha->getCard(id)->isKindOf("DragonPhoenix")){
                fldfid = id;
                break;
            }

            if (fldfid == -1)
                return false;

            if (triggerEvent == CardsMoveOneTime){
                if (move.to_place == Player::DiscardPile || (move.to_place == Player::PlaceEquip && move.to != player))
                    player->obtainCard(Sanguosha->getCard(fldfid));
            }
            else {
                if ((move.from == player && (move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceHand || move.from_places[move.card_ids.indexOf(fldfid)] == Player::PlaceEquip))
                    && (move.to != player || (move.to_place != Player::PlaceHand && move.to_place != Player::PlaceEquip && move.to_place != Player::DrawPile))) {
                        room->showCard(player, fldfid);
                        move.from_places.removeAt(move.card_ids.indexOf(fldfid));
                        move.card_ids.removeOne(fldfid);
                        data = QVariant::fromValue(move);
                        QList<int> to_move;
                        to_move << fldfid;
                        room->moveCardsToEndOfDrawpile(to_move);
                        room->drawCards(player, 2);
                }
            }
            return false;
    }
};

class ShouYue : public AttackRangeSkill {
public:
    ShouYue(): AttackRangeSkill("shouyue$"){
    }

    virtual int getExtra(const Player *target, bool) const{
        QList<const Player *> players = target->getAliveSiblings();
        foreach(const Player *p, players){
            if (p->hasLordSkill(objectName()) && target->getKingdom() == "shu")
                return 1;
        }
        return 0;
    }
};

class Jizhao : public TriggerSkill {
public:
    Jizhao(): TriggerSkill("jizhao"){
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@jizhao";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark(limit_mark) > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.who != player)
            return false;

        if (player->askForSkillInvoke(objectName(), data)){
            player->loseMark(limit_mark);
            if (player->getHandcardNum() < player->getMaxHp())
                room->drawCards(player, player->getMaxHp() - player->getHandcardNum());

            if (player->getHp() < 2){
                RecoverStruct rec;
                rec.recover = 2 - player->getHp();
                rec.who = player;
                room->recover(player, rec);
            }

            room->handleAcquireDetachSkills(player, "-shouyue|rende|neo2013renwang");
            if (player->isLord())
                room->handleAcquireDetachSkills(player, "jijiang");
        }
        return false;
    }
};

HFormationExPackage::HFormationExPackage()
    : Package("h_formationex")
{
    General *heg_liubei = new General(this, "heg_liubei$", "shu", 4);
    heg_liubei->addSkill(new Zhangwu);
    heg_liubei->addSkill(new ShouYue);
    heg_liubei->addSkill(new Jizhao);
}

ADD_PACKAGE(HFormationEx)
