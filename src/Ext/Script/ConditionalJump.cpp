#include "Body.h"

#include <Ext/Team/Body.h>
#include <Ext/Rules/Body.h>

// 1-based like the original action '6,n' (so the first script line is n=1)
void ScriptExt::ConditionalJumpIfTrue(TeamClass* pTeam, int newScriptLine = -1)
{
	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = newScriptLine;

	if (scriptArgument < 1)
		scriptArgument = curArgs;

	// if by mistake you put as first line=0 this corrects it because for WW/EALA this script argument is 1-based
	if (scriptArgument < 1)
		scriptArgument = 1;

	if (pTeamData->ConditionalJump_Evaluation)
	{

		const auto&[prevAct , prevArgs] = ScriptExt::GetSpecificAction(pScript, scriptArgument - 1 );

		Debug::Log("DEBUG: [%s] [%s] %d = %d,%d - Conditional Jump was a success! - New Line: %d = %d,%d\n",
		pTeam->Type->ID,
		pScript->Type->ID,
		pScript->CurrentMission,
		curAct,
		curArgs,
		scriptArgument - 1,
		prevAct,
		prevArgs
		);

		// Start conditional jump!
		// This is magic: for example, for jumping into line 0 of the script list you have to point to the "-1" line so in the next AI iteration the current line will be increased by 1 and then it will point to the desired line 0
		pScript->CurrentMission = scriptArgument - 2;

		// Cleaning Conditional Jump related variables
		if (pTeamData->ConditionalJump_ResetVariablesIfJump)
			ScriptExt::ConditionalJump_ResetVariables(pTeam);
	}

	// This action finished
	pTeam->StepCompleted = true;
}

// 1-based like the original action '6,n' (so the first script line is n=1)
void ScriptExt::ConditionalJumpIfFalse(TeamClass* pTeam, int newScriptLine = -1)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = newScriptLine;

	if (scriptArgument < 1)
		scriptArgument = curArgs;

	// if by mistake you put as first line=0 this corrects it because for WW/EALA this script argument is 1-based
	if (scriptArgument < 1)
		scriptArgument = 1;

	if (!pTeamData->ConditionalJump_Evaluation)
	{
		const auto&[prevAct , prevArgs] = ScriptExt::GetSpecificAction(pScript, scriptArgument - 1 );

		Debug::Log("DEBUG: [%s] [%s] %d = %d,%d - Conditional Jump was a success! - New Line: %d = %d,%d\n",
		pTeam->Type->ID,
		pScript->Type->ID,
		pScript->CurrentMission,
		curAct,
		curArgs,
		scriptArgument - 1,
		prevAct,
		prevArgs
		);

		// Start conditional jump!
		// This is magic: for example, for jumping into line 0 of the script list you have to point to the "-1" line so in the next AI iteration the current line will be increased by 1 and then it will point to the desired line 0
		pScript->CurrentMission = scriptArgument - 2;

		// Cleaning Conditional Jump related variables
		if (pTeamData->ConditionalJump_ResetVariablesIfJump)
			ScriptExt::ConditionalJump_ResetVariables(pTeam);
	}

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExt::ConditionalJump_KillEvaluation(TeamClass* pTeam)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);

	if (!pTeamData->ConditionalJump_EnabledKillsCount) {
		pTeam->StepCompleted = true;
		return;
	}

	if (pTeamData->ConditionalJump_Counter < 0)
		pTeamData->ConditionalJump_Counter = 0;

	int counter = pTeamData->ConditionalJump_Counter;
	int comparator = pTeamData->ConditionalJump_ComparatorValue;
	pTeamData->ConditionalJump_Evaluation = ScriptExt::ConditionalJump_MakeEvaluation(pTeamData->ConditionalJump_ComparatorMode, counter, comparator);

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExt::ConditionalJump_ManageKillsCounter(TeamClass* pTeam, int enable = -1)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = enable;

	if (scriptArgument < 0 || scriptArgument > 1)
		scriptArgument = curArgs;

	if (scriptArgument <= 0)
		scriptArgument = 0;
	else
		scriptArgument = 1;

	if (scriptArgument <= 0)
		pTeamData->ConditionalJump_EnabledKillsCount = false;
	else
		pTeamData->ConditionalJump_EnabledKillsCount = true;

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExt::ConditionalJump_SetIndex(TeamClass* pTeam, int index = -1000000)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = index;

	if (scriptArgument == -1000000)
		scriptArgument = curArgs;

	pTeamData->ConditionalJump_Index = scriptArgument;
	pTeam->StepCompleted = true;

}

