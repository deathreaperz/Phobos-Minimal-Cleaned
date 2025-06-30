#include "Body.h"

#include <Utilities/Cast.h>
#include <Utilities/Debug.h>
#include <AITriggerTypeClass.h>

#include <Ext/Techno/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/HouseType/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/ScriptType/Body.h>
#include <Ext/Team/Body.h>
#include <Locomotor/HoverLocomotionClass.h>
//#include <ExtraHeaders/StackVector.h>

/*
*	Scripts is a part of `TeamClass` that executed sequentally form `ScriptTypeClass`
*	Each script contains function that behave as it programmed
*/

// Performance optimization: Function pointer table for script actions
using ScriptActionHandler = void(*)(TeamClass*);
using ScriptActionHandlerWithArg = void(*)(TeamClass*, int);

// Forward declarations for action handlers
static void HandleTimedAreaGuard(TeamClass* pTeam);
static void HandleLoadIntoTransports(TeamClass* pTeam);
static void HandleWaitUntilFullAmmo(TeamClass* pTeam);
static void HandleDecreaseCurrentAITriggerWeight(TeamClass* pTeam);
static void HandleIncreaseCurrentAITriggerWeight(TeamClass* pTeam);
static void HandleWaitIfNoTarget(TeamClass* pTeam);
static void HandleTeamWeightReward(TeamClass* pTeam);
static void HandlePickRandomScript(TeamClass* pTeam);
static void HandleModifyTargetDistance(TeamClass* pTeam);
static void HandleSetMoveMissionEndMode(TeamClass* pTeam);
static void HandleUnregisterGreatSuccess(TeamClass* pTeam);
static void HandleGatherAroundLeader(TeamClass* pTeam);
static void HandleRandomSkipNextAction(TeamClass* pTeam);
static void HandleForceGlobalOnlyTargetHouseEnemy(TeamClass* pTeam);
static void HandleJumpBackToPreviousScript(TeamClass* pTeam);
static void HandleAbortActionAfterSuccessKill(TeamClass* pTeam);
static void HandleChronoshiftToEnemyBase(TeamClass* pTeam);
static void HandleSimpleDeployerDeploy(TeamClass* pTeam);
static void HandleRepairDestroyedBridge(TeamClass* pTeam);

// Attack handlers
static void HandleRepeatAttackCloserThreat(TeamClass* pTeam);
static void HandleRepeatAttackFartherThreat(TeamClass* pTeam);
static void HandleRepeatAttackCloser(TeamClass* pTeam);
static void HandleRepeatAttackFarther(TeamClass* pTeam);
static void HandleSingleAttackCloserThreat(TeamClass* pTeam);
static void HandleSingleAttackFartherThreat(TeamClass* pTeam);
static void HandleSingleAttackCloser(TeamClass* pTeam);
static void HandleSingleAttackFarther(TeamClass* pTeam);

// Move handlers  
static void HandleMoveToEnemyCloser(TeamClass* pTeam);
static void HandleMoveToEnemyFarther(TeamClass* pTeam);
static void HandleMoveToFriendlyCloser(TeamClass* pTeam);
static void HandleMoveToFriendlyFarther(TeamClass* pTeam);

// Trigger management handlers
static void HandleSetSideIdxForManagingTriggers(TeamClass* pTeam);
static void HandleSetHouseIdxForManagingTriggers(TeamClass* pTeam);
static void HandleManageAllAITriggers(TeamClass* pTeam);
static void HandleEnableTriggersFromList(TeamClass* pTeam);
static void HandleDisableTriggersFromList(TeamClass* pTeam);
static void HandleEnableTriggersWithObjects(TeamClass* pTeam);
static void HandleDisableTriggersWithObjects(TeamClass* pTeam);

// Conditional jump handlers
static void HandleConditionalJumpIfTrue(TeamClass* pTeam);
static void HandleConditionalJumpIfFalse(TeamClass* pTeam);

// Placeholder handlers for unimplemented actions
static void HandleNotImplemented(TeamClass* pTeam);

// Optimized action lookup table - only for frequently used actions
struct ScriptActionEntry {
    PhobosScripts action;
    ScriptActionHandler handler;
};

// Performance: Use constexpr for compile-time optimization
static constexpr ScriptActionEntry SCRIPT_ACTION_TABLE[] = {
    { PhobosScripts::TimedAreaGuard, HandleTimedAreaGuard },
    { PhobosScripts::LoadIntoTransports, HandleLoadIntoTransports },
    { PhobosScripts::WaitUntilFullAmmo, HandleWaitUntilFullAmmo },
    { PhobosScripts::DecreaseCurrentAITriggerWeight, HandleDecreaseCurrentAITriggerWeight },
    { PhobosScripts::IncreaseCurrentAITriggerWeight, HandleIncreaseCurrentAITriggerWeight },
    { PhobosScripts::WaitIfNoTarget, HandleWaitIfNoTarget },
    { PhobosScripts::TeamWeightReward, HandleTeamWeightReward },
    { PhobosScripts::PickRandomScript, HandlePickRandomScript },
    { PhobosScripts::ModifyTargetDistance, HandleModifyTargetDistance },
    { PhobosScripts::SetMoveMissionEndMode, HandleSetMoveMissionEndMode },
    { PhobosScripts::UnregisterGreatSuccess, HandleUnregisterGreatSuccess },
    { PhobosScripts::GatherAroundLeader, HandleGatherAroundLeader },
    { PhobosScripts::RandomSkipNextAction, HandleRandomSkipNextAction },
    { PhobosScripts::ForceGlobalOnlyTargetHouseEnemy, HandleForceGlobalOnlyTargetHouseEnemy },
    { PhobosScripts::JumpBackToPreviousScript, HandleJumpBackToPreviousScript },
    { PhobosScripts::AbortActionAfterSuccessKill, HandleAbortActionAfterSuccessKill },
    { PhobosScripts::ChronoshiftToEnemyBase, HandleChronoshiftToEnemyBase },
    { PhobosScripts::SimpleDeployerDeploy, HandleSimpleDeployerDeploy },
    { PhobosScripts::RepairDestroyedBridge, HandleRepairDestroyedBridge },
    
    // Attack actions
    { PhobosScripts::RepeatAttackCloserThreat, HandleRepeatAttackCloserThreat },
    { PhobosScripts::RepeatAttackFartherThreat, HandleRepeatAttackFartherThreat },
    { PhobosScripts::RepeatAttackCloser, HandleRepeatAttackCloser },
    { PhobosScripts::RepeatAttackFarther, HandleRepeatAttackFarther },
    { PhobosScripts::SingleAttackCloserThreat, HandleSingleAttackCloserThreat },
    { PhobosScripts::SingleAttackFartherThreat, HandleSingleAttackFartherThreat },
    { PhobosScripts::SingleAttackCloser, HandleSingleAttackCloser },
    { PhobosScripts::SingleAttackFarther, HandleSingleAttackFarther },
    
    // Move actions
    { PhobosScripts::MoveToEnemyCloser, HandleMoveToEnemyCloser },
    { PhobosScripts::MoveToEnemyFarther, HandleMoveToEnemyFarther },
    { PhobosScripts::MoveToFriendlyCloser, HandleMoveToFriendlyCloser },
    { PhobosScripts::MoveToFriendlyFarther, HandleMoveToFriendlyFarther },
    
    // Trigger management
    { PhobosScripts::SetSideIdxForManagingTriggers, HandleSetSideIdxForManagingTriggers },
    { PhobosScripts::SetHouseIdxForManagingTriggers, HandleSetHouseIdxForManagingTriggers },
    { PhobosScripts::ManageAllAITriggers, HandleManageAllAITriggers },
    { PhobosScripts::EnableTriggersFromList, HandleEnableTriggersFromList },
    { PhobosScripts::DisableTriggersFromList, HandleDisableTriggersFromList },
    { PhobosScripts::EnableTriggersWithObjects, HandleEnableTriggersWithObjects },
    { PhobosScripts::DisableTriggersWithObjects, HandleDisableTriggersWithObjects },
    
    // Conditional jumps
    { PhobosScripts::ConditionalJumpIfTrue, HandleConditionalJumpIfTrue },
    { PhobosScripts::ConditionalJumpIfFalse, HandleConditionalJumpIfFalse },
    
    // Placeholder for unimplemented
    { PhobosScripts::ChangeTeamGroup, HandleNotImplemented },
    { PhobosScripts::DistributedLoading, HandleNotImplemented },
    { PhobosScripts::FollowFriendlyByGroup, HandleNotImplemented },
    { PhobosScripts::RallyUnitWithSameGroup, HandleNotImplemented },
};

static constexpr size_t SCRIPT_ACTION_TABLE_SIZE = sizeof(SCRIPT_ACTION_TABLE) / sizeof(SCRIPT_ACTION_TABLE[0]);

// Performance: Binary search for action lookup (O(log n) instead of O(n))
static ScriptActionHandler FindActionHandler(PhobosScripts action) {
    // For small table sizes, linear search can be faster due to cache locality
    if constexpr (SCRIPT_ACTION_TABLE_SIZE <= 16) {
        for (const auto& entry : SCRIPT_ACTION_TABLE) {
            if (entry.action == action) {
                return entry.handler;
            }
        }
    } else {
        // Binary search for larger tables
        int left = 0;
        int right = SCRIPT_ACTION_TABLE_SIZE - 1;
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (SCRIPT_ACTION_TABLE[mid].action == action) {
                return SCRIPT_ACTION_TABLE[mid].handler;
            } else if (SCRIPT_ACTION_TABLE[mid].action < action) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
    }
    return nullptr;
}

ScriptActionNode NOINLINE ScriptExtData::GetSpecificAction(ScriptClass* pScript, int nIdx)
{
	if ((size_t)nIdx < (size_t)pScript->Type->ActionsCount)
		return pScript->Type->ScriptActions[nIdx];
	//	nIdx = pScript->Type->ActionsCount;
	//
	//auto const nIdxR = nIdx - ScriptTypeClass::MaxActions;
	//auto const pTypeExt = ScriptTypeExt::ExtMap.Find(pScript->Type);
	//
	//if (!pTypeExt->PhobosNode.empty() && nIdxR < (int)pTypeExt->PhobosNode.size()) {
	//	return pTypeExt->PhobosNode[nIdxR];
	//}
	//COMPILETIMEEVAL auto const nMax = ScriptTypeClass::MaxActions - 1;
	//return pScript->Type->ScriptActions[nMax];

	return { -1 , 0 };
}

// =============================
// container
ScriptExtContainer ScriptExtContainer::Instance;

#define stringify( name ) #name

