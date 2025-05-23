#include "RaidIccActions.h"
#include "strategy/values/NearestNpcsValue.h"
#include "ObjectAccessor.h"
#include "RaidIccStrategy.h"
#include "Playerbots.h"
#include "Timer.h"
#include "Vehicle.h"
#include "GenericSpellActions.h"
#include "GenericActions.h"
#include <fstream>

enum CreatureIds {
    NPC_KOR_KRON_BATTLE_MAGE                    = 37117,
    NPC_KOR_KRON_AXETHROWER                     = 36968,
    NPC_KOR_KRON_ROCKETEER                      = 36982,

    NPC_SKYBREAKER_SORCERER                     = 37116,
    NPC_SKYBREAKER_RIFLEMAN                     = 36969,
    NPC_SKYBREAKER_MORTAR_SOLDIER               = 36978,

    NPC_IGB_HIGH_OVERLORD_SAURFANG              = 36939,
    NPC_IGB_MURADIN_BRONZEBEARD                 = 36948,
};

const std::vector<uint32> availableTargets = {
    NPC_KOR_KRON_AXETHROWER,        NPC_KOR_KRON_ROCKETEER,     NPC_KOR_KRON_BATTLE_MAGE,
    NPC_IGB_HIGH_OVERLORD_SAURFANG, NPC_SKYBREAKER_RIFLEMAN,    NPC_SKYBREAKER_MORTAR_SOLDIER,
    NPC_SKYBREAKER_SORCERER,        NPC_IGB_MURADIN_BRONZEBEARD
};

static std::vector<ObjectGuid> sporeOrder;

//Lord Marrowgwar
bool IccLmTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "lord marrowgar");
    if (!boss)
        return false;

    if (botAI->IsTank(bot))
    {
        if ((botAI->HasAggro(boss) && botAI->IsMainTank(bot)) || botAI->IsAssistTank(bot))
        {
            float distance = bot->GetExactDist2d(ICC_LM_TANK_POSITION.GetPositionX(), ICC_LM_TANK_POSITION.GetPositionY());
            if (distance > 3.0f)
            {
                // Calculate direction vector
                float dirX = ICC_LM_TANK_POSITION.GetPositionX() - bot->GetPositionX();
                float dirY = ICC_LM_TANK_POSITION.GetPositionY() - bot->GetPositionY();
                float length = sqrt(dirX * dirX + dirY * dirY);
                dirX /= length;
                dirY /= length;

                // Move in increments of 3.0f
                float moveX = bot->GetPositionX() + dirX * 3.0f;
                float moveY = bot->GetPositionY() + dirY * 3.0f;

                return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(), false, false, false, false,
                              MovementPriority::MOVEMENT_COMBAT);
            }
        }
    }

    return false;
}

bool IccSpikeAction::Execute(Event event)
{
    Aura* aura = botAI->GetAura("Impaled", bot);

    // If we're impaled, we can't do anything
    if (aura)
        return false;

    // Find the boss
    Unit* boss = AI_VALUE2(Unit*, "find target", "lord marrowgar");
    if (!boss)
        return false;

    Aura* bossaura = botAI->GetAura("Bone Storm", boss);

    if (boss->isInFront(bot) && !botAI->IsTank(bot) && !bossaura)
    {
        float distance = bot->GetExactDist2d(-390.6757f, 2230.5283f);
        if (distance > 3.0f)
        {
            // Calculate direction vector
            float dirX = -390.6757f - bot->GetPositionX();
            float dirY = 2230.5283f - bot->GetPositionY();
            float length = sqrt(dirX * dirX + dirY * dirY);
            dirX /= length;
            dirY /= length;

            // Move in increments of 3.0f
            float moveX = bot->GetPositionX() + dirX * 3.0f;
            float moveY = bot->GetPositionY() + dirY * 3.0f;

            return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_COMBAT);
        }
        return false;
    }

    if (!botAI->IsTank(bot))
        return false;

    GuidVector spikes = AI_VALUE(GuidVector, "possible targets no los");
    const uint32 spikeEntries[] = {36619, 38711, 38712};  // spikes id's

    Unit* priorityTarget = nullptr;
    bool anySpikesExist = false;

    // First check for alive spikes
    for (uint32 entry : spikeEntries)
    {
        for (const ObjectGuid& guid : spikes)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && unit->GetEntry() == entry)  // Check if spike exists
            {
                anySpikesExist = true;  // At least one spike exists

                if (unit->IsAlive())  // Only consider alive ones for targeting
                {
                    priorityTarget = unit;
                    break;
                }
            }
        }
        if (priorityTarget)
            break;
    }

    // Only fallback to boss if NO spikes exist at all (alive or dead)
    if (!anySpikesExist && boss->IsAlive())
    {
        priorityTarget = boss;
    }

    // Update skull icon if needed
    if (priorityTarget)
    {
        if (Group* group = bot->GetGroup())
        {
            ObjectGuid currentSkull = group->GetTargetIcon(7);
            Unit* currentSkullUnit = botAI->GetUnit(currentSkull);

            bool needsUpdate = false;
            if (!currentSkullUnit || !currentSkullUnit->IsAlive())
            {
                needsUpdate = true;
            }
            else if (currentSkullUnit != priorityTarget)
            {
                needsUpdate = true;
            }

            if (needsUpdate)
            {
                group->SetTargetIcon(7, bot->GetGUID(), priorityTarget->GetGUID());
            }
        }
    }

    return false;

}

//Lady
bool IccDarkReckoningAction::Execute(Event event)
{
    if (bot->HasAura(69483) && bot->GetExactDist2d(ICC_DARK_RECKONING_SAFE_POSITION) > 2.0f) //dark reckoning spell id
    {
        return MoveTo(bot->GetMapId(), ICC_DARK_RECKONING_SAFE_POSITION.GetPositionX(),
                      ICC_DARK_RECKONING_SAFE_POSITION.GetPositionY(), ICC_DARK_RECKONING_SAFE_POSITION.GetPositionZ(), false,
                      false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }
    return false;
}

bool IccRangedPositionLadyDeathwhisperAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "lady deathwhisper");
    if (!boss)
        return false;

    float currentDistance = bot->GetDistance2d(boss);

    if (currentDistance < 7.0f || currentDistance > 30.0f)
        return false;

    if (botAI->IsRanged(bot) || botAI->IsHeal(bot))
    {
    float radius = 3.0f;
    float moveIncrement = 2.0f;
    bool isRanged = botAI->IsRanged(bot);

    GuidVector members = AI_VALUE(GuidVector, "group members");
    if (isRanged)
    {
        // Ranged: spread from other members
        for (auto& member : members)
        {
            Unit* unit = botAI->GetUnit(member);
            if (!unit || !unit->IsAlive() || unit == bot)
                continue;

            float dist = bot->GetExactDist2d(unit);
            if (dist < radius)
            {
                float moveDistance = std::min(moveIncrement, radius - dist + 1.0f);
                return FleePosition(unit->GetPosition(), moveDistance, 250U);
                // return MoveAway(unit, moveDistance);
            }
        }
    }

    return false;  // Everyone is in position
    }
    return false;
}

bool IccAddsLadyDeathwhisperAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "lady deathwhisper");
    if (!boss)
        return false;

    if (botAI->IsTank(bot) && boss->GetHealthPct() < 95.0f)
    {
        // Check if the bot is not the victim of a shade with entry 38222
        GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
        for (auto& npc : npcs)
        {
            Unit* unit = botAI->GetUnit(npc);
            if (unit && unit->GetEntry() == 38222 && unit->GetVictim() == bot)
            {
                return false;  // Exit if the bot is the victim of the shade
            }
        }

        float distance = bot->GetExactDist2d(ICC_LDW_TANK_POSTION.GetPositionX(), ICC_LDW_TANK_POSTION.GetPositionY());
        if (distance > 20.0f)
        {
            // Calculate direction vector
            float dirX = ICC_LDW_TANK_POSTION.GetPositionX() - bot->GetPositionX();
            float dirY = ICC_LDW_TANK_POSTION.GetPositionY() - bot->GetPositionY();
            float length = sqrt(dirX * dirX + dirY * dirY);
            dirX /= length;
            dirY /= length;

            // Move in increments of 3.0f
            float moveX = bot->GetPositionX() + dirX * 3.0f;
            float moveY = bot->GetPositionY() + dirY * 3.0f;

            return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(), false, false, false, false,
                          MovementPriority::MOVEMENT_COMBAT);
        }
    }

    if (!botAI->IsTank(bot))
        return false;

    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    
    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    const uint32 targetsEntries[] = {37949, 38394, 38625, 38626, 38010, 38397, 39000, 39001, 38136, 38396, 38632, 38633, 37890, 38393, 38628, 38629, 38135, 38395, 38634, 38009, 38398, 38630, 38631}; //fanatics and adherents

    Unit* priorityTarget = nullptr;
    bool hasValidAdds = false;

    // First check for alive adds
    for (uint32 entry : targetsEntries)
    {
        for (const ObjectGuid& guid : targets)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && unit->IsAlive() && unit->GetEntry() == entry)
            {
                priorityTarget = unit;
                hasValidAdds = true;
                break;
            }
        }
        if (priorityTarget)
            break;
    }

    // Only fallback to boss if NO adds exist
    if (!hasValidAdds && boss->IsAlive())
    {
        priorityTarget = boss;
    }

    // Update skull icon if needed
    if (priorityTarget)
    {
        if (Group* group = bot->GetGroup())
        {
            ObjectGuid currentSkull = group->GetTargetIcon(7);
            Unit* currentSkullUnit = botAI->GetUnit(currentSkull);

            bool needsUpdate = false;
            if (!currentSkullUnit || !currentSkullUnit->IsAlive())
            {
                needsUpdate = true;  // No valid skull target
            }
            else if (currentSkullUnit != priorityTarget)
            {
                needsUpdate = true;  // Different target than desired
            }

            if (needsUpdate)
            {
                group->SetTargetIcon(7, bot->GetGUID(), priorityTarget->GetGUID());
            }
        }
    }

    return false;

}

bool IccShadeLadyDeathwhisperAction::Execute(Event event)
{
    const float radius = 12.0f;

    // Get the nearest hostile NPCs
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || unit->GetEntry() != 38222)  // vengeful shade ID
            continue;

        // Only run away if the shade is targeting us
        // Check by GUID comparison to ensure we're accurately identifying the specific shade in 25HC multiple shades spawn.
        if (unit->GetVictim() && unit->GetVictim()->GetGUID() == bot->GetGUID())
        {
            float currentDistance = bot->GetDistance2d(unit);

            // Move away from the Vengeful Shade if the bot is too close
            if (currentDistance < radius)
            {
                botAI->Reset(); // forces bot to stop channeling or getting locked by any other action
                return MoveAway(unit, radius - currentDistance);
            }
        }
    }
    return false;
}

bool IccRottingFrostGiantTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "rotting frost giant");
    if (!boss)
        return false;

    bot->SetTarget(boss->GetGUID());
    if (botAI->IsTank(bot) || botAI->IsMainTank(bot) || botAI->IsAssistTank(bot))
    {
        if (bot->GetExactDist2d(ICC_ROTTING_FROST_GIANT_TANK_POSITION) > 5.0f)
            return MoveTo(bot->GetMapId(), ICC_ROTTING_FROST_GIANT_TANK_POSITION.GetPositionX(),
                          ICC_ROTTING_FROST_GIANT_TANK_POSITION.GetPositionY(), ICC_ROTTING_FROST_GIANT_TANK_POSITION.GetPositionZ(), false,
                          false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }

    float radius = 10.0f;
    float distanceExtra = 2.0f;

    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        if (bot->GetGUID() == member)
        {
            continue;
        }

        Unit* unit = botAI->GetUnit(member);
        if (unit && (botAI->IsHeal(bot) || botAI->IsDps(bot)) && bot->GetExactDist2d(unit) < radius)
        {
            return FleePosition(unit->GetPosition(), radius + distanceExtra - bot->GetExactDist2d(unit));
            // return MoveAway(unit, radius + distanceExtra - bot->GetExactDist2d(unit));
        }
    }
    return Attack(boss);
}

//Gunship
bool IccCannonFireAction::Execute(Event event)
{
    Unit* vehicleBase = bot->GetVehicleBase();
    Vehicle* vehicle = bot->GetVehicle();
    if (!vehicleBase || !vehicle)
        return false;

    GuidVector attackers = AI_VALUE(GuidVector, "possible targets no los");

    Unit* target = nullptr;
    for (auto i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (!unit)
            continue;
        for (uint32 entry : availableTargets)
        {
            if (unit->GetEntry() == entry) {
                target = unit;
                break;
            }
        }
        if (target)
            break;
    }
    if (!target)
        return false;

    if  (vehicleBase->GetPower(POWER_ENERGY) >= 90) {
        uint32 spellId = AI_VALUE2(uint32, "vehicle spell id", "incinerating blast");
        if (botAI->CanCastVehicleSpell(spellId, target) && botAI->CastVehicleSpell(spellId, target)) {
            vehicleBase->AddSpellCooldown(spellId, 0, 1000);
            return true;
        }
    }
    uint32 spellId = AI_VALUE2(uint32, "vehicle spell id", "cannon blast");
    if (botAI->CanCastVehicleSpell(spellId, target) && botAI->CastVehicleSpell(spellId, target)) {
        vehicleBase->AddSpellCooldown(spellId, 0, 1000);
        return true;
    }
    return false;
}

bool IccGunshipEnterCannonAction::Execute(Event event)
{
    // do not switch vehicles yet
    if (bot->GetVehicle())
        return false;

    Unit* vehicleToEnter = nullptr;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest vehicles");
    for (GuidVector::iterator i = npcs.begin(); i != npcs.end(); i++)
    {
        Unit* vehicleBase = botAI->GetUnit(*i);
        if (!vehicleBase)
            continue;

        if (vehicleBase->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
            continue;
        
        if (!vehicleBase->IsFriendlyTo(bot))
            continue;
        
        if (!vehicleBase->GetVehicleKit() || !vehicleBase->GetVehicleKit()->GetAvailableSeatCount())
            continue;

        uint32 entry = vehicleBase->GetEntry();
        if (entry != 36838 && entry != 36839)
            continue;
        
        if (vehicleBase->HasAura(69704) || vehicleBase->HasAura(69705))
            continue;
        
        if (!vehicleToEnter || bot->GetExactDist(vehicleToEnter) > bot->GetExactDist(vehicleBase))
            vehicleToEnter = vehicleBase;
    }

    if (!vehicleToEnter)
        return false;
    
    if (EnterVehicle(vehicleToEnter, true))
        return true;

    return false;
}

bool IccGunshipEnterCannonAction::EnterVehicle(Unit* vehicleBase, bool moveIfFar)
{
    float dist = bot->GetDistance(vehicleBase);
    
    if (dist > INTERACTION_DISTANCE && !moveIfFar)
        return false;

    if (dist > INTERACTION_DISTANCE)
        return MoveTo(vehicleBase);


    botAI->RemoveShapeshift();

    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    vehicleBase->HandleSpellClick(bot);

    if (!bot->IsOnVehicle(vehicleBase))
        return false;

    // dismount because bots can enter vehicle on mount
    WorldPacket emptyPacket;
    bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);
    return true;
}

bool IccGunshipTeleportAllyAction::Execute(Event event)
{
    // Find the Battle-Mage boss
    Unit* boss = AI_VALUE2(Unit*, "find target", "kor'kron battle-mage");

    // Check if we need to remove skull icon when boss is dead
    if (Group* group = bot->GetGroup())
    {
        ObjectGuid currentSkullTarget = group->GetTargetIcon(7);
        if (!currentSkullTarget.IsEmpty())
        {
            // If the current skull target is dead or doesn't exist, remove the icon
            if (Unit* skullTarget = ObjectAccessor::GetUnit(*bot, currentSkullTarget))
            {
                if (!skullTarget->IsAlive())
                    group->SetTargetIcon(7, bot->GetGUID(), ObjectGuid::Empty);
            }
            else
            {
                // Target not found, might have despawned, remove icon
                group->SetTargetIcon(7, bot->GetGUID(), ObjectGuid::Empty);
            }
        }
    }

    // If no boss found or boss is dead, nothing more to do
    if (!boss || !boss->IsAlive() || !boss->HasUnitState(UNIT_STATE_CASTING))
    {
        // If we're too far from waiting position, go there
        if (bot->GetExactDist2d(ICC_GUNSHIP_TELEPORT_ALLY2) > 45.0f)
            return bot->TeleportTo(bot->GetMapId(), ICC_GUNSHIP_TELEPORT_ALLY2.GetPositionX(),
                                   ICC_GUNSHIP_TELEPORT_ALLY2.GetPositionY(),
                                   ICC_GUNSHIP_TELEPORT_ALLY2.GetPositionZ(), bot->GetOrientation());
    }
    else if (boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(69705) && boss->IsAlive())
    {
        // Mark the boss with skull icon
        if (Group* group = bot->GetGroup())
            if (group->GetTargetIcon(7) != boss->GetGUID())
                group->SetTargetIcon(7, bot->GetGUID(), boss->GetGUID());

        // Teleport non-tank bots to attack position if not already there
        if (!botAI->IsAssistTank(bot) && bot->GetExactDist2d(ICC_GUNSHIP_TELEPORT_ALLY) > 15.0f)
            return bot->TeleportTo(bot->GetMapId(), ICC_GUNSHIP_TELEPORT_ALLY.GetPositionX(),
                                   ICC_GUNSHIP_TELEPORT_ALLY.GetPositionY(), ICC_GUNSHIP_TELEPORT_ALLY.GetPositionZ(),
                                   bot->GetOrientation());
    }

    return false;
}

