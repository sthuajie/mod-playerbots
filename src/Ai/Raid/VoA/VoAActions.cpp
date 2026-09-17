/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoAActions.h"
#include "Creature.h"
#include "Define.h"
#include "Event.h"
#include "Group.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "ThreatManager.h"
#include "Unit.h"
#include "VoATriggers.h"

const Position VOA_EMALON_RESTORE_POSITION = Position(-221.8f, -243.8f, 96.8f, 4.7f);

bool EmalonMarkBossAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss || !boss->IsAlive())
    {
        return false;
    }

    bool isMainTank = botAI->IsMainTank(bot);
    Unit* mainTankUnit = AI_VALUE(Unit*, "main tank");
    Player* mainTank = mainTankUnit ? mainTankUnit->ToPlayer() : nullptr;

    if (mainTank && !GET_PLAYERBOT_AI(mainTank))  // Main tank is a real player
    {
        // Iterate through the first 3 bot tanks to assign the Skull marker
        for (int i = 0; i < 3; ++i)
        {
            if (botAI->IsAssistTankOfIndex(bot, i) && GET_PLAYERBOT_AI(bot))  // Bot is a valid tank
            {
                Group* group = bot->GetGroup();
                if (group && boss)
                {
                    int8 skullIndex = 7;  // Skull
                    ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);

                    // If there's no skull set yet, or the skull is on a different target, set boss
                    if (!currentSkullTarget || (boss->GetGUID() != currentSkullTarget))
                    {
                        group->SetTargetIcon(skullIndex, bot->GetGUID(), boss->GetGUID());
                        return true;
                    }
                }
                break;  // Stop after finding the first valid bot tank
            }
        }
    }
    else if (isMainTank)  // Bot is the main tank
    {
        Group* group = bot->GetGroup();
        if (group)
        {
            int8 skullIndex = 7;  // Skull
            ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);

            // If there's no skull set yet, or the skull is on a different target, set the Eonar's Gift
            if (!currentSkullTarget || (boss->GetGUID() != currentSkullTarget))
            {
                group->SetTargetIcon(skullIndex, bot->GetGUID(), boss->GetGUID());
                return true;
            }
        }
    }

    return false;
}

bool EmalonMarkBossAction::isUseful()
{
    EmalonMarkBossTrigger emalonMarkBossTrigger(botAI);
    return emalonMarkBossTrigger.IsActive();
}

bool EmalonLightingNovaAction::Execute(Event /*event*/)
{
    const float radius = 25.0f;  // 20 yards + 5 yard for safety for 10 man. For 25man there is no maximum range but 25 yards should be ok

    Unit* boss = AI_VALUE2(Unit*, "find target", "emalon the storm watcher");
    if (!boss)
        return false;

    float currentDistance = bot->GetDistance2d(boss);

    if (currentDistance < radius)
    {
        return MoveAway(boss, radius - currentDistance);
    }

    return false;
}

bool EmalonLightingNovaAction::isUseful()
{
    EmalonLightingNovaTrigger emalonLightingNovaTrigger(botAI);
    return emalonLightingNovaTrigger.IsActive();
}

bool EmalonOverchargeAction::Execute(Event /*event*/)
{
    // Check if there is any overcharged minion
    Unit* minion = nullptr;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit)
            continue;

        uint32 entry = unit->GetEntry();
        if (entry == NPC_TEMPEST_MINION && unit->HasAura(AURA_OVERCHARGE))
        {
            minion = unit;
            break;
        }
    }
    if (!minion)
    {
        return false;
    }

    bool isMainTank = botAI->IsMainTank(bot);
    Unit* mainTankUnit = AI_VALUE(Unit*, "main tank");
    Player* mainTank = mainTankUnit ? mainTankUnit->ToPlayer() : nullptr;

    if (mainTank && !GET_PLAYERBOT_AI(mainTank))  // Main tank is a real player
    {
        // Iterate through the first 3 bot tanks to assign the Skull marker
        for (int i = 0; i < 3; ++i)
        {
            if (botAI->IsAssistTankOfIndex(bot, i) && GET_PLAYERBOT_AI(bot))  // Bot is a valid tank
            {
                Group* group = bot->GetGroup();
                if (group && minion)
                {
                    int8 skullIndex = 7;  // Skull
                    ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);

                    // If there's no skull set yet, or the skull is on a different target, set Tempest Minion
                    if (!currentSkullTarget || (minion->GetGUID() != currentSkullTarget))
                    {
                        group->SetTargetIcon(skullIndex, bot->GetGUID(), minion->GetGUID());
                        return true;
                    }
                }
                break;  // Stop after finding the first valid bot tank
            }
        }
    }
    else if (isMainTank)  // Bot is the main tank
    {
        Group* group = bot->GetGroup();
        if (group)
        {
            int8 skullIndex = 7;  // Skull
            ObjectGuid currentSkullTarget = group->GetTargetIcon(skullIndex);

            // If there's no skull set yet, or the skull is on a different target, set the Eonar's Gift
            if (!currentSkullTarget || (minion->GetGUID() != currentSkullTarget))
            {
                group->SetTargetIcon(skullIndex, bot->GetGUID(), minion->GetGUID());
                return true;
            }
        }
    }

    return false;
}