static NOINLINE const char* ToStrings(PhobosScripts from)
{
	switch (from)
	{
	case PhobosScripts::TimedAreaGuard:
		return "TimedAreaGuard";
	case PhobosScripts::LoadIntoTransports:
		return "LoadIntoTransports";
	case PhobosScripts::WaitUntilFullAmmo:
		return "WaitUntilFullAmmo";
	case PhobosScripts::RepeatAttackCloserThreat:
		return "RepeatAttackCloserThreat";
	case PhobosScripts::RepeatAttackFartherThreat:
		return "RepeatAttackFartherThreat";
	case PhobosScripts::RepeatAttackCloser:
		return "RepeatAttackCloser";
	case PhobosScripts::RepeatAttackFarther:
		return "RepeatAttackFarther";
	case PhobosScripts::SingleAttackCloserThreat:
		return "SingleAttackCloserThreat";
	case PhobosScripts::SingleAttackFartherThreat:
		return "SingleAttackFartherThreat";
	case PhobosScripts::SingleAttackCloser:
		return "SingleAttackCloser";
	case PhobosScripts::SingleAttackFarther:
		return "SingleAttackFarther";
	case PhobosScripts::DecreaseCurrentAITriggerWeight:
		return "DecreaseCurrentAITriggerWeight";
	case PhobosScripts::IncreaseCurrentAITriggerWeight:
		return "IncreaseCurrentAITriggerWeight";
	case PhobosScripts::RepeatAttackTypeCloserThreat:
		return "RepeatAttackTypeCloserThreat";
	case PhobosScripts::RepeatAttackTypeFartherThreat:
		return "RepeatAttackTypeFartherThreat";
	case PhobosScripts::RepeatAttackTypeCloser:
		return "RepeatAttackTypeCloser";
	case PhobosScripts::RepeatAttackTypeFarther:
		return "RepeatAttackTypeFarther";
	case PhobosScripts::SingleAttackTypeCloserThreat:
		return "SingleAttackTypeCloserThreat";
	case PhobosScripts::SingleAttackTypeFartherThreat:
		return "SingleAttackTypeFartherThreat";
	case PhobosScripts::SingleAttackTypeCloser:
		return "SingleAttackTypeCloser";
	case PhobosScripts::SingleAttackTypeFarther:
		return "SingleAttackTypeFarther";
	case PhobosScripts::WaitIfNoTarget:
		return "WaitIfNoTarget";
	case PhobosScripts::TeamWeightReward:
		return "TeamWeightReward";
	case PhobosScripts::PickRandomScript:
		return "PickRandomScript";
	case PhobosScripts::MoveToEnemyCloser:
		return "MoveToEnemyCloser";
	case PhobosScripts::MoveToEnemyFarther:
		return "MoveToEnemyFarther";
	case PhobosScripts::MoveToFriendlyCloser:
		return "MoveToFriendlyCloser";
	case PhobosScripts::MoveToFriendlyFarther:
		return "MoveToFriendlyFarther";
	case PhobosScripts::MoveToTypeEnemyCloser:
		return "MoveToTypeEnemyCloser";
	case PhobosScripts::MoveToTypeEnemyFarther:
		return "MoveToTypeEnemyFarther";
	case PhobosScripts::MoveToTypeFriendlyCloser:
		return "MoveToTypeFriendlyCloser";
	case PhobosScripts::MoveToTypeFriendlyFarther:
		return "MoveToTypeFriendlyFarther";
	case PhobosScripts::ModifyTargetDistance:
		return "ModifyTargetDistance";
	case PhobosScripts::RandomAttackTypeCloser:
		return "RandomAttackTypeCloser";
	case PhobosScripts::RandomAttackTypeFarther:
		return "RandomAttackTypeFarther";
	case PhobosScripts::RandomMoveToTypeEnemyCloser:
		return "RandomMoveToTypeEnemyCloser";
	case PhobosScripts::RandomMoveToTypeEnemyFarther:
		return "RandomMoveToTypeEnemyFarther";
	case PhobosScripts::RandomMoveToTypeFriendlyCloser:
		return "RandomMoveToTypeFriendlyCloser";
	case PhobosScripts::RandomMoveToTypeFriendlyFarther:
		return "RandomMoveToTypeFriendlyFarther";
	case PhobosScripts::SetMoveMissionEndMode:
		return "SetMoveMissionEndMode";
	case PhobosScripts::UnregisterGreatSuccess:
		return "UnregisterGreatSuccess";
	case PhobosScripts::GatherAroundLeader:
		return "GatherAroundLeader";
	case PhobosScripts::RandomSkipNextAction:
		return "RandomSkipNextAction";
	case PhobosScripts::ChangeTeamGroup:
		return "ChangeTeamGroup";
	case PhobosScripts::DistributedLoading:
		return "DistributedLoading";
	case PhobosScripts::FollowFriendlyByGroup:
		return "FollowFriendlyByGroup";
	case PhobosScripts::RallyUnitWithSameGroup:
		return "RallyUnitWithSameGroup";
	case PhobosScripts::StopForceJumpCountdown:
		return "StopForceJumpCountdown";
	case PhobosScripts::NextLineForceJumpCountdown:
		return "NextLineForceJumpCountdown";
	case PhobosScripts::SameLineForceJumpCountdown:
		return "SameLineForceJumpCountdown";
	case PhobosScripts::ForceGlobalOnlyTargetHouseEnemy:
		return "ForceGlobalOnlyTargetHouseEnemy";
	case PhobosScripts::OverrideOnlyTargetHouseEnemy:
		return "OverrideOnlyTargetHouseEnemy";
	case PhobosScripts::SetHouseAngerModifier:
		return "SetHouseAngerModifier";
	case PhobosScripts::ModifyHateHouseIndex:
		return "ModifyHateHouseIndex";
	case PhobosScripts::ModifyHateHousesList:
		return "ModifyHateHousesList";
	case PhobosScripts::ModifyHateHousesList1Random:
		return "ModifyHateHousesList1Randoms";
	case PhobosScripts::SetTheMostHatedHouseMinorNoRandom:
		return "SetTheMostHatedHouseMinorNoRandom";
	case PhobosScripts::SetTheMostHatedHouseMajorNoRandom:
		return "SetTheMostHatedHouseMajorNoRandom";
	case PhobosScripts::SetTheMostHatedHouseRandom:
		return "SetTheMostHatedHouseRandom";
	case PhobosScripts::ResetAngerAgainstHouses:
		return "ResetAngerAgainstHouses";
	case PhobosScripts::AggroHouse:
		return "AggroHouse";
	case PhobosScripts::SetSideIdxForManagingTriggers:
		return "SetSideIdxForManagingTriggers";
	case PhobosScripts::SetHouseIdxForManagingTriggers:
		return "SetHouseIdxForManagingTriggers";
	case PhobosScripts::ManageAllAITriggers:
		return "ManageAllAITriggers";
	case PhobosScripts::EnableTriggersFromList:
		return "EnableTriggersFromList";
	case PhobosScripts::DisableTriggersFromList:
		return "DisableTriggersFromList";
	case PhobosScripts::DisableTriggersWithObjects:
		return "DisableTriggersWithObjects";
	case PhobosScripts::EnableTriggersWithObjects:
		return "EnableTriggersWithObjects";
	case PhobosScripts::ConditionalJumpResetVariables:
		return "ConditionalJumpResetVariables";
	case PhobosScripts::ConditionalJumpManageResetIfJump:
		return "ConditionalJumpManageResetIfJump";
	case PhobosScripts::AbortActionAfterSuccessKill:
		return "AbortActionAfterSuccessKill";
	case PhobosScripts::ConditionalJumpManageKillsCounter:
		return "ConditionalJumpManageKillsCounter";
	case PhobosScripts::ConditionalJumpSetCounter:
		return "ConditionalJumpSetCounter";
	case PhobosScripts::ConditionalJumpSetComparatorMode:
		return "ConditionalJumpSetComparatorMode";
	case PhobosScripts::ConditionalJumpSetComparatorValue:
		return "ConditionalJumpSetComparatorValue";
	case PhobosScripts::ConditionalJumpSetIndex:
		return "ConditionalJumpSetIndex";
	case PhobosScripts::ConditionalJumpIfFalse:
		return "ConditionalJumpIfFalse";
	case PhobosScripts::ConditionalJumpIfTrue:
		return "ConditionalJumpIfTrue";
	case PhobosScripts::ConditionalJumpKillEvaluation:
		return "ConditionalJumpKillEvaluation";
	case PhobosScripts::ConditionalJumpCheckCount:
		return "ConditionalJumpCheckCount";
	case PhobosScripts::ConditionalJumpCheckAliveHumans:
		return "ConditionalJumpCheckAliveHumans";
	case PhobosScripts::ConditionalJumpCheckObjects:
		return "ConditionalJumpCheckObjects";
	case PhobosScripts::ConditionalJumpCheckHumanIsMostHated:
		return "ConditionalJumpCheckHumanIsMostHated";
	case PhobosScripts::JumpBackToPreviousScript:
		return "JumpBackToPreviousScript";
	case PhobosScripts::RepairDestroyedBridge:
		return "RepairDestroyedBridge";
	case PhobosScripts::ChronoshiftToEnemyBase:
		return "ChronoshiftToEnemyBase";
	case PhobosScripts::LocalVariableSet:
		return "LocalVariableSet";
	case PhobosScripts::LocalVariableAdd:
		return "LocalVariableAdd";
	case PhobosScripts::LocalVariableMinus:
		return "LocalVariableMinus";
	case PhobosScripts::LocalVariableMultiply:
		return "LocalVariableMultiply";
	case PhobosScripts::LocalVariableDivide:
		return "LocalVariableDivide";
	case PhobosScripts::LocalVariableMod:
		return "LocalVariableMod";
	case PhobosScripts::LocalVariableLeftShift:
		return "LocalVariableLeftShift";
	case PhobosScripts::LocalVariableRightShift:
		return "LocalVariableRightShift";
	case PhobosScripts::LocalVariableReverse:
		return "LocalVariableReverse";
	case PhobosScripts::LocalVariableXor:
		return "LocalVariableXor";
	case PhobosScripts::LocalVariableOr:
		return "LocalVariableOr";
	case PhobosScripts::LocalVariableAnd:
		return "LocalVariableAnd";
	case PhobosScripts::GlobalVariableSet:
		return "GlobalVariableSet";
	case PhobosScripts::GlobalVariableAdd:
		return "GlobalVariableAdd";
	case PhobosScripts::GlobalVariableMinus:
		return "GlobalVariableMinus";
	case PhobosScripts::GlobalVariableMultiply:
		return "GlobalVariableMultiply";
	case PhobosScripts::GlobalVariableDivide:
		return "GlobalVariableDivide";
	case PhobosScripts::GlobalVariableMod:
		return "GlobalVariableMod";
	case PhobosScripts::GlobalVariableLeftShift:
		return "GlobalVariableLeftShift";
	case PhobosScripts::GlobalVariableRightShift:
		return "GlobalVariableRightShift";
	case PhobosScripts::GlobalVariableReverse:
		return "GlobalVariableReverse";
	case PhobosScripts::GlobalVariableXor:
		return "GlobalVariableXor";
	case PhobosScripts::GlobalVariableOr:
		return "GlobalVariableOr";
	case PhobosScripts::GlobalVariableAnd:
		return "GlobalVariableAnd";
	case PhobosScripts::LocalVariableSetByLocal:
		return "LocalVariableSetByLocal";
	case PhobosScripts::LocalVariableAddByLocal:
		return "LocalVariableAddByLocal";
	case PhobosScripts::LocalVariableMinusByLocal:
		return "LocalVariableMinusByLocal";
	case PhobosScripts::LocalVariableMultiplyByLocal:
		return "LocalVariableMultiplyByLocal";
	case PhobosScripts::LocalVariableDivideByLocal:
		return "LocalVariableDivideByLocal";
	case PhobosScripts::LocalVariableModByLocal:
		return "LocalVariableModByLocal";
	case PhobosScripts::LocalVariableLeftShiftByLocal:
		return "LocalVariableLeftShiftByLocal";
	case PhobosScripts::LocalVariableRightShiftByLocal:
		return "LocalVariableRightShiftByLocal";
	case PhobosScripts::LocalVariableReverseByLocal:
		return "LocalVariableReverseByLocal";
	case PhobosScripts::LocalVariableXorByLocal:
		return "LocalVariableXorByLocal";
	case PhobosScripts::LocalVariableOrByLocal:
		return "LocalVariableOrByLocal";
	case PhobosScripts::LocalVariableAndByLocal:
		return "LocalVariableAndByLocal";
	case PhobosScripts::GlobalVariableSetByLocal:
		return "GlobalVariableSetByLocal";
	case PhobosScripts::GlobalVariableAddByLocal:
		return "GlobalVariableAddByLocal";
	case PhobosScripts::GlobalVariableMinusByLocal:
		return "GlobalVariableMinusByLocal";
	case PhobosScripts::GlobalVariableMultiplyByLocal:
		return "GlobalVariableMultiplyByLocal";
	case PhobosScripts::GlobalVariableDivideByLocal:
		return "GlobalVariableDivideByLocal";
	case PhobosScripts::GlobalVariableModByLocal:
		return "GlobalVariableModByLocal";
	case PhobosScripts::GlobalVariableLeftShiftByLocal:
		return "GlobalVariableLeftShiftByLocal";
	case PhobosScripts::GlobalVariableRightShiftByLocal:
		return "GlobalVariableRightShiftByLocal";
	case PhobosScripts::GlobalVariableReverseByLocal:
		return "GlobalVariableReverseByLocal";
	case PhobosScripts::GlobalVariableXorByLocal:
		return "GlobalVariableXorByLocal";
	case PhobosScripts::GlobalVariableOrByLocal:
		return "GlobalVariableOrByLocal";
	case PhobosScripts::GlobalVariableAndByLocal:
		return "GlobalVariableAndByLocal";
	case PhobosScripts::LocalVariableSetByGlobal:
		return "LocalVariableSetByGlobal";
	case PhobosScripts::LocalVariableAddByGlobal:
		return "LocalVariableAddByGlobal";
	case PhobosScripts::LocalVariableMinusByGlobal:
		return "LocalVariableMinusByGlobal";
	case PhobosScripts::LocalVariableMultiplyByGlobal:
		return "LocalVariableMultiplyByGlobal";
	case PhobosScripts::LocalVariableDivideByGlobal:
		return "LocalVariableDivideByGlobal";
	case PhobosScripts::LocalVariableModByGlobal:
		return "LocalVariableModByGlobal";
	case PhobosScripts::LocalVariableLeftShiftByGlobal:
		return "LocalVariableLeftShiftByGlobal";
	case PhobosScripts::LocalVariableRightShiftByGlobal:
		return "LocalVariableRightShiftByGlobal";
	case PhobosScripts::LocalVariableReverseByGlobal:
		return "LocalVariableReverseByGlobal";
	case PhobosScripts::LocalVariableXorByGlobal:
		return "LocalVariableXorByGlobal";
	case PhobosScripts::LocalVariableOrByGlobal:
		return "LocalVariableOrByGlobal";
	case PhobosScripts::LocalVariableAndByGlobal:
		return "LocalVariableAndByGlobal";
	case PhobosScripts::GlobalVariableSetByGlobal:
		return "GlobalVariableSetByGlobal";
	case PhobosScripts::GlobalVariableAddByGlobal:
		return "GlobalVariableAddByGlobal";
	case PhobosScripts::GlobalVariableMinusByGlobal:
		return "GlobalVariableMinusByGlobal";
	case PhobosScripts::GlobalVariableMultiplyByGlobal:
		return "GlobalVariableMultiplyByGlobal";
	case PhobosScripts::GlobalVariableDivideByGlobal:
		return "GlobalVariableDivideByGlobal";
	case PhobosScripts::GlobalVariableModByGlobal:
		return "GlobalVariableModByGlobal";
	case PhobosScripts::GlobalVariableLeftShiftByGlobal:
		return "GlobalVariableLeftShiftByGlobal";
	case PhobosScripts::GlobalVariableRightShiftByGlobal:
		return "GlobalVariableRightShiftByGlobal";
	case PhobosScripts::GlobalVariableReverseByGlobal:
		return "GlobalVariableReverseByGlobal";
	case PhobosScripts::GlobalVariableXorByGlobal:
		return "GlobalVariableXorByGlobal";
	case PhobosScripts::GlobalVariableOrByGlobal:
		return "GlobalVariableOrByGlobal";
	case PhobosScripts::GlobalVariableAndByGlobal:
		return "GlobalVariableAndByGlobal";
	case PhobosScripts::SimpleDeployerDeploy:
		return "SimpleDeployerDeploy";
	default:
		return GameStrings::NoneStr();
	}
}

