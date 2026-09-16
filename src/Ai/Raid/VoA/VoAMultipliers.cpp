/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoAMultipliers.h"
#include "ChooseTargetActions.h"
#include "Creature.h"
#include "DKActions.h"
#include "DruidBearActions.h"
#include "Group.h"
#include "PaladinActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SpellAuras.h"
#include "VoATriggers.h"
#include "WarriorActions.h"

// Toravon the Ice Watcher - anti-ping-pong gate.
//
// Once a tank has handed Toravon over, its own Frostbite is still at or above the swap
// threshold for the remainder of the debuff. Without a gate the generic taunt rotation
// would immediately pull the boss straight back, producing a taunt war between the two
// tanks. Suppressing the taunt-class actions of any tank that is currently over the
// threshold keeps the swap stable until the debuff falls off and the other tank in turn
// becomes eligible. Same approach as IccFestergutMultiplier for Gastric Bloat
// (src/Ai/Raid/ICC/ICCMultipliers.cpp:194-210), kept local to VoA so no ICC state or
// helper is pulled in.
float VoAToravonMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    // Containment: with no living Toravon in range this multiplier is inert, so Emalon,
    // Koralon, Archavon and every other encounter behave exactly as before.
    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, 60.0f);
    if (!toravon || !toravon->IsAlive() || !toravon->IsInCombat())
        return 1.0f;

    // Only tanks are gated; DPS, healers and pets are untouched.
    if (!botAI->IsTank(bot))
        return 1.0f;

    // --- Tank containment: a tank must never follow the Skull Frozen Orb ---------------
    //
    // Both tank targeting paths read the raid icons, and the orb is a real threat-holding
    // unit:
    //   * AttackRtiTargetAction attacks the RTI target directly (ChooseTargetActions.cpp:147)
    //   * TankAssistAction resolves through "tank target", and TankTargetValue::Calculate()
    //     returns the RTI target whenever that unit is attacking a non-tank player
    //     (TankTargetValue.cpp:112-124) - which is precisely what a Frozen Orb does, since
    //     npc_frozen_orbAI resets its threat and picks a random player every 10 s
    //     (boss_toravon.cpp:188-198).
    //
    // Suppression is deliberately conditional: only the paths that would actually take the
    // tank to the orb are zeroed, so the active tank keeps tanking Toravon as before.
    ObjectGuid const skullGuid = bot->GetGroup() ? bot->GetGroup()->GetTargetIcon(RtiTargetValue::skullIndex)
                                                 : ObjectGuid::Empty;
    Unit* skullUnit = skullGuid.IsEmpty() ? nullptr : botAI->GetUnit(skullGuid);
    bool const skullIsOrb = skullUnit && skullUnit->GetEntry() == NPC_FROZEN_ORB;

    if (skullIsOrb && dynamic_cast<AttackRtiTargetAction*>(action))
        return 0.0f;

    Unit* tankTarget = AI_VALUE(Unit*, "tank target");
    if (tankTarget && tankTarget->GetEntry() == NPC_FROZEN_ORB &&
        (dynamic_cast<TankAssistAction*>(action) || dynamic_cast<AttackAction*>(action)))
        return 0.0f;

    // Looked up by spell id: the Spell.dbc shipped with this build has its name fields
    // zeroed, so PlayerbotAI::GetAura(name, ...) is not dependable here.
    Aura* aura = bot->GetAura(SPELL_TORAVON_FROSTBITE);
    if (!aura || aura->GetStackAmount() < TORAVON_FROSTBITE_SWAP_STACKS)
        return 1.0f;

    // The tank that is actually holding Toravon is never suppressed.
    if (toravon->GetVictim() == bot)
        return 1.0f;

    // --- Old-tank suppression after a successful handoff -------------------------------
    //
    // This tank has just handed Toravon over and is still over the swap threshold. Without
    // a gate it would taunt, re-assist or simply keep attacking and pull the boss straight
    // back, collapsing the swap. Suppressing here keeps the handoff stable until Frostbite
    // falls off and this tank becomes eligible again.
    //
    // AttackAction covers only target-selecting actions (TankAssist, AttackRtiTarget,
    // DpsAssist, AggressiveTarget, AttackAnything, Melee, ...); spell and defensive
    // abilities derive from Action (GenericSpellActions.h) and movement from MovementAction,
    // so the old tank keeps its skills, its cooldowns and its positioning.
    if (dynamic_cast<CastTauntAction*>(action) ||
        dynamic_cast<CastDarkCommandAction*>(action) ||
        dynamic_cast<CastHandOfReckoningAction*>(action) ||
        dynamic_cast<CastGrowlAction*>(action) ||
        dynamic_cast<TankAssistAction*>(action) ||
        dynamic_cast<AttackRtiTargetAction*>(action) ||
        dynamic_cast<AttackAction*>(action))
        return 0.0f;

    return 1.0f;
}