bool IccGunshipTeleportHordeAction::Execute(Event event)
{
    // Find the Sorcerer boss
    Unit* boss = AI_VALUE2(Unit*, "find target", "skybreaker sorcerer");

    // Check if we need to remove skull icon when boss is dead
    if (Group* group = bot->GetGroup())
    {
        ObjectGuid currentSkullTarget = group->GetTargetIcon(7);
        if (!currentSkullTarget.IsEmpty())
        {
            // If the current skull target is dead or doesn't exist, remove the icon
            if (Unit* skullTarget = ObjectAccessor::GetUnit(*bot, currentSkullTarget))
            {
                if (!skullTarget->IsAlive())
                    group->SetTargetIcon(7, bot->GetGUID(), ObjectGuid::Empty);
            }
            else
            {
                // Target not found, might have despawned, remove icon
                group->SetTargetIcon(7, bot->GetGUID(), ObjectGuid::Empty);
            }
        }
    }

    // If no boss found or boss is dead, nothing more to do
    if (!boss || !boss->IsAlive() || !boss->HasUnitState(UNIT_STATE_CASTING))
    {
        // If we're too far from waiting position, go there
        if (bot->GetExactDist2d(ICC_GUNSHIP_TELEPORT_HORDE2) > 45.0f)
            return bot->TeleportTo(bot->GetMapId(), ICC_GUNSHIP_TELEPORT_HORDE2.GetPositionX(),
                                   ICC_GUNSHIP_TELEPORT_HORDE2.GetPositionY(),
                                   ICC_GUNSHIP_TELEPORT_HORDE2.GetPositionZ(), bot->GetOrientation());
    }
    else if (boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(69705) && boss->IsAlive())
    {
        // Mark the boss with skull icon
        if (Group* group = bot->GetGroup())
            if (group->GetTargetIcon(7) != boss->GetGUID())
                group->SetTargetIcon(7, bot->GetGUID(), boss->GetGUID());

        // Teleport non-tank bots to attack position if not already there
        if (!botAI->IsAssistTank(bot) && bot->GetExactDist2d(ICC_GUNSHIP_TELEPORT_HORDE) > 15.0f)
            return bot->TeleportTo(bot->GetMapId(), ICC_GUNSHIP_TELEPORT_HORDE.GetPositionX(),
                                   ICC_GUNSHIP_TELEPORT_HORDE.GetPositionY(), ICC_GUNSHIP_TELEPORT_HORDE.GetPositionZ(),
                                   bot->GetOrientation());
    }

    return false;
}

//DBS
bool IccDbsTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "deathbringer saurfang");
    if (!boss)
        return false;

    bot->SetTarget(boss->GetGUID());
    if (botAI->IsTank(bot) || botAI->IsMainTank(bot) || botAI->IsAssistTank(bot))
    {
        if (bot->GetExactDist2d(ICC_DBS_TANK_POSITION) > 5.0f)
            return MoveTo(bot->GetMapId(), ICC_DBS_TANK_POSITION.GetPositionX(), ICC_DBS_TANK_POSITION.GetPositionY(),
                          ICC_DBS_TANK_POSITION.GetPositionZ(), false, false, false, true,
                          MovementPriority::MOVEMENT_NORMAL);
    }

    if (botAI->GetAura("Rune of Blood", bot) && botAI->IsTank(bot))
        return true;

    if (botAI->IsRanged(bot) || botAI->IsHeal(bot))
    {
        const float evasion = 12.0f;

        // Get the nearest hostile NPCs
        GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
        for (auto& npc : npcs)
        {
            Unit* unit = botAI->GetUnit(npc);
            if (unit && (unit->GetEntry() == 38508 || unit->GetEntry() == 38596 || unit->GetEntry() == 38597 ||
                         unit->GetEntry() == 38598))  // blood beast
            {
                // Only run away if the blood beast is targeting us
                if (unit->GetVictim() == bot)
                {
                    float currentDistance = bot->GetDistance2d(unit);

                    // Move away from the blood beast if the bot is too close
                    if (currentDistance < evasion)
                    {
                        return MoveAway(unit, evasion - currentDistance);
                    }
                }
            }
        }

            // Get group and position in group
            Group* group = bot->GetGroup();
            if (!group)
                return false;

            // Find this bot's position among ranged/healers in the group
            int rangedIndex = -1;
            int currentIndex = 0;

            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAlive())
                    continue;

                if ((botAI->IsRanged(member) || botAI->IsHeal(member)) && !botAI->IsTank(member))
                {
                    if (member == bot)
                    {
                        rangedIndex = currentIndex;
                        break;
                    }
                    currentIndex++;
                }
            }

            if (rangedIndex == -1)
                return false;

            // Fixed positions calculation
            float tankToBossAngle = 3.14f;
            const float minBossDistance = 11.0f;
            const float spreadDistance = 10.0f;

            // Calculate position in a fixed grid (3 rows x 5 columns)
            int row = rangedIndex / 5;
            int col = rangedIndex % 5;

            // Calculate base position
            float xOffset = (col - 2) * spreadDistance;                // Center around tank position
            float yOffset = minBossDistance + (row * spreadDistance);  // Each row further back

            // Add zigzag offset for odd rows
            if (row % 2 == 1)
                xOffset += spreadDistance / 2;

            // Rotate position based on tank-to-boss angle
            float finalX = ICC_DBS_TANK_POSITION.GetPositionX() +
                           (cos(tankToBossAngle) * yOffset - sin(tankToBossAngle) * xOffset);
            float finalY = ICC_DBS_TANK_POSITION.GetPositionY() +
                           (sin(tankToBossAngle) * yOffset + cos(tankToBossAngle) * xOffset);
            float finalZ = ICC_DBS_TANK_POSITION.GetPositionZ();

            // Update Z coordinate
            bot->UpdateAllowedPositionZ(finalX, finalY, finalZ);

            // Move if not in position
            if (bot->GetExactDist2d(finalX, finalY) > 3.0f)
                return MoveTo(bot->GetMapId(), finalX, finalY, finalZ, false, false, false, true,
                              MovementPriority::MOVEMENT_COMBAT);

            return false;
        }

        return false;
    }

bool IccAddsDbsAction::Execute(Event event)
{

    Unit* boss = AI_VALUE2(Unit*, "find target", "deathbringer saurfang");
    if (!boss)
        return false;

    if (!botAI->IsMelee(bot))
        return false;

    Unit* currentTarget = AI_VALUE(Unit*, "current target");

    GuidVector targets = AI_VALUE(GuidVector, "possible targets no los");

    const uint32 targetsEntries[] = {38508, 38596, 38597, 38598};  // adds

    Unit* priorityTarget = nullptr;
    bool hasValidAdds = false;

    // First check for alive adds
    for (uint32 entry : targetsEntries)
    {
        for (const ObjectGuid& guid : targets)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (unit && unit->IsAlive() && unit->GetEntry() == entry)
            {
                priorityTarget = unit;
                hasValidAdds = true;
                break;
            }
        }
        if (priorityTarget)
            break;
    }

    // Only fallback to boss if NO adds exist
    if (!hasValidAdds && boss->IsAlive())
    {
        priorityTarget = boss;
    }

    // Update skull icon if needed
    if (priorityTarget)
    {
        if (Group* group = bot->GetGroup())
        {
            ObjectGuid currentSkull = group->GetTargetIcon(7);
            Unit* currentSkullUnit = botAI->GetUnit(currentSkull);

            bool needsUpdate = false;
            if (!currentSkullUnit || !currentSkullUnit->IsAlive())
            {
                needsUpdate = true;  // No valid skull target
            }
            else if (currentSkullUnit != priorityTarget)
            {
                needsUpdate = true;  // Different target than desired
            }

            if (needsUpdate)
            {
                group->SetTargetIcon(7, bot->GetGUID(), priorityTarget->GetGUID());
            }
        }
    }

    return false;
}

 //FESTERGUT
bool IccFestergutTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "festergut");
    if (!boss)
    return false;

    bot->SetTarget(boss->GetGUID());
    if (botAI->IsTank(bot) || botAI->IsMainTank(bot) || botAI->IsAssistTank(bot))
    {
        if (bot->GetExactDist2d(ICC_FESTERGUT_TANK_POSITION) > 5.0f)
        return MoveTo(bot->GetMapId(), ICC_FESTERGUT_TANK_POSITION.GetPositionX(),
                      ICC_FESTERGUT_TANK_POSITION.GetPositionY(), ICC_FESTERGUT_TANK_POSITION.GetPositionZ(), false,
                      false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }

    GuidVector members = AI_VALUE(GuidVector, "group members");
    bool sporesPresent = false;
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit)
            continue;

        if (unit->HasAura(69279))
        {
            sporesPresent = true;
            break;
        }
    }

    if (!sporesPresent && (botAI->IsRanged(bot) || botAI->IsHeal(bot)))
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        // Separate counters for healers and ranged DPS
        int healerIndex = -1;
        int rangedDpsIndex = -1;
        int currentHealerIndex = 0;
        int currentRangedDpsIndex = 0;
        
        // First pass: count total healers and ranged
        int totalHealers = 0;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || botAI->IsTank(member))
                continue;

            if (botAI->IsHeal(member))
                totalHealers++;
        }

        // Second pass: assign positions
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || botAI->IsTank(member))
                continue;

                if (member == bot)
                {
                if (botAI->IsHeal(bot))
                    healerIndex = currentHealerIndex;
                else if (botAI->IsRanged(bot))
                    rangedDpsIndex = currentRangedDpsIndex;
                    break;
                }

            if (botAI->IsHeal(member))
                currentHealerIndex++;
            else if (botAI->IsRanged(member))
                currentRangedDpsIndex++;
            }

        int positionIndex;
        if (healerIndex != -1)
        {
            // Healers get positions in first two rows
            int healersPerRow = (totalHealers + 1) / 2; // Round up
            positionIndex = healerIndex;
            // Ensure healer is in first two rows
            if (positionIndex >= healersPerRow)
            {
                positionIndex = positionIndex - healersPerRow + 5; // Move to second row
            }
        }
        else if (rangedDpsIndex != -1)
        {
            // Ranged DPS start from where healers end
            positionIndex = totalHealers + rangedDpsIndex;
        }
        else
            return false;

        // Fixed positions calculation
        float tankToBossAngle = 4.58f;
        const float minBossDistance = 20.0f;
        const float spreadDistance = 10.0f;
        
        // Calculate position in a fixed grid (3 rows x 5 columns)
        int row = positionIndex / 5;
        int col = positionIndex % 5;
        
        // Calculate base position
        float xOffset = (col - 2) * spreadDistance; // Center around tank position
        float yOffset = minBossDistance + (row * spreadDistance); // Each row further back
        
        // Add zigzag offset for odd rows
        if (row % 2 == 1)
            xOffset += spreadDistance / 2;

        // Rotate position based on tank-to-boss angle
        float finalX = ICC_FESTERGUT_TANK_POSITION.GetPositionX() + (cos(tankToBossAngle) * yOffset - sin(tankToBossAngle) * xOffset);
        float finalY = ICC_FESTERGUT_TANK_POSITION.GetPositionY() + (sin(tankToBossAngle) * yOffset + cos(tankToBossAngle) * xOffset);
        float finalZ = ICC_FESTERGUT_TANK_POSITION.GetPositionZ();
        
        // Update Z coordinate
        bot->UpdateAllowedPositionZ(finalX, finalY, finalZ);

        // Move if not in position
        if (bot->GetExactDist2d(finalX, finalY) > 3.0f)
            return MoveTo(bot->GetMapId(), finalX, finalY, finalZ,
                         false, false, false, true, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool IccFestergutSporeAction::Execute(Event event)
{
    const float POSITION_TOLERANCE = 4.0f;
    const float SPREAD_RADIUS = 2.0f;  // How far apart ranged should spread

    bool hasSpore = bot->HasAura(69279); // gas spore
    
    // If bot has spore, stop attacking
    if (hasSpore)
    {
        bot->AttackStop();
    }

    // Calculate a unique spread position for ranged
    float angle = (bot->GetGUID().GetCounter() % 16) * (M_PI / 8); // Divide circle into 16 positions
    Position spreadRangedPos = ICC_FESTERGUT_RANGED_SPORE;
    spreadRangedPos.m_positionX += cos(angle) * SPREAD_RADIUS;
    spreadRangedPos.m_positionY += sin(angle) * SPREAD_RADIUS;

    // Find all spored players and the one with lowest GUID
    ObjectGuid lowestGuid;
    bool isFirst = true;
    std::vector<Unit*> sporedPlayers;
    
    GuidVector members = AI_VALUE(GuidVector, "group members");
    for (auto& member : members)
    {
        Unit* unit = botAI->GetUnit(member);
        if (!unit)
            continue;

        if (unit->HasAura(69279))
        {
            sporedPlayers.push_back(unit);
            if (isFirst || unit->GetGUID() < lowestGuid)
            {
                lowestGuid = unit->GetGUID();
                isFirst = false;
            }
        }
    }

    // If no spores present at all, return
    if (sporedPlayers.empty())
        return false;

    Position targetPos;
    if (hasSpore)
    {
        bool mainTankHasSpore = false;
        GuidVector members = AI_VALUE(GuidVector, "group members");
        for (auto& member : members)
        {
            Unit* unit = botAI->GetUnit(member);
            if (!unit)
                continue;

            if (botAI->IsMainTank(unit->ToPlayer()) && unit->HasAura(69279))
            {
                mainTankHasSpore = true;
                break;
            }
        }

        // If bot is main tank, always go melee regardless of GUID
        if (botAI->IsMainTank(bot))
        {
            targetPos = ICC_FESTERGUT_MELEE_SPORE;
        }
        // If this bot has the lowest GUID among spored players AND is not a tank AND main tank is not spored
        else if (bot->GetGUID() == lowestGuid && !botAI->IsTank(bot) && !mainTankHasSpore)
        {
            targetPos = ICC_FESTERGUT_MELEE_SPORE;
        }
        // All other spored players go ranged
        else
        {
            targetPos = spreadRangedPos;
        }
    }
    else
    {
        // If bot doesn't have spore, go to position based on role
        targetPos = botAI->IsMelee(bot) ? ICC_FESTERGUT_MELEE_SPORE : spreadRangedPos;
    }

    // Only move if we're not already at the target position
    if (bot->GetExactDist2d(targetPos) > POSITION_TOLERANCE)
    {
        return MoveTo(bot->GetMapId(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(),
                     true, false, false, true, MovementPriority::MOVEMENT_FORCED);
    }

    return hasSpore;
}

//ROTFACE
bool IccRotfaceTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "rotface");
    if (!boss)
    return false;

    // Mark Rotface with skull if not already marked
    if (Group* group = bot->GetGroup())
    {
        ObjectGuid skullGuid = group->GetTargetIcon(7); // 7 = skull
        if (!skullGuid || !botAI->GetUnit(skullGuid))
        {
            group->SetTargetIcon(7, bot->GetGUID(), boss->GetGUID());
        }
    }

    // Main tank positioning logic
    if (botAI->IsMainTank(bot))
    {
        if (bot->GetExactDist2d(ICC_ROTFACE_TANK_POSITION) > 7.0f)
        return MoveTo(bot->GetMapId(), ICC_ROTFACE_TANK_POSITION.GetPositionX(),
                      ICC_ROTFACE_TANK_POSITION.GetPositionY(), ICC_ROTFACE_TANK_POSITION.GetPositionZ(), false,
                      false, false, true, MovementPriority::MOVEMENT_COMBAT);
    }

    bool hasOozeFlood = botAI->HasAura("Ooze Flood", bot);

    // Assist tank positioning for big ooze
    if (botAI->IsAssistTank(bot))
    {
        // If we have the ooze flood aura, move away
        if (hasOozeFlood)
        {
            return MoveTo(boss->GetMapId(), boss->GetPositionX() + 5.0f * cos(bot->GetAngle(boss)),
                         boss->GetPositionY() + 5.0f * sin(bot->GetAngle(boss)), bot->GetPositionZ(), false,
                         false, false, true, MovementPriority::MOVEMENT_COMBAT);
        }

        Unit* bigOoze = AI_VALUE2(Unit*, "find target", "big ooze");
        if (bigOoze)
        {
            // Taunt if not targeting us
            if (bigOoze->GetVictim() != bot)
            {
                if (botAI->CastSpell("taunt", bigOoze))
                    return true;
                return Attack(bigOoze);
            }

            // Keep big ooze at designated position
            if (bigOoze->GetVictim() == bot)
            {
                if (bot->GetExactDist2d(ICC_ROTFACE_BIG_OOZE_POSITION) > 5.0f)
                {
                    return MoveTo(bot->GetMapId(), ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionX(),
                              ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionY(), ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionZ(),
                              false, false, false, true, MovementPriority::MOVEMENT_COMBAT);
                }
                return Attack(bigOoze);
            }

            return Attack(bigOoze);
        }
    }

    return false;
}