#include "Lua/Wrapper.h"

bool ScriptExtData::ProcessScriptActions(TeamClass* pTeam, ScriptActionNode* pTeamMission, bool bThirdArd)
{
	auto const& [action, argument] = *pTeamMission;
	const PhobosScripts scriptAction = static_cast<PhobosScripts>(action);

	// Performance: Early return for variable operations (most common)
	if (IsExtVariableAction(action)) {
		return VariablesHandler(pTeam, scriptAction, argument);
	}

	// Performance: Use function pointer table for common actions
	if (auto handler = FindActionHandler(scriptAction)) {
		handler(pTeam);
		return true;
	}

	// Fallback to switch for remaining actions (less common ones)
	switch (scriptAction) {
	// Attack type handlers with arguments
	case PhobosScripts::RepeatAttackTypeCloserThreat:
		ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::idkZero, argument);
		break;
	case PhobosScripts::RepeatAttackTypeFartherThreat:
		ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::idkOne, argument);
		break;
	case PhobosScripts::RepeatAttackTypeCloser:
		ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::Closest, argument);
		break;
	case PhobosScripts::RepeatAttackTypeFarther:
		ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::Furtherst, argument);
		break;
	case PhobosScripts::SingleAttackTypeCloserThreat:
		ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::idkZero, argument);
		break;
	case PhobosScripts::SingleAttackTypeFartherThreat:
		ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::idkOne, argument);
		break;
	case PhobosScripts::SingleAttackTypeCloser:
		ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::Closest, argument);
		break;
	case PhobosScripts::SingleAttackTypeFarther:
		ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::Furtherst, argument);
		break;

	// Move type handlers with arguments  
	case PhobosScripts::MoveToTypeEnemyCloser:
		ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Closest, false, argument);
		break;
	case PhobosScripts::MoveToTypeEnemyFarther:
		ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Furtherst, false, argument);
		break;
	case PhobosScripts::MoveToTypeFriendlyCloser:
		ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Closest, true, argument);
		break;
	case PhobosScripts::MoveToTypeFriendlyFarther:
		ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Furtherst, true, argument);
		break;

	// Random handlers
	case PhobosScripts::RandomAttackTypeCloser:
		ScriptExtData::Mission_Attack_List1Random(pTeam, false, DistanceMode::Closest, argument);
		break;
	case PhobosScripts::RandomAttackTypeFarther:
		ScriptExtData::Mission_Attack_List1Random(pTeam, false, DistanceMode::Furtherst, argument);
		break;
	case PhobosScripts::RandomMoveToTypeEnemyCloser:
		ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Closest, false, argument, -1);
		break;
	case PhobosScripts::RandomMoveToTypeEnemyFarther:
		ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Furtherst, false, argument, -1);
		break;
	case PhobosScripts::RandomMoveToTypeFriendlyCloser:
		ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Closest, true, argument, -1);
		break;
	case PhobosScripts::RandomMoveToTypeFriendlyFarther:
		ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Furtherst, true, argument, -1);
		break;

	// Force jump actions
	case PhobosScripts::StopForceJumpCountdown:
		ScriptExtData::Stop_ForceJump_Countdown(pTeam);
		break;
	case PhobosScripts::NextLineForceJumpCountdown:
		ScriptExtData::Set_ForceJump_Countdown(pTeam, false, argument);
		break;
	case PhobosScripts::SameLineForceJumpCountdown:
		ScriptExtData::Set_ForceJump_Countdown(pTeam, true, argument);
		break;

	// Anger/house management
	case PhobosScripts::OverrideOnlyTargetHouseEnemy:
		ScriptExtData::OverrideOnlyTargetHouseEnemy(pTeam, argument);
		break;
	case PhobosScripts::SetHouseAngerModifier:
		ScriptExtData::SetHouseAngerModifier(pTeam, argument);
		break;
	case PhobosScripts::ModifyHateHouseIndex:
		ScriptExtData::ModifyHateHouse_Index(pTeam, argument);
		break;
	case PhobosScripts::ModifyHateHousesList:
		ScriptExtData::ModifyHateHouses_List(pTeam, argument);
		break;
	case PhobosScripts::ModifyHateHousesList1Random:
		ScriptExtData::ModifyHateHouses_List1Random(pTeam, argument);
		break;
	case PhobosScripts::SetTheMostHatedHouseMinorNoRandom:
		ScriptExtData::SetTheMostHatedHouse(pTeam, 1, 0, false);
		break;
	case PhobosScripts::SetTheMostHatedHouseMajorNoRandom:
		ScriptExtData::SetTheMostHatedHouse(pTeam, 2, 0, false);
		break;
	case PhobosScripts::SetTheMostHatedHouseRandom:
		ScriptExtData::SetTheMostHatedHouse(pTeam, argument, 0, true);
		break;
	case PhobosScripts::ResetAngerAgainstHouses:
		ScriptExtData::ResetAngerAgainstHouses(pTeam);
		break;
	case PhobosScripts::AggroHouse:
		ScriptExtData::AggroHouse(pTeam, argument);
		break;

	// Conditional jump actions with arguments
	case PhobosScripts::ConditionalJumpResetVariables:
		ScriptExtData::ConditionalJump_ResetVariables(pTeam);
		break;
	case PhobosScripts::ConditionalJumpManageResetIfJump:
		ScriptExtData::ConditionalJump_ManageResetIfJump(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpManageKillsCounter:
		ScriptExtData::ConditionalJump_ManageKillsCounter(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpSetCounter:
		ScriptExtData::ConditionalJump_SetCounter(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpSetComparatorMode:
		ScriptExtData::ConditionalJump_SetComparatorMode(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpSetComparatorValue:
		ScriptExtData::ConditionalJump_SetComparatorValue(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpSetIndex:
		ScriptExtData::ConditionalJump_SetIndex(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpKillEvaluation:
		ScriptExtData::ConditionalJump_KillEvaluation(pTeam);
		break;
	case PhobosScripts::ConditionalJumpCheckCount:
		ScriptExtData::ConditionalJump_CheckCount(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpCheckAliveHumans:
		ScriptExtData::ConditionalJump_CheckAliveHumans(pTeam, argument);
		break;
	case PhobosScripts::ConditionalJumpCheckObjects:
		ScriptExtData::ConditionalJump_CheckObjects(pTeam);
		break;
	case PhobosScripts::ConditionalJumpCheckHumanIsMostHated:
		ScriptExtData::ConditionalJump_CheckHumanIsMostHated(pTeam);
		break;

	default:
		return false;
	}

	return true;
}

// Action handler implementations
static void HandleTimedAreaGuard(TeamClass* pTeam) {
	ScriptExtData::ExecuteTimedAreaGuardAction(pTeam);
}

static void HandleLoadIntoTransports(TeamClass* pTeam) {
	ScriptExtData::LoadIntoTransports(pTeam);
}

static void HandleWaitUntilFullAmmo(TeamClass* pTeam) {
	ScriptExtData::WaitUntilFullAmmoAction(pTeam);
}

static void HandleDecreaseCurrentAITriggerWeight(TeamClass* pTeam) {
	ScriptExtData::DecreaseCurrentTriggerWeight(pTeam, true, 0);
}

static void HandleIncreaseCurrentAITriggerWeight(TeamClass* pTeam) {
	ScriptExtData::IncreaseCurrentTriggerWeight(pTeam, true, 0);
}

static void HandleWaitIfNoTarget(TeamClass* pTeam) {
	ScriptExtData::WaitIfNoTarget(pTeam, -1);
}

static void HandleTeamWeightReward(TeamClass* pTeam) {
	ScriptExtData::TeamWeightReward(pTeam, -1);
}

static void HandlePickRandomScript(TeamClass* pTeam) {
	ScriptExtData::PickRandomScript(pTeam, -1);
}

static void HandleModifyTargetDistance(TeamClass* pTeam) {
	ScriptExtData::SetCloseEnoughDistance(pTeam, -1);
}

static void HandleSetMoveMissionEndMode(TeamClass* pTeam) {
	ScriptExtData::SetMoveMissionEndMode(pTeam, -1);
}

static void HandleUnregisterGreatSuccess(TeamClass* pTeam) {
	ScriptExtData::UnregisterGreatSuccess(pTeam);
}

static void HandleGatherAroundLeader(TeamClass* pTeam) {
	ScriptExtData::Mission_Gather_NearTheLeader(pTeam, -1);
}

static void HandleRandomSkipNextAction(TeamClass* pTeam) {
	ScriptExtData::SkipNextAction(pTeam, -1);
}

static void HandleForceGlobalOnlyTargetHouseEnemy(TeamClass* pTeam) {
	ScriptExtData::ForceGlobalOnlyTargetHouseEnemy(pTeam, -1);
}

static void HandleJumpBackToPreviousScript(TeamClass* pTeam) {
	ScriptExtData::JumpBackToPreviousScript(pTeam);
}

static void HandleAbortActionAfterSuccessKill(TeamClass* pTeam) {
	ScriptExtData::SetAbortActionAfterSuccessKill(pTeam, -1);
}

static void HandleChronoshiftToEnemyBase(TeamClass* pTeam) {
	auto pScript = pTeam->CurrentScript;
	const auto& [curAct, curArgs] = pScript->GetCurrentAction();
	ScriptExtData::ChronoshiftToEnemyBase(pTeam, curArgs);
}

static void HandleSimpleDeployerDeploy(TeamClass* pTeam) {
	auto pScript = pTeam->CurrentScript;
	const auto& [curAct, curArgs] = pScript->GetCurrentAction();
	ScriptExtData::SimpleDeployerDeploy(pTeam, curArgs);
}

static void HandleRepairDestroyedBridge(TeamClass* pTeam) {
	ScriptExtData::RepairDestroyedBridge(pTeam, -1);
	pTeam->StepCompleted = true;
}

// Attack handlers
static void HandleRepeatAttackCloserThreat(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::idkZero, -1, -1);
}

static void HandleRepeatAttackFartherThreat(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::idkOne, -1, -1);
}

static void HandleRepeatAttackCloser(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::Closest, -1, -1);
}

static void HandleRepeatAttackFarther(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::Furtherst, -1, -1);
}

static void HandleSingleAttackCloserThreat(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::idkZero, -1, -1);
}

static void HandleSingleAttackFartherThreat(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::idkOne, -1, -1);
}

static void HandleSingleAttackCloser(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::Closest, -1, -1);
}

static void HandleSingleAttackFarther(TeamClass* pTeam) {
	ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::Furtherst, -1, -1);
}

