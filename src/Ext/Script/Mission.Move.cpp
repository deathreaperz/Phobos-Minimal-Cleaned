#include "Body.h"

#include <Ext/Rules/Body.h>
#include <Ext/Team/Body.h>
#include <Ext/Techno/Body.h>

// Contains ScriptExtData::Mission_Move and its helper functions.

void ScriptExtData::Mission_Move(TeamClass* pTeam, DistanceMode calcThreatMode, bool pickAllies = false, int attackAITargetType = -1, int idxAITargetTypeItem = -1)
{

	if (!pTeam->CurrentScript)
	{
		pTeam->StepCompleted = true;
		return;
	}

	auto pScript = pTeam->CurrentScript;
	auto const& [act, scriptArgument] = pScript->GetCurrentAction();
	auto const& [nextAct, nextArg] = pScript->GetNextAction();
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);

	if (!pTeamData)
	{
		pTeam->StepCompleted = true;
		ScriptExtData::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d -> (Reason: ExtData found)\n",
							pTeam->Type->ID, pScript->CurrentMission,
							act, scriptArgument, pScript->Type->ID,
							pScript->CurrentMission + 1, nextAct, nextArg);
		return;
	}

	if (pTeamData->WaitNoTargetTimer.InProgress())
	{
		return;
	}

	pTeamData->WaitNoTargetTimer.Stop();
	pTeamData->WaitNoTargetAttempts = (pTeamData->WaitNoTargetAttempts > 0) ? pTeamData->WaitNoTargetAttempts - 1 : 0;

	bool bAircraftsWithoutAmmo = false;

	for (auto pCur = pTeam->FirstUnit; pCur; pCur = pCur->NextTeamMember)
	{
		if (!ScriptExtData::IsUnitAvailable(pCur, false))
		{
			pTeam->RemoveMember(pCur, -1, 1);
		}
		else
		{
			auto pTechnoType = pCur->GetTechnoType();
			if (pCur->WhatAmI() == AbstractType::Aircraft &&
				!pCur->IsInAir() &&
				static_cast<AircraftTypeClass*>(pTechnoType)->AirportBound &&
				pCur->Ammo < pTechnoType->Ammo)
			{
				bAircraftsWithoutAmmo = true;
			}
		}
	}

	if (!pTeamData->TeamLeader)
	{
		pTeamData->TeamLeader = ScriptExtData::FindTheTeamLeader(pTeam);
	}

	if (!pTeamData->TeamLeader || bAircraftsWithoutAmmo)
	{
		// Reset team data
		pTeamData->IdxSelectedObjectFromAIList = -1;
		pTeamData->CloseEnough = -1;
		pTeamData->WaitNoTargetTimer.Stop();
		pTeamData->WaitNoTargetAttempts = 0;

		pTeam->StepCompleted = true;
		ScriptExtData::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d -> (Reasons: No Leader | Aircrafts without ammo)\n",
							pTeam->Type->ID, pScript->Type->ID,
							pScript->CurrentMission, act, scriptArgument,
							pScript->CurrentMission + 1, nextAct, nextArg);
		return;
	}

	TechnoTypeClass* pLeaderUnitType = pTeamData->TeamLeader->GetTechnoType();
	TechnoClass* pFocus = abstract_cast<TechnoClass*>(pTeam->Focus);

	if (!pFocus && !bAircraftsWithoutAmmo)
	{
		// Your logic for selecting a new target
	}
	else
	{
		// Update "Move" mission for each team unit
		if (ScriptExtData::MoveMissionEndStatus(pTeam, pFocus, pTeamData->TeamLeader, pTeamData->MoveMissionEndMode))
		{
			pTeamData->MoveMissionEndMode = 0;
			pTeamData->IdxSelectedObjectFromAIList = -1;
			pTeamData->CloseEnough = -1;

			pTeam->StepCompleted = true;
			ScriptExtData::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d (Reason: Reached destination)\n",
								pTeam->Type->ID, pScript->Type->ID,
								pScript->CurrentMission, act, scriptArgument,
								pScript->CurrentMission + 1, nextAct, nextArg);
		}
	}
}

