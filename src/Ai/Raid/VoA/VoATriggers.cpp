/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoATriggers.h"
#include "Creature.h"
#include "EventMap.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SpellAuras.h"
#include "ThreatManager.h"

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

// The single bot tank that should take Toravon over right now, or nullptr.
//
// Before this, every eligible off-tank could taunt on the same tick. That happens to be safe
// for a two-tank roster - the only off-tank is the one that is not the holder - but it is not
// safe for three or more. This picks one replacement deterministically: the eligible bot tank
// with the lowest assist-tank index, ties broken by GUID, so a larger tank line-up still
// produces exactly one taunt per swap. No state is cached anywhere; the roster is re-read
// every evaluation, so nothing can go stale.
static Player* ToravonSelectNextTank(Player* bot, Creature* toravon)
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    Unit* holder = toravon->GetThreatMgr().GetCurrentVictim();

    Player* best = nullptr;
    int32 bestIndex = 0;
    ObjectGuid::LowType bestGuid = 0;
    bool bestRanked = false;

    Group::MemberSlotList const& slots = group->GetMemberSlots();
    for (Group::member_citerator itr = slots.begin(); itr != slots.end(); ++itr)
    {
        Player* member = ObjectAccessor::FindPlayer(itr->guid);
        // The holder is not a replacement. This bot itself MUST stay in the running: the
        // caller asks whether it is the selected one, so excluding it here would make the
        // trigger permanently false.
        if (!member || !member->IsAlive() || member == holder)
            continue;

        PlayerbotAI* memberAi = GET_PLAYERBOT_AI(member);
        if (!memberAi || !memberAi->IsTank(member))
            continue;

        // A tank that is still stacked too high must not take over.
        if (ToravonFrostbiteStacks(member) >= TORAVON_FROSTBITE_SWAP_STACKS)
            continue;

        int32 index = 0;
        bool ranked = false;
        for (int32 i = 0; i < 3; ++i)
        {
            if (memberAi->IsAssistTankOfIndex(member, i))
            {
                index = i;
                ranked = true;
                break;
            }
        }

        bool better = false;
        if (!best)
            better = true;
        else if (ranked != bestRanked)
            better = ranked;  // a ranked assist tank outranks an unranked one
        else if (index != bestIndex)
            better = index < bestIndex;
        else
            better = member->GetGUID().GetCounter() < bestGuid;

        if (better)
        {
            best = member;
            bestIndex = index;
            bestRanked = ranked;
            bestGuid = member->GetGUID().GetCounter();
        }
    }

    return best;
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

    // Tank swaps are driven from the off-tank side only, and the current holder is read from
    // the threat manager rather than from toravon->GetVictim(): Unit::GetVictim() returns
    // m_attacking (Unit.h:904), which the creature AI only rewrites on its next tick, while a
    // taunt resolves synchronously inside the threat manager. Reading the stale one kept this
    // trigger firing for one extra tick after every successful swap.
    Unit* victim = toravon->GetThreatMgr().GetCurrentVictim();
    if (!victim || victim == bot || !victim->IsAlive())
        return false;

    // Swap only because the tank actually holding Toravon is stacked too high...
    if (ToravonFrostbiteStacks(victim) < TORAVON_FROSTBITE_SWAP_STACKS)
        return false;

    // ...and only when this bot is below the threshold itself.
    if (ToravonFrostbiteStacks(bot) >= TORAVON_FROSTBITE_SWAP_STACKS)
        return false;

    // Exactly one off-tank acts: this bot must be the selected replacement. A single-tank
    // raid never gets here at all, because the only tank is the holder.
    return bot == ToravonSelectNextTank(bot, toravon);
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
    // Driven by a bot tank on purpose: right role, always available around Toravon, and
    // unaffected by the DPS-only gate on the orb trigger above.
    if (!GET_PLAYERBOT_AI(bot) || !botAI->IsTank(bot))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Toravon alive and in the arena is the anchor. Nothing here depends on being in combat,
    // so the token can be established before the pull and survives the encounter ending.
    // While he is dead this returns false and nothing runs, which deliberately leaves the last
    // marker where it was instead of clearing it.
    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, TORAVON_RANGE);
    if (!toravon || !toravon->IsAlive())
        return false;

    ObjectGuid const skull = group->GetTargetIcon(RtiTargetValue::skullIndex);
    Unit* skullUnit = skull.IsEmpty() ? nullptr : botAI->GetUnit(skull);

    Creature* orb = bot->FindNearestCreature(NPC_FROZEN_ORB, TORAVON_RANGE);
    bool const orbAlive = orb && orb->IsAlive() && !orb->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE);

    if (orbAlive)
    {
        // An orb is up: Skull belongs on the focused living orb. Needs work when it is empty,
        // dead, stale, or pointing anywhere that is not a living orb - which is both the
        // initial claim and the advance to the next orb once the focused one dies.
        bool const skullIsLivingOrb =
            skullUnit && skullUnit->IsAlive() && skullUnit->GetEntry() == NPC_FROZEN_ORB;
        return !skullIsLivingOrb;
    }

    // No living orb: Skull belongs on Toravon - before the pull (Skull may be empty or on
    // something unrelated), between orb waves and after the last orb dies. Already being on
    // Toravon needs no rewrite, which is what stops this writing the same GUID every tick.
    return skullUnit != toravon;
}
