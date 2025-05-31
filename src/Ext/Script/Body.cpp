#include "Body.h"

#include <Utilities/Cast.h>
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
//#include <ExtraHeaders/StackVector.h>

/*
*	Scripts is a part of `TeamClass` that executed sequentally form `ScriptTypeClass`
*	Each script contains function that behave as it programmed
*/

ScriptActionNode NOINLINE ScriptExtData::GetSpecificAction(ScriptClass* pScript, int nIdx)
{
	if (nIdx == -1 || nIdx >= pScript->Type->ActionsCount)
		return { -1 , 0 };

	return pScript->Type->ScriptActions[nIdx];
}

// =============================
// container
ScriptExtContainer ScriptExtContainer::Instance;

#define stringify( name ) #name

static NOINLINE const char* ToStrings(PhobosScripts from)
{
	switch (from)
	{
#define PHOBOS_SCRIPT_CASE(name) case PhobosScripts::name: return #name;
		PHOBOS_SCRIPT_CASE(TimedAreaGuard)
		PHOBOS_SCRIPT_CASE(LoadIntoTransports)
		PHOBOS_SCRIPT_CASE(WaitUntilFullAmmo)
		PHOBOS_SCRIPT_CASE(RepeatAttackCloserThreat)
		PHOBOS_SCRIPT_CASE(RepeatAttackFartherThreat)
		PHOBOS_SCRIPT_CASE(RepeatAttackCloser)
		PHOBOS_SCRIPT_CASE(RepeatAttackFarther)
		PHOBOS_SCRIPT_CASE(SingleAttackCloserThreat)
		PHOBOS_SCRIPT_CASE(SingleAttackFartherThreat)
		PHOBOS_SCRIPT_CASE(SingleAttackCloser)
		PHOBOS_SCRIPT_CASE(SingleAttackFarther)
		PHOBOS_SCRIPT_CASE(DecreaseCurrentAITriggerWeight)
		PHOBOS_SCRIPT_CASE(IncreaseCurrentAITriggerWeight)
		PHOBOS_SCRIPT_CASE(RepeatAttackTypeCloserThreat)
		PHOBOS_SCRIPT_CASE(RepeatAttackTypeFartherThreat)
		PHOBOS_SCRIPT_CASE(RepeatAttackTypeCloser)
		PHOBOS_SCRIPT_CASE(RepeatAttackTypeFarther)
		PHOBOS_SCRIPT_CASE(SingleAttackTypeCloserThreat)
		PHOBOS_SCRIPT_CASE(SingleAttackTypeFartherThreat)
		PHOBOS_SCRIPT_CASE(SingleAttackTypeCloser)
		PHOBOS_SCRIPT_CASE(SingleAttackTypeFarther)
		PHOBOS_SCRIPT_CASE(WaitIfNoTarget)
		PHOBOS_SCRIPT_CASE(TeamWeightReward)
		PHOBOS_SCRIPT_CASE(PickRandomScript)
		PHOBOS_SCRIPT_CASE(MoveToEnemyCloser)
		PHOBOS_SCRIPT_CASE(MoveToEnemyFarther)
		PHOBOS_SCRIPT_CASE(MoveToFriendlyCloser)
		PHOBOS_SCRIPT_CASE(MoveToFriendlyFarther)
		PHOBOS_SCRIPT_CASE(MoveToTypeEnemyCloser)
		PHOBOS_SCRIPT_CASE(MoveToTypeEnemyFarther)
		PHOBOS_SCRIPT_CASE(MoveToTypeFriendlyCloser)
		PHOBOS_SCRIPT_CASE(MoveToTypeFriendlyFarther)
		PHOBOS_SCRIPT_CASE(ModifyTargetDistance)
		PHOBOS_SCRIPT_CASE(RandomAttackTypeCloser)
		PHOBOS_SCRIPT_CASE(RandomAttackTypeFarther)
		PHOBOS_SCRIPT_CASE(RandomMoveToTypeEnemyCloser)
		PHOBOS_SCRIPT_CASE(RandomMoveToTypeEnemyFarther)
		PHOBOS_SCRIPT_CASE(RandomMoveToTypeFriendlyCloser)
		PHOBOS_SCRIPT_CASE(RandomMoveToTypeFriendlyFarther)
		PHOBOS_SCRIPT_CASE(SetMoveMissionEndMode)
		PHOBOS_SCRIPT_CASE(UnregisterGreatSuccess)
		PHOBOS_SCRIPT_CASE(GatherAroundLeader)
		PHOBOS_SCRIPT_CASE(RandomSkipNextAction)
		PHOBOS_SCRIPT_CASE(ChangeTeamGroup)
		PHOBOS_SCRIPT_CASE(DistributedLoading)
		PHOBOS_SCRIPT_CASE(FollowFriendlyByGroup)
		PHOBOS_SCRIPT_CASE(RallyUnitWithSameGroup)
		PHOBOS_SCRIPT_CASE(StopForceJumpCountdown)
		PHOBOS_SCRIPT_CASE(NextLineForceJumpCountdown)
		PHOBOS_SCRIPT_CASE(SameLineForceJumpCountdown)
		PHOBOS_SCRIPT_CASE(ForceGlobalOnlyTargetHouseEnemy)
		PHOBOS_SCRIPT_CASE(OverrideOnlyTargetHouseEnemy)
		PHOBOS_SCRIPT_CASE(SetHouseAngerModifier)
		PHOBOS_SCRIPT_CASE(ModifyHateHouseIndex)
		PHOBOS_SCRIPT_CASE(ModifyHateHousesList)
		PHOBOS_SCRIPT_CASE(ModifyHateHousesList1Random)
		PHOBOS_SCRIPT_CASE(SetTheMostHatedHouseMinorNoRandom)
		PHOBOS_SCRIPT_CASE(SetTheMostHatedHouseMajorNoRandom)
		PHOBOS_SCRIPT_CASE(SetTheMostHatedHouseRandom)
		PHOBOS_SCRIPT_CASE(ResetAngerAgainstHouses)
		PHOBOS_SCRIPT_CASE(AggroHouse)
		PHOBOS_SCRIPT_CASE(SetSideIdxForManagingTriggers)
		PHOBOS_SCRIPT_CASE(SetHouseIdxForManagingTriggers)
		PHOBOS_SCRIPT_CASE(ManageAllAITriggers)
		PHOBOS_SCRIPT_CASE(EnableTriggersFromList)
		PHOBOS_SCRIPT_CASE(DisableTriggersFromList)
		PHOBOS_SCRIPT_CASE(DisableTriggersWithObjects)
		PHOBOS_SCRIPT_CASE(EnableTriggersWithObjects)
		PHOBOS_SCRIPT_CASE(ConditionalJumpResetVariables)
		PHOBOS_SCRIPT_CASE(ConditionalJumpManageResetIfJump)
		PHOBOS_SCRIPT_CASE(AbortActionAfterSuccessKill)
		PHOBOS_SCRIPT_CASE(ConditionalJumpManageKillsCounter)
		PHOBOS_SCRIPT_CASE(ConditionalJumpSetCounter)
		PHOBOS_SCRIPT_CASE(ConditionalJumpSetComparatorMode)
		PHOBOS_SCRIPT_CASE(ConditionalJumpSetComparatorValue)
		PHOBOS_SCRIPT_CASE(ConditionalJumpSetIndex)
		PHOBOS_SCRIPT_CASE(ConditionalJumpIfFalse)
		PHOBOS_SCRIPT_CASE(ConditionalJumpIfTrue)
		PHOBOS_SCRIPT_CASE(ConditionalJumpKillEvaluation)
		PHOBOS_SCRIPT_CASE(ConditionalJumpCheckCount)
		PHOBOS_SCRIPT_CASE(ConditionalJumpCheckAliveHumans)
		PHOBOS_SCRIPT_CASE(ConditionalJumpCheckObjects)
		PHOBOS_SCRIPT_CASE(ConditionalJumpCheckHumanIsMostHated)
		PHOBOS_SCRIPT_CASE(JumpBackToPreviousScript)
		PHOBOS_SCRIPT_CASE(RepairDestroyedBridge)
		PHOBOS_SCRIPT_CASE(ChronoshiftToEnemyBase)
		PHOBOS_SCRIPT_CASE(LocalVariableSet)
		PHOBOS_SCRIPT_CASE(LocalVariableAdd)
		PHOBOS_SCRIPT_CASE(LocalVariableMinus)
		PHOBOS_SCRIPT_CASE(LocalVariableMultiply)
		PHOBOS_SCRIPT_CASE(LocalVariableDivide)
		PHOBOS_SCRIPT_CASE(LocalVariableMod)
		PHOBOS_SCRIPT_CASE(LocalVariableLeftShift)
		PHOBOS_SCRIPT_CASE(LocalVariableRightShift)
		PHOBOS_SCRIPT_CASE(LocalVariableReverse)
		PHOBOS_SCRIPT_CASE(LocalVariableXor)
		PHOBOS_SCRIPT_CASE(LocalVariableOr)
		PHOBOS_SCRIPT_CASE(LocalVariableAnd)
		PHOBOS_SCRIPT_CASE(GlobalVariableSet)
		PHOBOS_SCRIPT_CASE(GlobalVariableAdd)
		PHOBOS_SCRIPT_CASE(GlobalVariableMinus)
		PHOBOS_SCRIPT_CASE(GlobalVariableMultiply)
		PHOBOS_SCRIPT_CASE(GlobalVariableDivide)
		PHOBOS_SCRIPT_CASE(GlobalVariableMod)
		PHOBOS_SCRIPT_CASE(GlobalVariableLeftShift)
		PHOBOS_SCRIPT_CASE(GlobalVariableRightShift)
		PHOBOS_SCRIPT_CASE(GlobalVariableReverse)
		PHOBOS_SCRIPT_CASE(GlobalVariableXor)
		PHOBOS_SCRIPT_CASE(GlobalVariableOr)
		PHOBOS_SCRIPT_CASE(GlobalVariableAnd)
		PHOBOS_SCRIPT_CASE(LocalVariableSetByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableAddByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableMinusByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableMultiplyByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableDivideByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableModByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableLeftShiftByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableRightShiftByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableReverseByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableXorByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableOrByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableAndByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableSetByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableAddByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableMinusByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableMultiplyByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableDivideByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableModByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableLeftShiftByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableRightShiftByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableReverseByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableXorByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableOrByLocal)
		PHOBOS_SCRIPT_CASE(GlobalVariableAndByLocal)
		PHOBOS_SCRIPT_CASE(LocalVariableSetByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableAddByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableMinusByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableMultiplyByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableDivideByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableModByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableLeftShiftByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableRightShiftByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableReverseByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableXorByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableOrByGlobal)
		PHOBOS_SCRIPT_CASE(LocalVariableAndByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableSetByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableAddByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableMinusByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableMultiplyByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableDivideByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableModByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableLeftShiftByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableRightShiftByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableReverseByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableXorByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableOrByGlobal)
		PHOBOS_SCRIPT_CASE(GlobalVariableAndByGlobal)
#undef PHOBOS_SCRIPT_CASE
	default:
		return GameStrings::NoneStr();
	}
}