TechnoClass* ScriptExtData::FindBestObject(TechnoClass* pTechno, int method, DistanceMode calcThreatMode, bool pickAllies, int attackAITargetType, int idxAITargetTypeItem)
{
	TechnoClass* bestObject = nullptr;
	double bestVal = -1.0;
	HouseClass* enemyHouse = nullptr;
	auto pTechnoType = pTechno->GetTechnoType();

	if (!pickAllies && pTechno->BelongsToATeam())
	{
		if (auto pFoot = abstract_cast<FootClass*>(pTechno))
		{
			const int enemyHouseIndex = pFoot->Team->FirstUnit->Owner->EnemyHouseIndex;
			const auto pHouseExt = HouseExtContainer::Instance.Find(pFoot->Team->Owner);
			const bool onlyTargetHouseEnemy = pHouseExt->ForceOnlyTargetHouseEnemyMode != -1 ? pFoot->Team->Type->OnlyTargetHouseEnemy : pHouseExt->m_ForceOnlyTargetHouseEnemy;

			if (onlyTargetHouseEnemy && enemyHouseIndex >= 0)
				enemyHouse = HouseClass::Array->Items[enemyHouseIndex];
		}
	}

	TechnoClass::Array->for_each([&](TechnoClass* pObj)
	{
		if (!ScriptExtData::IsUnitAvailable(pObj, true) || pObj == pTechno || (enemyHouse && enemyHouse != pObj->Owner) || pObj->InWhichLayer() == Layer::Underground)
			return;

		if (auto objectType = pObj->GetTechnoType())
		{
			if (pObj->CloakState == CloakState::Cloaked && !objectType->Naval)
				return;

			if (pObj->CloakState == CloakState::Cloaked && objectType->Underwater &&
				(pTechnoType->NavalTargeting == NavalTargetingType::Underwater_never || pTechnoType->NavalTargeting == NavalTargetingType::Naval_none))
			{
				return;
			}

			if (objectType->Naval && pTechnoType->LandTargeting == LandTargetingType::Land_not_okay && pObj->GetCell()->LandType != LandType::Water)
			{
				return;
			}

			if ((pickAllies && pTechno->Owner->IsAlliedWith(pObj)) || (!pickAllies && !pTechno->Owner->IsAlliedWith(pObj)))
			{
				double value = 0.0;

				if (ScriptExtData::EvaluateObjectWithMask(pObj, method, attackAITargetType, idxAITargetTypeItem, pTechno))
				{
					if (calcThreatMode == DistanceMode::idkZero || calcThreatMode == DistanceMode::idkOne)
					{
						double threatMultiplier = 128.0;
						double objectThreatValue = pObj->GetThreatValue();

						if (objectType->SpecialThreatValue > 0)
						{
							objectThreatValue += objectType->SpecialThreatValue * RulesClass::Instance->TargetSpecialThreatCoefficientDefault;
						}

						if (pTechno->Owner == HouseClass::Array->Items[pObj->Owner->EnemyHouseIndex])
						{
							objectThreatValue += RulesClass::Instance->EnemyHouseThreatBonus;
						}

						objectThreatValue += pObj->Health * (1 - pObj->GetHealthPercentage());
						value = (objectThreatValue * threatMultiplier) / ((pTechno->DistanceFrom(pObj) / 256.0) + 1.0);

						if ((calcThreatMode == DistanceMode::idkZero && (value > bestVal || bestVal < 0)) ||
							(calcThreatMode == DistanceMode::idkOne && (value < bestVal || bestVal < 0)))
						{
							bestObject = pObj;
							bestVal = value;
						}
					}
					else
					{
						double distance = pTechno->DistanceFrom(pObj) / 256.0;

						if ((calcThreatMode == DistanceMode::Closest && (distance < bestVal || bestVal < 0)) ||
							(calcThreatMode == DistanceMode::Furtherst && (distance > bestVal || bestVal < 0)))
						{
							bestObject = pObj;
							bestVal = distance;
						}
					}
				}
			}
		}
	});

	return bestObject;
}