// Move handlers
static void HandleMoveToEnemyCloser(TeamClass* pTeam) {
	ScriptExtData::Mission_Move(pTeam, DistanceMode::Closest, false, -1, -1);
}

static void HandleMoveToEnemyFarther(TeamClass* pTeam) {
	ScriptExtData::Mission_Move(pTeam, DistanceMode::Furtherst, false, -1, -1);
}

static void HandleMoveToFriendlyCloser(TeamClass* pTeam) {
	ScriptExtData::Mission_Move(pTeam, DistanceMode::Closest, true, -1, -1);
}

static void HandleMoveToFriendlyFarther(TeamClass* pTeam) {
	ScriptExtData::Mission_Move(pTeam, DistanceMode::Furtherst, true, -1, -1);
}

// Trigger management handlers
static void HandleSetSideIdxForManagingTriggers(TeamClass* pTeam) {
	ScriptExtData::SetSideIdxForManagingTriggers(pTeam, -1);
}

static void HandleSetHouseIdxForManagingTriggers(TeamClass* pTeam) {
	ScriptExtData::SetHouseIdxForManagingTriggers(pTeam, 1000000);
}

static void HandleManageAllAITriggers(TeamClass* pTeam) {
	ScriptExtData::ManageAITriggers(pTeam, -1);
}

static void HandleEnableTriggersFromList(TeamClass* pTeam) {
	ScriptExtData::ManageTriggersFromList(pTeam, -1, true);
}

static void HandleDisableTriggersFromList(TeamClass* pTeam) {
	ScriptExtData::ManageTriggersFromList(pTeam, -1, false);
}

static void HandleEnableTriggersWithObjects(TeamClass* pTeam) {
	ScriptExtData::ManageTriggersWithObjects(pTeam, -1, true);
}

static void HandleDisableTriggersWithObjects(TeamClass* pTeam) {
	ScriptExtData::ManageTriggersWithObjects(pTeam, -1, false);
}

// Conditional jump handlers
static void HandleConditionalJumpIfTrue(TeamClass* pTeam) {
	auto pScript = pTeam->CurrentScript;
	const auto& [curAct, curArgs] = pScript->GetCurrentAction();
	ScriptExtData::ConditionalJumpIfTrue(pTeam, curArgs);
}

static void HandleConditionalJumpIfFalse(TeamClass* pTeam) {
	auto pScript = pTeam->CurrentScript;
	const auto& [curAct, curArgs] = pScript->GetCurrentAction();
	ScriptExtData::ConditionalJumpIfFalse(pTeam, curArgs);
}

// Placeholder for unimplemented actions
static void HandleNotImplemented(TeamClass* pTeam) {
	auto pScript = pTeam->CurrentScript;
	const auto& [curAct, curArgs] = pScript->GetCurrentAction();
	Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", 
		(void*)pTeam, pTeam->get_ID(), (void*)pScript, pScript->get_ID(), (int)curAct);
	pTeam->StepCompleted = true;
}

void NOINLINE ScriptExtData::ExecuteTimedAreaGuardAction(TeamClass* pTeam)
{
	auto const pScript = pTeam->CurrentScript;
	auto const pScriptType = pScript->Type;

	if (pScriptType->ScriptActions[pScript->CurrentMission].Argument <= 0)
	{
		pTeam->StepCompleted = true;
		return;
	}

	const auto Isticking = pTeam->GuardAreaTimer.IsTicking();
	const auto TimeLeft = pTeam->GuardAreaTimer.GetTimeLeft();

	if (!Isticking && !TimeLeft)
	{
		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			if (TechnoExtData::IsInWarfactory(pUnit))
				continue; // held back Timed area guard if one of the member still in warfactory

			pUnit->QueueMission(Mission::Area_Guard, true);
		}

		pTeam->GuardAreaTimer.Start(15 * pScriptType->ScriptActions[pScript->CurrentMission].Argument);
	}
	else if (Isticking && !TimeLeft)
	{
		pTeam->GuardAreaTimer.Stop(); // Needed
		pTeam->StepCompleted = true;
	}
}

#include <ExtraHeaders/StackVector.h>

void ScriptExtData::LoadIntoTransports(TeamClass* pTeam)
{
	StackVector<FootClass*, 10> transports {};

	// Collect available transports
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		auto const pType = pUnit->GetTechnoType();

		if (pType->Passengers > 0
			&& pUnit->Passengers.NumPassengers < pType->Passengers
			&& (TechnoTypeExtContainer::Instance.Find(pType)->Passengers_BySize
				? pUnit->Passengers.GetTotalSize() : pUnit->Passengers.NumPassengers)
			 < pType->Passengers)
		{
			transports->push_back(pUnit);
		}
	}

	if (transports->empty())
	{
		// This action finished
		pTeam->StepCompleted = true;
		return;
	}

	// Now load units into transports
	for (auto pTransport : transports.container())
	{
		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			auto const pTransportType = pTransport->GetTechnoType();
			auto const pUnitType = pUnit->GetTechnoType();

			if (pTransport != pUnit
				&& pUnitType->WhatAmI() != AbstractType::AircraftType
				&& !pUnit->InLimbo && !pUnitType->ConsideredAircraft
				&& pUnit->Health > 0)
			{
				if (pUnit->GetTechnoType()->Size > 0
					&& pUnitType->Size <= pTransportType->SizeLimit
					&& pUnitType->Size <= pTransportType->Passengers - pTransport->Passengers.GetTotalSize())
				{
					// If is still flying wait a bit more
					if (pTransport->IsInAir())
						return;

					// All fine
					if (pUnit->GetCurrentMission() != Mission::Enter)
					{
						pUnit->QueueMission(Mission::Enter, false);
						pUnit->SetTarget(nullptr);
						pUnit->SetDestination(pTransport, true);
						pUnit->NextMission();
						return;
					}
				}
			}
		}
	}

	// Is loading
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (pUnit->GetCurrentMission() == Mission::Enter)
			return;
	}

	auto const pExt = TeamExtContainer::Instance.Find(pTeam);

	FootClass* pLeaderUnit = FindTheTeamLeader(pTeam);
	pLeaderUnit->IsTeamLeader = 1;
	pExt->TeamLeader = pLeaderUnit;

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExtData::WaitUntilFullAmmoAction(TeamClass* pTeam)
{
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (ScriptExtData::IsUnitAvailable(pUnit, false))
		{
			const auto pType = pUnit->GetTechnoType();

			if (pType->Ammo > 0 && pUnit->Ammo < pType->Ammo)
			{
				// If an aircraft object have AirportBound it must be evaluated
				if (auto const pAircraft = cast_to<AircraftClass*, false>(pUnit))
				{
					if (pAircraft->Type->AirportBound)
					{
						// Reset last target, at long term battles this prevented the aircraft to pick a new target (rare vanilla YR bug)
						pUnit->SetTarget(nullptr);
						pUnit->LastTarget = nullptr;
						// Fix YR bug (when returns from the last attack the aircraft switch in loop between Mission::Enter & Mission::Guard, making it impossible to land in the dock)
						if (pUnit->IsInAir() && pUnit->CurrentMission != Mission::Enter)
							pUnit->QueueMission(Mission::Enter, true);

						break;
					}
				}
				else if (pType->Reload != 0) // Don't skip units that can reload themselves
					break;
			}
		}
	}

	pTeam->StepCompleted = true;
}

void ScriptExtData::Mission_Gather_NearTheLeader(TeamClass* pTeam, int countdown = -1)
{
	// This team has no units! END
	if (!pTeam)
	{
		// This action finished
		pTeam->StepCompleted = true;
		return;
	}

	FootClass* pLeaderUnit = nullptr;
	int initialCountdown = pTeam->CurrentScript->GetCurrentAction().Argument;
	bool gatherUnits = false;
	auto const pExt = TeamExtContainer::Instance.Find(pTeam);

	// Load countdown
	if (pExt->Countdown_RegroupAtLeader >= 0)
		countdown = pExt->Countdown_RegroupAtLeader;

	// Gather permanently until all the team members are near of the Leader
	if (initialCountdown == 0)
		gatherUnits = true;

	// Countdown updater
	if (initialCountdown > 0)
	{
		if (countdown > 0)
		{
			countdown--; // Update countdown
			gatherUnits = true;
		}
		else if (countdown == 0) // Countdown ended
			countdown = -1;
		else // Start countdown.
		{
			countdown = initialCountdown * 15;
			gatherUnits = true;
		}

		// Save counter
		pExt->Countdown_RegroupAtLeader = countdown;
	}

	if (!gatherUnits)
	{
		// This action finished
		pTeam->StepCompleted = true;
		return;
	}
	else
	{
		// Move all around the leader, the leader always in "Guard Area" Mission or simply in Guard Mission
		int nTogether = 0;
		int nUnits = -1; // Leader counts here
		double closeEnough;

		// Find the Leader
		pLeaderUnit = pExt->TeamLeader;

		if (!ScriptExtData::IsUnitAvailable(pLeaderUnit, true))
		{
			if (pLeaderUnit)
				pLeaderUnit->IsTeamLeader = false;

			pLeaderUnit = ScriptExtData::FindTheTeamLeader(pTeam);
			pLeaderUnit->IsTeamLeader = true;
			pExt->TeamLeader = pLeaderUnit;
		}

		if (!pLeaderUnit)
		{
			pExt->Countdown_RegroupAtLeader = -1;
			// This action finished
			pTeam->StepCompleted = true;

			return;
		}

		// Leader's area radius where the Team members are considered "near" to the Leader
		if (pExt->CloseEnough > 0)
		{
			// This a one-time-use value
			closeEnough = std::exchange(pExt->CloseEnough, -1);
		}
		else
		{
			closeEnough = RulesClass::Instance->CloseEnough.ToCell();
		}

		// The leader should stay calm & be the group's center
		if (pLeaderUnit->Locomotor.GetInterfacePtr()->Is_Moving_Now())
			pLeaderUnit->SetDestination(nullptr, false);

		pLeaderUnit->QueueMission(Mission::Guard, false);

		// Check if units are around the leader
		for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
		{
			if (ScriptExtData::IsUnitAvailable(pUnit, true))
			{
				auto pTypeUnit = pUnit->GetTechnoType();

				if (pUnit == pLeaderUnit)
				{
					nUnits++;
					continue;
				}

				// Aircraft case
				if (pTypeUnit->WhatAmI() == AbstractType::AircraftType && pUnit->Ammo <= 0 && pTypeUnit->Ammo > 0)
				{
					auto pAircraft = static_cast<AircraftTypeClass*>(pUnit->GetTechnoType());

					if (pAircraft->AirportBound)
					{
						// This aircraft won't count for the script action
						pUnit->EnterIdleMode(false, true);

						continue;
					}
				}

				nUnits++;

				if ((pUnit->DistanceFrom(pLeaderUnit) / 256.0) > closeEnough)
				{
					// Leader's location is too far from me. Regroup
					if (pUnit->Destination != pLeaderUnit)
					{
						pUnit->SetDestination(pLeaderUnit, false);
						pUnit->QueueMission(Mission::Move, true);
					}
				}
				else
				{
					// Is near of the leader, then protect the area
					if (pUnit->GetCurrentMission() != Mission::Area_Guard || pUnit->GetCurrentMission() != Mission::Attack)
						pUnit->QueueMission(Mission::Area_Guard, true);

					nTogether++;
				}
			}
		}

		if (nUnits >= 0
			&& nUnits == nTogether
			&& (initialCountdown == 0
				|| (initialCountdown > 0
					&& countdown <= 0)))
		{
			pExt->Countdown_RegroupAtLeader = -1;
			// This action finished
			pTeam->StepCompleted = true;

			return;
		}
	}
}

void ScriptExtData::DecreaseCurrentTriggerWeight(TeamClass* pTeam, bool forceJumpLine = true, double modifier = 0)
{
	if (modifier <= 0)
		modifier = pTeam->CurrentScript->GetCurrentAction().Argument;

	if (modifier <= 0)
		modifier = RulesClass::Instance->AITriggerFailureWeightDelta;
	else
		modifier = modifier * (-1);

	ScriptExtData::ModifyCurrentTriggerWeight(pTeam, forceJumpLine, modifier);

	// This action finished
	if (forceJumpLine)
		pTeam->StepCompleted = true;

	return;
}

void ScriptExtData::IncreaseCurrentTriggerWeight(TeamClass* pTeam, bool forceJumpLine = true, double modifier = 0)
{
	if (modifier <= 0)
		modifier = pTeam->CurrentScript->GetCurrentAction().Argument;

	if (modifier <= 0)
		modifier = Math::abs(RulesClass::Instance->AITriggerSuccessWeightDelta);

	ScriptExtData::ModifyCurrentTriggerWeight(pTeam, forceJumpLine, modifier);

	// This action finished
	if (forceJumpLine)
		pTeam->StepCompleted = true;

	return;
}

