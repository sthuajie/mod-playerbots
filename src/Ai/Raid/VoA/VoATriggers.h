/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_VOATRIGGERS_H
#define PLAYERBOTS_VOATRIGGERS_H

#include "GenericTriggers.h"
#include "Trigger.h"

enum VoAIDs
{
    // Emalon the Storm Watcher
    AURA_OVERCHARGE = 64217,
    BOSS_EMALON = 33993,
    NPC_TEMPEST_MINION = 33998,
    SPELL_LIGHTNING_NOVA_10_MAN = 64216,
    SPELL_LIGHTNING_NOVA_25_MAN = 65279,

    // Toravon the Ice Watcher
    // Boss entry from the current world DB: acore_world.creature_template.entry = 38433
    // (ScriptName 'boss_toravon'), spawning on map 624 (Vault of Archavon).
    BOSS_TORAVON = 38433,
    // Frozen Orb creature entry. creature_template holds four rows named "Frozen Orb"
    // (29849, 30054, 38456, 38698); only 38456 carries ScriptName 'npc_frozen_orb',
    // which is the CreatureScript registered in boss_toravon.cpp.
    NPC_FROZEN_ORB = 38456,
    // Stacking debuff Toravon applies to his current tank through Frozen Mallet (71993).
    // 72004 is the spell id Frozen Mallet links to in the current client Spell.dbc
    // (field 116) - the same link slot the encounter's other spell pairs use
    // (72067->72068, 72081->72082, 72091->72092) - and 71993 also has a spell_proc row
    // (ProcFlags=4, HitMask=12287, Cooldown=3000), so it is reapplied to the tank
    // repeatedly. Resolved by id, never by name: the Spell.dbc shipped with this build
    // has every name/description offset zeroed.
    SPELL_TORAVON_FROSTBITE = 72004,
    // Taunt spell ids for the tank swap are deliberately NOT declared here. VoAIDs is an
    // unscoped enum and BuildSharedTriggerContexts.cpp pulls this header into the same
    // translation unit as ICCTriggers.h, which already declares SPELL_TAUNT_WARRIOR,
    // SPELL_TAUNT_PALADIN, SPELL_TAUNT_DK and SPELL_TAUNT_DRUID for these same ids, so
    // declaring them here made that unit fail to compile with a C2365 redefinition. They
    // are file-local constants in VoAActions.cpp instead, which is where their only user
    // lives - the same way RSTriggers.h prefixes its copies with RS_.
    // Frostbite stacks at which the off-tank takes over. The mechanic is a 4-5 stack
    // swap; 4 is used as the conservative bot threshold.
    TORAVON_FROSTBITE_SWAP_STACKS = 4,
};

//
// Emalon the Storm Watcher
//
class EmalonMarkBossTrigger : public Trigger
{
public:
    EmalonMarkBossTrigger(PlayerbotAI* ai) : Trigger(ai, "emalon mark boss trigger") {}
    bool IsActive() override;
};

class EmalonLightingNovaTrigger : public Trigger
{
public:
    EmalonLightingNovaTrigger(PlayerbotAI* ai) : Trigger(ai, "emalon lighting nova trigger") {}
    bool IsActive() override;
};

class EmalonOverchargeTrigger : public Trigger
{
public:
    EmalonOverchargeTrigger(PlayerbotAI* ai) : Trigger(ai, "emalon overcharge trigger") {}
    bool IsActive() override;
};

class EmalonFallFromFloorTrigger : public Trigger
{
public:
    EmalonFallFromFloorTrigger(PlayerbotAI* ai) : Trigger(ai, "emalon fall from floor trigger") {}
    bool IsActive() override;
};

//
// Toravon the Ice Watcher
//
class ToravonFrostbiteSwapTrigger : public Trigger
{
public:
    ToravonFrostbiteSwapTrigger(PlayerbotAI* ai) : Trigger(ai, "toravon frostbite swap trigger") {}
    bool IsActive() override;
};

class ToravonFrozenOrbTrigger : public Trigger
{
public:
    ToravonFrozenOrbTrigger(PlayerbotAI* ai) : Trigger(ai, "toravon frozen orb trigger") {}
    bool IsActive() override;
};

class ToravonMarkSkullTrigger : public Trigger
{
public:
    ToravonMarkSkullTrigger(PlayerbotAI* ai) : Trigger(ai, "toravon mark skull trigger") {}
    bool IsActive() override;
};

#endif
