#ifndef _LING2013_H
#define _LING2013_H

#include "package.h"
#include "card.h"

#include "yjcm.h"
#include "lingex.h"
#include "hegemony.h"

class Ling2013Package: public Package {
    Q_OBJECT

public:
    Ling2013Package();
};

class Neo2013XinzhanCard: public XinzhanCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XinzhanCard();
};

class Neo2013FanjianCard: public NeoFanjianCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013FanjianCard();
    virtual void DifferentEffect(Room *, ServerPlayer *, ServerPlayer *) const;
};

class Neo2013PujiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013PujiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013XiechanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XiechanCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Neo2013ZhoufuCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013ZhoufuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013XiongyiCard: public XiongyiCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013XiongyiCard();
    virtual int getDrawNum() const;
};

class Neo2013JiejiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013JiejiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Neo2013JinanCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013JinanCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Neo2013YongyiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE Neo2013YongyiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

#endif
