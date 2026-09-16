/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoATriggers.h"
#include "Creature.h"
#include "EventMap.h"
#include "Object.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SpellAuras.h"

bool EmalonMarkBossTrigger::IsActive()
{
    // Only tank bot can mark target
    if (!botAI->IsTank(bot))
    {
        return false;
    }

    // Check boss and it is alive
    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    // Check if boss have skull mark
    Group* group = bot->GetGroup();
    int8 skullIndex = 7;  // Skull
    ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);
    if (currentSkullTarget == boss->GetGUID())
    {
        return false;
    }

    // Check if there is any overcharged minion
    Unit* overchargedMinion = nullptr;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit)
            continue;

        uint32 entry = unit->GetEntry();
        if (entry == NPC_TEMPEST_MINION && unit->HasAura(AURA_OVERCHARGE))
        {
            overchargedMinion = unit;
            break;
        }
    }
    if (overchargedMinion)
    {
        return false;
    }

    return true;
}

bool EmalonLightingNovaTrigger::IsActive()
{
    // Check boss and it is alive
    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    // Tank dont need to move
    if (botAI->IsTank(bot))
    {
        return false;
    }

    // Check if boss is casting Lightning Nova
    bool isCasting = boss->HasUnitState(UNIT_STATE_CASTING);
    bool isLightingNova = boss->FindCurrentSpellBySpellId(SPELL_LIGHTNING_NOVA_10_MAN) ||
                          boss->FindCurrentSpellBySpellId(SPELL_LIGHTNING_NOVA_25_MAN);
    return isCasting && isLightingNova;
}

bool EmalonOverchargeTrigger::IsActive()
{
    // Only tank bot can mark target
    if (!botAI->IsTank(bot))
    {
        return false;
    }

    // Check boss and it is alive
    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    // Check if there is any overcharged minion
    Unit* overchargedMinion = nullptr;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit)
            continue;

        uint32 entry = unit->GetEntry();
        if (entry == NPC_TEMPEST_MINION && unit->HasAura(AURA_OVERCHARGE))
        {
            overchargedMinion = unit;
            break;
        }
    }
    if (!overchargedMinion)
    {
        return false;
    }

    // Check if minion have skull mark
    Group* group = bot->GetGroup();
    int8 skullIndex = 7;  // Skull
    ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);
    if (currentSkullTarget == overchargedMinion->GetGUID())
    {
        return false;
    }

    return true;
}

bool EmalonFallFromFloorTrigger::IsActive()
{
    // Check boss and it is alive
    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    // Check if bot is on the floor
    return bot->GetPositionZ() < 80.0f;
}

//
// Toravon the Ice Watcher
//

// Distance that covers the Vault of Archavon arena: Toravon, his Frozen Orbs and both
// tanks are always inside it, and nothing outside the encounter resolves.
static constexpr float TORAVON_RANGE = 60.0f;

// Frostbite stacks currently on `unit`, or 0 when the debuff is absent.
// Looked up by spell id: the Spell.dbc shipped with this build has its name fields
// zeroed, so PlayerbotAI::GetAura(name, ...) - which resolves names through the
// "spell id" value - is not dependable here.
static int ToravonFrostbiteStacks(Unit* unit)
{
    if (!unit)
        return 0;

    Aura* aura = unit->GetAura(SPELL_TORAVON_FROSTBITE);
    return aura ? static_cast<int>(aura->GetStackAmount()) : 0;
}

bool ToravonFrostbiteSwapTrigger::IsActive()
{
    // Bot tanks only. A real player tank is never driven by this trigger; a bot
    // off-tank may still take over from a human tank below.
    if (!GET_PLAYERBOT_AI(bot) || !botAI->IsTank(bot))
        return false;

    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, TORAVON_RANGE);
    if (!toravon || !toravon->IsAlive() || !toravon->IsInCombat())
        return false;

    // Tank swaps are driven from the off-tank side only, and the current holder is
    // read from the boss itself rather than from a "main tank" label.
    Unit* victim = toravon->GetVictim();
    if (!victim || victim == bot || !victim->IsAlive())
        return false;

    // Swap only because the tank actually holding Toravon is stacked too high...
    if (ToravonFrostbiteStacks(victim) < TORAVON_FROSTBITE_SWAP_STACKS)
        return false;

    // ...and only when this bot is below the threshold itself.
    if (ToravonFrostbiteStacks(bot) >= TORAVON_FROSTBITE_SWAP_STACKS)
        return false;

    // A single-tank raid never reaches this point: the only tank is the victim, so
    // there is no taunt loop and no AttackStop behaviour in that case.
    return true;
}