bool IccRotfaceGroupPositionAction::Execute(Event event)
{
    // Find Rotface
    Unit* boss = AI_VALUE2(Unit*, "find target", "rotface");
    if (!boss)
        return false;   

 

    // Check for puddles and move away if too close
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    bool hasOozeFlood = botAI->HasAura("Ooze Flood", bot);
    
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (unit)
        {
            if (unit->GetEntry() == 37013) // puddle
            {
                float puddleDistance = bot->GetExactDist2d(unit);
               
                
                if (puddleDistance < 30.0f && (hasOozeFlood))
                {
                    float dx = boss->GetPositionX() - unit->GetPositionX();
                    float dy = boss->GetPositionY() - unit->GetPositionY();
                    float angle = atan2(dy, dx);
                    
                    // Move away from puddle in smaller increment
                    float moveDistance = std::min(35.0f - puddleDistance, 5.0f);
                    float moveX = boss->GetPositionX() + (moveDistance * cos(angle));
                    float moveY = boss->GetPositionY() + (moveDistance * sin(angle));
                    
                    // Check if position is in LoS before moving
                    if (!bot->IsWithinLOS(moveX, moveY, boss->GetPositionZ()))
                        return false;
                    
                    return MoveTo(boss->GetMapId(), moveX, moveY, boss->GetPositionZ(), 
                        false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
                }
            }
        }
    }

    // Check if we're targeted by little ooze
    Unit* smallOoze = AI_VALUE2(Unit*, "find target", "little ooze");
    bool hasMutatedInfection = botAI->HasAura("Mutated Infection", bot);

    if ((smallOoze && smallOoze->GetVictim() == bot) || hasMutatedInfection)
    {
        if (bot->GetExactDist2d(ICC_ROTFACE_BIG_OOZE_POSITION) > 3.0f)
        {
            // Check if position is in LoS before moving
            if (!bot->IsWithinLOS(ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionX(),
                ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionY(),
                ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionZ()))
                return false;

            return MoveTo(bot->GetMapId(), ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionX(),
                        ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionY(), ICC_ROTFACE_BIG_OOZE_POSITION.GetPositionZ(),
                        false, false, false, true, MovementPriority::MOVEMENT_COMBAT);
        }
        return true; // Stay at position
    }

    if(botAI->IsRanged(bot) || botAI->IsHeal(bot))
    {
        if (!hasOozeFlood)
        {
            float radius = 10.0f;
            Unit* closestMember = nullptr;
            float minDist = radius;
            GuidVector members = AI_VALUE(GuidVector, "group members");
        
            for (auto& member : members)
            {
                Unit* unit = botAI->GetUnit(member);
                if (!unit || bot->GetGUID() == member)
                    continue;

                // Skip distance check if the other unit is an assist tank
                if (botAI->IsAssistTank(bot))
                    continue;

             float dist = bot->GetExactDist2d(unit);
                if (dist < minDist)
                {
                    minDist = dist;
                    closestMember = unit;
                }
            }

         if (closestMember)
            {
                float distToCenter = bot->GetExactDist2d(ICC_ROTFACE_TANK_POSITION);
                float moveDistance = (distToCenter > 25.0f) ? 2.0f : 3.0f;
                // return MoveAway(closestMember, moveDistance);
                return FleePosition(closestMember->GetPosition(), moveDistance, 250U);
            }
            
            return false;
        }
    }

    return false;
}

bool IccRotfaceMoveAwayFromExplosionAction::Execute(Event event)
{
    if (botAI->IsMainTank(bot) || bot->HasAura(71215))
        return false;

    // Stop current actions first
    bot->AttackStop();
    bot->InterruptNonMeleeSpells(false);
    
    // Generate random angle between 0 and 2π
    float angle = frand(0, 2 * M_PI);
    
    // Calculate position 20 yards away in random direction
    float moveX = bot->GetPositionX() + 20.0f * cos(angle);
    float moveY = bot->GetPositionY() + 20.0f * sin(angle);
    float moveZ = bot->GetPositionZ();
    
    // Check if position is in LoS before moving
    if (!bot->IsWithinLOS(moveX, moveY, moveZ))
        return false;
    
    // Move to the position
    return MoveTo(bot->GetMapId(), moveX, moveY, moveZ,
                 false, false, false, false, MovementPriority::MOVEMENT_FORCED);
}

//PP

bool IccPutricideGrowingOozePuddleAction::Execute(Event event)
{

    // Constants moved outside to prevent recreation on each call
    static const float BASE_RADIUS = 2.0f;
    static const float STACK_MULTIPLIER = 0.5f;
    static const float MIN_DISTANCE = 0.1f;
    static const float BUFFER_DISTANCE = 2.0f;
    static const uint32 GROWING_OOZE_PUDDLE_ID = 37690;
    static const uint32 GROW_AURA_ID = 70347;

    // Cache bot position to avoid multiple calls
    float botX = bot->GetPositionX();
    float botY = bot->GetPositionY();
    float botZ = bot->GetPositionZ();

    // Get the nearest hostile NPCs - only once and store locally
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    if (npcs.empty())
        return false;

    // Find puddles and their safe distances in one pass
    std::vector<std::tuple<Unit*, float, float>> puddles;  // Unit*, distance, safeDistance
    puddles.reserve(8);                                    // Pre-allocate a reasonable size

    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || unit->GetEntry() != GROWING_OOZE_PUDDLE_ID)
            continue;

        float currentDistance = std::max(MIN_DISTANCE, bot->GetExactDist(unit));

        float safeDistance = BASE_RADIUS;
        if (Aura* grow = unit->GetAura(GROW_AURA_ID))
            safeDistance += (grow->GetStackAmount() * STACK_MULTIPLIER);

        puddles.emplace_back(unit, currentDistance, safeDistance);
    }

    // If no puddles found, exit early
    if (puddles.empty())
        return false;

    // Find the closest threatening puddle
    Unit* closestPuddle = nullptr;
    float closestDistance = FLT_MAX;
    float closestSafeDistance = BASE_RADIUS;
    bool needToMove = false;

    for (const auto& [puddle, distance, safeDistance] : puddles)
    {
        if (distance < safeDistance && distance < closestDistance)
        {
            closestDistance = distance;
            closestSafeDistance = safeDistance;
            closestPuddle = puddle;
            needToMove = true;
        }
    }

    // If we don't need to move, exit early
    if (!needToMove)
        return false;

    // Calculate vector from puddle to bot
    float dx = botX - closestPuddle->GetPositionX();
    float dy = botY - closestPuddle->GetPositionY();
    float dist = std::max(MIN_DISTANCE, sqrt(dx * dx + dy * dy));

    // If we're too close or inside, pick a random direction to move
    if (dist < MIN_DISTANCE * 2)
    {
        float randomAngle = float(rand()) / float(RAND_MAX) * 2 * M_PI;
        dx = cos(randomAngle);
        dy = sin(randomAngle);
    }
    else
    {
        dx /= dist;
        dy /= dist;
    }

    // Calculate move distance once
    float moveDistance = closestSafeDistance - closestDistance + BUFFER_DISTANCE;

    // Try different angles to find a safe path
    const int numAngles = 8;
    for (int i = 0; i < numAngles; i++)
    {
        float angle = (2 * M_PI * i) / numAngles;
        float rotatedDx = dx * cos(angle) - dy * sin(angle);
        float rotatedDy = dx * sin(angle) + dy * cos(angle);

        float testX = botX + rotatedDx * moveDistance;
        float testY = botY + rotatedDy * moveDistance;

        // Skip LOS check if too close to other puddles
        bool tooCloseToOtherPuddle = false;
        for (const auto& [otherPuddle, _, otherSafeDistance] : puddles)
        {
            if (otherPuddle == closestPuddle)
                continue;

            float newDist =
                sqrt(pow(testX - otherPuddle->GetPositionX(), 2) + pow(testY - otherPuddle->GetPositionY(), 2));

            if (newDist < otherSafeDistance)
            {
                tooCloseToOtherPuddle = true;
                break;
            }
        }

        if (!tooCloseToOtherPuddle && bot->IsWithinLOS(testX, testY, botZ))
        {
            // Found a safe path, move there
            return MoveTo(bot->GetMapId(), testX, testY, botZ, false, false, false, false,
                          MovementPriority::MOVEMENT_COMBAT);
        }
    }

    // If we couldn't find a safe path, at least try to move away from the closest puddle
    return MoveTo(bot->GetMapId(), botX + dx * moveDistance, botY + dy * moveDistance, botZ, false, false, false, false,
                  MovementPriority::MOVEMENT_COMBAT);
}

bool IccPutricideVolatileOozeAction::Execute(Event event)
{
    const float STACK_DISTANCE = 8.0f;

    // Find the ooze
    Unit* ooze = AI_VALUE2(Unit*, "find target", "volatile ooze");
    if (!ooze)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "professor putricide");
    if (!boss)
        return false;

    if (botAI->IsTank(bot) && bot->GetExactDist2d(ICC_PUTRICIDE_TANK_POSITION) > 20.0f && !boss->HealthBelowPct(36) && boss->GetVictim() == bot)
    {
        return MoveTo(bot->GetMapId(), ICC_PUTRICIDE_TANK_POSITION.GetPositionX(),
                      ICC_PUTRICIDE_TANK_POSITION.GetPositionY(), ICC_PUTRICIDE_TANK_POSITION.GetPositionZ(), false, false,
                      false, true, MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    bool botHasAura = botAI->HasAura("Volatile Ooze Adhesive", bot);
    bool botHasAura2 = botAI->HasAura("Gaseous Bloat", bot);
    bool botHasAura3 = botAI->HasAura("Unbound Plague", bot);

    if (botHasAura2 || botHasAura3)
        return false;

    // Mark Volatile Ooze with skull if not already marked
    if (Group* group = bot->GetGroup())
    {
        ObjectGuid skullGuid = group->GetTargetIcon(7); // 7 = skull
        Unit* markedUnit = botAI->GetUnit(skullGuid);
        
        // Clear mark if current marked target is dead
        if (markedUnit && !markedUnit->IsAlive())
        {
            group->SetTargetIcon(7, bot->GetGUID(), ObjectGuid::Empty);
        }

        // Mark new ooze if it exists and nothing is marked
        if (ooze && ooze->IsAlive() && (!skullGuid || !markedUnit))
        {
            group->SetTargetIcon(7, bot->GetGUID(), ooze->GetGUID());
        }
    }

    // Check for aura on any group member
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Unit* auraTarget = nullptr;
    Unit* stackTarget = nullptr;
    bool anyoneHasAura = false;

    // First, try to find someone with the aura
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || member == bot)
            continue;
            
        if (botAI->HasAura("Volatile Ooze Adhesive", member))
        {
            anyoneHasAura = true;
            auraTarget = member;
            stackTarget = member;
            break;
        }
    }

    // If no one has aura, find a ranged player to stack with
    /*if (!anyoneHasAura && !stackTarget)
    {
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || member == bot || 
                botAI->IsTank(member) || botAI->HasAura("Gaseous Bloat", member) || 
                botAI->HasAura("Unbound Plague", member))
                continue;

            if (botAI->IsRanged(member) && )
            {
                stackTarget = member;
                break;
            }
        }
    }
    */

    /*
    // For melee old stacking
    if (botAI->IsMelee(bot) && !botAI->IsMainTank(bot))
    {
        // If ooze exists and someone has aura, attack the ooze
        if (ooze && anyoneHasAura)
        {
            bot->SetTarget(ooze->GetGUID());
            return Attack(ooze);
        }
        // Otherwise stack with ranged
        else if (stackTarget)
        {
            if (bot->GetDistance2d(stackTarget) > STACK_DISTANCE)
            {
                return MoveTo(bot->GetMapId(), stackTarget->GetPositionX(),
                            stackTarget->GetPositionY(), stackTarget->GetPositionZ(),
                            false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
            }
        }
    }
    */

    // For melee new only attacking
    if (botAI->IsMelee(bot) && !botAI->IsMainTank(bot))
    {
        // If ooze exists and someone has aura, attack the ooze
        if (ooze)
        {
            bot->SetTarget(ooze->GetGUID());
            return Attack(ooze);
        }
    }

    // For ranged and healers
    if (botAI->IsRanged(bot) || botAI->IsHeal(bot))
    {
        // Always try to stack
        if (stackTarget && bot->GetDistance2d(stackTarget) > STACK_DISTANCE)
        {
            return MoveTo(bot->GetMapId(), stackTarget->GetPositionX(),
                        stackTarget->GetPositionY(), stackTarget->GetPositionZ(),
                        false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }

        // If stacked and ooze exists, attack it (except healers)
        if (ooze && !botAI->IsHeal(bot) && stackTarget && 
            bot->GetDistance2d(stackTarget) <= STACK_DISTANCE)
        {
            bot->SetTarget(ooze->GetGUID());
            return Attack(ooze);
        }
        else if (botAI->IsHeal(bot))
        {
            return false; // Allow healer to continue with normal healing actions
        }
    }

    return false;
}

bool IccPutricideGasCloudAction::Execute(Event event)
{
    if (botAI->IsMainTank(bot)) 
        return false;

    Unit* gasCloud = AI_VALUE2(Unit*, "find target", "gas cloud");
    if (!gasCloud)
        return false;

    Unit* volatileOoze = AI_VALUE2(Unit*, "find target", "volatile ooze");

    Unit* boss = AI_VALUE2(Unit*, "find target", "professor putricide");
    if (!boss)
        return false;

    bool botHasAura = botAI->HasAura("Gaseous Bloat", bot);
    
    if(!botHasAura && volatileOoze)
        return false;

    if (botAI->IsTank(bot) && bot->GetExactDist2d(ICC_PUTRICIDE_TANK_POSITION) > 20.0f && !boss->HealthBelowPct(36) &&
        boss->GetVictim() == bot)
    {
        return MoveTo(bot->GetMapId(), ICC_PUTRICIDE_TANK_POSITION.GetPositionX(),
                      ICC_PUTRICIDE_TANK_POSITION.GetPositionY(), ICC_PUTRICIDE_TANK_POSITION.GetPositionZ(), false,
                      false, false, true, MovementPriority::MOVEMENT_COMBAT, true, false);
    }

    if (botHasAura)
    {
        float botX = bot->GetPositionX();
        float botY = bot->GetPositionY();
        float botZ = bot->GetPositionZ();
        
        float cloudX = gasCloud->GetPositionX();
        float cloudY = gasCloud->GetPositionY();
        float cloudDist = gasCloud->GetExactDist2d(botX, botY);
        
        // Only move if cloud is close enough to be dangerous
        if (cloudDist < 25.0f)
        {
        // Calculate vector from cloud to bot
        float dx = botX - cloudX;
        float dy = botY - cloudY;
        float dist = sqrt(dx * dx + dy * dy);
        
            if (dist > 0)
            {
                dx /= dist;
                dy /= dist;

                // Try different angles to find a safe path
                const int numAngles = 16;  // Increased for more precise movement
                float bestMoveX = botX;
                float bestMoveY = botY;
                float bestDist = cloudDist;
                bool foundPath = false;
            
                for (int i = 0; i < numAngles; i++)
                {
                    float angle = (2 * M_PI * i) / numAngles;
                    float rotatedDx = dx * cos(angle) - dy * sin(angle);
                    float rotatedDy = dx * sin(angle) + dy * cos(angle);
                
                    // Try different distances
                    for (float testDist = 5.0f; testDist <= 15.0f; testDist += 5.0f)
                    {
                        float testX = botX + rotatedDx * testDist;
                        float testY = botY + rotatedDy * testDist;
                        float testZ = botZ;

                        float newCloudDist = gasCloud->GetExactDist2d(testX, testY);
                    
                        // Check if this position is better
                        if (newCloudDist > bestDist && bot->IsWithinLOS(testX, testY, testZ))
                        {
                            bestMoveX = testX;
                            bestMoveY = testY;
                            bestDist = newCloudDist;
                            foundPath = true;
                        }
                    }
                }

                if (foundPath)
                {
                    return MoveTo(bot->GetMapId(), bestMoveX, bestMoveY, botZ,
                        false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
                }
                else if (cloudDist < 8.0f)  // Emergency move if very close and no good path found
                {
                    // Try to move directly away
                    float emergencyX = botX + dx * 10.0f;
                    float emergencyY = botY + dy * 10.0f;
                
                        if (bot->IsWithinLOS(emergencyX, emergencyY, botZ))
                        {
                            return MoveTo(bot->GetMapId(), emergencyX, emergencyY, botZ,
                                false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
                        }
                }
            }
        }
    }

    else
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        bool someoneHasAura = false;
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member)
                continue;
                
            if (botAI->HasAura("Gaseous Bloat", member))
            {
                someoneHasAura = true;
                break;
            }
        }

        if (someoneHasAura && !botAI->IsHeal(bot))
        {
            return Attack(gasCloud);
        }
    }

    return false;
}

