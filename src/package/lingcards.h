#ifndef _LINGCARDS_H
#define _LINGCARDS_H

#include "package.h"
#include "card.h"

#include "standard.h"

class LingCardsPackage: public Package{
    Q_OBJECT

public:
    LingCardsPackage();
};

class AwaitExhausted: public TrickCard{
    Q_OBJECT

public:
    Q_INVOKABLE AwaitExhausted(Card::Suit suit, int number);

    virtual QString getSubtype() const;

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class BefriendAttacking: public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE BefriendAttacking(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class KnownBoth: public SingleTargetTrick{
    Q_OBJECT

public:
    Q_INVOKABLE KnownBoth(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NeoDrowning: public AOE{
    Q_OBJECT

public:
    Q_INVOKABLE NeoDrowning(Card::Suit suit, int number);
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SixSwords: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE SixSwords(Card::Suit suit, int number);
};

class SixSwordsCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE SixSwordsCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Triblade: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE Triblade(Card::Suit suit, int number);
};

class TribladeCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE TribladeCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class DragonPhoenix: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE DragonPhoenix(Card::Suit suit, int number);
};

class PeaceSpell: public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE PeaceSpell(Card::Suit suit, int number);
    virtual void onUninstall(ServerPlayer *player) const;
};

class PeaceSpellCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE PeaceSpellCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif
