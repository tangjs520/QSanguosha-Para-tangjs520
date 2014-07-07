#ifndef _H_MOMENTUMEX_H
#define _H_MOMENTUMEX_H

#include "package.h"
#include "card.h"

class WuxinCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE WuxinCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
    static QList<const Player *> ServerPlayerList2PlayerList(QList<ServerPlayer *> thelist);
};

class WendaoCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE WendaoCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HongfaCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE HongfaCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class HMomentumExPackage: public Package {
    Q_OBJECT

public:
    HMomentumExPackage();
};

#endif
