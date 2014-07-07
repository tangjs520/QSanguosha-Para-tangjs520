#ifndef _LINGEX_H
#define _LINGEX_H

#include "package.h"
#include "card.h"

#include "skill.h"

class LingExPackage: public Package {
    Q_OBJECT

public:
    LingExPackage();
};

class LuoyiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE LuoyiCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NeoFanjianCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE NeoFanjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual void DifferentEffect(Room *, ServerPlayer *, ServerPlayer *) const;
};

class Zhulou: public PhaseChangeSkill {
public:
    Zhulou();
    virtual QString getAskForCardPattern() const;
    virtual bool onPhaseChange(ServerPlayer *gongsun) const;
};

#endif
