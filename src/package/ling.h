#ifndef _LING_H
#define _LING_H

#include "package.h"
#include "card.h"
#include "skill.h"

class LingPackage : public Package
{
    Q_OBJECT

public:
    LingPackage();
};

class Yishi : public TriggerSkill
{
public:
    Yishi();
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const;
};

#endif