bool ToravonFrozenOrbTrigger::IsActive()
{
    // All non-tank, non-healer DPS assist on the orb: the active tank keeps Toravon and
    // healers keep healing, while both melee and ranged DPS switch to the Frozen Orb.
    if (!GET_PLAYERBOT_AI(bot) || botAI->IsTank(bot) || botAI->IsHeal(bot))
        return false;

    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, TORAVON_RANGE);
    if (!toravon || !toravon->IsAlive() || !toravon->IsInCombat())
        return false;

    Creature* orb = bot->FindNearestCreature(NPC_FROZEN_ORB, TORAVON_RANGE);
    if (!orb || !orb->IsAlive() || orb->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;

    // 25-man spawns three orbs per wave. Every DPS bot resolves the group's Skull orb,
    // which is claimed by the first bot that finds Skull unusable, so the whole raid
    // focuses one orb at a time instead of splitting across three. The advance to the
    // next orb and the release back to Toravon are driven by ToravonMarkSkullTrigger,
    // which keeps running after the last orb dies and this trigger is already false.
    return true;
}

bool ToravonMarkSkullTrigger::IsActive()
{
    // Driven by a bot tank on purpose: right role, always present during the encounter,
    // and unaffected by the DPS-only gate on the orb trigger above.
    if (!GET_PLAYERBOT_AI(bot) || !botAI->IsTank(bot))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    ObjectGuid const skull = group->GetTargetIcon(RtiTargetValue::skullIndex);
    if (skull.IsEmpty())
        return false;

    // Release path. A kill, a wipe and leaving the instance all end with the group still
    // holding the Skull this encounter put up, and every other Toravon gate is false by
    // then, so nothing would ever take it down. Runs only while this bot is out of combat,
    // and only while no Toravon is still being fought - an off-tank that has not engaged
    // yet must not release the focus token of a running encounter. The icon is released
    // only when it is recognisably ours: Toravon, a Frozen Orb (alive or dead), or a GUID
    // that no longer resolves because the unit despawned. A Skull on anything else belongs
    // to somebody else and is left alone, and an already empty Skull returned above, so
    // this cannot rewrite the same value every tick.
    if (!bot->IsInCombat())
    {
        Creature* running = bot->FindNearestCreature(BOSS_TORAVON, TORAVON_RANGE);
        if (running && running->IsAlive() && running->IsInCombat())
            return false;

        Unit* staleUnit = botAI->GetUnit(skull);
        return !staleUnit || staleUnit->GetEntry() == BOSS_TORAVON || staleUnit->GetEntry() == NPC_FROZEN_ORB;
    }

    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, TORAVON_RANGE);
    if (!toravon || !toravon->IsAlive() || !toravon->IsInCombat())
        return false;

    Creature* orb = bot->FindNearestCreature(NPC_FROZEN_ORB, TORAVON_RANGE);
    bool const orbAlive = orb && orb->IsAlive() && !orb->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE);

    Unit* skullUnit = botAI->GetUnit(skull);
    bool const skullIsLivingOrb =
        skullUnit && skullUnit->IsAlive() && skullUnit->GetEntry() == NPC_FROZEN_ORB;

    // An orb is up but Skull is not on a living orb: focus, or advance to the next orb.
    if (orbAlive && !skullIsLivingOrb)
        return true;

    // No orb left and Skull still points at an orb - dead, or a GUID that no longer
    // resolves: hand Skull back to Toravon. A Skull that is already on anything else is
    // left untouched.
    if (!orbAlive && (!skullUnit || skullUnit->GetEntry() == NPC_FROZEN_ORB))
        return true;

    return false;
}