bool EmalonOverchargeAction::isUseful()
{
    EmalonOverchargeTrigger emalonOverchargeTrigger(botAI);
    return emalonOverchargeTrigger.IsActive();
}

bool EmalonFallFromFloorAction::Execute(Event /*event*/)
{
    return bot->TeleportTo(bot->GetMapId(), VOA_EMALON_RESTORE_POSITION.GetPositionX(),
                           VOA_EMALON_RESTORE_POSITION.GetPositionY(), VOA_EMALON_RESTORE_POSITION.GetPositionZ(),
                           VOA_EMALON_RESTORE_POSITION.GetOrientation());
}

bool EmalonFallFromFloorAction::isUseful()
{
    EmalonFallFromFloorTrigger emalonFallFromFloorTrigger(botAI);
    return emalonFallFromFloorTrigger.IsActive();
}

//
// Toravon the Ice Watcher
//

namespace
{
// Taunt spells by class, kept file-local on purpose. These ids are also declared in
// ICCTriggers.h (SPELL_TAUNT_WARRIOR / _PALADIN / _DK / _DRUID) for the same spells, and
// both headers reach one translation unit through BuildSharedTriggerContexts.cpp, so
// declaring them in VoATriggers.h as well made that unit fail to compile with a C2365
// redefinition. Nothing outside this file needs them. RSTriggers.h resolves the same
// collision by prefixing its copies with RS_.
constexpr uint32 TORAVON_TAUNT_WARRIOR = 355;    // Taunt
constexpr uint32 TORAVON_TAUNT_PALADIN = 62124;  // Hand of Reckoning
constexpr uint32 TORAVON_TAUNT_DK = 56222;       // Dark Command
constexpr uint32 TORAVON_TAUNT_DRUID = 6795;     // Growl

// Minimal VoA-local class taunt.
//
// Mirrors the class selection of IccCastClassTaunt (src/Ai/Raid/ICC/ICCShared.cpp:49)
// but deliberately pulls in neither IcecrownHelpers nor any ICC instance state, and casts
// by spell id instead of by name: the Spell.dbc shipped with this build has its name
// fields zeroed, so PlayerbotAI's name-based cast path is not dependable here.
bool ToravonCastClassTaunt(Player* bot, PlayerbotAI* botAI, Unit* target)
{
    if (!bot || !botAI || !target || !target->IsAlive())
        return false;

    uint32 tauntSpell = 0;
    switch (bot->getClass())
    {
        case CLASS_WARRIOR:
            tauntSpell = TORAVON_TAUNT_WARRIOR;
            break;
        case CLASS_PALADIN:
            tauntSpell = TORAVON_TAUNT_PALADIN;
            break;
        case CLASS_DEATH_KNIGHT:
            tauntSpell = TORAVON_TAUNT_DK;
            break;
        case CLASS_DRUID:
            tauntSpell = TORAVON_TAUNT_DRUID;
            break;
        default:
            return false;
    }

    // The spell's own cooldown is the throttle: a bot cannot taunt more often than the
    // taunt allows, so a swap that has already landed is not re-issued every tick.
    if (!bot->HasSpell(tauntSpell) || bot->HasSpellCooldown(tauntSpell))
        return false;

    // Deliberately a plain class taunt, with no threat injection and no fixate.
    //
    // IccCastClassTaunt additionally does AddThreat(bot, 1000000.0f, ...) + FixateTarget(bot)
    // when the raid cheat is on (src/Ai/Raid/ICC/ICCShared.cpp:60-67), but that contract does
    // not transfer to a repeating tank swap: ThreatManager::FixateTarget() sets a _fixateRef
    // that nothing releases except FixateTarget(nullptr) or the fixated unit leaving the
    // threat list (ThreatManager.cpp:606, 919-920) - it is not tied to the taunt aura, and
    // TauntUpdate()/UpdateVictim() never touch it. ReselectVictim() returns the fixated target
    // first (ThreatManager.cpp:641-642), so the first replacement would be pinned for good and
    // the SECOND swap could never happen.
    //
    // A normal taunt is enough on its own: the taunt aura runs TauntUpdate() -> UpdateVictim()
    // (SpellAuraEffects.cpp:3588, ThreatManager.cpp:550), the heap puts the taunting reference
    // on top (CompareThreatLessThan -> CompareReferencesLT, ThreatManager.h:326-329), and
    // taunt state outranks numeric threat there (ThreatManager.cpp:703-704 with
    // TAUNT_STATE_DETAUNT=0 < NONE=1 < TAUNT=2, ThreatManager.h:269), which bypasses the
    // 110%/130% checks in ReselectVictim() entirely. So casting is all that is needed.
    return botAI->CastSpell(tauntSpell, target);
}
}  // namespace