bool AvoidMalleableGooAction::Execute(Event event)
{

    bool hasUnboundPlague = botAI->HasAura("Unbound Plague", bot);
    const float UNBOUND_PLAGUE_DISTANCE = 15.0f;

    // If bot has unbound plague, keep away from all other players
    if (hasUnboundPlague)
    {
        Group* group = bot->GetGroup();
        if (!group)
            return false;

        float closestDistance = UNBOUND_PLAGUE_DISTANCE;
        Unit* closestPlayer = nullptr;

        // Find closest player
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || member == bot)
                continue;

            float dist = bot->GetDistance2d(member);
            if (dist < closestDistance)
            {
                closestDistance = dist;
                closestPlayer = member;
            }
        }

        // Move away from closest player if too close
        if (closestPlayer)
        {
            float dx = bot->GetPositionX() - closestPlayer->GetPositionX();
            float dy = bot->GetPositionY() - closestPlayer->GetPositionY();
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist > 0)
            {
                dx /= dist;
                dy /= dist;
                float moveDistance = UNBOUND_PLAGUE_DISTANCE - closestDistance + 2.0f;
                
                float moveX = bot->GetPositionX() + dx * moveDistance;
                float moveY = bot->GetPositionY() + dy * moveDistance;
                
                if (bot->IsWithinLOS(moveX, moveY, bot->GetPositionZ()))
                {
                    return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(),
                        false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
                }
            }
        }
    }

    if (botAI->IsRanged(bot) || botAI->IsHeal(bot))
    {
        float radius = 7.0f;
        bool isRanged = botAI->IsRanged(bot);

        GuidVector members = AI_VALUE(GuidVector, "group members");
        if (isRanged)
        {
            for (auto& member : members)
            {
                Unit* unit = botAI->GetUnit(member);
                if (!unit || !unit->IsAlive() || unit == bot || botAI->IsTank(bot) || botAI->IsMelee(bot))
                    continue;

                float dist = bot->GetExactDist2d(unit);
                if (dist < radius)
                {   
                    float moveDistance = radius - dist + 1.0f;
                    
                    // Calculate potential new position
                    float angle = bot->GetAngle(unit);
                    float newX = bot->GetPositionX() + cos(angle + M_PI) * moveDistance;
                    float newY = bot->GetPositionY() + sin(angle + M_PI) * moveDistance;
                    
                    // Only move if we have line of sight
                    if (bot->IsWithinLOS(newX, newY, bot->GetPositionZ()))
                    {
                        return FleePosition(unit->GetPosition(), moveDistance);
                        // return MoveAway(unit, moveDistance);
                    }
                }
            }
        }
        return false;
    }

    return false;
}

//BPC
bool IccBpcKelesethTankAction::Execute(Event event)
{
    if (!botAI->IsAssistTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "prince keleseth");
    if (!boss)
        return false;

    // Check if we're not the victim of Keleseth's attack
    if (!(boss->GetVictim() == bot))
        return Attack(boss);

    // First check for any nucleus that needs to be picked up
    bool isCollectingNuclei = false;
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->IsAlive() && unit->GetEntry() == 38369) // Dark Nucleus entry
        {
            if (!unit->GetVictim() || unit->GetVictim() != bot)
            {
                isCollectingNuclei = true;
                return Attack(unit); // Pick up any nucleus that isn't targeting us
            }
        }
    }

    // If not collecting nuclei, move to OT position
    if (!isCollectingNuclei && bot->GetExactDist2d(ICC_BPC_OT_POSITION) > 20.0f)
        return MoveTo(bot->GetMapId(), ICC_BPC_OT_POSITION.GetPositionX(),
                    ICC_BPC_OT_POSITION.GetPositionY(), ICC_BPC_OT_POSITION.GetPositionZ(),
                    false, true, false, true, MovementPriority::MOVEMENT_COMBAT);

    return Attack(boss);
}

bool IccBpcNucleusAction::Execute(Event event)
{
    if (!botAI->IsAssistTank(bot))
        return false;

    // Actively look for any nucleus that isn't targeting us
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    for (auto i = targets.begin(); i != targets.end(); ++i)
    {
        Unit* unit = botAI->GetUnit(*i);
        if (unit && unit->IsAlive() && unit->GetEntry() == 38369) // Dark Nucleus entry
        {
            if (!unit->GetVictim() || unit->GetVictim() != bot)
                return Attack(unit); // Pick up any nucleus that isn't targeting us
        }
    }

    return false;
}

bool IccBpcMainTankAction::Execute(Event event)
{
    if (botAI->IsMainTank(bot))
    {
        // Move to MT position if we're not there
        if (bot->GetExactDist2d(ICC_BPC_MT_POSITION) > 20.0f)
            return MoveTo(bot->GetMapId(), ICC_BPC_MT_POSITION.GetPositionX(),
                        ICC_BPC_MT_POSITION.GetPositionY(), ICC_BPC_MT_POSITION.GetPositionZ(),
                        false, false, false, false, MovementPriority::MOVEMENT_COMBAT);

        Unit* valanar = AI_VALUE2(Unit*, "find target", "prince valanar");
        Unit* taldaram = AI_VALUE2(Unit*, "find target", "prince taldaram");

        // Attack any prince that's not targeting us
        if (valanar && valanar->IsAlive() && (!valanar->GetVictim() || valanar->GetVictim() != bot))
            return Attack(valanar);
        if (taldaram && taldaram->IsAlive() && (!taldaram->GetVictim() || taldaram->GetVictim() != bot))
            return Attack(taldaram);

        // If both princes are targeting us or dead, maintain current target
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        if (currentTarget && currentTarget->IsAlive() && 
            (currentTarget == valanar || currentTarget == taldaram))
            return Attack(currentTarget);

        return false;
    }
    
    if (!botAI->IsTank(bot))
    {
        Unit* currentTarget = AI_VALUE(Unit*, "current target");
        GuidVector targets = AI_VALUE(GuidVector, "possible targets");

        // If no valid skull target, search for empowered prince
        Unit* empoweredPrince = nullptr;
        for (auto i = targets.begin(); i != targets.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (!unit || !unit->IsAlive())
                continue;

            if (unit->HasAura(71596))
            {
                if (unit->GetEntry() == 37972 ||    // Keleseth
                    unit->GetEntry() == 37973 ||    // Taldaram
                    unit->GetEntry() == 37970)      // Valanar
                {
                empoweredPrince = unit;

                    // Mark empowered prince with skull if in group and not already marked
                    if (Group* group = bot->GetGroup())
                    {
                        ObjectGuid currentSkullGuid = group->GetTargetIcon(7);
                        if (currentSkullGuid.IsEmpty() || currentSkullGuid != unit->GetGUID())
                        {
                        group->SetTargetIcon(7, bot->GetGUID(), unit->GetGUID()); // 7 = skull
                    }
                    }
                    break;
                }
            }
        }
    }
    return false;
}

bool IccBpcEmpoweredVortexAction::Execute(Event event)
{
    Unit* valanar = AI_VALUE2(Unit*, "find target", "prince valanar");
    if (!valanar || !valanar->HasUnitState(UNIT_STATE_CASTING))
        return false;

    float const MIN_SPREAD = 12.0f;
    float const MOVE_INCREMENT = 10.0f;
    
    // Use MT position as reference point to move away from
    Position const* mtPos = &ICC_BPC_MT_POSITION;
    float centerX = mtPos->GetPositionX();
    float centerY = mtPos->GetPositionY();
    float centerZ = mtPos->GetPositionZ();

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Get all alive group members and sort by GUID for consistent movement directions
    std::vector<std::pair<ObjectGuid, Player*>> sortedMembers;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && !botAI->IsTank(member))
        {
            sortedMembers.push_back(std::make_pair(member->GetGUID(), member));
        }
    }
    std::sort(sortedMembers.begin(), sortedMembers.end());

    // Find this bot's index to determine movement direction
    int botIndex = -1;
    for (size_t i = 0; i < sortedMembers.size(); ++i)
    {
        if (sortedMembers[i].first == bot->GetGUID())
        {
            botIndex = i;
            break;
        }
    }

    if (botIndex == -1)
        return false;

    // Calculate base angle based on bot index (split into 12 directions)
    float baseAngle = botIndex * (2.0f * M_PI / 12.0f);

    // Calculate current distance from MT position
    float currentDist = bot->GetDistance2d(centerX, centerY);
    
    // If too close to others, move further out
    bool needToMove = false;
    if (currentDist < MIN_SPREAD)
        needToMove = true;
    else
    {
        // Check distance to other players
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || member == bot)
                continue;

            if (bot->GetDistance2d(member) < MIN_SPREAD)
            {
                needToMove = true;
                break;
            }
        }
    }

    if (!needToMove)
        return false;

    // Calculate new position further out in our assigned direction
    float moveDistance = std::max(MOVE_INCREMENT, currentDist + MOVE_INCREMENT);
    float targetX = centerX + cos(baseAngle) * moveDistance;
    float targetY = centerY + sin(baseAngle) * moveDistance;
    float targetZ = centerZ;

    // Update Z coordinate and check LOS
    bot->UpdateAllowedPositionZ(targetX, targetY, targetZ);
    if (!bot->IsWithinLOS(targetX, targetY, targetZ))
    {
        // Try adjusting angle if LOS fails
        for (float angleAdjust = -M_PI/6; angleAdjust <= M_PI/6; angleAdjust += M_PI/12)
        {
            if (angleAdjust == 0)
                continue;

            float newX = centerX + cos(baseAngle + angleAdjust) * moveDistance;
            float newY = centerY + sin(baseAngle + angleAdjust) * moveDistance;
            float newZ = centerZ;
            
            bot->UpdateAllowedPositionZ(newX, newY, newZ);
            if (bot->IsWithinLOS(newX, newY, newZ))
            {
                targetX = newX;
                targetY = newY;
                targetZ = newZ;
                break;
            }
        }
    }

    return MoveTo(bot->GetMapId(), targetX, targetY, targetZ, 
                false, false, false, false, MovementPriority::MOVEMENT_COMBAT, true, false);
}

bool IccBpcKineticBombAction::Execute(Event event)
{
    // Early exit if not ranged DPS
    if (!botAI->IsRangedDps(bot))
        return false;

    // Static constants to avoid recreating them every call
    static const float MAX_HEIGHT_DIFF = 25.0f;
    static const float SAFE_HEIGHT = 371.16473f;
    static const float TELEPORT_HEIGHT = 366.16473f;

    // Handle the edge case where bot is too high (prevent teleport to entrance)
    if (bot->GetPositionZ() > SAFE_HEIGHT)
        return bot->TeleportTo(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), TELEPORT_HEIGHT,
                               bot->GetOrientation());

    // Cache bot position once
    float botZ = bot->GetPositionZ();

    // Check if we're already handling a valid bomb
    Unit* currentTarget = AI_VALUE(Unit*, "current target");
    if (currentTarget && currentTarget->IsAlive() && currentTarget->GetName() == "Kinetic Bomb")
    {
        float heightDiff = currentTarget->GetPositionZ() - botZ;
        if (heightDiff < MAX_HEIGHT_DIFF)
            return false;  // Continue current attack
    }

    // Get possible targets once
    GuidVector targets = AI_VALUE(GuidVector, "possible targets");
    if (targets.empty())
        return false;

    // Cache group only once
    Group* group = bot->GetGroup();

    // Find the lowest reachable bomb
    Unit* bestBomb = nullptr;
    float lowestHeightDiff = MAX_HEIGHT_DIFF;

    for (auto& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive() || unit->GetName() != "Kinetic Bomb")
            continue;

        float heightDiff = unit->GetPositionZ() - botZ;
        if (heightDiff >= lowestHeightDiff)
            continue;

        // Skip if bot is too far to realistically hit this bomb
        if (bot->GetDistance(unit) > 30.0f)
            continue;

        // Check if any closer ranged DPS is already handling this bomb
        bool alreadyHandled = false;

        if (group)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || member == bot || !member->IsAlive() || !botAI->IsRangedDps(member))
                    continue;

                if (member->GetTarget() == unit->GetGUID() && member->GetDistance(unit) < bot->GetDistance(unit))
                {
                    alreadyHandled = true;
                    break;
                }
            }
        }

        if (!alreadyHandled)
        {
            bestBomb = unit;
            lowestHeightDiff = heightDiff;
        }
    }

    // Attack the best bomb if found
    if (bestBomb)
        return Attack(bestBomb);

    return false;
}

//BQL

bool IccBqlTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "blood-queen lana'thel");
    Aura* aura = botAI->GetAura("Frenzied Bloodthirst", bot);
    Aura* aura2 = botAI->GetAura("Swarming Shadows", bot);

    if (!boss)
        return false;

    // If tank is not at position, move there
    if ((botAI->IsTank(bot) || botAI->IsMainTank(bot) || botAI->IsAssistTank(bot)) && !(aura || aura2))
    {
        if (bot->GetExactDist2d(ICC_BQL_TANK_POSITION) > 10.0f)
            return MoveTo(bot->GetMapId(), ICC_BQL_TANK_POSITION.GetPositionX(),
                        ICC_BQL_TANK_POSITION.GetPositionY(), ICC_BQL_TANK_POSITION.GetPositionZ(),
                        false, false, false, true, MovementPriority::MOVEMENT_FORCED);
    }

    // If assist tank and no blood mirror, move to extact postion of main tank
    if (botAI->IsAssistTank(bot) && !botAI->GetAura("Blood Mirror", bot) && !(aura || aura2))
    {
        Unit* mainTank = AI_VALUE(Unit*, "main tank");
        if (!mainTank)
            return false;

        return MoveTo(bot->GetMapId(), mainTank->GetPositionX(), mainTank->GetPositionY(), mainTank->GetPositionZ(), 
                     false, false, false, true, MovementPriority::MOVEMENT_FORCED);
    }
    
    float radius = 8.0f;
    float moveIncrement = 3.0f;
    bool isRanged = botAI->IsRanged(bot);
    bool isMelee = botAI->IsMelee(bot);

    //if bot has Swarming Shadows, move to the wall
    if (aura2)
    {
        // Get current position and map
        float currentX = bot->GetPositionX();
        float currentY = bot->GetPositionY();
        float currentZ = bot->GetPositionZ();
        Map* map = bot->GetMap();

        float bestDist = 100.0f;
        float bestX = currentX;
        float bestY = currentY;
        bool foundWall = false;

        // Check only east (0) and west (π) directions for walls
        float angles[2] = {M_PI_2, -M_PI_2};  // East = π/2, West = -π/2
        for (float angle : angles)
        {
            float dx = cos(angle);
            float dy = sin(angle);
            
            // Binary search to find the wall
            float minDist = 5.0f;
            float maxDist = 100.0f;
            float wallDist = maxDist;
            
            for (int i = 0; i < 8; i++)
            {
                float testDist = (minDist + maxDist) / 2;
                float testX = currentX + dx * testDist;
                float testY = currentY + dy * testDist;
                float testZ = currentZ;
                
                bool heightFound = map->GetHeight(testX, testY, testZ);
                if (!heightFound)
                    testZ = currentZ;
                
                bool hasLos = map->isInLineOfSight(currentX, currentY, currentZ + 2.0f,
                                                 testX, testY, testZ + 2.0f,
                                                 bot->GetPhaseMask(), LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::Nothing);
                
                if (hasLos)
                {
                    minDist = testDist;
                }
                else
                {
                    maxDist = testDist;
                    wallDist = testDist;
                    foundWall = true;
                }
            }
            
            if (foundWall && wallDist < bestDist)
            {
                bestDist = wallDist;
                bestX = currentX + dx * (wallDist - 2.0f);  // Stay 2 yards from wall
                bestY = currentY + dy * (wallDist - 2.0f);
            }
        }

        // Only move if we're too far from the wall
        if (foundWall && bestDist > 10.0f)
        {
            // Verify we still have the aura before moving
            if (!botAI->GetAura("Swarming Shadows", bot))
                return false;

            return MoveTo(bot->GetMapId(), bestX, bestY, bot->GetPositionZ(), 
                         false, false, false, true, MovementPriority::MOVEMENT_FORCED);
        }
    }

    GuidVector members = AI_VALUE(GuidVector, "group members");
    
    if (isRanged && !aura && !aura2) //frenzied bloodthrist
    {
        // Ranged: spread from other ranged
        for (auto& member : members)
        {
            Unit* unit = botAI->GetUnit(member);
            if (!unit || !unit->IsAlive() || unit == bot || botAI->GetAura("Frenzied Bloodthirst", unit) || botAI->GetAura("Uncontrollable Frenzy", unit))
                continue;

            float dist = bot->GetExactDist2d(unit);
            if (dist < radius)
            {
                float moveDistance = std::min(moveIncrement, radius - dist + 1.0f);
                return FleePosition(unit->GetPosition(), moveDistance, 250U);
                //return MoveAway(unit, moveDistance);
            }
        }
    }

    if (isMelee && !aura && !aura2 && ((boss->GetPositionZ() - bot->GetPositionZ()) > 5.0f)) // melee also spread
    {
        // Melee: spread from other melee
        for (auto& member : members)
        {
            Unit* unit = botAI->GetUnit(member);
            if (!unit || !unit->IsAlive() || unit == bot || botAI->GetAura("Frenzied Bloodthirst", unit) || botAI->GetAura("Uncontrollable Frenzy", unit))
                continue;

            float dist = bot->GetExactDist2d(unit);
            if (dist < radius)
            {
                // Calculate direction away from nearby player
                float dx = bot->GetPositionX() - unit->GetPositionX();
                float dy = bot->GetPositionY() - unit->GetPositionY();
                float angle = atan2(dy, dx);
                
                // Move 8 yards away from the player
                float newX = unit->GetPositionX() + cos(angle) * 8.0f;
                float newY = unit->GetPositionY() + sin(angle) * 8.0f;
                
                return MoveTo(bot->GetMapId(), newX, newY, bot->GetPositionZ(),
                            false, false, false, true, MovementPriority::MOVEMENT_FORCED);
            }
        }
    }
    return false;  // Everyone is in position
}

bool IccBqlPactOfDarkfallenAction::Execute(Event event)
{
    // Check if bot has Pact of the Darkfallen
    Aura* aura = botAI->GetAura("Pact of the Darkfallen", bot);
    if (!aura) 
        return false;

    const float POSITION_TOLERANCE = 1.0f;  // Within 1 yards to break the link

    // Find other players with Pact of the Darkfallen
    std::vector<Player*> playersWithAura;
    Player* tankWithAura = nullptr;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Gather all players with the aura
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || member->GetGUID() == bot->GetGUID())
            continue;

        if (botAI->GetAura("Pact of the Darkfallen", member)) //pact of darkfallen
        {
            playersWithAura.push_back(member);
            // If this player is a tank, store them
            if (botAI->IsTank(member))
                tankWithAura = member;
        }
    }

    // If we found other players with the aura
    if (!playersWithAura.empty())
    {
        Position targetPos;
        
        if (playersWithAura.size() >= 2)  // 3 or more total (including this bot)
        {
            if (tankWithAura)
            {
                // Move to tank's position if we're not a tank
                if (!botAI->IsTank(bot))
                {
                    targetPos.Relocate(tankWithAura);
                }
                else
                {
                    // If we are the tank, stay put
                    return true;
                }
            }
            else
            {
                // Calculate center position of all affected players
                float sumX = bot->GetPositionX();
                float sumY = bot->GetPositionY();
                float sumZ = bot->GetPositionZ();
                int count = 1;  // Start with 1 for this bot

                for (Player* player : playersWithAura)
                {
                    sumX += player->GetPositionX();
                    sumY += player->GetPositionY();
                    sumZ += player->GetPositionZ();
                    count++;
                }

                targetPos.Relocate(sumX / count, sumY / count, sumZ / count);
            }
        }
        else  // Only one other player has aura
        {
            targetPos.Relocate(playersWithAura[0]);
        }

        // Move to target position if we're not already there
        if (bot->GetDistance(targetPos) > POSITION_TOLERANCE)
        {
            return MoveTo(bot->GetMapId(), targetPos.GetPositionX(), targetPos.GetPositionY(), targetPos.GetPositionZ(),
                         false, false, false, true, MovementPriority::MOVEMENT_FORCED);
        }
        return true;
    }

    // If no other players found with aura, move to center
    if (bot->GetDistance(ICC_BQL_CENTER_POSITION) > POSITION_TOLERANCE)
    {
        botAI->SetNextCheckDelay(500);
        return MoveTo(bot->GetMapId(), ICC_BQL_CENTER_POSITION.GetPositionX(), ICC_BQL_CENTER_POSITION.GetPositionY(), ICC_BQL_CENTER_POSITION.GetPositionZ(),
                     false, false, false, true, MovementPriority::MOVEMENT_FORCED);
    }

    return false;
}

bool IccBqlVampiricBiteAction::Execute(Event event)
{
    // Only act when bot has Frenzied Bloodthirst
    Aura* aura = botAI->GetAura("Frenzied Bloodthirst", bot);

    if (!aura) 
        return false;

    const float BITE_RANGE = 2.0f;
    Player* target = nullptr;
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Create lists for potential targets with their distances
    std::vector<std::pair<Player*, float>> dpsTargets;
    std::vector<std::pair<Player*, float>> healTargets;

    // Get list of players who are currently being targeted
    std::set<ObjectGuid> currentlyTargetedPlayers;
    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || member == bot)
            continue;

        if (botAI->GetAura("Frenzied Bloodthirst", member))
        {
            if (ObjectGuid targetGuid = member->GetTarget())
            {
                currentlyTargetedPlayers.insert(targetGuid);
            }
        }
    }

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        // Skip if already has essence, frenzy, or is a tank, or uncontrollable frenzy
        if (botAI->GetAura("Frenzied Bloodthirst", member) || botAI->GetAura("Essence of the Blood Queen", member) || botAI->GetAura("Uncontrollable Frenzy", member) || botAI->IsTank(member))
        {
            continue;
        }

        // Skip if this player is currently targeted by another bot
        if (currentlyTargetedPlayers.find(member->GetGUID()) != currentlyTargetedPlayers.end())
            continue;

        float distance = bot->GetDistance(member);
        
        if (botAI->IsDps(member))
            dpsTargets.push_back(std::make_pair(member, distance));
        else if (botAI->IsHeal(member))
            healTargets.push_back(std::make_pair(member, distance));
    }

    // Sort both vectors by distance
    auto sortByDistance = [](const std::pair<Player*, float>& a, const std::pair<Player*, float>& b) {
        return a.second < b.second;
    };

    std::sort(dpsTargets.begin(), dpsTargets.end(), sortByDistance);
    std::sort(healTargets.begin(), healTargets.end(), sortByDistance);

    // First try closest DPS target
    if (!dpsTargets.empty())
    {
        target = dpsTargets[0].first;
    }
    // If no DPS available, try closest healer
    else if (!healTargets.empty())
    {
        target = healTargets[0].first;
    }

    if (!target)
    {
        return false;
    }

    // Double check target is still alive
    if (!target->IsAlive() || (botAI->GetAura("Frenzied Bloodthirst", target)
        || botAI->GetAura("Essence of the Blood Queen", target)
        || botAI->GetAura("Uncontrollable Frenzy", target)))
    {
        return false;
    }

    // Check if we can reach the target
    float x = target->GetPositionX();
    float y = target->GetPositionY();
    float z = target->GetPositionZ();

    if (bot->IsWithinLOS(x, y, z) && bot->GetExactDist2d(target) > BITE_RANGE
        && !(botAI->GetAura("Frenzied Bloodthirst", target) 
        || botAI->GetAura("Essence of the Blood Queen", target)
        || botAI->GetAura("Uncontrollable Frenzy", target)))
    {
        return MoveTo(target->GetMapId(), x, y, z, false, false, false, true, MovementPriority::MOVEMENT_FORCED);
    }

    // If in range and can see target, cast the bite
    if (bot->IsWithinLOS(x, y, z) && bot->GetExactDist2d(target) <= BITE_RANGE)
    {
        // Final alive check before casting
        if (!target->IsAlive() || (botAI->GetAura("Frenzied Bloodthirst", target)
            || botAI->GetAura("Essence of the Blood Queen", target)
            || botAI->GetAura("Uncontrollable Frenzy", target)))
        {
            return false;
        }

        if (botAI->CanCastSpell("Vampiric Bite", target))
        {
            return botAI->CastSpell("Vampiric Bite", target);
        }
    }

    return false;
}

//VDW

//Valkyre 38248 spear, 50307 spear in inv, Sister Svalna 37126, aether shield 71463

bool IccValkyreSpearAction::Execute(Event event)
{
    // Find the nearest spear
    Creature* spear = bot->FindNearestCreature(38248, 100.0f);
    if (!spear)
        return false;

    // Move to the spear if not in range
    if (!spear->IsWithinDistInMap(bot, INTERACTION_DISTANCE))
    {
        return MoveTo(spear, INTERACTION_DISTANCE);
    }

    // Remove shapeshift forms
    botAI->RemoveShapeshift();

    // Stop movement and click the spear
    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    spear->HandleSpellClick(bot);

    // Dismount if mounted
    WorldPacket emptyPacket;
    bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);

    return false;
}

bool IccSisterSvalnaAction::Execute(Event event)
{
    Unit* svalna = AI_VALUE2(Unit*, "find target", "sister svalna");
    if (!svalna || !svalna->HasAura(71463)) // Check for Aether Shield aura
        return false;

    // Check if bot has the spear item
    if (!botAI->HasItemInInventory(50307))
        return false;

    // Get all items from inventory
    std::vector<Item*> items = botAI->GetInventoryItems();
    for (Item* item : items)
    {
        if (item->GetEntry() == 50307) // Spear ID
        {
            botAI->ImbueItem(item, svalna); // Use spear on Svalna
            return false;
        }
    }

    return false;
}

bool IccValithriaPortalAction::Execute(Event event)
{
    //Added movement for non healers, didnt want to make another action just for this
    if (!botAI->IsHeal(bot))
       return MoveTo(bot->GetMapId(), ICC_VDW_GROUP_POSITION.GetPositionX(), ICC_VDW_GROUP_POSITION.GetPositionY(), ICC_VDW_GROUP_POSITION.GetPositionZ(),
                     false, false, false, true, MovementPriority::MOVEMENT_COMBAT);

    //Portal action
    if (!botAI->IsHeal(bot) || bot->HasAura(70766))
        return false;

    // Find the nearest portal
    Creature* portal = bot->FindNearestCreature(37945, 100.0f); // Dream Portal
    if (!portal)
        portal = bot->FindNearestCreature(38430, 100.0f); // Nightmare Portal

    if (!portal)
        return false;

    // Move exactly to portal position
    float portalX = portal->GetPositionX();
    float portalY = portal->GetPositionY();
    float portalZ = portal->GetPositionZ();

    // If not at exact portal position, move there
    if (bot->GetDistance2d(portalX, portalY) > 0.1f)
    {
        return MoveTo(portal->GetMapId(), portalX, portalY, portalZ, false, false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }

    // Remove shapeshift forms
    botAI->RemoveShapeshift();

    // Stop movement and click the portal
    bot->GetMotionMaster()->Clear();
    bot->StopMoving();
    portal->HandleSpellClick(bot);

    // Dismount if mounted
    WorldPacket emptyPacket;
    bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);

    return false;
}

bool IccValithriaHealAction::Execute(Event event)
{
    if (!botAI->IsHeal(bot))
        return false;

    if (bot->GetHealthPct() < 50.0f)
        return false;

    if (!bot->HasAura(70766)) //dream state
    {
        bot->SetSpeed(MOVE_RUN, 1.0f, true);
        bot->SetSpeed(MOVE_WALK, 1.0f, true);
        bot->SetSpeed(MOVE_FLIGHT, 1.0f, true);
    }
    // Find Valithria
    
    if (bot->GetPositionZ() > 367.961f)
        return bot->TeleportTo(bot->GetMapId(), bot->GetPositionX(),
                          bot->GetPositionY(), 365.0f, bot->GetOrientation());

    if (Creature* valithria = bot->FindNearestCreature(36789, 100.0f))
    {
        switch (bot->getClass())
        {
            case CLASS_DRUID:
            {
                // Check for Rejuvenation (48441)
                if (!valithria->HasAura(48441))
                    return botAI->CastSpell(48441, valithria);

                // Check for Regrowth (48443)
                if (!valithria->HasAura(48443))
                    return botAI->CastSpell(48443, valithria);

                // Check for Lifebloom stacks (48451)
                Aura* lifebloom = valithria->GetAura(48451);
                if (!lifebloom || lifebloom->GetStackAmount() < 3)
                    return botAI->CastSpell(48451, valithria);

                // If all HoTs are up with full stacks, cast Wild Growth (53251)
                return botAI->CastSpell(53251, valithria);
            }
            case CLASS_SHAMAN:
                return valithria->HasAura(61301) ? botAI->CastSpell(49273, valithria) : botAI->CastSpell(61301, valithria); // Cast Healing Wave if Riptide is up, otherwise cast Riptide
            case CLASS_PRIEST:
                return valithria->HasAura(48068) ? botAI->CastSpell(48063, valithria) : botAI->CastSpell(48068, valithria); // Cast Greater Heal if Renew is up, otherwise cast Renew
            case CLASS_PALADIN:
                return valithria->HasAura(53563) ? botAI->CastSpell(48782, valithria) : botAI->CastSpell(53563, valithria); // Cast Holy Light if Beacon is up, otherwise cast Beacon of Light
            default:
                return false;
        }
    }

    return false;
}