void ScriptExtData::ModifyCurrentTriggerWeight(TeamClass* pTeam, bool forceJumpLine = true, double modifier = 0)
{
	AITriggerTypeClass* pTriggerType = nullptr;
	auto pTeamType = pTeam->Type;
	bool found = false;

	for (int i = 0; i < AITriggerTypeClass::Array->Count && !found; i++)
	{
		auto pTriggerTeam1Type = AITriggerTypeClass::Array->Items[i]->Team1;
		auto pTriggerTeam2Type = AITriggerTypeClass::Array->Items[i]->Team2;

		if (pTeamType
			&& ((pTriggerTeam1Type && pTriggerTeam1Type == pTeamType)
				|| (pTriggerTeam2Type && pTriggerTeam2Type == pTeamType)))
		{
			found = true;
			pTriggerType = AITriggerTypeClass::Array->Items[i];
		}
	}

	if (found)
	{
		pTriggerType->Weight_Current += modifier;

		if (pTriggerType->Weight_Current > pTriggerType->Weight_Maximum)
		{
			pTriggerType->Weight_Current = pTriggerType->Weight_Maximum;
		}
		else
		{
			if (pTriggerType->Weight_Current < pTriggerType->Weight_Minimum)
				pTriggerType->Weight_Current = pTriggerType->Weight_Minimum;
		}
	}
}

void ScriptExtData::WaitIfNoTarget(TeamClass* pTeam, int attempts = 0)
{
	// This method modifies the new attack actions preventing Team's Trigger to jump to next script action
	// attempts == number of times the Team will wait if Mission_Attack(...) can't find a new target.
	if (attempts < 0)
		attempts = pTeam->CurrentScript->GetCurrentAction().Argument;

	auto pExt = TeamExtContainer::Instance.Find(pTeam);

	if (attempts <= 0)
		pExt->WaitNoTargetAttempts = -1; // Infinite waits if no target
	else
		pExt->WaitNoTargetAttempts = attempts;

	// This action finished
	pTeam->StepCompleted = true;

	return;
}

void ScriptExtData::TeamWeightReward(TeamClass* pTeam, double award = 0)
{
	if (award <= 0)
		award = pTeam->CurrentScript->GetCurrentAction().Argument;

	if (award > 0)
		TeamExtContainer::Instance.Find(pTeam)->NextSuccessWeightAward = award;

	// This action finished
	pTeam->StepCompleted = true;

	return;
}

void ScriptExtData::PickRandomScript(TeamClass* pTeam, int idxScriptsList = -1)
{
	if (idxScriptsList < 0)
		idxScriptsList = pTeam->CurrentScript->GetCurrentAction().Argument;

	const auto& scriptList = RulesExtData::Instance()->AIScriptsLists;
	if ((size_t)idxScriptsList < scriptList.size())
	{
		const auto& objectsList = scriptList[idxScriptsList];

		if (!objectsList.empty())
		{
			int IdxSelectedObject = ScenarioClass::Instance->Random.RandomFromMax(objectsList.size() - 1);

			ScriptTypeClass* pNewScript = objectsList[IdxSelectedObject];

			if (pNewScript->ActionsCount > 0 && pNewScript != pTeam->CurrentScript->Type)
			{
				TeamExtContainer::Instance.Find(pTeam)->PreviousScript = pTeam->CurrentScript->Type;
				GameDelete<true, false>(pTeam->CurrentScript);
				pTeam->CurrentScript = GameCreate<ScriptClass>(pNewScript);
				// Ready for jumping to the first line of the new script
				pTeam->CurrentScript->CurrentMission = -1;
				pTeam->StepCompleted = true;

				return;
			}
			else
			{
				pTeam->StepCompleted = true;
				Debug::LogInfo("AI Scripts - PickRandomScript: [{}] Aborting Script change because [{}] has 0 Action scripts!",
					pTeam->Type->ID,
					pNewScript->ID
				);

				return;
			}
		}
	}

	pTeam->StepCompleted = true;
	Debug::LogInfo("AI Scripts - PickRandomScript: [{}] [{}] Failed to change the Team Script with index [{}]!",
		pTeam->Type->ID,
		pTeam->CurrentScript->Type->ID,
		idxScriptsList
	);
}

void ScriptExtData::SetCloseEnoughDistance(TeamClass* pTeam, double distance = -1)
{
	// This passive method replaces the CloseEnough value from rulesmd.ini by a custom one. Used by Mission_Move()
	if (distance <= 0)
		distance = pTeam->CurrentScript->GetCurrentAction().Argument;

	auto const pTeamData = TeamExtContainer::Instance.Find(pTeam);

	if (distance > 0)
		pTeamData->CloseEnough = distance;

	if (distance <= 0)
		pTeamData->CloseEnough = RulesClass::Instance->CloseEnough.ToCell();

	// This action finished
	pTeam->StepCompleted = true;

	return;
}

void ScriptExtData::UnregisterGreatSuccess(TeamClass* pTeam)
{
	pTeam->AchievedGreatSuccess = false;
	pTeam->StepCompleted = true;
}

void ScriptExtData::SetMoveMissionEndMode(TeamClass* pTeam, int mode = 0)
{
	// This passive method replaces the CloseEnough value from rulesmd.ini by a custom one. Used by Mission_Move()
	if (mode < 0 || mode > 2)
		mode = pTeam->CurrentScript->GetCurrentAction().Argument;

	if (auto const pTeamData = TeamExtContainer::Instance.Find(pTeam))
	{
		if (mode >= 0 && mode <= 2)
			pTeamData->MoveMissionEndMode = mode;
	}

	// This action finished
	pTeam->StepCompleted = true;

	return;
}

bool ScriptExtData::MoveMissionEndStatus(TeamClass* pTeam, TechnoClass* pFocus, FootClass* pLeader = nullptr, int mode = 0)
{
	if (!pTeam || mode < 0)
		return false;

	if (!ScriptExtData::IsUnitAvailable(pFocus, false))
	{
		pTeam->ArchiveTarget = nullptr;
		return false;
	}

	auto const pTeamData = TeamExtContainer::Instance.Find(pTeam);

	if (mode != 2 && mode != 1 && !ScriptExtData::IsUnitAvailable(pLeader, false))
	{
		pTeamData->TeamLeader = nullptr;
		return false;
	}

	double closeEnough = 0.0;

	if (pTeamData->CloseEnough > 0)
		closeEnough = pTeamData->CloseEnough;
	else
		closeEnough = RulesClass::Instance->CloseEnough.ToCell();

	bool bForceNextAction = mode == 2;

	// Team already have a focused target
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (ScriptExtData::IsUnitAvailable(pUnit, true)
			&& !pUnit->TemporalTargetingMe
			&& !pUnit->BeingWarpedOut)
		{
			if (mode == 2)
			{
				// Default mode: all members in range
				if ((pUnit->DistanceFrom(pFocus->GetCell()) / 256.0) > closeEnough)
				{
					bForceNextAction = false;

					if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo > 0)
						pUnit->QueueMission(Mission::Move, true);

					continue;
				}
				else
				{
					if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo <= 0)
					{
						pUnit->EnterIdleMode(false, true);

						continue;
					}
				}
			}
			else
			{
				if (mode == 1)
				{
					// Any member in range
					if ((pUnit->DistanceFrom(pFocus->GetCell()) / 256.0) > closeEnough)
					{
						if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo > 0)
							pUnit->QueueMission(Mission::Move, true);

						continue;
					}
					else
					{
						bForceNextAction = true;

						if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo <= 0)
						{
							pUnit->EnterIdleMode(false, true);

							continue;
						}
					}
				}
				else
				{
					// All other cases: Team Leader mode in range
					if (ScriptExtData::IsUnitAvailable(pLeader, false))
					{
						if ((pUnit->DistanceFrom(pFocus->GetCell()) / 256.0) > closeEnough)
						{
							if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo > 0)
								pUnit->QueueMission(Mission::Move, true);

							continue;
						}
						else
						{
							if (pUnit->IsTeamLeader)
								bForceNextAction = true;

							if (pUnit->WhatAmI() == AbstractType::Aircraft && pUnit->Ammo <= 0)
							{
								pUnit->EnterIdleMode(false, true);

								continue;
							}
						}
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return bForceNextAction;
}

void ScriptExtData::SkipNextAction(TeamClass* pTeam, int successPercentage = 0)
{
	// This team has no units! END
	//if (!pTeam)
	//{
	//	// This action finished
	//	pTeam->StepCompleted = true;
	//	const auto&[curAct, curArg] = pTeam->CurrentScript->GetCurrentAction();
	//	const auto&[nextAct, nextArg] = pTeam->CurrentScript->GetNextAction();
	//	Debug::LogInfo("AI Scripts - SkipNextAction: [{}] [{}] (line: {}) Jump to next line: {} = {},{} -> (No team members alive)",
	//		pTeam->Type->ID,
	//		pTeam->CurrentScript->Type->ID,
	//		pTeam->CurrentScript->CurrentMission,
	//		curAct,
	//		curArg,
	//		pTeam->CurrentScript->CurrentMission + 1,
	//		nextAct,
	//		nextArg);
	//
	// 	return;
	// }

	if (successPercentage < 0 || successPercentage > 100)
		successPercentage = pTeam->CurrentScript->GetCurrentAction().Argument;

	if (successPercentage < 0)
		successPercentage = 0;

	if (successPercentage > 100)
		successPercentage = 100;

	int percentage = ScenarioClass::Instance->Random.RandomRanged(1, 100);

	if (percentage <= successPercentage)
	{
		// const auto& [curAct, curArg] = pTeam->CurrentScript->GetCurrentAction();
		// const auto& [nextAct, nextArg] = ScriptExtData::GetSpecificAction(pTeam->CurrentScript, pTeam->CurrentScript->CurrentMission + 2);

		// Debug::LogInfo("AI Scripts - SkipNextAction: [{}] [{}] (line: {} = {},{}) Next script line skipped successfuly. Next line will be: {} = {},{}",
		// 	pTeam->Type->ID,
		// 	pTeam->CurrentScript->Type->ID,
		// 	pTeam->CurrentScript->CurrentMission,
		// 	(int)curAct,
		// 	curArg,
		// 	pTeam->CurrentScript->CurrentMission + 2,
		// 	(int)nextAct,
		// 	nextArg
		// );

		pTeam->CurrentScript->CurrentMission++;
	}

	// This action finished
	pTeam->StepCompleted = true;
}

// Performance: Move operation functors outside function to avoid recreation
struct operation_set { constexpr int operator()(const int& a, const int& b) const { return b; } };
struct operation_add { constexpr int operator()(const int& a, const int& b) const { return a + b; } };
struct operation_minus { constexpr int operator()(const int& a, const int& b) const { return a - b; } };
struct operation_multiply { constexpr int operator()(const int& a, const int& b) const { return a * b; } };
struct operation_divide { constexpr int operator()(const int& a, const int& b) const { return b != 0 ? a / b : 0; } };
struct operation_mod { constexpr int operator()(const int& a, const int& b) const { return b != 0 ? a % b : 0; } };
struct operation_leftshift { constexpr int operator()(const int& a, const int& b) const { return a << b; } };
struct operation_rightshift { constexpr int operator()(const int& a, const int& b) const { return a >> b; } };
struct operation_reverse { constexpr int operator()(const int& a, const int& b) const { return ~a; } };
struct operation_xor { constexpr int operator()(const int& a, const int& b) const { return a ^ b; } };
struct operation_or { constexpr int operator()(const int& a, const int& b) const { return a | b; } };
struct operation_and { constexpr int operator()(const int& a, const int& b) const { return a & b; } };

bool ScriptExtData::VariablesHandler(TeamClass* pTeam, PhobosScripts eAction, int nArg)
{

	int nLoArg = LOWORD(nArg);
	int nHiArg = HIWORD(nArg);

	switch (eAction)
	{
	case PhobosScripts::LocalVariableSet:
		VariableOperationHandler<false, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAdd:
		VariableOperationHandler<false, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMinus:
		VariableOperationHandler<false, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMultiply:
		VariableOperationHandler<false, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableDivide:
		VariableOperationHandler<false, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMod:
		VariableOperationHandler<false, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableLeftShift:
		VariableOperationHandler<false, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableRightShift:
		VariableOperationHandler<false, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableReverse:
		VariableOperationHandler<false, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableXor:
		VariableOperationHandler<false, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableOr:
		VariableOperationHandler<false, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAnd:
		VariableOperationHandler<false, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableSet:
		VariableOperationHandler<true, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAdd:
		VariableOperationHandler<true, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMinus:
		VariableOperationHandler<true, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMultiply:
		VariableOperationHandler<true, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableDivide:
		VariableOperationHandler<true, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMod:
		VariableOperationHandler<true, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableLeftShift:
		VariableOperationHandler<true, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableRightShift:
		VariableOperationHandler<true, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableReverse:
		VariableOperationHandler<true, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableXor:
		VariableOperationHandler<true, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableOr:
		VariableOperationHandler<true, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAnd:
		VariableOperationHandler<true, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableSetByLocal:
		VariableBinaryOperationHandler<false, false, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAddByLocal:
		VariableBinaryOperationHandler<false, false, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMinusByLocal:
		VariableBinaryOperationHandler<false, false, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMultiplyByLocal:
		VariableBinaryOperationHandler<false, false, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableDivideByLocal:
		VariableBinaryOperationHandler<false, false, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableModByLocal:
		VariableBinaryOperationHandler<false, false, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableLeftShiftByLocal:
		VariableBinaryOperationHandler<false, false, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableRightShiftByLocal:
		VariableBinaryOperationHandler<false, false, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableReverseByLocal:
		VariableBinaryOperationHandler<false, false, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableXorByLocal:
		VariableBinaryOperationHandler<false, false, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableOrByLocal:
		VariableBinaryOperationHandler<false, false, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAndByLocal:
		VariableBinaryOperationHandler<false, false, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableSetByLocal:
		VariableBinaryOperationHandler<false, true, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAddByLocal:
		VariableBinaryOperationHandler<false, true, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMinusByLocal:
		VariableBinaryOperationHandler<false, true, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMultiplyByLocal:
		VariableBinaryOperationHandler<false, true, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableDivideByLocal:
		VariableBinaryOperationHandler<false, true, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableModByLocal:
		VariableBinaryOperationHandler<false, true, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableLeftShiftByLocal:
		VariableBinaryOperationHandler<false, true, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableRightShiftByLocal:
		VariableBinaryOperationHandler<false, true, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableReverseByLocal:
		VariableBinaryOperationHandler<false, true, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableXorByLocal:
		VariableBinaryOperationHandler<false, true, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableOrByLocal:
		VariableBinaryOperationHandler<false, true, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAndByLocal:
		VariableBinaryOperationHandler<false, true, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableSetByGlobal:
		VariableBinaryOperationHandler<true, false, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAddByGlobal:
		VariableBinaryOperationHandler<true, false, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMinusByGlobal:
		VariableBinaryOperationHandler<true, false, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableMultiplyByGlobal:
		VariableBinaryOperationHandler<true, false, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableDivideByGlobal:
		VariableBinaryOperationHandler<true, false, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableModByGlobal:
		VariableBinaryOperationHandler<true, false, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableLeftShiftByGlobal:
		VariableBinaryOperationHandler<true, false, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableRightShiftByGlobal:
		VariableBinaryOperationHandler<true, false, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableReverseByGlobal:
		VariableBinaryOperationHandler<true, false, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableXorByGlobal:
		VariableBinaryOperationHandler<true, false, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableOrByGlobal:
		VariableBinaryOperationHandler<true, false, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::LocalVariableAndByGlobal:
		VariableBinaryOperationHandler<true, false, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableSetByGlobal:
		VariableBinaryOperationHandler<true, true, operation_set>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAddByGlobal:
		VariableBinaryOperationHandler<true, true, operation_add>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMinusByGlobal:
		VariableBinaryOperationHandler<true, true, operation_minus>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableMultiplyByGlobal:
		VariableBinaryOperationHandler<true, true, operation_multiply>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableDivideByGlobal:
		VariableBinaryOperationHandler<true, true, operation_divide>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableModByGlobal:
		VariableBinaryOperationHandler<true, true, operation_mod>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableLeftShiftByGlobal:
		VariableBinaryOperationHandler<true, true, operation_leftshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableRightShiftByGlobal:
		VariableBinaryOperationHandler<true, true, operation_rightshift>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableReverseByGlobal:
		VariableBinaryOperationHandler<true, true, operation_reverse>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableXorByGlobal:
		VariableBinaryOperationHandler<true, true, operation_xor>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableOrByGlobal:
		VariableBinaryOperationHandler<true, true, operation_or>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::GlobalVariableAndByGlobal:
		VariableBinaryOperationHandler<true, true, operation_and>(pTeam, nLoArg, nHiArg); break;
	case PhobosScripts::SimpleDeployerDeploy:
		VariableOperationHandler<false, operation_set>(pTeam, nLoArg, nHiArg); break;
	default:
		return false;
	}

	return true;
}

template<bool IsGlobal, class _Pr>
void ScriptExtData::VariableOperationHandler(TeamClass* pTeam, int nVariable, int Number)
{
	if (auto itr = ScenarioExtData::GetVariables(IsGlobal)->tryfind(nVariable))
	{
		itr->Value = _Pr()(itr->Value, Number);
		if (IsGlobal)
			TagClass::NotifyGlobalChanged(nVariable);
		else
			TagClass::NotifyLocalChanged(nVariable);
	}

	pTeam->StepCompleted = true;
}

template<bool IsSrcGlobal, bool IsGlobal, class _Pr>
void ScriptExtData::VariableBinaryOperationHandler(TeamClass* pTeam, int nVariable, int nVarToOperate)
{
	if (auto itr = ScenarioExtData::GetVariables(IsGlobal)->tryfind(nVarToOperate))
		VariableOperationHandler<IsGlobal, _Pr>(pTeam, nVariable, itr->Value);

	pTeam->StepCompleted = true;
}

FootClass* ScriptExtData::FindTheTeamLeader(TeamClass* pTeam)
{
	FootClass* pLeaderUnit = nullptr;
	int bestUnitLeadershipValue = -1;

	if (!pTeam)
		return pLeaderUnit;

	// Find the Leader or promote a new one
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (!IsUnitAvailable(pUnit, true) || !(pUnit->IsTeamLeader || pUnit->WhatAmI() == AbstractType::Aircraft))
			continue;

		// The team Leader will be used for selecting targets, if there are living Team Members then always exists 1 Leader.
		int unitLeadershipRating = pUnit->GetTechnoType()->LeadershipRating;

		if (unitLeadershipRating > bestUnitLeadershipValue)
		{
			pLeaderUnit = pUnit;
			bestUnitLeadershipValue = unitLeadershipRating;
		}
	}

	return pLeaderUnit;
}

// Performance: Inline and optimize variable action check
inline bool ScriptExtData::IsExtVariableAction(int action)
{
	// Use single range check instead of two comparisons for better performance
	const int offset = action - static_cast<int>(PhobosScripts::LocalVariableSet);
	return static_cast<unsigned int>(offset) <= 
		static_cast<unsigned int>(PhobosScripts::GlobalVariableAndByGlobal) - 
		static_cast<unsigned int>(PhobosScripts::LocalVariableSet);
}

void ScriptExtData::Set_ForceJump_Countdown(TeamClass* pTeam, bool repeatLine = false, int count = 0)
{
	if (!pTeam)
		return;
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);
	const auto& [curAct, curArgs] = pTeam->CurrentScript->GetCurrentAction();

	if (count <= 0)
		count = 15 * curArgs;

	if (count > 0)
	{
		pTeamData->ForceJump_InitialCountdown = count;
		pTeamData->ForceJump_Countdown.Start(count);
		pTeamData->ForceJump_RepeatMode = repeatLine;
	}
	else
	{
		pTeamData->ForceJump_InitialCountdown = -1;
		pTeamData->ForceJump_Countdown.Stop();
		pTeamData->ForceJump_RepeatMode = false;
	}

	auto pScript = pTeam->CurrentScript;

	// This action finished
	pTeam->StepCompleted = true;
	Debug::LogInfo("AI Scripts - SetForceJumpCountdown: [{}] [{}](line: {} = {},{}) Set Timed Jump -> (Countdown: {}, repeat action: {})",
		pTeam->Type->ID,
		pScript->Type->ID,
		pScript->CurrentMission,
		(int)curAct,
		curArgs,
		count, repeatLine
	);
}

void ScriptExtData::Stop_ForceJump_Countdown(TeamClass* pTeam)
{
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);
	pTeamData->ForceJump_InitialCountdown = -1;
	pTeamData->ForceJump_Countdown.Stop();
	pTeamData->ForceJump_RepeatMode = false;

	auto pScript = pTeam->CurrentScript;

	// This action finished
	pTeam->StepCompleted = true;
	const auto& [curAct, curArgs] = pTeam->CurrentScript->GetCurrentAction();
	Debug::LogInfo("AI Scripts - StopForceJumpCountdown: [{}] [{}](line: {} = {},{}): Stopped Timed Jump",
		pTeam->Type->ID,
		pScript->Type->ID,
		pScript->CurrentMission,
		(int)curAct,
		curArgs
	);
}

void ScriptExtData::ChronoshiftToEnemyBase(TeamClass* pTeam, int extraDistance)
{
	//auto pScript = pTeam->CurrentScript;
	auto const pLeader = ScriptExtData::FindTheTeamLeader(pTeam);
	//const auto& [curAct, curArgs] = pTeam->CurrentScript->GetCurrentAction();
	//const auto& [nextAct, nextArgs] = pTeam->CurrentScript->GetNextAction();
	pLeader->IsTeamLeader = true;

	if (!pLeader)
	{
		pTeam->StepCompleted = true;
		return;
	}

	int houseIndex = pLeader->Owner->EnemyHouseIndex;
	HouseClass* pEnemy = houseIndex != -1 ? HouseClass::Array->Items[houseIndex] : nullptr;

	if (!pEnemy)
	{
		pTeam->StepCompleted = true;
		return;
	}

	auto const pTargetCell = HouseExtData::GetEnemyBaseGatherCell(pEnemy, pLeader->Owner, pLeader->GetCoords(), pLeader->GetTechnoType()->SpeedType, extraDistance);

	if (!pTargetCell)
	{
		pTeam->StepCompleted = true;
		return;
	}

	ScriptExtData::ChronoshiftTeamToTarget(pTeam, pLeader, pTargetCell);
}

#include <Ext/SWType/Body.h>

void ScriptExtData::ChronoshiftTeamToTarget(TeamClass* pTeam, TechnoClass* pTeamLeader, AbstractClass* pTarget)
{
	if (!pTeam || !pTeamLeader || !pTarget)
		return;

	//auto pScript = pTeam->CurrentScript;
	HouseClass* pOwner = pTeamLeader->Owner;
	SuperClass* pSuperChronosphere = nullptr;
	SuperClass* pSuperChronowarp = nullptr;

	for (auto& pSuper : pOwner->Supers)
	{
		if (!SWTypeExtContainer::Instance.Find(pSuper->Type)->IsAvailable(pOwner))
			continue;

		if (!pSuperChronosphere && pSuper->Type->Type == SuperWeaponType::ChronoSphere)
			pSuperChronosphere = pSuper;

		if (!pSuperChronowarp && pSuper->Type->Type == SuperWeaponType::ChronoWarp)
			pSuperChronowarp = pSuper;

		if (pSuperChronosphere && pSuperChronowarp)
			break;
	}

	//const auto& [curAct, curArgs] = pTeam->CurrentScript->GetCurrentAction();
	//const auto& [nextAct, nextArgs] = pTeam->CurrentScript->GetNextAction();

	if (!pSuperChronosphere || !pSuperChronowarp)
	{
		pTeam->StepCompleted = true;
		return;
	}

	if (!pSuperChronosphere->IsCharged || (pSuperChronosphere->IsPowered() && !pOwner->Is_Powered()))
	{
		if (pSuperChronosphere->Granted)
		{
			int rechargeTime = pSuperChronosphere->GetRechargeTime();
			int timeLeft = pSuperChronosphere->RechargeTimer.GetTimeLeft();

			if ((1.0 - RulesClass::Instance->AIMinorSuperReadyPercent) < ((double)timeLeft / rechargeTime))
			{
				return;
			}
		}
		else
		{
			pTeam->StepCompleted = true;
			return;
		}
	}

	auto pTargetCell = MapClass::Instance->TryGetCellAt(pTarget->GetCoords());

	if (pTargetCell)
	{
		auto coords = pTeam->Zone->GetCoords();
		pOwner->Fire_SW(pSuperChronosphere->Type->ArrayIndex, CellClass::Coord2Cell(coords));
		pOwner->Fire_SW(pSuperChronowarp->Type->ArrayIndex, pTargetCell->MapCoords);
		pTeam->AssignMissionTarget(pTargetCell);
	}

	pTeam->StepCompleted = true;
	return;
}

