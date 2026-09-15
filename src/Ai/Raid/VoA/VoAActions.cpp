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
            tauntSpell = SPELL_TAUNT_WARRIOR;
            break;
        case CLASS_PALADIN:
            tauntSpell = SPELL_TAUNT_PALADIN;
            break;
        case CLASS_DEATH_KNIGHT:
            tauntSpell = SPELL_TAUNT_DEATH_KNIGHT;
            break;
        case CLASS_DRUID:
            tauntSpell = SPELL_TAUNT_DRUID;
            break;
        default:
            return false;
    }

    // The spell's own cooldown is the throttle: a bot cannot taunt more often than the
    // taunt allows, so a swap that has already landed is not re-issued every tick.
    if (!bot->HasSpell(tauntSpell) || bot->HasSpellCooldown(tauntSpell))
        return false;

    return botAI->CastSpell(tauntSpell, target);
}
}  // namespace

bool ToravonFrostbiteTauntAction::Execute(Event /*event*/)
{
    Creature* toravon = bot->FindNearestCreature(BOSS_TORAVON, 60.0f);
    if (!toravon || !toravon->IsAlive())
        return false;

    return ToravonCastClassTaunt(bot, botAI, toravon);
}

bool ToravonFrostbiteTauntAction::isUseful()
{
    ToravonFrostbiteSwapTrigger trigger(botAI);
    return trigger.IsActive();
}

bool ToravonAttackFrozenOrbAction::Execute(Event /*event*/)
{
    Creature* orb = bot->FindNearestCreature(NPC_FROZEN_ORB, 60.0f);
    if (!orb || !orb->IsAlive() || orb->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;

    if (bot->GetVictim() == orb)
        return false;

    // Re-target onto the nearest living orb. Stateless by design: no GUID is cached, so
    // each ranged DPS resolves its own orb (25-man spawns three) and a dead orb simply
    // stops resolving, letting the generic Toravon DPS target resume.
    //
    // The melee flag mirrors AttackAction's own rule
    // (AttackAction.cpp:141: IsWithinMeleeRange(target) || IsMelee(bot)) so ranged DPS
    // shoot the orb from range instead of being forced into melee attack state.
    bool const shouldMelee = bot->IsWithinMeleeRange(orb) || botAI->IsMelee(bot);
    bot->SetSelection(orb->GetGUID());
    bot->Attack(orb, shouldMelee);
    return true;
}

bool ToravonAttackFrozenOrbAction::isUseful()
{
    ToravonFrozenOrbTrigger trigger(botAI);
    return trigger.IsActive();
}