bool IccValithriaDreamCloudAction::Execute(Event event)
{
    // Only execute if we're in dream state
    if (!bot->HasAura(70766))
        return false;

    // Set speed to match players in dream state
    if (bot->HasAura(70766))
    {
        bot->SetSpeed(MOVE_RUN, 2.0f, true);
        bot->SetSpeed(MOVE_WALK, 2.0f, true);
        bot->SetSpeed(MOVE_FLIGHT, 2.0f, true);
    }
   

    // Find nearest cloud of either type that we haven't collected
    Creature* dreamCloud = bot->FindNearestCreature(37985, 100.0f);
    Creature* nightmareCloud = bot->FindNearestCreature(38421, 100.0f);

    // If we have emerald vigor, prioritize dream clouds
    if (bot->HasAura(70873))
    {
        if (dreamCloud)
            return MoveTo(dreamCloud->GetMapId(), dreamCloud->GetPositionX(), dreamCloud->GetPositionY(), dreamCloud->GetPositionZ(), false, false, false, true, MovementPriority::MOVEMENT_NORMAL);
        if (nightmareCloud)
            return MoveTo(nightmareCloud->GetMapId(), nightmareCloud->GetPositionX(), nightmareCloud->GetPositionY(), nightmareCloud->GetPositionZ(), false, false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }
    // Otherwise prioritize nightmare clouds
    else
    {
        if (nightmareCloud)
            return MoveTo(nightmareCloud->GetMapId(), nightmareCloud->GetPositionX(), nightmareCloud->GetPositionY(), nightmareCloud->GetPositionZ(), false, false, false, true, MovementPriority::MOVEMENT_NORMAL);
        if (dreamCloud)
            return MoveTo(dreamCloud->GetMapId(), dreamCloud->GetPositionX(), dreamCloud->GetPositionY(), dreamCloud->GetPositionZ(), false, false, false, true, MovementPriority::MOVEMENT_NORMAL);
    }

    return false;
}

//Sindragosa

bool IccSindragosaTankPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sindragosa");
    if (!boss || boss->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY))
        return false;

    if ((botAI->IsTank(bot) || botAI->IsMainTank(bot) || botAI->IsAssistTank(bot)) && (boss->GetVictim() == bot)) 
    {
        float distBossToCenter = boss->GetExactDist2d(ICC_SINDRAGOSA_CENTER_POSITION);
        float distToTankPos = bot->GetExactDist2d(ICC_SINDRAGOSA_TANK_POSITION);
        float targetOrientation = M_PI / 2; // We want boss to face east
        float currentOrientation = boss->GetOrientation();
        
        // Normalize both orientations to 0-2π range
        currentOrientation = fmod(currentOrientation + 2 * M_PI, 2 * M_PI);
        targetOrientation = fmod(targetOrientation + 2 * M_PI, 2 * M_PI);
        
        float orientationDiff = currentOrientation - targetOrientation;
        
        // Normalize the difference to be between -PI and PI
        while (orientationDiff > M_PI) orientationDiff -= 2 * M_PI;
        while (orientationDiff < -M_PI) orientationDiff += 2 * M_PI;
        
        
        // Stage 1: Move boss to center if too far
        if (distBossToCenter > 20.0f)
        {
            
            // Calculate direction vector from boss to center
            float dirX = ICC_SINDRAGOSA_CENTER_POSITION.GetPositionX() - boss->GetPositionX();
            float dirY = ICC_SINDRAGOSA_CENTER_POSITION.GetPositionY() - boss->GetPositionY();
            
            // Move 10 yards beyond center in the same direction
            float moveX = ICC_SINDRAGOSA_CENTER_POSITION.GetPositionX() + (dirX / distBossToCenter) * 10.0f;
            float moveY = ICC_SINDRAGOSA_CENTER_POSITION.GetPositionY() + (dirY / distBossToCenter) * 10.0f;
            
            return MoveTo(bot->GetMapId(), moveX, moveY, boss->GetPositionZ(),
                         false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
        }
        
        // Stage 2: Get to tank position when boss is centered
        if (distToTankPos > 5.0f)
        {
            return MoveTo(bot->GetMapId(), ICC_SINDRAGOSA_TANK_POSITION.GetPositionX(),
                      ICC_SINDRAGOSA_TANK_POSITION.GetPositionY(),
                      ICC_SINDRAGOSA_TANK_POSITION.GetPositionZ(),
                      false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
        }
        
        // Stage 3: Adjust orientation when in position
        bool needsOrientationAdjust = std::abs(orientationDiff) > 0.15f;
        if (needsOrientationAdjust)
        {
            // When we have negative difference (currentOrientation < targetOrientation)
            // We need to move south to make the orientation more positive
            float currentX = bot->GetPositionX();
            float currentY = bot->GetPositionY();
            float moveX, moveY;
            
            // For negative difference (need to increase orientation) -> move south
            // For positive difference (need to decrease orientation) -> move north
            if (orientationDiff < 0)
            {
                moveX = currentX - 2.0f; // Move south to increase orientation
                moveY = currentY;
            }
            else
            {
                moveX = currentX + 2.0f; // Move north to decrease orientation
                moveY = currentY;
            }
            
            return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(),
                        false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
        }
        return false;
    }
    
    // Non-tanks should stay on the left flank to avoid both cleave and tail smash
    if (boss && !(boss->GetVictim() == bot) /*&& !bot->HasAura(69762)*/)
    {
        if (botAI->IsRanged(bot))
        {
            float const TOLERANCE = 15.0f;        // 15 yard tolerance
            float const MAX_STEP = 5.0f;          // Maximum distance to move in one step
            
            float distToTarget = bot->GetExactDist2d(ICC_SINDRAGOSA_RANGED_POSITION);

            // Only move if outside tolerance
            if (distToTarget > TOLERANCE)
            {
                // Calculate direction vector to target
                float dirX = ICC_SINDRAGOSA_RANGED_POSITION.GetPositionX() - bot->GetPositionX();
                float dirY = ICC_SINDRAGOSA_RANGED_POSITION.GetPositionY() - bot->GetPositionY();
                
                // Normalize direction vector
                float length = sqrt(dirX * dirX + dirY * dirY);
                dirX /= length;
                dirY /= length;

                // Calculate intermediate point
                float stepSize = std::min(MAX_STEP, distToTarget);
                float moveX = bot->GetPositionX() + dirX * stepSize;
                float moveY = bot->GetPositionY() + dirY * stepSize;
                
                return MoveTo(bot->GetMapId(), moveX, moveY, ICC_SINDRAGOSA_RANGED_POSITION.GetPositionZ(),
                             false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
            }
            return false;
        }
        else
        {
            float const TOLERANCE = 10.0f;  // 10 yard tolerance for melee
            float const MAX_STEP = 5.0f;    // Maximum distance to move in one step

            float distToTarget = bot->GetExactDist2d(ICC_SINDRAGOSA_MELEE_POSITION);

            // Only move if outside tolerance
            if (distToTarget > TOLERANCE)
            {
                // Calculate direction vector to target
                float dirX = ICC_SINDRAGOSA_MELEE_POSITION.GetPositionX() - bot->GetPositionX();
                float dirY = ICC_SINDRAGOSA_MELEE_POSITION.GetPositionY() - bot->GetPositionY();
                
                // Normalize direction vector
                float length = sqrt(dirX * dirX + dirY * dirY);
                dirX /= length;
                dirY /= length;

                // Calculate intermediate point
                float stepSize = std::min(MAX_STEP, distToTarget);
                float moveX = bot->GetPositionX() + dirX * stepSize;
                float moveY = bot->GetPositionY() + dirY * stepSize;

                return MoveTo(bot->GetMapId(), moveX, moveY, ICC_SINDRAGOSA_MELEE_POSITION.GetPositionZ(),
                            false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
            }
            return false;
        }
    }
    return false;
}

bool IccSindragosaTankSwapPositionAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sindragosa");
    if (!boss)
        return false;

    // Only for assist tank
    if (!botAI->IsAssistTank(bot))
        return false;

    float distToTankPos = bot->GetExactDist2d(ICC_SINDRAGOSA_TANK_POSITION);
    
    // Move to tank position
    if (distToTankPos > 3.0f)  // Tighter tolerance for tank swap
    {
        return MoveTo(bot->GetMapId(), ICC_SINDRAGOSA_TANK_POSITION.GetPositionX(),
                     ICC_SINDRAGOSA_TANK_POSITION.GetPositionY(),
                     ICC_SINDRAGOSA_TANK_POSITION.GetPositionZ(),
                     false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
    }

    return false;
}

bool IccSindragosaFrostBeaconAction::Execute(Event event)
{
    float const POSITION_TOLERANCE = 1.0f;
    
    Unit* boss = AI_VALUE2(Unit*, "find target", "sindragosa");
    if (!boss)
        return false;
   
    // Handle beaconed players
    if (bot->HasAura(70126))
    {
        if (boss && boss->HealthBelowPct(35) && !boss->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY)) 
        {
            // Only move if not already at position (with tolerance)
            if (bot->GetExactDist2d(ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionX(), ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionY()) > POSITION_TOLERANCE)
            {
                return MoveTo(bot->GetMapId(), 
                            ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionX(),
                            ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionY(),
                            ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionZ(),
                            false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
            }
            return false;
        }
        else
        {
            Position const* tombPosition;
            uint8 beaconIndex = 0;
            bool foundSelf = false;
            
            Group* group = bot->GetGroup();
            if (!group)
                return false;

            // Find this bot's index among players with Frost Beacon
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (!member || !member->IsAlive() || !member->HasAura(70126)) // Only count alive players with Frost Beacon
                    continue;

                if (member == bot)
                {
                    foundSelf = true;
                    break;
                }
                beaconIndex++;
            }

            if (!foundSelf)
                return false;

            switch (beaconIndex) {
                case 0:
                    tombPosition = &ICC_SINDRAGOSA_THOMB1_POSITION;
                    break;
                case 1:
                    tombPosition = &ICC_SINDRAGOSA_THOMB2_POSITION;
                    break;
                case 2:
                    tombPosition = &ICC_SINDRAGOSA_THOMB3_POSITION;
                    break;
                case 3:
                    tombPosition = &ICC_SINDRAGOSA_THOMB4_POSITION;
                    break;
                default:
                    tombPosition = &ICC_SINDRAGOSA_THOMB5_POSITION;
                    break;
            }
            
            // Only move if not already at position (with tolerance)
            float dist = bot->GetExactDist2d(tombPosition->GetPositionX(), tombPosition->GetPositionY());
            if (dist > POSITION_TOLERANCE)
            {
                return MoveTo(bot->GetMapId(), tombPosition->GetPositionX(),
                              tombPosition->GetPositionY(),
                              tombPosition->GetPositionZ(),
                              false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
            }
            return false;
        }
    }
    // Handle non-beaconed players
    else
    {
        float const MIN_SAFE_DISTANCE = 13.0f;
        float const MAX_SAFE_DISTANCE = 30.0f;
        float const MOVE_TOLERANCE = 2.0f; // Tolerance for movement to reduce jitter
        
        GuidVector members = AI_VALUE(GuidVector, "group members");
        std::vector<Unit*> beaconedPlayers;
        
        for (auto& member : members)
        {
            Unit* player = botAI->GetUnit(member);
            if (!player || player->GetGUID() == bot->GetGUID())
                continue;
                
            if (player->HasAura(70126)) // Frost Beacon
                beaconedPlayers.push_back(player);
        }
        
        if (beaconedPlayers.empty())
            return false;

        // Different behavior for air phase
        Difficulty diff = bot->GetRaidDifficulty();
        if (boss->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY) && (diff == RAID_DIFFICULTY_25MAN_NORMAL || diff == RAID_DIFFICULTY_25MAN_HEROIC))
        {
            if (!bot->HasAura(70126)) // If not beaconed, move to safe position
            {
                float dist = bot->GetExactDist2d(ICC_SINDRAGOSA_FBOMB_POSITION.GetPositionX(),
                                               ICC_SINDRAGOSA_FBOMB_POSITION.GetPositionY());
                if (dist > POSITION_TOLERANCE)
                {
                    return MoveTo(bot->GetMapId(), ICC_SINDRAGOSA_FBOMB_POSITION.GetPositionX(),
                                ICC_SINDRAGOSA_FBOMB_POSITION.GetPositionY(),
                                ICC_SINDRAGOSA_FBOMB_POSITION.GetPositionZ(),
                                false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
                }
            }
            return false;
        }
        else if (boss->HasUnitMovementFlag(MOVEMENTFLAG_DISABLE_GRAVITY) && (diff == RAID_DIFFICULTY_10MAN_NORMAL || diff == RAID_DIFFICULTY_10MAN_HEROIC))
        {
            if (!bot->HasAura(70126)) // If not beaconed, move to safe position
            {
                float dist = bot->GetExactDist2d(ICC_SINDRAGOSA_FBOMB10_POSITION.GetPositionX(),
                                               ICC_SINDRAGOSA_FBOMB10_POSITION.GetPositionY());
                if (dist > POSITION_TOLERANCE)
                {
                    return MoveTo(bot->GetMapId(), ICC_SINDRAGOSA_FBOMB10_POSITION.GetPositionX(),
                                ICC_SINDRAGOSA_FBOMB10_POSITION.GetPositionY(),
                                ICC_SINDRAGOSA_FBOMB10_POSITION.GetPositionZ(),
                                false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
                }
            }
            return false;
        }
        else
        {
            // Ground phase - use existing vector-based movement
            bool needToMove = false;
            float moveX = 0, moveY = 0;
            
            for (Unit* beaconedPlayer : beaconedPlayers)
            {
                float dist = bot->GetExactDist2d(beaconedPlayer);
                if (dist < MIN_SAFE_DISTANCE + MOVE_TOLERANCE)
                {
                    needToMove = true;
                    float angle = bot->GetAngle(beaconedPlayer);
                    float moveDistance = std::min(5.0f, MIN_SAFE_DISTANCE - dist + MOVE_TOLERANCE);
                    
                    moveX += cos(angle + M_PI) * moveDistance;
                    moveY += sin(angle + M_PI) * moveDistance;
                }
            }
            
            if (needToMove && !bot->HasAura(70126))
            {
                float posX = bot->GetPositionX() + moveX;
                float posY = bot->GetPositionY() + moveY;
                float posZ = bot->GetPositionZ();
                bot->UpdateAllowedPositionZ(posX, posY, posZ);
                
                // Only move if the change in position is significant
                if (std::abs(moveX) > MOVE_TOLERANCE || std::abs(moveY) > MOVE_TOLERANCE)
                {
                    return MoveTo(bot->GetMapId(), posX, posY, posZ,
                                false, false, false, false, MovementPriority::MOVEMENT_FORCED, true, false);
                }
            }
        }
    }

    return false;
}

bool IccSindragosaBlisteringColdAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "sindragosa");
    if (!boss)
        return false;

    // Only non-tanks should move out
    if (botAI->IsMainTank(bot))
        return false;

    float dist = bot->GetDistance(boss);
    
    // If we're already at safe distance, no need to move
    if (dist >= 30.0f)
        return false;

    // Check boss health to determine movement target
    bool isPhaseThree = boss->GetHealthPct() <= 35;
    Position const& targetPos = isPhaseThree ? ICC_SINDRAGOSA_LOS2_POSITION : ICC_SINDRAGOSA_BLISTERING_COLD_POSITION;

    // Only move if we're too close to the boss (< 30 yards)
    if (dist < 30.0f)
    {
        // If in phase 3, check if already at LOS2 position
        if (isPhaseThree && bot->IsWithinDist2d(targetPos.GetPositionX(), targetPos.GetPositionY(), 1.0f))
            return false;

        float const STEP_SIZE = 10.0f;
        float distToTarget = bot->GetDistance2d(targetPos.GetPositionX(), targetPos.GetPositionY());
        
        if (distToTarget > 0.1f)  // Avoid division by zero
        {
            // Calculate direction vector
            float dirX = targetPos.GetPositionX() - bot->GetPositionX();
            float dirY = targetPos.GetPositionY() - bot->GetPositionY();
            
            // Normalize direction vector
            float length = sqrt(dirX * dirX + dirY * dirY);
            dirX /= length;
            dirY /= length;
            
            // Move STEP_SIZE yards in that direction
            float moveX = bot->GetPositionX() + dirX * STEP_SIZE;
            float moveY = bot->GetPositionY() + dirY * STEP_SIZE;

            return MoveTo(bot->GetMapId(), moveX, moveY, bot->GetPositionZ(),
                         false, false, false, true, MovementPriority::MOVEMENT_FORCED, true, false);
        }
    }
    return false;
}

bool IccSindragosaUnchainedMagicAction::Execute(Event event)
{
    if (bot->HasAura(69762)) // unchained magic

    if (Aura* aura = bot->GetAura(69766))
    {
        if (aura->GetStackAmount() >= 3)
            return true; // Stop casting spells
    }

    return false;
}

bool IccSindragosaChilledToTheBoneAction::Execute(Event event)
{
    if (bot->HasAura(70106)) // Chilled to the Bone
    {
         if (Aura* aura = bot->GetAura(70106))
        {
            if (aura->GetStackAmount() >= 8)
            {
                bot->AttackStop();
                return true; // Stop casting spells
            }
        }
    }

    return false;  
}

bool IccSindragosaMysticBuffetAction::Execute(Event event)
{

    // Get boss to check if it exists
    Unit* boss = AI_VALUE2(Unit*, "find target", "sindragosa");
    if (!boss || !bot || !bot->IsAlive())
        return false;

    // Check if we have Mystic Buffet
    Aura* aura = botAI->GetAura("mystic buffet", bot, false, true);
    if (!aura)
        return false;

    // Skip if we have Frost Beacon
    if (bot->HasAura(70126))
        return false;

    uint8 stacks = aura->GetStackAmount();

    // Check if already at LOS2 position
    if (bot->IsWithinDist2d(ICC_SINDRAGOSA_LOS2_POSITION.GetPositionX(), 
                           ICC_SINDRAGOSA_LOS2_POSITION.GetPositionY(), 1.0f))
    {
        return false;
    }

    // Find nearest ice tomb
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Unit* nearestTomb = nullptr;
    float minDist = 150.0f;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || !member->IsAlive() || member == bot)
            continue;

        if (member->HasAura(70157))  // Ice Tomb
        {
            float dist = bot->GetDistance(member);
            if (dist < minDist)
            {
                minDist = dist;
                nearestTomb = member;
            }
        }
    }

    // If no tombs found or tomb not at MB2 position, don't move
    if (!nearestTomb || !nearestTomb->IsWithinDist2d(ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionX(), 
        ICC_SINDRAGOSA_THOMBMB2_POSITION.GetPositionY(), 1.0f))
        return false;

    // Move to LOS2 position
        return MoveTo(bot->GetMapId(),
                     ICC_SINDRAGOSA_LOS2_POSITION.GetPositionX(),
                     ICC_SINDRAGOSA_LOS2_POSITION.GetPositionY(),
                     ICC_SINDRAGOSA_LOS2_POSITION.GetPositionZ(),
                     false, false, false, false, MovementPriority::MOVEMENT_FORCED);
}

bool IccSindragosaFrostBombAction::Execute(Event event)
{
    if (!bot || !bot->IsAlive() || bot->HasAura(70157))  // Skip if dead or in Ice Tomb
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Find frost bomb marker and tombs
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    const uint32 tombEntries[] = {36980, 38320, 38321, 38322};  // tomb id's
    Unit* marker = nullptr;
    GuidVector tombGuids;

    // Process all NPCs
    for (const ObjectGuid& guid : npcs)
    {
        Unit* unit = botAI->GetUnit(guid);
        if (!unit || !unit->IsAlive())
            continue;

        if (unit->HasAura(70022))  // Frost bomb visual
            marker = unit;

        // Check if unit is a tomb
        for (uint32 entry : tombEntries)
        {
            if (unit->GetEntry() == entry)
            {
                tombGuids.push_back(guid);
                break;
            }
        }
    }

    // Exit if required elements aren't found
    if (!marker || tombGuids.empty())
        return false;

    // Sort tomb GUIDs by health (highest first)
    std::sort(tombGuids.begin(), tombGuids.end(),
              [this](const ObjectGuid& a, const ObjectGuid& b)
              {
                  Unit* unitA = botAI->GetUnit(a);
                  Unit* unitB = botAI->GetUnit(b);
                  if (!unitA || !unitB)
                      return false;
                  return unitA->GetHealthPct() > unitB->GetHealthPct();
              });

    // Get the tomb with highest HP for LOS
    Unit* highestHPTomb = botAI->GetUnit(tombGuids[0]);

    // Handle moon marking (much simpler now)
    ObjectGuid currentMoon = group->GetTargetIcon(4);

    // Check if current moon mark is still valid
    bool moonMarkValid = false;
    if (!currentMoon.IsEmpty())
    {
        for (const ObjectGuid& tombGuid : tombGuids)
        {
            if (tombGuid == currentMoon)
            {
                Unit* tomb = botAI->GetUnit(tombGuid);
                if (tomb && tomb->IsAlive())
                {
                    moonMarkValid = true;
                    break;
                }
            }
        }
    }

    // If no valid moon mark exists, mark the first tomb
    if (!moonMarkValid && !tombGuids.empty())
    {
        Unit* tombToMark = botAI->GetUnit(tombGuids[0]);
        if (tombToMark)
        {
            group->SetTargetIcon(4, bot->GetGUID(), tombToMark->GetGUID());  // Moon
            // Reset bot's target after marking
            bot->SetTarget(ObjectGuid::Empty);
        }
    }

    // Use marked tomb or highest HP tomb for LOS check
    Unit* losTarget = nullptr;
    if (moonMarkValid)
    {
        losTarget = botAI->GetUnit(currentMoon);
    }
    else
    {
        losTarget = highestHPTomb;
    }


    // Position handling using highest HP tomb
    float angle = marker->GetAngle(losTarget);
    float posX = losTarget->GetPositionX() + cos(angle) * 1.0f;
    float posY = losTarget->GetPositionY() + sin(angle) * 1.0f;
    float posZ = losTarget->GetPositionZ();
    if (bot->GetDistance2d(posX, posY) > 2.0f)
        return MoveTo(bot->GetMapId(), posX, posY, posZ, false, false, false, true, MovementPriority::MOVEMENT_FORCED);

    return false;
}

bool IccLichKingShadowTrapAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "the lich king");
    if (!boss)
        return false;

    if (!botAI->IsRanged(bot))
        return false;

    // search for all nearby traps
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    std::vector<Unit*> nearbyTraps;
    bool needToMove = false;

    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || !unit->IsAlive())
            continue;

        if (unit->GetEntry() == 39137)  // shadow trap
        {
            float distance = bot->GetDistance(unit);
            if (distance < 7.0f)
            {
                needToMove = true;
                nearbyTraps.push_back(unit);

                // Immediate danger check - if we're very close to any trap, move away faster
                if (distance < 3.0f)
                {
                    float angle = bot->GetAngle(unit);
                    float x = bot->GetPositionX() + cos(angle + M_PI) * 5.0f;  // Move opposite direction
                    float y = bot->GetPositionY() + sin(angle + M_PI) * 5.0f;
                    float z = bot->GetPositionZ();

                    bot->UpdateAllowedPositionZ(x, y, z);
                    if (bot->IsWithinLOS(x, y, z))
                    {
                        botAI->Reset();
                        return MoveTo(bot->GetMapId(), x, y, z, false, false, false, true,
                                      MovementPriority::MOVEMENT_FORCED, true, false);
                    }
                }
            }
        }
    }

    if (!needToMove || nearbyTraps.empty())
        return false;

    // Try different angles to find a safe spot
    float const MOVE_DISTANCE = 5.0f;        
    float const ANGLE_INCREMENT = M_PI / 4;  
    float bestAngle = 0;
    float maxMinDistance = 0;
    bool foundSafeSpot = false;

    // Try 8 primary directions first (faster evaluation)
    for (int i = 0; i < 8; i++)
    {
        float tryAngle = i * ANGLE_INCREMENT;
        float testX = bot->GetPositionX() + cos(tryAngle) * MOVE_DISTANCE;
        float testY = bot->GetPositionY() + sin(tryAngle) * MOVE_DISTANCE;
        float testZ = bot->GetPositionZ();  // Use current Z as base

        bot->UpdateAllowedPositionZ(testX, testY, testZ);

        // Skip invalid positions
        if (!bot->IsWithinLOS(testX, testY, testZ))
            continue;

        // Find minimum distance to any trap from this position
        float minTrapDistance = FLT_MAX;
        for (Unit* trap : nearbyTraps)
        {
            float distToTrap = sqrt(pow(testX - trap->GetPositionX(), 2) + pow(testY - trap->GetPositionY(), 2));
            minTrapDistance = std::min(minTrapDistance, distToTrap);
        }

        // If this position gets us to safety, move there immediately
        if (minTrapDistance >= 7.0f)
        {
            bestAngle = tryAngle;
            foundSafeSpot = true;
            break;
        }

        // Otherwise track the best option
        if (minTrapDistance > maxMinDistance)
        {
            maxMinDistance = minTrapDistance;
            bestAngle = tryAngle;
        }
    }

    // If we didn't find a completely safe spot but can improve our situation
    if (!foundSafeSpot && maxMinDistance > bot->GetDistance(nearbyTraps[0]))
    {
        foundSafeSpot = true;
    }

    // Move to the selected position
    if (foundSafeSpot)
    {
        float x = bot->GetPositionX() + cos(bestAngle) * MOVE_DISTANCE;
        float y = bot->GetPositionY() + sin(bestAngle) * MOVE_DISTANCE;
        float z = bot->GetPositionZ();

        bot->UpdateAllowedPositionZ(x, y, z);
        if (bot->IsWithinLOS(x, y, z))
        {
            botAI->Reset();
            return MoveTo(bot->GetMapId(), x, y, z, false, false, false, true, MovementPriority::MOVEMENT_FORCED, true,
                          false);
        }
    }

    // Fallback: Just move backward if all else fails
    float angle = bot->GetAngle(nearbyTraps[0]);
    float x = bot->GetPositionX() + cos(angle + M_PI) * 5.0f;
    float y = bot->GetPositionY() + sin(angle + M_PI) * 5.0f;
    float z = bot->GetPositionZ();

    bot->UpdateAllowedPositionZ(x, y, z);
    botAI->Reset();
    return MoveTo(bot->GetMapId(), x, y, z, false, false, false, true, MovementPriority::MOVEMENT_FORCED, true, false);

    return false;
}

bool IccLichKingNecroticPlagueAction::Execute(Event event)
{
    bool hasPlague = botAI->HasAura("Necrotic Plague", bot);

    // Only execute if we have the plague
    if (!hasPlague)
        return false;

    // Find closest shambling and nearest shadow trap
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    Unit* closestHorror = nullptr;
    Unit* nearestTrap = nullptr;
    float minHorrorDist = 100.0f;
    float minTrapDist = 100.0f;

    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || !unit->IsAlive())
            continue;

        uint32 entry = unit->GetEntry();
        if (entry == 37698 || entry == 39299 || entry == 39300 || entry == 39301) //shambling horror
        {
            float distance = bot->GetDistance(unit);
            if (distance < minHorrorDist)
            {
                minHorrorDist = distance;
                closestHorror = unit;
            }
        }
        else if (entry == 39137) //shadow trap
        {
            float distance = bot->GetDistance(unit);
            if (distance < minTrapDist)
            {
                minTrapDist = distance;
                nearestTrap = unit;
            }
        }
    }

    // If we found a shambling and we're not close enough, move to it
    if (closestHorror)
    {
        if (closestHorror->isInFront(bot))
            return FleePosition(closestHorror->GetPosition(), 2.0f, 250U);

        // If we're too far, run to it
        if (minHorrorDist > 2.0f)
        {
            // Check if there's a trap in our path
            if (nearestTrap && minTrapDist < 9.0f)
        {
                // Calculate a safe position that avoids the trap
                float angle = nearestTrap->GetAngle(closestHorror);
                float safeX = nearestTrap->GetPositionX() + cos(angle + M_PI_2) * 10.0f;
                float safeY = nearestTrap->GetPositionY() + sin(angle + M_PI_2) * 10.0f;
                float safeZ = 840.857f;

                // Move to safe position first
                botAI->Reset();
                return MoveTo(bot->GetMapId(), safeX, safeY, safeZ,
                            false, false, false, false, MovementPriority::MOVEMENT_FORCED);
            }
            
            // No traps nearby, move directly to horror
            botAI->Reset();
            return MoveTo(closestHorror, 3.0f, MovementPriority::MOVEMENT_FORCED);
        }
        else
        {
            // We're close enough, stop moving
            bot->StopMoving();
            return true;
        }
    }

    return false;
}

bool IccLichKingWinterAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "the lich king");
    if (!boss)
        return false;

    float currentDistance = bot->GetDistance2d(boss);
    // Move behind target if bot is not tank, is close, and is in front
    Unit* currentTarget = AI_VALUE(Unit*, "current target");

    if (!currentTarget)
    {}
    else if (!botAI->IsRanged(bot) && currentTarget->isInFront(bot) && currentTarget->IsAlive() && !botAI->IsTank(bot) && currentTarget->GetEntry() != 36597 &&
             currentTarget->GetEntry() != 36633 && currentTarget->GetEntry() != 39305 && currentTarget->GetEntry() != 39306 && currentTarget->GetEntry() != 39307)
    {
            float orientation = currentTarget->GetOrientation() + M_PI + M_PI / 8;
            float x = currentTarget->GetPositionX();
            float y = currentTarget->GetPositionY();
            float z = currentTarget->GetPositionZ();
            float rx = x + cos(orientation) * 4.0f;
            float ry = y + sin(orientation) * 4.0f;
            return MoveTo(bot->GetMapId(), rx, ry, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT,
                          true, false);
    }

    if (botAI->IsRanged(bot))
    {
        // Calculate distances to group positions
        float dist1 =
            bot->GetDistance2d(ICC_LK_FROSTR1_POSITION.GetPositionX(), ICC_LK_FROSTR1_POSITION.GetPositionY());
        float dist2 =
            bot->GetDistance2d(ICC_LK_FROSTR2_POSITION.GetPositionX(), ICC_LK_FROSTR2_POSITION.GetPositionY());
        float dist3 =
            bot->GetDistance2d(ICC_LK_FROSTR3_POSITION.GetPositionX(), ICC_LK_FROSTR3_POSITION.GetPositionY());

        const Position* targetPos = &ICC_LK_FROSTR1_POSITION;
        if (dist2 < dist1 && dist2 < dist3)
            targetPos = &ICC_LK_FROSTR2_POSITION;
        else if (dist3 < dist1 && dist3 < dist2)
            targetPos = &ICC_LK_FROSTR3_POSITION;

        float distToTarget = bot->GetDistance2d(targetPos->GetPositionX(), targetPos->GetPositionY());
        if (distToTarget > 3.0f)
        {
            float angle = bot->GetAngle(targetPos);
            float posX = bot->GetPositionX() + cos(angle) * 2.0f;
            float posY = bot->GetPositionY() + sin(angle) * 2.0f;
            return MoveTo(bot->GetMapId(), posX, posY, 840.857f, false, false, false, true,
                          MovementPriority::MOVEMENT_FORCED, true, false);
        }
    }
    else
    {
        // Calculate distances to tank and melee positions
        float dist1 = bot->GetDistance2d(ICC_LK_FROST1_POSITION.GetPositionX(), ICC_LK_FROST1_POSITION.GetPositionY());
        float dist2 = bot->GetDistance2d(ICC_LK_FROST2_POSITION.GetPositionX(), ICC_LK_FROST2_POSITION.GetPositionY());
        float dist3 = bot->GetDistance2d(ICC_LK_FROST3_POSITION.GetPositionX(), ICC_LK_FROST3_POSITION.GetPositionY());

        const Position* targetPos = &ICC_LK_FROST1_POSITION;
        if (dist2 < dist1 && dist2 < dist3)
            targetPos = &ICC_LK_FROST2_POSITION;
        else if (dist3 < dist1 && dist3 < dist2)
            targetPos = &ICC_LK_FROST3_POSITION;

        float distToTarget = bot->GetDistance2d(targetPos->GetPositionX(), targetPos->GetPositionY());
        if (distToTarget > 6.0f && !botAI->IsTank(bot))
        {
            float angle = bot->GetAngle(targetPos);
            float posX = bot->GetPositionX() + cos(angle) * 3.0f;
            float posY = bot->GetPositionY() + sin(angle) * 3.0f;
            return MoveTo(bot->GetMapId(), posX, posY, 840.857f, false, false, false, false,
                          MovementPriority::MOVEMENT_FORCED, true, false);
        }

        if (botAI->IsAssistTank(bot) && distToTarget < 10)
        {
            // Actively look for any shambling/spirit/ghoul that isn't targeting our tank
            GuidVector targets = AI_VALUE(GuidVector, "possible targets");
            for (auto i = targets.begin(); i != targets.end(); ++i)
            {
                Unit* unit = botAI->GetUnit(*i);
                if (unit && unit->IsAlive() &&
                    (unit->GetEntry() == 37698 || unit->GetEntry() == 39299 || unit->GetEntry() == 39300 ||
                     unit->GetEntry() == 39301 ||  // Shambling entry
                     unit->GetEntry() == 36701 || unit->GetEntry() == 39302 || unit->GetEntry() == 39303 ||
                     unit->GetEntry() == 39304 ||  // Spirits entry
                     unit->GetEntry() == 37695 || unit->GetEntry() == 39309 || unit->GetEntry() == 39310 ||
                     unit->GetEntry() == 39311))  // Drudge Ghouls entry
                {
                    Unit* mt = AI_VALUE(Unit*, "main tank");                   
                    if (unit->GetVictim() != mt)
                    {
                        bot->SetFacingToObject(unit);
                        return Attack(unit);  // Attack any shambling that isn't targeting a tank
                    }
                }
            }
        }

        if (botAI->IsMainTank(bot))
        {
            // Actively look for any shambling/spirit/ghoul that isn't targeting our tank
            GuidVector targets = AI_VALUE(GuidVector, "possible targets");
            for (auto i = targets.begin(); i != targets.end(); ++i)
            {
                Unit* unit = botAI->GetUnit(*i);
                if (unit && unit->IsAlive() &&
                    (unit->GetEntry() == 37698 || unit->GetEntry() == 39299 || unit->GetEntry() == 39300 ||
                     unit->GetEntry() == 39301 ||  // Shambling entry
                     unit->GetEntry() == 36701 || unit->GetEntry() == 39302 || unit->GetEntry() == 39303 ||
                     unit->GetEntry() == 39304 ||  // Spirits entry
                     unit->GetEntry() == 37695 || unit->GetEntry() == 39309 || unit->GetEntry() == 39310 ||
                     unit->GetEntry() == 39311))  // Drudge Ghouls entry
                {
                    Unit* mt = AI_VALUE(Unit*, "main tank");
                    if (unit->GetVictim() != mt && unit->GetDistance2d(targetPos->GetPositionX(), targetPos->GetPositionY()) < 3.0f)
                    {
                        bot->SetFacingToObject(unit);
                        return Attack(unit);  // Attack any shambling that isn't targeting a tank
                    }
                }
            }
        }

        if (distToTarget > 2.0f && botAI->IsTank(bot))
        {
            float angle = bot->GetAngle(targetPos);
            float posX = bot->GetPositionX() + cos(angle) * 1.5f;
            float posY = bot->GetPositionY() + sin(angle) * 1.5f;
            return MoveTo(bot->GetMapId(), posX, posY, 840.857f, false, false, false, true,
                 MovementPriority::MOVEMENT_FORCED, true, false);
        }
        
    }

   // Check for spheres if we're at a safe distance
    if (botAI->IsRangedDps(bot))
    {
        // First check if the group has a hunter
        bool hasHunter = false;
        Group* group = bot->GetGroup();
        if (group)
        {
            for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
            {
                Player* member = itr->GetSource();
                if (member && member->IsAlive() && member->IsInWorld() && member->getClass() == CLASS_HUNTER)
                {
                    hasHunter = true;
                    break;
                }
            }
        }

        // If the bot is a hunter or the group has no hunter, allow attacking spheres
        if (bot->getClass() == CLASS_HUNTER || !hasHunter)
        {
            Unit* closestSphere = nullptr;
            float closestDist = 100.0f;
            GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
            for (auto& npc : npcs)
            {
                Unit* unit = botAI->GetUnit(npc);
                if (!unit || !unit->IsAlive())
                    continue;
                uint32 entry = unit->GetEntry();
                if (entry == 36633 || entry == 39305 || entry == 39306 || entry == 39307)
                {
                    float dist = bot->GetDistance(unit);
                    if (!closestSphere || dist < closestDist)
                    {
                        closestSphere = unit;
                        closestDist = dist;
                    }
                }
            }
            if (closestSphere)
            {
                botAI->Reset();
                bot->SetFacingToObject(closestSphere);
                return Attack(closestSphere);
            }
        }
    }

    return false;
}