void ScriptExtData::Mission_Move_List(TeamClass* pTeam, DistanceMode calcThreatMode, bool pickAllies, int attackAITargetType)
{
	TeamExtData* pTeamExt = TeamExtContainer::Instance.Find(pTeam);
	pTeamExt->IdxSelectedObjectFromAIList = -1;

	const auto& [curAct, curArg] = pTeam->CurrentScript->GetCurrentAction();

	if (attackAITargetType < 0)
		attackAITargetType = curArg;

	const auto& Arr = RulesExtData::Instance()->AITargetTypesLists;
	if (static_cast<size_t>(attackAITargetType) < Arr.size() && !Arr[attackAITargetType].empty())
	{
		ScriptExtData::Mission_Move(pTeam, calcThreatMode, pickAllies, attackAITargetType, -1);
		return;
	}

	pTeam->StepCompleted = true;
	ScriptExtData::Log("AI Scripts - Mission_Move_List: [%s] [%s] (line: %d = %d,%d) Failed to get the list index [AITargetTypes][%d]! out of bound: %d\n",
		pTeam->Type->ID,
		pTeam->CurrentScript->Type->ID,
		pTeam->CurrentScript->CurrentMission,
		curAct,
		curArg,
		attackAITargetType,
		Arr.size());
}

static std::vector<int> Mission_Move_List1Random_validIndexes;

void ScriptExtData::Mission_Move_List1Random(TeamClass* pTeam, DistanceMode calcThreatMode, bool pickAllies, int attackAITargetType, int idxAITargetTypeItem = -1)
{
	auto pScript = pTeam->CurrentScript;
	Mission_Move_List1Random_validIndexes.clear();
	auto pTeamData = TeamExtContainer::Instance.Find(pTeam);
	const auto& [curAct, curArg] = pScript->GetCurrentAction();

	if (attackAITargetType < 0)
	{
		attackAITargetType = curArg;
	}

	if ((size_t)attackAITargetType < RulesExtData::Instance()->AITargetTypesLists.size())
	{
		const auto& aitargetTypesList = RulesExtData::Instance()->AITargetTypesLists[attackAITargetType];

		if ((size_t)pTeamData->IdxSelectedObjectFromAIList < aitargetTypesList.size())
		{
			ScriptExtData::Mission_Move(pTeam, calcThreatMode, pickAllies, attackAITargetType, pTeamData->IdxSelectedObjectFromAIList);
			return;
		}

		// Still no random target selected
		if (!aitargetTypesList.empty())
		{
			TechnoClass::Array->for_each([&](TechnoClass* pTechno)
			{
				if (!ScriptExtData::IsUnitAvailable(pTechno, true))
				{
					return;
				}

				auto pTechnoType = pTechno->GetTechnoType();
				bool found = false;

				for (auto j = 0u; j < aitargetTypesList.size() && !found; j++)
				{
					if (pTechnoType == aitargetTypesList[j] &&
						((pickAllies && pTeam->FirstUnit->Owner->IsAlliedWith(pTechno)) ||
							(!pickAllies && !pTeam->FirstUnit->Owner->IsAlliedWith(pTechno))))
					{
						Mission_Move_List1Random_validIndexes.push_back(j);
						found = true;
					}
				}
			});

			if (!Mission_Move_List1Random_validIndexes.empty())
			{
				const int idxsel = Mission_Move_List1Random_validIndexes[ScenarioClass::Instance->Random.RandomFromMax(Mission_Move_List1Random_validIndexes.size() - 1)];
				pTeamData->IdxSelectedObjectFromAIList = idxsel;
				ScriptExtData::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Picked a random Techno from the list index [AITargetTypes][%d][%d] = %s\n",
					pTeam->Type->ID,
					pTeam->CurrentScript->Type->ID,
					pScript->CurrentMission,
					curAct,
					curArg,
					attackAITargetType, idxsel,
					RulesExtData::Instance()->AITargetTypesLists[attackAITargetType][idxsel]->ID);

				ScriptExtData::Mission_Move(pTeam, calcThreatMode, pickAllies, attackAITargetType, idxsel);

				return;
			}
		}
	}

	// This action finished
	pTeam->StepCompleted = true;
	ScriptExtData::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Failed to pick a random Techno from the list index [AITargetTypes][%d]! Valid Technos in the list: %d\n",
		pTeam->Type->ID,
		pTeam->CurrentScript->Type->ID,
		pScript->CurrentMission,
		curAct,
		curArg,
		attackAITargetType,
		Mission_Move_List1Random_validIndexes.size());
}