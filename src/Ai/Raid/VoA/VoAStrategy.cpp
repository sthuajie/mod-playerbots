/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "VoAStrategy.h"
#include "Action.h"
#include "Strategy.h"
#include "Trigger.h"
#include "VoAMultipliers.h"
#include "vector"

void RaidVoAStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    //
    // Emalon the Storm Watcher
    //
    triggers.push_back(new TriggerNode(
        "emalon lighting nova trigger",
        { NextAction("emalon lighting nova action", ACTION_RAID + 1) }));

    triggers.push_back(new TriggerNode(
        "emalon mark boss trigger",
        { NextAction("emalon mark boss action", ACTION_RAID) }));

    triggers.push_back(new TriggerNode(
        "emalon overcharge trigger",
        { NextAction("emalon overcharge action", ACTION_RAID) }));

    triggers.push_back(new TriggerNode(
        "emalon fall from floor trigger",
        { NextAction("emalon fall from floor action", ACTION_RAID) }));

    triggers.push_back(new TriggerNode(
        "emalon nature resistance trigger",
        { NextAction("emalon nature resistance action", ACTION_RAID) }));

    //
    // Koralon the Flame Watcher
    //

    triggers.push_back(new TriggerNode(
        "koralon fire resistance trigger",
        { NextAction("koralon fire resistance action", ACTION_RAID) }));

    //
    // Toravon the Ice Watcher
    //
    // Tank swap: the off-tank takes over once the tank actually holding Toravon reaches
    // TORAVON_FROSTBITE_SWAP_STACKS. The trigger itself is inactive unless Toravon is
    // present, alive and in combat, so this adds nothing to Emalon/Koralon pulls.
    triggers.push_back(new TriggerNode(
        "toravon frostbite swap trigger",
        { NextAction("toravon frostbite taunt action", ACTION_RAID + 1) }));

    // Frozen Orb priority for all DPS (tanks and healers excluded).
    triggers.push_back(new TriggerNode(
        "toravon frozen orb trigger",
        { NextAction("toravon attack frozen orb action", ACTION_RAID) }));
}

void RaidVoAStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
    multipliers.push_back(new VoAToravonMultiplier(botAI));
}