void ScriptExtData::ForceGlobalOnlyTargetHouseEnemy(TeamClass* pTeam, int mode = -1)
{
	if (!pTeam->CurrentScript)
	{
		pTeam->StepCompleted = true;
		return;
	}

	//auto pHouseExt = HouseExtContainer::Instance.Find(pTeam->Owner);
	const auto& [curAct, curArgs] = pTeam->CurrentScript->GetCurrentAction();

	if (mode < 0 || mode > 2)
		mode = curArgs;

	if (mode < -1 || mode > 2)
		mode = -1;

	HouseExtData::ForceOnlyTargetHouseEnemy(pTeam->OwnerHouse, mode);

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExtData::JumpBackToPreviousScript(TeamClass* pTeam)
{
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);
	if (pTeamData->PreviousScript)
	{
		GameDelete<true, false>(pTeam->CurrentScript);
		pTeam->CurrentScript = GameCreate<ScriptClass>(pTeamData->PreviousScript);
		pTeam->CurrentScript->CurrentMission = -1;
		pTeam->StepCompleted = true;
		return;
	}
	else
	{
		auto pScript = pTeam->CurrentScript;
		auto const& [curAct, curArgs] = pScript->GetCurrentAction();

		Debug::LogInfo("DEBUG: [{}] [{}](line: {} = {},{}): Can't find the previous script! This script action must be used after PickRandomScript.",
			pTeam->Type->ID,
			pScript->Type->ID,
			pScript->CurrentMission,
			(int)curAct,
			curArgs
		);

		pTeam->StepCompleted = true;
		return;
	}
}

void ScriptExtData::SetAbortActionAfterSuccessKill(TeamClass* pTeam, int enable = -1)
{
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);
	int scriptArgument = enable;

	if (scriptArgument < 0)
	{
		auto pScript = pTeam->CurrentScript;
		const auto& [curAct, curArgs] = pScript->GetCurrentAction();
		scriptArgument = curArgs;
	}

	if (scriptArgument >= 1)
		pTeamData->AbortActionAfterKilling = true;
	else
		pTeamData->AbortActionAfterKilling = false;

	// This action finished
	pTeam->StepCompleted = true;
}

bool ScriptExtData::IsUnitAvailable(TechnoClass* pTechno, bool checkIfInTransportOrAbsorbed)
{
	if (!pTechno || !pTechno->Owner)
		return false;

	bool isAvailable = pTechno->IsAlive && pTechno->Health > 0 && !pTechno->InLimbo && pTechno->IsOnMap;

	if (checkIfInTransportOrAbsorbed)
		isAvailable &= !pTechno->Absorbed && !pTechno->Transporter;

	return isAvailable;
}

std::pair<WeaponTypeClass*, WeaponTypeClass*> ScriptExtData::GetWeapon(TechnoClass* pTechno)
{
	if (!pTechno)
		return { nullptr , nullptr };

	return { TechnoExtData::GetCurrentWeapon(pTechno, false),TechnoExtData::GetCurrentWeapon(pTechno, true) };
}

void ScriptExtData::RepairDestroyedBridge(TeamClass* pTeam, int mode = -1)
{
	if (!pTeam)
		return;

	auto const pTeamData = TeamExtContainer::Instance.Find(pTeam);
	if (!pTeamData)
		return;

	auto pScript = pTeam->CurrentScript;
	auto const& [curAction, curArgument] = pTeam->CurrentScript->GetCurrentAction();
	auto const& [nextAction, nextArgument] = pTeam->CurrentScript->GetNextAction();

	// The first time this team runs this kind of script the repair huts list will updated. The only reason of why it isn't stored in ScenarioClass is because always exists the possibility of a modder to make destroyable Repair Huts
	if (pTeamData->BridgeRepairHuts.empty())
	{
		for (auto pBuilding : *BuildingClass::Array)
		{
			if (pBuilding->Type->BridgeRepairHut)
				pTeamData->BridgeRepairHuts.push_back(pBuilding);
		}

		if (pTeamData->BridgeRepairHuts.empty())
		{
			pTeam->StepCompleted = true;
			Debug::LogInfo("AI Scripts - [{}] [{}] (line: {} = {},{}) Jump to next line: {} = {},{} -> (Reason: No repair huts found)",
				pTeam->Type->ID,
				pScript->Type->ID,
				pScript->CurrentMission,
				(int)curAction,
				curArgument,
				pScript->CurrentMission + 1,
				(int)nextAction,
				nextArgument);

			return;
		}
	}

	// Reset Team's target if the current target isn't a repair hut
	if (pTeam->ArchiveTarget)
	{
		if (pTeam->ArchiveTarget->WhatAmI() != AbstractType::Building)
		{
			pTeam->ArchiveTarget = nullptr;
		}
		else
		{
			auto pBuilding = static_cast<BuildingClass*>(pTeam->ArchiveTarget);

			if (!pBuilding->Type->BridgeRepairHut)
			{
				pTeam->ArchiveTarget = nullptr;
			}
			else
			{
				CellStruct cell = pBuilding->GetCell()->MapCoords;

				// If the Bridge was repaired then the repair hut isn't valid anymore
				if (!MapClass::Instance->IsLinkedBridgeDestroyed(cell))
					pTeam->ArchiveTarget = nullptr;
			}
		}
	}

	TechnoClass* selectedTarget = pTeam->ArchiveTarget ? static_cast<TechnoClass*>(pTeam->ArchiveTarget) : nullptr;
	bool isEngineerAmphibious = false;
	StackVector<FootClass*, 512> engineers {};
	StackVector<FootClass*, 512> otherTeamMembers {};

	// Check if there are no engineers
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		if (!IsUnitAvailable(pUnit, true))
			continue;

		if (!pTeam->ArchiveTarget)
		{
			pUnit->SetTarget(nullptr);
			pUnit->SetDestination(nullptr, false);
			pUnit->ForceMission(Mission::Guard);
		}

		if (pUnit->WhatAmI() == AbstractType::Infantry)
		{
			auto pInf = static_cast<InfantryClass*>(pUnit);

			if (pInf->IsEngineer())
			{
				if (pInf->Type->MovementZone == MovementZone::Amphibious
				|| pInf->Type->MovementZone == MovementZone::AmphibiousCrusher
				|| pInf->Type->MovementZone == MovementZone::AmphibiousDestroyer)
				{
					isEngineerAmphibious = true;
				}

				engineers->push_back(pUnit);

				continue;
			}
		}

		// Non-engineers will receive a different command
		otherTeamMembers->push_back(pUnit);
	}

	if (engineers->empty())
	{
		pTeam->StepCompleted = true;
		Debug::LogInfo("AI Scripts - [{}] [{}] (line: {} = {},{}) Jump to next line: {} = {},{} -> (Reason: Team has no engineers)",
			pTeam->Type->ID,
			pScript->Type->ID,
			pScript->CurrentMission,
			(int)curAction,
			curArgument,
			pScript->CurrentMission + 1,
			(int)nextAction,
			nextArgument);

		return;
	}

	StackVector<BuildingClass*, 10> validHuts {};

	if (!selectedTarget)
	{
		for (auto pTechno : pTeamData->BridgeRepairHuts)
		{
			CellStruct cell = pTechno->GetCell()->MapCoords;

			// Skip all huts linked to non-destroyed bridges
			if (!MapClass::Instance->IsLinkedBridgeDestroyed(cell))
				continue;

			if (isEngineerAmphibious)
			{
				validHuts->push_back(pTechno);
			}
			else
			{
				auto coords = pTechno->GetCenterCoords();

				// Only huts reachable by the (first) engineer are valid
				if (engineers[0]->IsInSameZone(&coords))
					validHuts->push_back(pTechno);
			}
		}

		// Find the best repair hut
		int bestVal = -1;

		//auto hut = pTechno;

		if (mode < 0)
			mode = curArgument;

		if (mode < 0)
		{
			// Pick a random bridge
			selectedTarget = validHuts[ScenarioClass::Instance->Random.RandomFromMax(validHuts->size() - 1)];
		}
		else
		{
			for (auto& pHut : validHuts.container())
			{
				int value = engineers[0]->DistanceFrom(pHut); // Note: distance is in leptons (*256)

				if (mode > 0)
				{
					// Pick the farthest target
					if (value >= bestVal || bestVal < 0)
					{
						bestVal = value;
						selectedTarget = pHut;
					}
				}
				else
				{
					// Pick the closest target
					if (value < bestVal || bestVal < 0)
					{
						bestVal = value;
						selectedTarget = pHut;
					}
				}
			}
		}
	}

	if (!selectedTarget)
	{
		pTeam->StepCompleted = true;

		//Debug::LogInfo("AI Scripts - [{}] [{}] (line: {} = {},{}) Jump to next line: {} = {},{} -> (Reason: Can not select a bridge repair hut)",
		//	pTeam->Type->ID,
		//	pScript->Type->ID,
		//	pScript->CurrentMission,
		//	(int)curAction,
		//	curArgument,
		//	pScript->CurrentMission + 1,
		//	(int)nextAction,
		//	nextArgument);

		return;
	}

	// Setting the team's target & mission
	pTeam->ArchiveTarget = selectedTarget;

	for (auto engineer : engineers.container())
	{
		if (engineer->Destination != selectedTarget)
		{
			engineer->SetTarget(selectedTarget);
			engineer->QueueMission(Mission::Capture, true);
		}
	}

	if (!otherTeamMembers->empty())
	{
		double closeEnough = 0.0; // Note: this value is in leptons (*256)
		if (pTeamData->CloseEnough > 0)
			closeEnough = pTeamData->CloseEnough * 256.0;
		else
			closeEnough = RulesClass::Instance->CloseEnough.value;

		for (auto pFoot : otherTeamMembers.container())
		{
			if (!pFoot->Destination
				|| (selectedTarget->DistanceFrom(pFoot->Destination) > closeEnough))
			{
				// Reset previous command
				pFoot->SetTarget(nullptr);
				pFoot->SetDestination(nullptr, false);
				pFoot->ForceMission(Mission::Guard);

				// Get a cell near the target
				pFoot->QueueMission(Mission::Move, false);
				CoordStruct coord = TechnoExtData::PassengerKickOutLocation(selectedTarget, pFoot);
				CellClass* pCellDestination = MapClass::Instance->TryGetCellAt(coord);
				pFoot->SetDestination(pCellDestination, true);
			}

			// Reached destination, stay in guard until next action
			if (pFoot->DistanceFrom(pFoot->Destination) < closeEnough)
				pFoot->QueueMission(Mission::Area_Guard, false);

			pFoot->NextMission();
		}
	}
}