bool IccLichKingAddsAction::Execute(Event event)
{
    if (bot->HasAura(68985)  || !bot->IsAlive())  // Don't process actions if bot is picked up by Val'kyr or is dead
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "the lich king");
    Unit* spiritWarden = AI_VALUE2(Unit*, "find target", "spirit warden");
    bool hasPlague = botAI->HasAura("Necrotic Plague", bot);

    if (bot->HasAura(30440))  // Random aura tracking whether bot has fallen off edge / been thrown by Val'kyr
    {
        if (bot->GetPositionZ() > 779.0f)
            return JumpTo(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), 740.01f);
        else
        {
            bot->KillPlayer();  // If bot has jumped past the kill Z (780), kill
            return true;
        }
    }

    if (!boss)
    {}
    else
    {

        bool hasWinterAura = boss->HasAura(72259) || boss->HasAura(74273) || boss->HasAura(74274) || boss->HasAura(74275);
        bool hasWinter2Aura = boss->HasAura(68981) || boss->HasAura(74270) || boss->HasAura(74271) || boss->HasAura(74272);

        if (boss && boss->GetHealthPct() < 70 && boss->GetHealthPct() > 40 && !hasWinterAura &&
            !hasWinter2Aura)  // If boss is in p2, check if bot has been thrown off platform
        {
            float dx = bot->GetPositionX() - 503.0f;
            float dy = bot->GetPositionY() - (-2124.0f);
            float distance = sqrt(dx * dx + dy * dy);  // Calculate distance from the center of the platform

            if (distance > 52.0f && distance < 70.0f &&
                bot->GetPositionZ() > 844)  // If bot has fallen off edge, distance is over 52
            {
                bot->AddAura(30440, bot);  // Apply random 30 sec aura to track that we've initiated a jump
                return JumpTo(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(),
                          740.01f);  // Start jumping to the abyss
            }
        }
    }

    //temp solution for bots going underground due to buggy ice platfroms and adds that go underground
    if (abs(bot->GetPositionZ() - 840.857f) > 1.0f)
        return bot->TeleportTo(bot->GetMapId(), bot->GetPositionX(),
                          bot->GetPositionY(), 840.857f, bot->GetOrientation());

    //temp soultion for bots when they get teleport far away into another dimension (they are unable to attack spirit warden) in heroic they will be in safe spot while player is surviving vile spirits
    Difficulty diff = bot->GetRaidDifficulty();
    if (!(diff == RAID_DIFFICULTY_10MAN_HEROIC || diff == RAID_DIFFICULTY_25MAN_HEROIC) && abs(bot->GetPositionY() - -2095.7915f) > 200.0f)
    {
        return bot->TeleportTo(bot->GetMapId(), ICC_LICH_KING_ADDS_POSITION.GetPositionX(),
                          ICC_LICH_KING_ADDS_POSITION.GetPositionY(), ICC_LICH_KING_ADDS_POSITION.GetPositionZ(), bot->GetOrientation());
    }

    if (!boss)
    {}
    else if (boss && boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(72262) && bot->GetExactDist2d(boss) > 20.0f)    //quake
    {
        float angle = bot->GetAngle(boss);
        float posX = bot->GetPositionX() + cos(angle) * 10.0f;
        float posY = bot->GetPositionY() + sin(angle) * 10.0f;
        return MoveTo(bot->GetMapId(), posX, posY, boss->GetPositionZ(), 
                     false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }

    // Find closest shambling horror
    GuidVector npcs2 = AI_VALUE(GuidVector, "nearest hostile npcs");
    Unit* closestHorror = nullptr;
    float minHorrorDistance = std::numeric_limits<float>::max();

    for (auto& npc : npcs2)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (unit && unit->IsAlive() &&
            (unit->GetEntry() == 37698 || unit->GetEntry() == 39299 || unit->GetEntry() == 39300 ||
             unit->GetEntry() == 39301))  // Shambling horror entries
        {
            float distance = bot->GetDistance(unit);
            if (distance < minHorrorDistance)
            {
                minHorrorDistance = distance;
                closestHorror = unit;
            }
        }
    }

    if (!closestHorror || hasPlague)
    {}
    else if (!hasPlague && closestHorror->isInFront(bot) && closestHorror->IsAlive() && !botAI->IsTank(bot) && bot->GetDistance2d(closestHorror) < 3.0f)
        return FleePosition(closestHorror->GetPosition(), 2.0f, 250U);

    // If bot is hunter and shambling is enraged, use Tranquilizing Shot
    if (bot->getClass() == CLASS_HUNTER && closestHorror && botAI->HasAura("Enrage", closestHorror))
        return botAI->CastSpell("Tranquilizing Shot", closestHorror);

    if (!boss)
    {}
    else if (botAI->IsAssistTank(bot) && !boss->HealthBelowPct(71))
    {
        // Actively look for any shambling/spirit/ghoul that isn't targeting us
        GuidVector targets = AI_VALUE(GuidVector, "possible targets");
        for (auto i = targets.begin(); i != targets.end(); ++i)
        {
            Unit* unit = botAI->GetUnit(*i);
            if (unit && unit->IsAlive() &&
                (unit->GetEntry() == 37698 || unit->GetEntry() == 39299 || unit->GetEntry() == 39300 ||
                 unit->GetEntry() == 39301 ||  // Shambling entry
                 unit->GetEntry() == 36701 || unit->GetEntry() == 39302 || unit->GetEntry() == 39303 ||
                 unit->GetEntry() == 39304 ||  // Spirits entry
                 unit->GetEntry() == 37695 || unit->GetEntry() == 39309 || unit->GetEntry() == 39310 ||
                 unit->GetEntry() == 39311))  // Drudge Ghouls entry
            {
                if (!unit->GetVictim() || unit->GetVictim() != bot)
                {
                    bot->SetFacingToObject(unit);
                    return Attack(unit);  // Pick up any shambling that isn't targeting us
                }
            }
        }

        // Return to adds position if we're too far
        if (bot->GetExactDist2d(ICC_LICH_KING_ADDS_POSITION) > 1.0f && !boss->HealthBelowPct(71))
        {
            return MoveTo(bot->GetMapId(), ICC_LICH_KING_ADDS_POSITION.GetPositionX(),
                          ICC_LICH_KING_ADDS_POSITION.GetPositionY(), ICC_LICH_KING_ADDS_POSITION.GetPositionZ(), false,
                          false, false, true, MovementPriority::MOVEMENT_FORCED, true, false);
        }
        
        return false;  // Stay in position and keep facing current target
    }

    if (!boss)
    {}
    else if (botAI->IsMelee(bot) && !botAI->IsAssistTank(bot) && !boss->HealthBelowPct(71) && !hasPlague)
    {
        float currentDist = bot->GetDistance(ICC_LICH_KING_MELEE_POSITION);
        if (currentDist > 2.0f)
        {
            return MoveTo(bot->GetMapId(), ICC_LICH_KING_MELEE_POSITION.GetPositionX(),
                          ICC_LICH_KING_MELEE_POSITION.GetPositionY(), ICC_LICH_KING_MELEE_POSITION.GetPositionZ(),
                          false, false, false, true, MovementPriority::MOVEMENT_FORCED, true, false);
        }
    }

    if (!boss)
    {}
    else if (botAI->IsRanged(bot) && !boss->HealthBelowPct(71) && !hasPlague && !(diff == RAID_DIFFICULTY_10MAN_HEROIC || diff == RAID_DIFFICULTY_25MAN_HEROIC))
    {
        float currentDist = bot->GetDistance(ICC_LICH_KING_RANGED_POSITION);
        if (currentDist > 2.0f)
        {
            return MoveTo(bot->GetMapId(), ICC_LICH_KING_RANGED_POSITION.GetPositionX(),
                          ICC_LICH_KING_RANGED_POSITION.GetPositionY(), ICC_LICH_KING_RANGED_POSITION.GetPositionZ(),
                          false, false, false, true, MovementPriority::MOVEMENT_FORCED, true, false);
        }
    }

    std::vector<Unit*> defiles;
    Unit* closestDefile = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    // First gather all defiles
    GuidVector npcs1 = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs1)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (unit && unit->IsAlive() && unit->GetEntry() == 38757)
        {
            defiles.push_back(unit);
            float dist = bot->GetDistance(unit);
            if (dist < closestDistance)
            {
                closestDistance = dist;
                closestDefile = unit;
            }
        }
    }
    if (!defiles.empty())
    {
        float baseRadius = 6.0f;
        float safetyMargin = 3.0f;  // Fixed 3-yard safety margin
        
        // First, find if we need to move from any defile
        bool needToMove = false;
        float bestAngle = 0.0f;
        float maxSafeDistance = 0.0f;

        // Check all defiles first to see if we need to move
        for (Unit* defile : defiles)
        {
            if (!defile || !defile->IsAlive())
                continue;

            float currentRadius = baseRadius;
            Aura* growAura = defile->GetAura(72756);
            if (!growAura)
            {
                growAura = defile->GetAura(74162);
                if (!growAura)
                    growAura = defile->GetAura(74163);
                if (!growAura)
                    growAura = defile->GetAura(74164);  // 25hc mabye 74164
            }

            if (growAura)
            {
                uint8 stacks = growAura->GetStackAmount();
                if (diff == RAID_DIFFICULTY_10MAN_HEROIC || diff == RAID_DIFFICULTY_10MAN_NORMAL)
                currentRadius = baseRadius + (stacks * 1.4f);
                else
                currentRadius = baseRadius + (stacks * 0.95f);
            }

            float dx = bot->GetPositionX() - defile->GetPositionX();
            float dy = bot->GetPositionY() - defile->GetPositionY();
            float distanceToCenter = sqrt(dx * dx + dy * dy);

            if (distanceToCenter < (currentRadius + safetyMargin))
            {
                needToMove = true;
                break;
            }
        }

        // If we need to move, find the safest direction
        if (!boss)
        {}
        else if (needToMove)
        {
            // Try 16 different angles for more precise movement
            for (int i = 0; i < 16; i++)
            {
                float testAngle = i * M_PI / 8;
                float moveDistance = 5.0f; // Move in 5-yard increments
                
                float testX = bot->GetPositionX() + moveDistance * cos(testAngle);
                float testY = bot->GetPositionY() + moveDistance * sin(testAngle);
                float testZ = 840.857f;
                
                bot->UpdateAllowedPositionZ(testX, testY, testZ);

                // Skip if not in LOS or too much height difference
                if (!bot->IsWithinLOS(testX, testY, testZ) || 
                    fabs(testZ - bot->GetPositionZ()) >= 5.0f)
                {
                    continue;
                }

                // Calculate minimum distance to any defile from this position
                float minDefileDistance = std::numeric_limits<float>::max();
                float distanceToBoss = boss->GetDistance2d(testX, testY);
                
                for (Unit* defile : defiles)
                {
                    if (!defile || !defile->IsAlive())
                        continue;

                    float dx = testX - defile->GetPositionX();
                    float dy = testY - defile->GetPositionY();
                    float dist = sqrt(dx * dx + dy * dy);
                    minDefileDistance = std::min(minDefileDistance, dist);
                }

                // Favor positions that are both safe from defiles and closer to the boss
                float safetyScore = minDefileDistance;
                float bossScore = 100.0f - std::min(100.0f, distanceToBoss); // Convert distance to a 0-100 score
                float totalScore = safetyScore + (bossScore * 0.5f); // Weight safety more than boss proximity

                // If this position is better than our previous best, update it
                if (totalScore > maxSafeDistance)
                {
                    maxSafeDistance = totalScore;
                    bestAngle = testAngle;
                }
            }

            // Move in the best direction found
            if (maxSafeDistance > 0)
            {
                float moveDistance = 5.0f;
                float testX = bot->GetPositionX() + moveDistance * cos(bestAngle);
                float testY = bot->GetPositionY() + moveDistance * sin(bestAngle);
                float testZ = 840.857f;
                
                bot->UpdateAllowedPositionZ(testX, testY, testZ);
                return MoveTo(bot->GetMapId(), testX, testY, testZ,
                    false, false, false, true, MovementPriority::MOVEMENT_FORCED);
            }
        }
    }

    // Check if LK is casting Defile - make bots spread
    if (!boss)
    {}
    else if (boss && boss->HasUnitState(UNIT_STATE_CASTING) && boss->FindCurrentSpellBySpellId(72762))
    {
        uint32 playerCount = 0;
        uint32 botIndex = 0;
        uint32 currentIndex = 0;
        
        Map::PlayerList const& players = bot->GetMap()->GetPlayers();
        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            Player* player = itr->GetSource();
            if (!player || !player->IsAlive())
                continue;
                
            if (player == bot)
                botIndex = currentIndex;
                
            currentIndex++;
            playerCount++;
        }

        // Calculate this bot's preferred angle based on its index
        float preferredAngle = (float(botIndex) / float(playerCount)) * 2 * M_PI;
        float moveDistance = 12.0f;  // Fixed distance for consistent spread
        
        // Start checking from preferred angle, then try adjacent angles if blocked
        float bestAngle = preferredAngle;
        bool foundSafeSpot = false;
        
        // Try positions at increasing offsets from preferred angle
        for (int offset = 0; offset <= 8; offset++)  // Try up to 8 positions on each side
        {
            for (int direction = -1; direction <= 1; direction += 2)  // Check both clockwise and counterclockwise
            {
                if (offset == 0 && direction > 0)  // Skip second check of preferred angle
                    continue;
                    
                float testAngle = preferredAngle + (direction * offset * M_PI / 16);
                float testX = bot->GetPositionX() + moveDistance * cos(testAngle);
                float testY = bot->GetPositionY() + moveDistance * sin(testAngle);
                float testZ = 840.857f;
                
                bot->UpdateAllowedPositionZ(testX, testY, testZ);
                
                if (!bot->IsWithinLOS(testX, testY, testZ) || 
                    fabs(testZ - bot->GetPositionZ()) >= 5.0f)
                {
                    continue;
                }

                // Check if position is safe from defiles
                bool isSafeFromDefiles = true;
                for (Unit* defile : defiles)
                {
                    if (!defile || !defile->IsAlive())
                        continue;

                    float currentRadius = 6.0f;
                    Aura* growAura = defile->GetAura(72756);
                    if (!growAura)
                    {
                        growAura = defile->GetAura(74162);
                        if (!growAura)
                            growAura = defile->GetAura(74163);
                        if (!growAura)
                            growAura = defile->GetAura(74164);
                    }

                    if (growAura)
                    {
                        uint8 stacks = growAura->GetStackAmount();
                        if (diff == RAID_DIFFICULTY_10MAN_HEROIC || diff == RAID_DIFFICULTY_10MAN_NORMAL)
                            currentRadius = 6.0f + (stacks * 1.4f);
                        else
                            currentRadius = 6.0f + (stacks * 0.95f);
                    }

                    float dx = testX - defile->GetPositionX();
                    float dy = testY - defile->GetPositionY();
                    float distToDefile = sqrt(dx * dx + dy * dy);

                    if (distToDefile < (currentRadius + 3.0f))
                    {
                        isSafeFromDefiles = false;
                        break;
                    }
                }

                if (!isSafeFromDefiles)
                    continue;

                // Check distance to other players
                bool tooCloseToPlayers = false;
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* player = itr->GetSource();
                    if (!player || !player->IsAlive() || player == bot)
                        continue;
                        
                    float dx = testX - player->GetPositionX();
                    float dy = testY - player->GetPositionY();
                    float dist = sqrt(dx * dx + dy * dy);
                    
                    if (dist < 5.0f)  // Minimum spacing between players
                    {
                        tooCloseToPlayers = true;
                        break;
                    }
                }
                
                if (tooCloseToPlayers)
                    continue;

                float distanceToBoss = boss->GetDistance2d(testX, testY);
                if (distanceToBoss > 40.0f)
                    continue;

                // We found a safe spot
                bestAngle = testAngle;
                foundSafeSpot = true;
                break;
            }
            
            if (foundSafeSpot)
                break;
        }
        
        if (foundSafeSpot)
        {
            float testX = bot->GetPositionX() + moveDistance * cos(bestAngle);
            float testY = bot->GetPositionY() + moveDistance * sin(bestAngle);
            float testZ = 840.857f;
            
            bot->UpdateAllowedPositionZ(testX, testY, testZ);
            return MoveTo(bot->GetMapId(), testX, testY, testZ,
                false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
        }
    }

    // Check for Val'kyr Shadowguards grabbing units
    GuidVector npcs = AI_VALUE(GuidVector, "nearest hostile npcs");
    Unit* highestPriorityValkyr = nullptr;
    float closestValkyrDistance = std::numeric_limits<float>::max();

    // First pass: Find all Val'kyrs and identify the highest priority target
    for (auto& npc : npcs)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || !unit->IsAlive())
            continue;

        // Check if this is a Val'kyr Shadowguard
        if (unit->GetEntry() == 36609 || unit->GetEntry() == 39120 || unit->GetEntry() == 39121 ||
            unit->GetEntry() == 39122)
        {
            float currentDistance = bot->GetDistance(unit);

            // Check if this Val'kyr is grabbing someone (aura 68985)
            bool isGrabbing = unit->HasAura(68985);

            // Prioritize Val'kyrs that are currently grabbing someone, then the closest one
            if (!highestPriorityValkyr ||
                (isGrabbing && (!highestPriorityValkyr->HasAura(68985) || currentDistance < closestValkyrDistance)) ||
                (!highestPriorityValkyr->HasAura(68985) && currentDistance < closestValkyrDistance))
            {
                highestPriorityValkyr = unit;
                closestValkyrDistance = currentDistance;
            }
        }
    }

    // Second pass: Handle the highest priority Val'kyr
    if (highestPriorityValkyr && !botAI->IsMainTank(bot) && !botAI->IsHeal(bot))
    {
        // Try to CC the Val'kyr based on class priority
        bool ccAttempted = false;

        if (bot->getClass() == CLASS_MAGE && !botAI->HasAura("Frost Nova", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Frost Nova", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_DRUID && !botAI->HasAura("Entangling Roots", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Entangling Roots", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_PALADIN && !botAI->HasAura("Hammer of Justice", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Hammer of Justice", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_WARRIOR && !botAI->HasAura("Hamstring", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Hamstring", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_HUNTER && !botAI->HasAura("Concussive Shot", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Concussive Shot", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_ROGUE && !botAI->HasAura("Kidney Shot", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Kidney Shot", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_SHAMAN && !botAI->HasAura("Frost Shock", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Frost Shock", highestPriorityValkyr);
        }
        else if (bot->getClass() == CLASS_DEATH_KNIGHT && !botAI->HasAura("Chains of Ice", highestPriorityValkyr))
        {
            ccAttempted = botAI->CastSpell("Chains of Ice", highestPriorityValkyr);
        }

        // If CC was attempted (success or fail) or if no CC available, attack the Val'kyr
        if (ccAttempted)
        {
            bot->SetFacingToObject(highestPriorityValkyr);
            return Attack(highestPriorityValkyr);
        }
        return false;

    }

    const float radiusvile = 12.0f;

    // Get the nearest hostile NPCs
    GuidVector npcs3 = AI_VALUE(GuidVector, "nearest hostile npcs");
    for (auto& npc : npcs3)
    {
        Unit* unit = botAI->GetUnit(npc);
        if (!unit || unit->GetEntry() != 37799 || unit->GetEntry() != 39284 || unit->GetEntry() != 39285 ||
            unit->GetEntry() != 39286)  // vile spirit ID
            continue;

        // Only run away if the spirit is targeting us
        // Check by GUID comparison to ensure we're accurately identifying the specific spirit in 25HC multiple spirits spawn
        if (unit->GetVictim() && unit->GetVictim()->GetGUID() == bot->GetGUID())
        {
            float currentDistance = bot->GetDistance2d(unit);

            // Move away from the spirit if the bot is too close
            if (currentDistance < radiusvile)
            {
                botAI->Reset();  // forces bot to stop channeling or getting locked by any other action
                return MoveAway(unit, radiusvile - currentDistance);
            }
        }
    }
    return false;
}