#include "Lua/Wrapper.h"

bool ScriptExtData::ProcessScriptActions(TeamClass* pTeam)
{
	auto const& [action, argument] = pTeam->CurrentScript->GetCurrentAction();

	//Debug::LogInfo("[{} - {}] Executing[{} - {}] [{} - {}]",
	//pTeam->Owner->get_ID(),
	//pTeam->Owner,
	//pTeam->get_ID(),
	//pTeam, action ,
	//argument
	//);

	//only find stuffs on the range , reducing the load
	//if ((AresScripts)action >= AresScripts::count)
	{
		//Debug::LogInfo("[{} - {}] Executing[{} - {}] [{} ({}) - {}]",
		//	pTeam->Owner->get_ID(),
		//	pTeam->Owner,
		//	pTeam->get_ID(),
		//	pTeam, action ,
		//	ToStrings((PhobosScripts)action), argument
		//);
		int Action = (int)action;

		switch (PhobosScripts(Action))
		{
		case PhobosScripts::TimedAreaGuard:
		{
			ScriptExtData::ExecuteTimedAreaGuardAction(pTeam); //checked
			break;
		}
		case PhobosScripts::LoadIntoTransports:
		{
			ScriptExtData::LoadIntoTransports(pTeam); //fixed
			break;
		}
		case PhobosScripts::WaitUntilFullAmmo:
		{
			ScriptExtData::WaitUntilFullAmmoAction(pTeam); //
			break;
		}
		case PhobosScripts::DecreaseCurrentAITriggerWeight:
		{
			ScriptExtData::DecreaseCurrentTriggerWeight(pTeam, true, 0); //
			break;
		}
		case PhobosScripts::IncreaseCurrentAITriggerWeight:
		{
			ScriptExtData::IncreaseCurrentTriggerWeight(pTeam, true, 0);
			break;
		}
		case PhobosScripts::WaitIfNoTarget:
		{
			ScriptExtData::WaitIfNoTarget(pTeam, -1);
			break;
		}
		case PhobosScripts::TeamWeightReward:
		{
			ScriptExtData::TeamWeightReward(pTeam, -1);
			break;
		}
		case PhobosScripts::PickRandomScript:
		{
			ScriptExtData::PickRandomScript(pTeam, -1);
			break;
		}
		case PhobosScripts::ModifyTargetDistance:
		{
			// AISafeDistance equivalent for Mission_Move()
			ScriptExtData::SetCloseEnoughDistance(pTeam, -1);
			break;
		}
		case PhobosScripts::SetMoveMissionEndMode:
		{
			// Set the condition for ending the Mission_Move Actions.
			ScriptExtData::SetMoveMissionEndMode(pTeam, -1);
			break;
		}
		case PhobosScripts::UnregisterGreatSuccess:
		{
			// Un-register success for AITrigger weight adjustment (this is the opposite of 49,0)
			ScriptExtData::UnregisterGreatSuccess(pTeam);
			break;
		}
		case PhobosScripts::GatherAroundLeader:
		{
			ScriptExtData::Mission_Gather_NearTheLeader(pTeam, -1);
			break;
		}
		case PhobosScripts::RandomSkipNextAction:
		{
			ScriptExtData::SkipNextAction(pTeam, -1);
			break;
		}
		case PhobosScripts::ForceGlobalOnlyTargetHouseEnemy:
		{
			ScriptExtData::ForceGlobalOnlyTargetHouseEnemy(pTeam, -1);
			break;
		}
		case PhobosScripts::ChangeTeamGroup:
		{
			//	ScriptExtData::TeamMemberSetGroup(pTeam, argument);
			Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", (void*)pTeam, pTeam->get_ID(), (void*)pTeam->CurrentScript, pTeam->CurrentScript->get_ID(), (int)action);
			pTeam->StepCompleted = true;
			break;
		}
		//TODO
		case PhobosScripts::DistributedLoading:
		{
			//ScriptExtData::DistributedLoadOntoTransport(pTeam, argument); //which branch is this again ?
			Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", (void*)pTeam, pTeam->get_ID(), (void*)pTeam->CurrentScript, pTeam->CurrentScript->get_ID(), (int)action);
			pTeam->StepCompleted = true;
			break;
		}
		case PhobosScripts::FollowFriendlyByGroup:
		{
			//ScriptExtData::FollowFriendlyByGroup(pTeam, argument); //which branch is this again ?
			Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", (void*)pTeam, pTeam->get_ID(), (void*)pTeam->CurrentScript, pTeam->CurrentScript->get_ID(), (int)action);
			pTeam->StepCompleted = true;
			break;
		}
		case PhobosScripts::RallyUnitWithSameGroup:
		{
			//ScriptExtData::RallyUnitInMap(pTeam, argument); //which branch is this again ?
			Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", (void*)pTeam, pTeam->get_ID(), (void*)pTeam->CurrentScript, pTeam->CurrentScript->get_ID(), (int)action);
			pTeam->StepCompleted = true;
			break;
		}
		case PhobosScripts::AbortActionAfterSuccessKill:
		{
			ScriptExtData::SetAbortActionAfterSuccessKill(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::JumpBackToPreviousScript:
		{
			ScriptExtData::JumpBackToPreviousScript(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::RepairDestroyedBridge:
		{
			// Start Timed Jump that jumps to the same line when the countdown finish (in frames)
			ScriptExtData::RepairDestroyedBridge(pTeam, -1);  //which branch is this again ?
			Debug::LogInfo("Team[{} - {} , Script [{} - {}] Action [{}] - No AttachedFunction", (void*)pTeam, pTeam->get_ID(), (void*)pTeam->CurrentScript, pTeam->CurrentScript->get_ID(), (int)action);
			pTeam->StepCompleted = true;
			break;
		}
		case PhobosScripts::ChronoshiftToEnemyBase: //#1077
		{
			// Chronoshift to enemy base, argument is additional distance modifier
			ScriptExtData::ChronoshiftToEnemyBase(pTeam, argument);
			break;
		}

#pragma region Mission_Attack
		case PhobosScripts::RepeatAttackCloserThreat:
		{
			// Threats that are close have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::idkZero, -1, -1); //done
			break;
		}
		case PhobosScripts::RepeatAttackFartherThreat:
		{
			// Threats that are far have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::idkOne, -1, -1); //done
			break;
		}
		case PhobosScripts::RepeatAttackCloser:
		{
			// Closer targets from Team Leader have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::Closest, -1, -1); //done
			break;
		}
		case PhobosScripts::RepeatAttackFarther:
		{
			// Farther targets from Team Leader have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack(pTeam, true, DistanceMode::Furtherst, -1, -1); //done
			break;
		}
		case PhobosScripts::SingleAttackCloserThreat:
		{
			// Threats that are close have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::idkZero, -1, -1); //done
			break;
		}
		case PhobosScripts::SingleAttackFartherThreat:
		{
			// Threats that are far have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::idkOne, -1, -1); //done
			break;
		}
		case PhobosScripts::SingleAttackCloser:
		{
			// Closer targets from Team Leader have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::Closest, -1, -1); //done
			break;
		}
		case PhobosScripts::SingleAttackFarther:
		{
			// Farther targets from Team Leader have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack(pTeam, false, DistanceMode::Furtherst, -1, -1); //done
			break;
		}
#pragma endregion

#pragma region Mission_Attack_List
		case PhobosScripts::RepeatAttackTypeCloserThreat:
		{
			// Threats specific targets that are close have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::idkZero, -1);
			break;
		}
		case PhobosScripts::RepeatAttackTypeFartherThreat:
		{
			// Threats specific targets that are far have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::idkOne, -1);
			break;
		}
		case PhobosScripts::RepeatAttackTypeCloser:
		{
			// Closer specific targets targets from Team Leader have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::Closest, -1);
			break;
		}
		case PhobosScripts::RepeatAttackTypeFarther:
		{
			// Farther specific targets targets from Team Leader have more priority. Kill until no more targets.
			ScriptExtData::Mission_Attack_List(pTeam, true, DistanceMode::Furtherst, -1);
			break;
		}
		case PhobosScripts::SingleAttackTypeCloserThreat:
		{
			// Threats specific targets that are close have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::idkZero, -1);
			break;
		}
		case PhobosScripts::SingleAttackTypeFartherThreat:
		{
			// Threats specific targets that are far have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::idkOne, -1);
			break;
		}
		case PhobosScripts::SingleAttackTypeCloser:
		{
			// Closer specific targets from Team Leader have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::Closest, -1);
			break;
		}
		case PhobosScripts::SingleAttackTypeFarther:
		{
			// Farther specific targets from Team Leader have more priority. 1 kill only (good for xx=49,0 combos)
			ScriptExtData::Mission_Attack_List(pTeam, false, DistanceMode::Furtherst, -1);
			break;
		}
#pragma endregion

#pragma region Mission_Attack_List1Random
		case PhobosScripts::RandomAttackTypeCloser:
		{
			// Pick 1 closer random objective from specific list for attacking it
			ScriptExtData::Mission_Attack_List1Random(pTeam, true, DistanceMode::Closest, -1);
			break;
		}
		case PhobosScripts::RandomAttackTypeFarther:
		{
			// Pick 1 farther random objective from specific list for attacking it
			ScriptExtData::Mission_Attack_List1Random(pTeam, true, DistanceMode::Furtherst, -1);
			break;
		}
#pragma endregion

#pragma region Mission_Move
		case PhobosScripts::MoveToEnemyCloser:
		{
			// Move to the closest enemy target
			ScriptExtData::Mission_Move(pTeam, DistanceMode::Closest, false, -1, -1);
			break;
		}
		case PhobosScripts::MoveToEnemyFarther:
		{
			// Move to the farther enemy target
			ScriptExtData::Mission_Move(pTeam, DistanceMode::Furtherst, false, -1, -1);
			break;
		}
		case PhobosScripts::MoveToFriendlyCloser:
		{
			// Move to the closest friendly target
			ScriptExtData::Mission_Move(pTeam, DistanceMode::Closest, true, -1, -1);
			break;
		}
		case PhobosScripts::MoveToFriendlyFarther:
		{
			// Move to the farther friendly target
			ScriptExtData::Mission_Move(pTeam, DistanceMode::Furtherst, true, -1, -1);
			break;
		}
#pragma endregion

#pragma region Mission_Move_List
		case PhobosScripts::MoveToTypeEnemyCloser:
		{
			// Move to the closest specific enemy target
			ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Closest, false, -1);
			break;
		}
		case PhobosScripts::MoveToTypeEnemyFarther:
		{
			// Move to the farther specific enemy target
			ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Furtherst, false, -1);
			break;
		}
		case PhobosScripts::MoveToTypeFriendlyCloser:
		{
			// Move to the closest specific friendly target
			ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Closest, true, -1);
			break;
		}
		case PhobosScripts::MoveToTypeFriendlyFarther:
		{
			// Move to the farther specific friendly target
			ScriptExtData::Mission_Move_List(pTeam, DistanceMode::Furtherst, true, -1);
			break;
		}
#pragma endregion

#pragma region Mission_Move_List1Random
		case PhobosScripts::RandomMoveToTypeEnemyCloser:
		{
			// Pick 1 closer enemy random objective from specific list for moving to it
			ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Closest, false, -1, -1);
			break;
		}
		case PhobosScripts::RandomMoveToTypeEnemyFarther:
		{
			// Pick 1 farther enemy random objective from specific list for moving to it
			ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Furtherst, false, -1, -1);
			break;
		}
		case PhobosScripts::RandomMoveToTypeFriendlyCloser:
		{
			// Pick 1 closer friendly random objective from specific list for moving to it
			ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Closest, true, -1, -1);
			break;
		}
		case PhobosScripts::RandomMoveToTypeFriendlyFarther:
		{
			// Pick 1 farther friendly random objective from specific list for moving to it
			ScriptExtData::Mission_Move_List1Random(pTeam, DistanceMode::Furtherst, true, -1, -1);
			break;
		}
#pragma endregion

#pragma region Stop_ForceJump_Countdown
		case PhobosScripts::StopForceJumpCountdown:
		{
			// Stop Timed Jump
			ScriptExtData::Stop_ForceJump_Countdown(pTeam);
			break;
		}
		case PhobosScripts::NextLineForceJumpCountdown:
		{
			// Start Timed Jump that jumps to the next line when the countdown finish (in frames)
			ScriptExtData::Set_ForceJump_Countdown(pTeam, false, -1);
			break;
		}
		case PhobosScripts::SameLineForceJumpCountdown:
		{
			// Start Timed Jump that jumps to the same line when the countdown finish (in frames)
			ScriptExtData::Set_ForceJump_Countdown(pTeam, true, -1);
			break;
		}
#pragma endregion

#pragma region AngerNodes
		case PhobosScripts::SetHouseAngerModifier:
		{
			ScriptExtData::SetHouseAngerModifier(pTeam, 0); //which branch is this again ?
			break;
		}
		case PhobosScripts::OverrideOnlyTargetHouseEnemy:
		{
			ScriptExtData::OverrideOnlyTargetHouseEnemy(pTeam, -1); //which branch is this again ?
			break;
		}
		case PhobosScripts::ModifyHateHouseIndex:
		{
			ScriptExtData::ModifyHateHouse_Index(pTeam, -1); //which branch is this again ?
			break;
		}
		case PhobosScripts::ModifyHateHousesList:
		{
			ScriptExtData::ModifyHateHouses_List(pTeam, -1); //which branch is this again ?
			break;
		}
		case PhobosScripts::ModifyHateHousesList1Random:
		{
			ScriptExtData::ModifyHateHouses_List1Random(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::SetTheMostHatedHouseMinorNoRandom:
		{
			// <, no random
			ScriptExtData::SetTheMostHatedHouse(pTeam, 0, 0, false);  //which branch is this again ?
			break;
		}
		case PhobosScripts::SetTheMostHatedHouseMajorNoRandom:
		{
			// >, no random
			ScriptExtData::SetTheMostHatedHouse(pTeam, 0, 1, false);
			break;
		}
		case PhobosScripts::SetTheMostHatedHouseRandom:
		{
			// random
			ScriptExtData::SetTheMostHatedHouse(pTeam, 0, 0, true);
			break;
		}
		case PhobosScripts::ResetAngerAgainstHouses:
		{
			ScriptExtData::ResetAngerAgainstHouses(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::AggroHouse:
		{
			ScriptExtData::AggroHouse(pTeam, -1);  //which branch is this again ?
			break;
		}
#pragma endregion

#pragma region ConditionalJump //#599
		case PhobosScripts::ConditionalJumpSetCounter:
		{
			ScriptExtData::ConditionalJump_SetCounter(pTeam, -100000000);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpSetComparatorMode:
		{
			ScriptExtData::ConditionalJump_SetComparatorMode(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpSetComparatorValue:
		{
			ScriptExtData::ConditionalJump_SetComparatorValue(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpSetIndex:
		{
			ScriptExtData::ConditionalJump_SetIndex(pTeam, -1000000);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpResetVariables:
		{
			ScriptExtData::ConditionalJump_ResetVariables(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpIfFalse:
		{
			ScriptExtData::ConditionalJumpIfFalse(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpIfTrue:
		{
			ScriptExtData::ConditionalJumpIfTrue(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpManageKillsCounter:
		{
			ScriptExtData::ConditionalJump_ManageKillsCounter(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpCheckAliveHumans:
		{
			ScriptExtData::ConditionalJump_CheckAliveHumans(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpCheckHumanIsMostHated:
		{
			ScriptExtData::ConditionalJump_CheckHumanIsMostHated(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpKillEvaluation:
		{
			ScriptExtData::ConditionalJump_KillEvaluation(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpCheckObjects:
		{
			ScriptExtData::ConditionalJump_CheckObjects(pTeam);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpCheckCount:
		{
			ScriptExtData::ConditionalJump_CheckCount(pTeam, 0);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ConditionalJumpManageResetIfJump:
		{
			ScriptExtData::ConditionalJump_ManageResetIfJump(pTeam, -1);  //which branch is this again ?
			break;
		}
#pragma endregion

#pragma region ManagingTriggers
		case PhobosScripts::SetSideIdxForManagingTriggers:
		{
			ScriptExtData::SetSideIdxForManagingTriggers(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::SetHouseIdxForManagingTriggers:
		{
			ScriptExtData::SetHouseIdxForManagingTriggers(pTeam, 1000000);  //which branch is this again ?
			break;
		}
		case PhobosScripts::ManageAllAITriggers:
		{
			ScriptExtData::ManageAITriggers(pTeam, -1);  //which branch is this again ?
			break;
		}
		case PhobosScripts::EnableTriggersFromList:
		{
			ScriptExtData::ManageTriggersFromList(pTeam, -1, true);  //which branch is this again ?
			break;
		}
		case PhobosScripts::DisableTriggersFromList:
		{
			ScriptExtData::ManageTriggersFromList(pTeam, -1, false);  //which branch is this again ?
			break;
		}
		case PhobosScripts::EnableTriggersWithObjects:
		{
			ScriptExtData::ManageTriggersWithObjects(pTeam, -1, true);  //which branch is this again ?
			break;
		}
		case PhobosScripts::DisableTriggersWithObjects:
		{
			ScriptExtData::ManageTriggersWithObjects(pTeam, -1, false);  //which branch is this again ?
			break;
		}
#pragma endregion
		default:
			// Do nothing because or it is a wrong Action number or it is an Ares/YR action...
			if (IsExtVariableAction(Action))
			{
				VariablesHandler(pTeam, static_cast<PhobosScripts>(Action), argument);
				break;
			}

			//dont prematurely finish the `Script` ,...
			//bailout the script if the `Action` already -1
			//this will free the Member and allow them to be recuited
			if ((TeamMissionType)Action == TeamMissionType::none || (TeamMissionType)Action >= TeamMissionType::count && (AresScripts)action >= AresScripts::count)
			{
				// Unknown action. This action finished
				pTeam->StepCompleted = true;
				auto const pAction = pTeam->CurrentScript->GetCurrentAction();
				Debug::LogInfo("AI Scripts : [{}] Team [{}][{}]  ( {} CurrentScript {} / {} line {}): Unknown Script Action: {}",
					(void*)pTeam,
					pTeam->Type->ID,
					pTeam->Type->Name,

					(void*)pTeam->CurrentScript,
					pTeam->CurrentScript->Type->ID,
					pTeam->CurrentScript->Type->Name,
					pTeam->CurrentScript->CurrentMission,

					(int)pAction.Action);
			}

			return false;
		}
	}

	return true;
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

	if (pExt)
	{
		FootClass* pLeaderUnit = FindTheTeamLeader(pTeam);
		pExt->TeamLeader = pLeaderUnit;
	}

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
			pLeaderUnit = ScriptExtData::FindTheTeamLeader(pTeam);
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

			if (pNewScript->ActionsCount > 0)
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
	//	return;
	//}

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

void ScriptExtData::VariablesHandler(TeamClass* pTeam, PhobosScripts eAction, int nArg)
{
	struct operation_set { int operator()(const int& a, const int& b) { return b; } };
	struct operation_add { int operator()(const int& a, const int& b) { return a + b; } };
	struct operation_minus { int operator()(const int& a, const int& b) { return a - b; } };
	struct operation_multiply { int operator()(const int& a, const int& b) { return a * b; } };
	struct operation_divide { int operator()(const int& a, const int& b) { return a / b; } };
	struct operation_mod { int operator()(const int& a, const int& b) { return a % b; } };
	struct operation_leftshift { int operator()(const int& a, const int& b) { return a << b; } };
	struct operation_rightshift { int operator()(const int& a, const int& b) { return a >> b; } };
	struct operation_reverse { int operator()(const int& a, const int& b) { return ~a; } };
	struct operation_xor { int operator()(const int& a, const int& b) { return a ^ b; } };
	struct operation_or { int operator()(const int& a, const int& b) { return a | b; } };
	struct operation_and { int operator()(const int& a, const int& b) { return a & b; } };

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
	}
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

bool ScriptExtData::IsExtVariableAction(int action)
{
	auto eAction = static_cast<PhobosScripts>(action);
	return eAction >= PhobosScripts::LocalVariableAdd && eAction <= PhobosScripts::GlobalVariableAndByGlobal;
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
		pOwner->Fire_SW(pSuperChronosphere->Type->ArrayIndex, pTeam->SpawnCell->MapCoords);
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

	HouseExtData::ForceOnlyTargetHouseEnemy(pTeam->Owner, mode);

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