void ScriptExtData::SimpleDeployerDeploy(TeamClass* pTeam, int mode)
{
	// mode: -1 = use argument, 0 = deploy, 1 = undeploy
	if (!pTeam)
	{
		Debug::Log("[SimpleDeployerDeploy] Error: pTeam is null\n");
		return;
	}

	auto pScript = pTeam->CurrentScript;
	if (!pScript)
	{
		Debug::Log("[SimpleDeployerDeploy] Error: pScript is null\n");
		pTeam->StepCompleted = true;
		return;
	}

	// Static optimization caches - declare at top of function
	static std::map<UnitClass*, std::pair<CellClass*, int>> cellSearchCache;
	static std::map<UnitClass*, int> unloadTimeoutCounter;
	static int frameCounter = 0;
	static int lastLogFrame = 0;
	frameCounter++;

	const auto& [curAct, scriptArgument] = pScript->GetCurrentAction();
	if (mode < 0)
		mode = scriptArgument;

	// Reduce logging frequency to avoid spam
	bool shouldLog = (frameCounter - lastLogFrame) > 30; // Log every second

	if (shouldLog)
	{
		lastLogFrame = frameCounter;
		Debug::Log("[SimpleDeployerDeploy] Team: %s, Mode: %d (Frame %d)\n",
			pTeam->Type ? pTeam->Type->get_ID() : "Unknown", mode, frameCounter);
	}

	// Clean up dead team members first
	FootClass* pCur = pTeam->FirstUnit;
	while (pCur)
	{
		FootClass* pNext = pCur->NextTeamMember;
		if (!ScriptExtData::IsUnitAvailable(pCur, false))
		{
			pTeam->RemoveMember(pCur, -1, 1);
		}
		pCur = pNext;
	}

	// Check if team has any units left
	if (!pTeam->FirstUnit)
	{
		Debug::Log("[SimpleDeployerDeploy] Error: No units in team\n");
		pTeam->StepCompleted = true;
		return;
	}

	bool allUnitsProcessed = true;
	int unitsFound = 0;
	int simpleDeployerUnits = 0;

	// Process each unit in the team
	for (auto pUnit = pTeam->FirstUnit; pUnit; pUnit = pUnit->NextTeamMember)
	{
		unitsFound++;

		if (!ScriptExtData::IsUnitAvailable(pUnit, false))
			continue;

		auto pSimpleUnit = cast_to<UnitClass*, false>(pUnit);
		if (!pSimpleUnit || !pSimpleUnit->Type->IsSimpleDeployer)
			continue;

		simpleDeployerUnits++;
#ifdef _DEBUG
		Debug::Log("[SimpleDeployerDeploy] Processing SimpleDeployer unit %d (%s), Deployed: %d\n",
			simpleDeployerUnits, pSimpleUnit->Type->get_ID(), pSimpleUnit->Deployed);
#endif

		bool shouldDeploy = false;
		bool shouldUndeploy = false;

		// Determine action based on mode
		switch (mode)
		{
		case 0: // Deploy
		default:
			if (!pSimpleUnit->Deployed)
				shouldDeploy = true;
			break;
		case 1: // Undeploy
			if (pSimpleUnit->Deployed)
				shouldUndeploy = true;
			break;
		}

		// Check if unit is already in deployment process
		if (pSimpleUnit->Deploying || pSimpleUnit->DeployAnim)
		{
			// Unit is busy, wait for it to complete
			allUnitsProcessed = false;
			continue;
		}

		// Special handling for Mission::Unload completion issues with timeout
		if (pSimpleUnit->CurrentMission == Mission::Unload)
		{
			// Initialize timeout counter if not exists
			if (unloadTimeoutCounter.find(pSimpleUnit) == unloadTimeoutCounter.end())
			{
				unloadTimeoutCounter[pSimpleUnit] = frameCounter;
			}

			// Check if Mission::Unload should complete for SimpleDeployer
			bool shouldComplete = (!pSimpleUnit->Deploying && !pSimpleUnit->DeployAnim);
			bool timedOut = (frameCounter - unloadTimeoutCounter[pSimpleUnit]) > 150; // ~5 seconds timeout

			if (shouldComplete || timedOut)
			{
				if (timedOut)
				{
					Debug::Log("[SimpleDeployerDeploy] Mission::Unload timeout for %s, forcing completion\n", pSimpleUnit->Type->get_ID());
				}
				else
				{
					Debug::Log("[SimpleDeployerDeploy] Force completing Mission::Unload for %s\n", pSimpleUnit->Type->get_ID());
				}
				pSimpleUnit->ForceMission(Mission::Guard);
				unloadTimeoutCounter.erase(pSimpleUnit); // Clean up timeout counter
			}
			else
			{
				// Still in unload process, wait
				allUnitsProcessed = false;
				continue;
			}
		}
		else
		{
			// Clean up timeout counter if unit is not in Mission::Unload
			unloadTimeoutCounter.erase(pSimpleUnit);
		}

		// Execute deployment/undeployment action
		if (shouldDeploy)
		{
			// Additional terrain checks before deployment
			bool canDeploy = pSimpleUnit->CanDeploySlashUnload();
			bool terrainSuitable = true;

			if (canDeploy)
			{
				auto pCell = pSimpleUnit->GetCell();
				if (pCell)
				{
					// Check terrain suitability for deployment
					bool cellClear = pCell->Cell_Occupier() == nullptr || pCell->Cell_Occupier() == pSimpleUnit;
					bool flatGround = pCell->GetLevel() == 0; // Level 0 = flat ground
					bool passableTerrain = pCell->CanThisExistHere(pSimpleUnit->Type->SpeedType, nullptr, pSimpleUnit->Owner);
					bool noBridge = !pCell->ContainsBridge();
					bool notRampCliff = !pCell->Tile_Is_Ramp() && !pCell->Tile_Is_Cliff();

					terrainSuitable = cellClear && flatGround && passableTerrain && noBridge && notRampCliff;

					Debug::Log("[SimpleDeployerDeploy] Terrain check for %s: Clear=%d, Flat=%d, Passable=%d, NoBridge=%d, NotRamp=%d\n",
						pSimpleUnit->Type->get_ID(), cellClear ? 1 : 0, flatGround ? 1 : 0,
						passableTerrain ? 1 : 0, noBridge ? 1 : 0, notRampCliff ? 1 : 0);
				}
			}

			Debug::Log("[SimpleDeployerDeploy] Unit %s shouldDeploy=true, canDeploy=%d, terrainSuitable=%d, deployed=%d\n",
				pSimpleUnit->Type->get_ID(), canDeploy ? 1 : 0, terrainSuitable ? 1 : 0, pSimpleUnit->Deployed ? 1 : 0);

			if (canDeploy && terrainSuitable)
			{
				// Try direct deployment first
				if (pSimpleUnit->TryToDeploy())
				{
					Debug::Log("[SimpleDeployerDeploy] TryToDeploy succeeded for %s\n", pSimpleUnit->Type->get_ID());
					allUnitsProcessed = false; // Wait for deployment animation
				}
				else
				{
					// Fallback to Mission::Unload (working method)
					Debug::Log("[SimpleDeployerDeploy] TryToDeploy failed, using Mission::Unload fallback for %s\n", pSimpleUnit->Type->get_ID());
					pSimpleUnit->QueueMission(Mission::Unload, false);
					pSimpleUnit->NextMission();
					allUnitsProcessed = false; // Wait for deployment to complete
				}
			}
			else if (canDeploy && !terrainSuitable)
			{
				// Check cache first to avoid expensive repeated searches
				CellClass* pBestCell = nullptr;
				auto currentCell = pSimpleUnit->GetMapCoords();

				// Check if we have a cached result that's still valid
				auto cacheIt = cellSearchCache.find(pSimpleUnit);
				if (cacheIt != cellSearchCache.end())
				{
					auto& [cachedCell, cacheFrame] = cacheIt->second;
					// Use cache if it's less than 30 frames old (~1 second)
					if (frameCounter - cacheFrame < 30 && cachedCell)
					{
						// Validate cached cell is still suitable
						bool cellClear = cachedCell->Cell_Occupier() == nullptr;
						if (cellClear)
						{
							pBestCell = cachedCell;
							Debug::Log("[SimpleDeployerDeploy] Using cached suitable cell for %s\n", pSimpleUnit->Type->get_ID());
						}
						else
						{
							// Cache invalid, remove it
							cellSearchCache.erase(cacheIt);
						}
					}
					else
					{
						// Cache expired, remove it
						cellSearchCache.erase(cacheIt);
					}
				}

				// If no valid cache, perform search
				if (!pBestCell)
				{
					Debug::Log("[SimpleDeployerDeploy] Current terrain not suitable, searching nearby cells for %s\n", pSimpleUnit->Type->get_ID());

					// Search in 3x3 area around current position
					for (int dx = -1; dx <= 1 && !pBestCell; dx++)
					{
						for (int dy = -1; dy <= 1 && !pBestCell; dy++)
						{
							if (dx == 0 && dy == 0) continue; // Skip current cell (already checked)

							CellStruct targetCell = { short(currentCell.X + dx), short(currentCell.Y + dy) };
							if (!MapClass::Instance->IsValidCell(targetCell)) continue;

							auto pNearbyCell = MapClass::Instance->GetCellAt(targetCell);
							if (!pNearbyCell) continue;

							// Check if nearby cell is suitable
							bool cellClear = pNearbyCell->Cell_Occupier() == nullptr;
							bool flatGround = pNearbyCell->GetLevel() == 0;
							bool passableTerrain = pNearbyCell->CanThisExistHere(pSimpleUnit->Type->SpeedType, nullptr, pSimpleUnit->Owner);
							bool noBridge = !pNearbyCell->ContainsBridge();
							bool notRampCliff = !pNearbyCell->Tile_Is_Ramp() && !pNearbyCell->Tile_Is_Cliff();

							if (cellClear && flatGround && passableTerrain && noBridge && notRampCliff)
							{
								pBestCell = pNearbyCell;
								// Cache the result
								cellSearchCache[pSimpleUnit] = std::make_pair(pBestCell, frameCounter);
								Debug::Log("[SimpleDeployerDeploy] Found suitable cell at [%d,%d] for %s\n",
									targetCell.X, targetCell.Y, pSimpleUnit->Type->get_ID());
							}
						}
					}
				}

				if (pBestCell)
				{
					// Move to suitable cell first, then deploy
					Debug::Log("[SimpleDeployerDeploy] Moving %s to suitable cell for deployment\n", pSimpleUnit->Type->get_ID());
					pSimpleUnit->SetDestination(pBestCell, true);
					pSimpleUnit->QueueMission(Mission::Move, false);
					pSimpleUnit->NextMission();
					allUnitsProcessed = false; // Wait for movement to complete
				}
				else
				{
					// No suitable cells found - force deploy at current location as fallback
					Debug::Log("[SimpleDeployerDeploy] No suitable cells found, forcing deployment at current location for %s\n", pSimpleUnit->Type->get_ID());
					pSimpleUnit->QueueMission(Mission::Unload, false);
					pSimpleUnit->NextMission();
					allUnitsProcessed = false; // Wait for deployment to complete
				}
			}
			else
			{
				Debug::Log("[SimpleDeployerDeploy] Cannot deploy %s (CanDeploySlashUnload=false)\n", pSimpleUnit->Type->get_ID());
			}
		}
		else if (shouldUndeploy)
		{
			// Try to undeploy using proper game method
			Debug::Log("[SimpleDeployerDeploy] Unit %s shouldUndeploy=true, deployed=%d\n",
				pSimpleUnit->Type->get_ID(), pSimpleUnit->Deployed ? 1 : 0);

			if (pSimpleUnit->Deployed)
			{
				pSimpleUnit->Undeploy();
				allUnitsProcessed = false; // Wait for undeployment to complete
			}
		}
		else
		{
			Debug::Log("[SimpleDeployerDeploy] Unit %s no action (deployed=%d, mode=%d)\n",
				pSimpleUnit->Type->get_ID(), pSimpleUnit->Deployed ? 1 : 0, mode);
		}
	}

	// Clean up cache entries for dead/invalid units every 300 frames (~10 seconds)
	if (frameCounter % 300 == 0)
	{
		for (auto it = cellSearchCache.begin(); it != cellSearchCache.end();)
		{
			if (!ScriptExtData::IsUnitAvailable(it->first, false))
			{
				it = cellSearchCache.erase(it);
			}
			else
			{
				++it;
			}
		}

		for (auto it = unloadTimeoutCounter.begin(); it != unloadTimeoutCounter.end();)
		{
			if (!ScriptExtData::IsUnitAvailable(it->first, false))
			{
				it = unloadTimeoutCounter.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// Script action completes when all units have finished their deployment actions
	pTeam->StepCompleted = allUnitsProcessed;
}

//
//ASMJIT_PATCH(0x6913F8, ScriptClass_CTOR, 0x5)
//{
//	GET(ScriptClass* const, pThis, ESI);
//	ScriptExtData::ExtMap.FindOrAllocate(pThis);
//	return 0x0;
//}
//
//ASMJIT_PATCH_AGAIN(0x691F06, ScriptClass_DTOR, 0x6)
//ASMJIT_PATCH(0x691486, ScriptClass_DTOR, 0x6)
//{
//	GET(ScriptClass*, pThis, ESI);
//	ScriptExtData::ExtMap.Remove(pThis);
//	return 0x0;
//}
//
//
//ASMJIT_PATCH_AGAIN(0x691690, ScriptClass_SaveLoad_Prefix, 0x8)
//ASMJIT_PATCH(0x691630, ScriptClass_SaveLoad_Prefix, 0x5)
//{
//	GET_STACK(ScriptClass*, pItem, 0x4);
//	GET_STACK(IStream*, pStm, 0x8);
//	ScriptExtData::ExtMap.PrepareStream(pItem, pStm);
//	return 0;
//}
//
//ASMJIT_PATCH(0x69166F, ScriptClass_Load_Suffix, 0x9)
//{
//	GET(ScriptClass*, pThis, ESI);
//
//	SwizzleManagerClass::Instance->Swizzle((void**)&pThis->Type);
//	TeamExtContainer::Instance.LoadStatic();
//
//	return 0x69167D;
//}
//
//ASMJIT_PATCH(0x6916A4, ScriptClass_Save_Suffix, 0x6)
//{
//	GET(HRESULT const, nRes, EAX);
//
//	if (SUCCEEDED(nRes))
//	{
//		TeamExtContainer::Instance.SaveStatic();
//		return 0x6916A8;
//	}
//
//	return 0x6916AA;
//}