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
#include "ThreatManager.h"
#include "VoATriggers.h"
#include "WarriorActions.h"

namespace
{
// Generic tank taunts that can retarget Toravon and spend the class's taunt resource.
//
// Every one of these was checked against the shipped Spell.dbc (SPELL_AURA_MOD_TAUNT on the
// spell or, for Righteous Defense, on the spell its core handler casts):
//   CastTauntAction          Warrior  Taunt 355            MOD_TAUNT, cat 82, 8 s
//   CastHandOfReckoningAction Paladin HoR 62124            MOD_TAUNT, 8 s
//   CastRighteousDefenseAction Paladin RD 31789 -> 31790   MOD_TAUNT (SpellEffects.cpp:723-748);
//                            this is the alternative of the Hand of Reckoning action node
//                            (TankPaladinStrategy.cpp:52-60), so suppressing only HoR would let
//                            the fallback taunt through - the engine pushes a node's
//                            alternatives whenever a multiplier zeroes it (Engine.cpp:231-235).
//   CastDarkCommandAction    DK       Dark Command 56222   MOD_TAUNT, cat 82, 8 s
//   CastGrowlAction          Druid    Growl 6795           MOD_TAUNT, cat 82, 8 s
//   CastChallengingRoarAction Druid   Challenging Roar 5209 MOD_TAUNT, 180 s (AoE taunt)
//
// Deliberately NOT listed, with the evidence:
//   Death Grip 49576   no MOD_TAUNT in Spell.dbc, core only spell-fixes it
//                      (SpellInfoCorrections.cpp:861-869) -> cannot retarget, spends no taunt
//   Heroic Throw 57755 / Shield Slam 23922 / Icy Touch 45477
//                      no MOD_TAUNT; the Warrior and DK action nodes fall back to these and
//                      they are ordinary threat abilities, so they stay available.
//
// The scripted encounter action (ToravonFrostbiteTauntAction) is a different class and is never
// matched here, so the swap the encounter asks for is always allowed through.
bool IsGenericBossTaunt(Action* action)
{
    return dynamic_cast<CastTauntAction*>(action) ||
           dynamic_cast<CastHandOfReckoningAction*>(action) ||
           dynamic_cast<CastRighteousDefenseAction*>(action) ||
           dynamic_cast<CastDarkCommandAction*>(action) ||
           dynamic_cast<CastGrowlAction*>(action) ||
           dynamic_cast<CastChallengingRoarAction*>(action);
}

// Is this unit a tank? Bots are resolved by their tank strategy, real players by their talent
// spec, which is what lets a human tank hold Toravon without the bot off-tank taunting it away.
bool IsTankHolder(Unit* holder)
{
    Player* player = holder ? holder->ToPlayer() : nullptr;
    if (!player)
        return false;

    PlayerbotAI* holderAi = GET_PLAYERBOT_AI(player);
    return holderAi ? holderAi->IsTank(player) : PlayerbotAI::IsTank(player, true);
}
}  // namespace

