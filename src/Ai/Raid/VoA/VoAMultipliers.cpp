/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoAMultipliers.h"
#include "Creature.h"
#include "DKActions.h"
#include "DruidBearActions.h"
#include "PaladinActions.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
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

    // Looked up by spell id: the Spell.dbc shipped with this build has its name fields
    // zeroed, so PlayerbotAI::GetAura(name, ...) is not dependable here.
    Aura* aura = bot->GetAura(SPELL_TORAVON_FROSTBITE);
    if (!aura || aura->GetStackAmount() < TORAVON_FROSTBITE_SWAP_STACKS)
        return 1.0f;

    if (dynamic_cast<CastTauntAction*>(action) ||
        dynamic_cast<CastDarkCommandAction*>(action) ||
        dynamic_cast<CastHandOfReckoningAction*>(action) ||
        dynamic_cast<CastGrowlAction*>(action))
        return 0.0f;

    return 1.0f;
}