void ScriptExt::ConditionalJump_SetComparatorValue(TeamClass* pTeam, int value = -1)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = value;

	if (scriptArgument < 0)
		scriptArgument = curArgs;

	pTeamData->ConditionalJump_ComparatorValue = scriptArgument;
	pTeam->StepCompleted = true;
}

// Possible values are 3:">=" -> 0:"<", 1:"<=", 2:"==", 3:">=", 4:">", 5:"!="
void ScriptExt::ConditionalJump_SetComparatorMode(TeamClass* pTeam, int value = -1)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();
	int scriptArgument = value;

	if (scriptArgument < 0 || scriptArgument > 5)
		scriptArgument = curArgs;

	if (scriptArgument < 0 || scriptArgument > 5)
		scriptArgument = 3; // >=

	pTeamData->ConditionalJump_ComparatorMode = scriptArgument;
	pTeam->StepCompleted = true;

}

void ScriptExt::ConditionalJump_SetCounter(TeamClass* pTeam, int value = -100000000)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();

	if (value == -100000000)
		value = curArgs;

	pTeamData->ConditionalJump_Counter = value;
	pTeam->StepCompleted = true;

}

void ScriptExt::ConditionalJump_ResetVariables(TeamClass* pTeam)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	// Cleaning Conditional Jump related variables
	pTeamData->ConditionalJump_Evaluation = false;
	pTeamData->ConditionalJump_ComparatorMode = 3; // >=
	pTeamData->ConditionalJump_ComparatorValue = 1;
	pTeamData->ConditionalJump_EnabledKillsCount = false;
	pTeamData->ConditionalJump_Counter = 0;
	pTeamData->AbortActionAfterKilling = false;
	pTeamData->ConditionalJump_Index = -1000000;

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExt::ConditionalJump_ManageResetIfJump(TeamClass* pTeam, int enable = -1)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();

	if (enable < 0)
		enable = curArgs;

	if (enable > 0)
		pTeamData->ConditionalJump_ResetVariablesIfJump = true;
	else
		pTeamData->ConditionalJump_ResetVariablesIfJump = false;

	// This action finished
	pTeam->StepCompleted = true;
}

// Count objects from [AITargetTypes] lists
void ScriptExt::ConditionalJump_CheckObjects(TeamClass* pTeam)
{
	

	long countValue = 0;
	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	int index = pTeamData->ConditionalJump_Index;
	const auto& targetTypeList = RulesExt::Global()->AITargetTypesLists;

	if (index >= 0 && !targetTypeList.empty() && (size_t)index < targetTypeList.size())
	{
		const auto& objectsList = targetTypeList[index];

		if (objectsList.empty()) {
			pTeam->StepCompleted = true;
			return;
		}

		for (auto pTechno : *TechnoClass::Array)
		{
			if(!ScriptExt::IsUnitAvailable(pTechno , true))
				continue;

			if(!pTeam->FirstUnit->Owner->IsAlliedWith(pTechno) ||
				ScriptExt::IsUnitMindControlledFriendly(pTeam->FirstUnit->Owner, pTechno))
			{
				for (size_t i = 0; i < objectsList.size(); i++)
				{
					if (objectsList[i] == pTechno->GetTechnoType()
						//TeamExt::GroupAllowed(objectsList[i] , pTechno->GetTechnoType())
						) {
						countValue++;
						break;
					}
				}
			}
		}

		int comparatorValue = pTeamData->ConditionalJump_ComparatorValue;
		pTeamData->ConditionalJump_Evaluation = ScriptExt::ConditionalJump_MakeEvaluation(pTeamData->ConditionalJump_ComparatorMode, countValue, comparatorValue);
	}

	// This action finished
	pTeam->StepCompleted = true;
}