// Valid living Frozen Orb currently holding the group's Skull icon, or nullptr.
// Every check the encounter needs lives here: resolvable, alive, right entry, selectable
// and inside the Toravon arena (which also implies the same map and instance).
Unit* ToravonSkullOrb(PlayerbotAI* botAI, Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    ObjectGuid const skull = group->GetTargetIcon(RtiTargetValue::skullIndex);
    if (skull.IsEmpty())
        return nullptr;

    Unit* unit = botAI->GetUnit(skull);
    if (!unit || !unit->IsAlive() || unit->GetEntry() != NPC_FROZEN_ORB)
        return nullptr;
    if (unit->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return nullptr;
    if (bot->GetExactDist(unit) > 60.0f)
        return nullptr;

    return unit;
}

// Nearest living, selectable Frozen Orb this bot could legitimately attack.
Creature* ToravonFindLivingOrb(Player* bot)
{
    Creature* orb = bot->FindNearestCreature(NPC_FROZEN_ORB, 60.0f);
    if (!orb || !orb->IsAlive() || orb->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return nullptr;

    return orb;
}

// Toravon as something the encounter can still be marked on: himself while alive, or his
// corpse while it is still present. nullptr once he is gone entirely.
//
// FindNearestCreature() defaults to alive-only (Object.h:646) and its checker compares
// IsAlive() == alive exactly (GridNotifiers.h:1360-1368), so the corpse needs the second call
// with alive = false. Group::SetTargetIcon() stores and broadcasts whatever GUID it is handed
// and performs no liveness check at all (Group.cpp:1824-1843), so a dead boss can be marked -
// which is what lets Skull be finalised onto Toravon after the kill.
static Creature* ToravonResolve(Player* bot)
{
    if (Creature* alive = bot->FindNearestCreature(BOSS_TORAVON, 60.0f))
        return alive;

    return bot->FindNearestCreature(BOSS_TORAVON, 60.0f, false);
}

bool ToravonFrostbiteTauntAction::Execute(Event /*event*/)
{
    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, 60.0f);
    if (!toravon || !toravon->IsAlive())
        return false;

    // The holder is read from the threat manager rather than from toravon->GetVictim().
    // A creature's GetVictim() is Unit::m_attacking (Unit.h:904), which the creature AI only
    // rewrites in CreatureAI::UpdateVictim() on its next tick, whereas a taunt resolves
    // synchronously inside ThreatManager (the taunt aura calls TauntUpdate() -> UpdateVictim()
    // from SpellAuraEffects.cpp:3588). So the threat manager is the only same-tick truth.
    ThreatManager& mgr = toravon->GetThreatMgr();
    Unit* oldVictim = mgr.GetCurrentVictim();

    if (!ToravonCastClassTaunt(bot, botAI, toravon))
        return false;

    // A successful taunt request IS the handoff. There is deliberately no same-tick
    // "has the boss retargeted me yet" test here: that test cannot pass on the taunting tick,
    // because m_attacking only changes on the boss's next AI update. Keeping it would reject
    // every real handoff, and the old tank would never be stopped - driving both the missing
    // swap and the taunt war it produced.
    if (oldVictim == bot)
        return false;

    // Finish the handoff for a bot off-tank: stop it attacking so it neither rebuilds threat
    // on Toravon nor keeps swinging at it. A human player is never touched, so a human old
    // tank is left exactly as it was.
    if (oldVictim && oldVictim->IsAlive())
    {
        Player* oldPlayer = oldVictim->ToPlayer();
        PlayerbotAI* oldAi = oldPlayer ? GET_PLAYERBOT_AI(oldPlayer) : nullptr;
        if (oldPlayer && oldAi && oldAi->IsTank(oldPlayer))
            oldPlayer->AttackStop();
    }

    return true;
}

bool ToravonFrostbiteTauntAction::isUseful()
{
    ToravonFrostbiteSwapTrigger trigger(botAI);
    return trigger.IsActive();
}

bool ToravonAttackFrozenOrbAction::Execute(Event /*event*/)
{
    // Skull is the group focus token: a live Skull orb is authoritative and no other orb
    // is considered while it stands.
    Unit* focus = ToravonSkullOrb(botAI, bot);

    if (!focus)
    {
        // Skull is empty, dead, or points at something that is not a living orb: claim the
        // nearest valid orb for the group. Only the first writer matters - every later bot
        // sees a valid Skull and simply follows it, so focus stays on one orb.
        Creature* orb = ToravonFindLivingOrb(bot);
        if (!orb)
            return false;

        if (Group* group = bot->GetGroup())
            group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), orb->GetGUID());

        focus = orb;
    }

    // Melee DPS has to actually close on the focused orb, otherwise a target change alone
    // leaves it standing where it was. Ranged DPS is deliberately not dragged into melee.
    if (botAI->IsMelee(bot) && !bot->IsWithinMeleeRange(focus))
        return ReachCombatTo(focus, sPlayerbotAIConfig.meleeDistance);

    if (AI_VALUE(Unit*, "current target") == focus)
        return false;

    // AttackAction::Attack() owns selection, current/old target, the melee-vs-ranged
    // attack mode and the combat engine transition - the same contract UK's
    // AttackFrostTombAction relies on (UKActions.cpp:28-32).
    return Attack(focus);
}

