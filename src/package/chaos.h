#ifndef _CHAOS_H
#define _CHAOS_H

#include "package.h"
#include "skill.h"

class ChaosPackage: public Package {
    Q_OBJECT

public:
    ChaosPackage();
};

class Fentian: public PhaseChangeSkill {
public:
    Fentian();
    virtual bool onPhaseChange(ServerPlayer *hanba) const;
};

class Zhiri: public PhaseChangeSkill {
public:
    Zhiri();
    virtual bool onPhaseChange(ServerPlayer *hanba) const;
};

class XintanCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE XintanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

#endif