// A simple counter. The count can be increased or decreased
void ScriptExt::ConditionalJump_CheckCount(TeamClass* pTeam, int modifier = 0)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto pScript = pTeam->CurrentScript;
	const auto&[curAct , curArgs] = pScript->GetCurrentAction();

	if (modifier == 0)
		modifier = curArgs;

	if (modifier == 0)
	{
		// This action finished
		pTeam->StepCompleted = true;
		return;
	}

	pTeamData->ConditionalJump_Counter += modifier;
	int currentCount = pTeamData->ConditionalJump_Counter;
	int comparatorValue = pTeamData->ConditionalJump_ComparatorValue;
	pTeamData->ConditionalJump_Evaluation = ScriptExt::ConditionalJump_MakeEvaluation(pTeamData->ConditionalJump_ComparatorMode, currentCount, comparatorValue);

	// This action finished
	pTeam->StepCompleted = true;
}

bool ScriptExt::ConditionalJump_MakeEvaluation(int comparatorMode, int studiedValue, int comparatorValue)
{
	

	int result = false;

	// Comparators are like in [AITriggerTypes] from aimd.ini
	switch (comparatorMode)
	{
		case 0:
			// <
			if (studiedValue < comparatorValue)
				result = true;
			break;
		case 1:
			// <=
			if (studiedValue <= comparatorValue)
				result = true;
			break;
		case 2:
			// ==
			if (studiedValue == comparatorValue)
				result = true;
			break;
		case 3:
			// >=
			if (studiedValue >= comparatorValue)
				result = true;
			break;
		case 4:
			// >
			if (studiedValue > comparatorValue)
				result = true;
			break;
		case 5:
			// !=
			if (studiedValue != comparatorValue)
				result = true;
			break;
		default:
			break;
	}

	return result;
}

void ScriptExt::ConditionalJump_CheckHumanIsMostHated(TeamClass* pTeam)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	HouseClass* pEnemyHouse = nullptr;
	auto pHouse = pTeam->Owner;
	int angerLevel = -1;
	bool isHumanHouse = false;

	for (const auto& pNode : pHouse->AngerNodes)
	{
		if (!pNode.House->Type->MultiplayPassive
			&& !pNode.House->Defeated
			&& !pNode.House->IsObserver()
			&& ((pNode.AngerLevel > angerLevel
					&& !pHouse->IsAlliedWith(pNode.House))
					|| angerLevel < 0))
		{
			angerLevel = pNode.AngerLevel;
			pEnemyHouse = pNode.House;
		}
	}

	if (pEnemyHouse && pEnemyHouse->IsControlledByHuman())
		isHumanHouse = true;

	pTeamData->ConditionalJump_Evaluation = isHumanHouse;

	// This action finished
	pTeam->StepCompleted = true;
}

void ScriptExt::ConditionalJump_CheckAliveHumans(TeamClass* pTeam, int mode = 0)
{
	

	auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	const auto&[curAct , curArgs] = pTeam->CurrentScript->GetCurrentAction();

	if (mode < 0 || mode > 2)
		mode = curArgs;

	if (mode < 0 || mode > 2)
		mode = 0;

	auto pHouse = pTeam->Owner;

	{
		pTeamData->ConditionalJump_Evaluation = false;

		// Find an alive human House
		for (const auto& pNode : pHouse->AngerNodes)
		{
			if (!pNode.House->Type->MultiplayPassive
				&& !pNode.House->Defeated
				&& !pNode.House->IsObserver()
				&& pNode.House->IsControlledByHuman())
			{
				if (mode == 1 && !pHouse->IsAlliedWith(pNode.House)) // Mode 1: Enemy humans
				{
					pTeamData->ConditionalJump_Evaluation = true;
					break;
				}
				else if (mode == 2 && !pHouse->IsAlliedWith(pNode.House)) // Mode 2: Friendly humans
				{
					pTeamData->ConditionalJump_Evaluation = true;
					break;
				}

				// mode 0: Any human
				pTeamData->ConditionalJump_Evaluation = true;
				break;
			}
		}

		// If we are looking for any human the own House should be checked
		if (mode == 0 && pHouse->IsControlledByHuman())
			pTeamData->ConditionalJump_Evaluation = true;
	}

	// This action finished
	pTeam->StepCompleted = true;
}