// Toravon the Ice Watcher - conditional gate on the generic tank taunt.
//
// The encounter owns one specific action: when the tank holding Toravon reaches the Frostbite
// threshold, ToravonFrostbiteSwapTrigger picks exactly one replacement with
// ToravonSelectNextTank() and ToravonFrostbiteTauntAction performs the handoff. Everything
// else the tank AI might do to Toravon has to stay out of the way of that, which is what the
// four states below express.
//
// The reason a gate is needed at all is that "lose aggro" is what a healthy off-tank looks
// like. LoseAggroTrigger is literally !has aggro (GenericTriggers.cpp:108), so both tanks
// constantly want to taunt while the other one holds the boss. Without a gate the off-tank
// spends its one taunt the moment its Frostbite drops below the threshold - which is exactly
// the window in which the scripted swap is about to need it - and the swap then fails on
// HasSpellCooldown. It also needlessly feeds Toravon's taunt diminishing returns, which the
// boss does obey (CREATURE_FLAG_EXTRA_OBEYS_TAUNT_DIMINISHING_RETURNS is set on 38433, and
// DIMINISHING_TAUNT only resets after 15 s without a taunt, Unit.cpp:11771).
//
// This mirrors the conditioned suppression ICC uses for its own stack swaps - Festergut
// (ICCMultipliers.cpp:194-203), Saurfang (:85-94), Putricide (:397-403) - without taking any
// of the ICC cheat path: no AddThreat, no FixateTarget, no cooldown removal. See also
// /toravon-v3-2-design notes in the repository report.
//
//   STATE A - healthy holder, below threshold, this bot is the off-tank
//             -> generic boss-taunt suppressed; movement, defensives, threat abilities and the
//                scripted encounter action all keep working
//   STATE B - the holder reaches the threshold
//             -> nothing to do here: the scripted action already owns the handoff and is not a
//                generic taunt class, so it is never zeroed
//   STATE C - this bot just handed Toravon over and is still stacked
//             -> keep the strong anti-ping-pong gate (taunt + assist + attack paths)
//   STATE D - no valid living tank holder (dead, gone, or the boss fell to a non-tank)
//             -> suppress nothing: the surviving tank must be able to recover the boss with its
//                normal lose-aggro reaction
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

    // --- Who is holding Toravon? -------------------------------------------------------
    //
    // Read from the threat manager, not from toravon->GetVictim(): Unit::GetVictim() returns
    // m_attacking (Unit.h:904), which the creature AI only rewrites on its next tick, so on the
    // tick a handoff lands it would still name the tank that was just replaced - leaving exactly
    // that tank unsuppressed at the one moment it must not be. The threat manager is updated
    // synchronously by the taunt itself (the taunt aura calls TauntUpdate() -> UpdateVictim(),
    // SpellAuraEffects.cpp:3588).
    Unit* holder = toravon->GetThreatMgr().GetCurrentVictim();

    // STATE D. A dead, despawned or non-tank holder is not a healthy tank line-up, so nothing
    // is gated: this is what keeps the generic "lose aggro" recovery available when the active
    // tank dies - a huntard with threat must not be able to lock the surviving tank out.
    if (!holder || !holder->IsAlive() || !holder->IsInWorld() || !IsTankHolder(holder))
        return 1.0f;

    // The holder is never suppressed, whatever its Frostbite looks like.
    if (holder == bot)
        return 1.0f;

    // Frostbite is looked up by spell id: the Spell.dbc shipped with this build has its name
    // fields zeroed, so PlayerbotAI::GetAura(name, ...) is not dependable here.
    Aura* aura = bot->GetAura(SPELL_TORAVON_FROSTBITE);
    bool const overThreshold = aura && aura->GetStackAmount() >= TORAVON_FROSTBITE_SWAP_STACKS;

    // STATE C. This tank has handed Toravon over and is still over the swap threshold. Without
    // a gate it would taunt, re-assist or simply keep attacking and pull the boss straight back,
    // collapsing the swap. Suppressing here keeps the handoff stable until Frostbite falls off
    // and this tank becomes eligible again.
    //
    // AttackAction covers only target-selecting actions (TankAssist, AttackRtiTarget,
    // DpsAssist, AggressiveTarget, AttackAnything, Melee, ...); spell and defensive abilities
    // derive from Action (GenericSpellActions.h) and movement from MovementAction, so the old
    // tank keeps its skills, its cooldowns and its positioning.
    if (overThreshold)
    {
        if (IsGenericBossTaunt(action) ||
            dynamic_cast<TankAssistAction*>(action) ||
            dynamic_cast<AttackRtiTargetAction*>(action) ||
            dynamic_cast<AttackAction*>(action))
            return 0.0f;

        return 1.0f;
    }

    // STATE A. A valid living tank holds Toravon, it is below the swap threshold, and this bot
    // is the off-tank with no stacks: not holding the boss is the intended state, not a
    // "lose aggro" error to correct. Gate only the generic boss-taunt so the class taunt and
    // Toravon's taunt diminishing counter survive intact for the scripted swap that
    // ToravonFrostbiteSwapTrigger will request once the holder reaches the threshold.
    //
    // This is a gate, not a lock-out: the STATE D branch above returns 1.0f for everything the
    // moment the holder stops being a living tank, and movement, defensives and threat
    // abilities are never touched here.
    if (IsGenericBossTaunt(action))
        return 0.0f;

    return 1.0f;
}