bool ToravonAttackFrozenOrbAction::isUseful()
{
    ToravonFrozenOrbTrigger trigger(botAI);
    return trigger.IsActive();
}

// Keeps the group's Skull token on the right Toravon target: the focused living orb while an
// orb is up, and Toravon himself - alive or dead - whenever no living orb should hold it.
//
// Deliberately never releases the icon to empty, and deliberately keeps working after the
// boss dies: if he dies while the token sits on an orb, this finalises it back onto him.
// Driven by a tank bot on purpose - when the last orb dies the DPS orb trigger is already
// false, so this is the only path that still runs. Mirrors Emalon's existing marking pattern.
bool ToravonMarkSkullAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Living Toravon, or his corpse while it is still there. Once he is gone entirely there is
    // nothing to mark, and doing nothing leaves the last marker in place rather than clearing it.
    Creature* toravon = ToravonResolve(bot);
    if (!toravon)
        return false;

    // A living boss with a living orb up: the token belongs on the focused orb.
    if (toravon->IsAlive())
    {
        if (Creature* orb = ToravonFindLivingOrb(bot))
        {
            if (ToravonSkullOrb(botAI, bot))
                return false;  // already focused on a living orb

            group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), orb->GetGUID());
            return true;
        }
    }

    // Otherwise the token belongs on Toravon: before the pull (Skull may be empty or on
    // something unrelated), between orb waves, after the last orb dies, and after the boss dies
    // - including when he died with Skull still on an orb, which this pulls back onto him.
    // Never ObjectGuid::Empty. Writing only when the icon is not already on Toravon is what
    // keeps this from rewriting the same GUID every tick.
    ObjectGuid const skull = group->GetTargetIcon(RtiTargetValue::skullIndex);
    if (!skull.IsEmpty() && botAI->GetUnit(skull) == toravon)
        return false;

    group->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), toravon->GetGUID());
    return true;
}

bool ToravonMarkSkullAction::isUseful()
{
    ToravonMarkSkullTrigger trigger(botAI);
    return trigger.IsActive();
}
