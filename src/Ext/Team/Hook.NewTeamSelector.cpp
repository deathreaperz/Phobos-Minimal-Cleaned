#include "Body.h"

#include <Ext/House/Body.h>
#include <Ext/HouseType/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>

#include <AITriggerTypeClass.h>

// TODO :
// - Optimization a lot of duplicate code ,..
// - Type convert probably not handled properly yet
// - Prereq checking use vanilla function instead

enum class TeamCategory
{
	None = 0, // No category. Should be default value
	Ground = 1,
	Air = 2,
	Naval = 3,
	Unclassified = 4
};

struct TriggerElementWeight
{
	double Weight { 0.0 };
	AITriggerTypeClass* Trigger { nullptr };
	TeamCategory Category { TeamCategory::None };

	//need to define a == operator so it can be used in array classes
	COMPILETIMEEVAL bool operator==(const TriggerElementWeight& other) const
	{
		return (Trigger == other.Trigger && Weight == other.Weight && Category == other.Category);
	}

	//unequality
	COMPILETIMEEVAL bool operator!=(const TriggerElementWeight& other) const
	{
		return (Trigger != other.Trigger || Weight != other.Weight || Category == other.Category);
	}

	COMPILETIMEEVAL bool operator<(const TriggerElementWeight& other) const
	{
		return (Weight < other.Weight);
	}

	COMPILETIMEEVAL bool operator<(const double other) const
	{
		return (Weight < other);
	}

	COMPILETIMEEVAL bool operator>(const TriggerElementWeight& other) const
	{
		return (Weight > other.Weight);
	}

	COMPILETIMEEVAL bool operator>(const double other) const
	{
		return (Weight > other);
	}

	COMPILETIMEEVAL bool operator==(const double other) const
	{
		return (Weight == other);
	}

	COMPILETIMEEVAL bool operator!=(const double other) const
	{
		return (Weight != other);
	}
};

COMPILETIMEEVAL bool IsUnitAvailable(TechnoClass* pTechno, bool checkIfInTransportOrAbsorbed)
{
	if (!pTechno)
		return false;

	bool isAvailable = pTechno->IsAlive && pTechno->Health > 0 && !pTechno->InLimbo && pTechno->IsOnMap;

	if (checkIfInTransportOrAbsorbed)
		isAvailable &= !pTechno->Absorbed && !pTechno->Transporter;

	return isAvailable;
}

COMPILETIMEEVAL bool IsValidTechno(TechnoClass* pTechno)
{
	if (!pTechno)
		return false;

	bool isValid = !pTechno->Dirty
		&& IsUnitAvailable(pTechno, true)
		&& pTechno->Owner
		&& (pTechno->WhatAmI() == AbstractType::Infantry
			|| pTechno->WhatAmI() == AbstractType::Unit
			|| pTechno->WhatAmI() == AbstractType::Building
			|| pTechno->WhatAmI() == AbstractType::Aircraft);

	return isValid;
}

enum class ComparatorOperandTypes
{
	LessThan, LessOrEqual, Equal, MoreOrEqual, More, NotSame
};

COMPILETIMEEVAL void ModifyOperand(bool& result, int counter, AITriggerConditionComparator& cond)
{
	switch ((ComparatorOperandTypes)cond.ComparatorOperand)
	{
	case ComparatorOperandTypes::LessThan:
		result = counter < cond.ComparatorType;
		break;
	case ComparatorOperandTypes::LessOrEqual:
		result = counter <= cond.ComparatorType;
		break;
	case ComparatorOperandTypes::Equal:
		result = counter == cond.ComparatorType;
		break;
	case ComparatorOperandTypes::MoreOrEqual:
		result = counter >= cond.ComparatorType;
		break;
	case ComparatorOperandTypes::More:
		result = counter > cond.ComparatorType;
		break;
	case ComparatorOperandTypes::NotSame:
		result = counter != cond.ComparatorType;
		break;
	default:
		break;
	}
}

bool OwnStuffs(TechnoTypeClass* pItem, TechnoClass* list)
{
	if (auto pItemUnit = cast_to<UnitTypeClass*, false>(pItem))
	{
		if (auto pListBld = cast_to<BuildingClass*, false>(list))
		{
			if (pItemUnit->DeploysInto == pListBld->Type)
				return true;

			if (pListBld->Type->UndeploysInto == pItemUnit)
				return true;
		}
	}

	if (auto pItemUnit = cast_to<BuildingTypeClass*, false>(pItem))
	{
		if (auto pListBld = cast_to<UnitClass*, false>(list))
		{
			if (pItemUnit->UndeploysInto == pListBld->Type)
				return true;

			if (pListBld->Type->DeploysInto == pItemUnit)
				return true;
		}
	}

	return TechnoExtContainer::Instance.Find(list)->Type == pItem || list->GetTechnoType() == pItem;
}

// Generalized helper for ownership checks

template <typename TypeList, typename OwnerPredicate>
NOINLINE bool CheckOwnershipCondition(
	AITriggerTypeClass* pThis,
	TypeList& typeList,
	OwnerPredicate ownerPredicate,
	bool useAndLogic = false)
{
	bool result = useAndLogic ? true : false;
	if (typeList.empty())
		return useAndLogic ? false : result;

	for (auto const& pItem : typeList)
	{
		int counter = 0;
		if (useAndLogic) result = true;
		for (auto const pObject : *TechnoClass::Array)
		{
			if (!IsValidTechno(pObject)) continue;
			if (ownerPredicate(pObject) && OwnStuffs(pItem, pObject))
			{
				counter++;
			}
		}
		ModifyOperand(result, counter, *pThis->Conditions);
		if (useAndLogic && !result) break;
	}
	return result;
}

// Overload for single TechnoTypeClass*
template <typename OwnerPredicate>
NOINLINE bool CheckOwnershipCondition(
	AITriggerTypeClass* pThis,
	TechnoTypeClass* pItem,
	OwnerPredicate ownerPredicate)
{
	bool result = false;
	int counter = 0;
	for (auto const pObject : *TechnoClass::Array)
	{
		if (!IsValidTechno(pObject)) continue;
		if (ownerPredicate(pObject) && OwnStuffs(pItem, pObject))
		{
			counter++;
		}
	}
	ModifyOperand(result, counter, *pThis->Conditions);
	return result;
}

// HouseOwns wrappers
NOINLINE bool HouseOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, bool allies, std::vector<TechnoTypeClass*>& list)
{
	auto ownerPredicate = [pHouse, allies](TechnoClass* pObject)
		{
			return ((!allies && pObject->Owner == pHouse) || (allies && pHouse != pObject->Owner && pHouse->IsAlliedWith(pObject->Owner)))
				&& !pObject->Owner->Type->MultiplayPassive;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, false);
}

NOINLINE bool HouseOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, bool allies, TechnoTypeClass* pItem)
{
	auto ownerPredicate = [pHouse, allies](TechnoClass* pObject)
		{
			return ((!allies && pObject->Owner == pHouse) || (allies && pHouse != pObject->Owner && pHouse->IsAlliedWith(pObject->Owner)))
				&& !pObject->Owner->Type->MultiplayPassive;
		};
	return CheckOwnershipCondition(pThis, pItem, ownerPredicate);
}

// EnemyOwns wrappers
NOINLINE bool EnemyOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, HouseClass* pEnemy, bool onlySelectedEnemy, TechnoTypeClass* pItem)
{
	if (pEnemy && pHouse->IsAlliedWith(pEnemy) && !onlySelectedEnemy)
		pEnemy = nullptr;
	auto ownerPredicate = [pHouse, pEnemy](TechnoClass* pObject)
		{
			return pObject->Owner != pHouse
				&& (!pEnemy || (pEnemy && !pHouse->IsAlliedWith(pEnemy)))
				&& !pObject->Owner->Type->MultiplayPassive;
		};
	return CheckOwnershipCondition(pThis, pItem, ownerPredicate);
}

NOINLINE bool EnemyOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, HouseClass* pEnemy, bool onlySelectedEnemy, std::vector<TechnoTypeClass*>& list)
{
	if (pEnemy && pHouse->IsAlliedWith(pEnemy) && !onlySelectedEnemy)
		pEnemy = nullptr;
	auto ownerPredicate = [pHouse, pEnemy](TechnoClass* pObject)
		{
			return pObject->Owner != pHouse
				&& (!pEnemy || (pEnemy && !pHouse->IsAlliedWith(pEnemy)))
				&& !pObject->Owner->Type->MultiplayPassive;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, false);
}

// NeutralOwns wrappers
NOINLINE bool NeutralOwns(AITriggerTypeClass* pThis, std::vector<TechnoTypeClass*>& list)
{
	auto pCiv = HouseExtData::FindFirstCivilianHouse();
	auto ownerPredicate = [pCiv](TechnoClass* pObject)
		{
			return pObject->Owner == pCiv;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, false);
}

NOINLINE bool NeutralOwns(AITriggerTypeClass* pThis, TechnoTypeClass* pItem)
{
	auto pCiv = HouseExtData::FindFirstCivilianHouse();
	auto ownerPredicate = [pCiv](TechnoClass* pObject)
		{
			return pObject->Owner == pCiv;
		};
	return CheckOwnershipCondition(pThis, pItem, ownerPredicate);
}

// HouseOwnsAll wrappers
NOINLINE bool HouseOwnsAll(AITriggerTypeClass* pThis, HouseClass* pHouse, std::vector<TechnoTypeClass*>& list)
{
	auto ownerPredicate = [pHouse](TechnoClass* pObject)
		{
			return pObject->Owner == pHouse;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, true);
}

// EnemyOwnsAll wrappers
NOINLINE bool EnemyOwnsAll(AITriggerTypeClass* pThis, HouseClass* pHouse, HouseClass* pEnemy, std::vector<TechnoTypeClass*>& list)
{
	if (pEnemy && pHouse->IsAlliedWith(pEnemy))
		pEnemy = nullptr;
	auto ownerPredicate = [pHouse, pEnemy](TechnoClass* pObject)
		{
			return pObject->Owner != pHouse
				&& (!pEnemy || (pEnemy && !pHouse->IsAlliedWith(pEnemy)))
				&& !pObject->Owner->Type->MultiplayPassive;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, true);
}

// NeutralOwnsAll wrappers
NOINLINE bool NeutralOwnsAll(AITriggerTypeClass* pThis, std::vector<TechnoTypeClass*>& list)
{
	auto pCiv = HouseExtData::FindFirstCivilianHouse();
	auto ownerPredicate = [pCiv](TechnoClass* pObject)
		{
			return pObject->Owner == pCiv;
		};
	return CheckOwnershipCondition(pThis, list, ownerPredicate, true);
}

NOINLINE bool CountConditionMet(AITriggerTypeClass* pThis, int nObjects)
{
	bool result = true;

	if (nObjects < 0)
		return false;

	ModifyOperand(result, nObjects, *pThis->Conditions);
	return result;
}

NOINLINE bool UpdateTeam(HouseClass* pHouse)
{
	if (!RulesExtData::Instance()->NewTeamsSelector)
		return false;

	auto pHouseTypeExt = HouseTypeExtContainer::Instance.Find(pHouse->Type);
	// Reset Team selection countdown
	pHouse->TeamDelayTimer.Start(RulesClass::Instance->TeamDelays[(int)pHouse->AIDifficulty]);

	// Use arrays indexed by TeamCategory for candidates and weights
	constexpr int CategoryCount = 5; // None, Ground, Air, Naval, Unclassified
	std::array<HelperedVector<TriggerElementWeight>, CategoryCount> validTriggerCandidatesByCategory;
	std::array<double, CategoryCount> totalWeightByCategory {};
	HelperedVector<TriggerElementWeight> validTriggerCandidates;
	double totalWeight = 0.0;

	int dice = ScenarioClass::Instance->Random.RandomRanged(1, 100);

	if (dice <= pHouse->RatioAITriggerTeam && pHouse->AITriggersActive)
	{
		int mergeUnclassifiedCategoryWith = -1;
		TeamCategory validCategory = TeamCategory::None;
		bool splitTriggersByCategory = RulesExtData::Instance()->NewTeamsSelector_SplitTriggersByCategory;
		bool isFallbackEnabled = RulesExtData::Instance()->NewTeamsSelector_EnableFallback;

		double percentageByCategory[CategoryCount] = { 0 };
		if (splitTriggersByCategory)
		{
			mergeUnclassifiedCategoryWith = pHouseTypeExt->NewTeamsSelector_MergeUnclassifiedCategoryWith.Get(RulesExtData::Instance()->NewTeamsSelector_MergeUnclassifiedCategoryWith);
			percentageByCategory[(int)TeamCategory::Unclassified] = pHouseTypeExt->NewTeamsSelector_UnclassifiedCategoryPercentage.Get(RulesExtData::Instance()->NewTeamsSelector_UnclassifiedCategoryPercentage);
			percentageByCategory[(int)TeamCategory::Ground] = pHouseTypeExt->NewTeamsSelector_GroundCategoryPercentage.Get(RulesExtData::Instance()->NewTeamsSelector_GroundCategoryPercentage);
			percentageByCategory[(int)TeamCategory::Naval] = pHouseTypeExt->NewTeamsSelector_NavalCategoryPercentage.Get(RulesExtData::Instance()->NewTeamsSelector_NavalCategoryPercentage);
			percentageByCategory[(int)TeamCategory::Air] = pHouseTypeExt->NewTeamsSelector_AirCategoryPercentage.Get(RulesExtData::Instance()->NewTeamsSelector_AirCategoryPercentage);

			// Merge mixed category with another category, if set
			if (mergeUnclassifiedCategoryWith >= 0)
			{
				percentageByCategory[mergeUnclassifiedCategoryWith] += percentageByCategory[(int)TeamCategory::Unclassified];
				percentageByCategory[(int)TeamCategory::Unclassified] = 0.0;
			}

			for (int i = 0; i < CategoryCount; ++i)
				if (percentageByCategory[i] < 0.0 || percentageByCategory[i] > 1.0) percentageByCategory[i] = 0.0;

			double totalPercentages = 0.0;
			for (int i = 0; i < CategoryCount; ++i) totalPercentages += percentageByCategory[i];
			if (totalPercentages > 1.0 || totalPercentages <= 0.0)
				splitTriggersByCategory = false;

			if (splitTriggersByCategory)
			{
				int categoryDice = ScenarioClass::Instance->Random.RandomRanged(1, 100);
				int acc = 0;
				for (int i = 0; i < CategoryCount; ++i)
				{
					if (percentageByCategory[i] > 0.0)
					{
						acc += (int)(percentageByCategory[i] * 100.0);
						if (categoryDice <= acc)
						{
							validCategory = (TeamCategory)i;
							break;
						}
					}
				}
				if (validCategory == TeamCategory::None) splitTriggersByCategory = false;
			}
		}

		int parentCountryTypeIdx = pHouse->Type->FindParentCountryIndex();
		int houseTypeIdx = parentCountryTypeIdx >= 0 ? parentCountryTypeIdx : pHouse->Type->ArrayIndex;
		int houseIdx = pHouse->ArrayIndex;
		int parentCountrySideTypeIdx = pHouse->Type->FindParentCountry()->SideIndex;
		int sideTypeIdx = parentCountrySideTypeIdx >= 0 ? parentCountrySideTypeIdx + 1 : pHouse->Type->SideIndex + 1;
		auto houseDifficulty = pHouse->AIDifficulty;
		int minBaseDefenseTeams = RulesClass::Instance->MinimumAIDefensiveTeams[(int)houseDifficulty];
		int maxBaseDefenseTeams = RulesClass::Instance->MaximumAIDefensiveTeams[(int)houseDifficulty];
		int activeDefenseTeamsCount = 0;
		int maxTeamsLimit = RulesClass::Instance->TotalAITeamCap[(int)houseDifficulty];

		HelperedVector<TeamClass*> activeTeamsList;
		for (auto const pRunningTeam : *TeamClass::Array)
		{
			if (pRunningTeam->Owner->ArrayIndex != houseIdx) continue;
			activeTeamsList.push_back(pRunningTeam);
			if (pRunningTeam->Type->IsBaseDefense) activeDefenseTeamsCount++;
		}

		int defensiveTeamsLimit = RulesClass::Instance->UseMinDefenseRule ? minBaseDefenseTeams : maxBaseDefenseTeams;
		bool hasReachedMaxTeamsLimit = (int)activeTeamsList.size() >= maxTeamsLimit;
		bool hasReachedMaxDefensiveTeamsLimit = activeDefenseTeamsCount >= defensiveTeamsLimit;

		bool onlyPickDefensiveTeams = false;
		int defensiveDice = ScenarioClass::Instance->Random.RandomRanged(0, 99);
		int defenseTeamSelectionThreshold = 50;
		if ((defensiveDice < defenseTeamSelectionThreshold) && !hasReachedMaxDefensiveTeamsLimit)
			onlyPickDefensiveTeams = true;
		if (hasReachedMaxDefensiveTeamsLimit)
			Debug::LogInfo("DEBUG: House [{}] (idx: {}) reached the MaximumAIDefensiveTeams value!", pHouse->Type->ID, pHouse->ArrayIndex);
		if (hasReachedMaxTeamsLimit)
		{
			Debug::LogInfo("DEBUG: House [{}] (idx: {}) reached the TotalAITeamCap value!", pHouse->Type->ID, pHouse->ArrayIndex);
			return true;
		}

		int destroyedBridgesCount = 0;
		int undamagedBridgesCount = 0;
		PhobosMap<TechnoTypeClass*, int> ownedRecruitables;
		for (auto const pTechno : *TechnoClass::Array)
		{
			if (!IsValidTechno(pTechno)) continue;
			if (pTechno->WhatAmI() == AbstractType::Building)
			{
				auto const pBuilding = static_cast<BuildingClass*>(pTechno);
				if (pBuilding && pBuilding->Type->BridgeRepairHut)
				{
					CellStruct cell = pTechno->GetCell()->MapCoords;
					if (MapClass::Instance->IsLinkedBridgeDestroyed(cell))
						destroyedBridgesCount++;
					else
						undamagedBridgesCount++;
				}
			}
			else
			{
				auto const pFoot = static_cast<FootClass*>(pTechno);
				bool allow = true;
				if (auto pContact = pFoot->GetRadioContact())
				{
					if (auto pBldC = cast_to<BuildingClass*, false>(pContact))
						if (pBldC->Type->Bunker) allow = false;
				}
				else if (auto pBld = pFoot->GetCell()->GetBuilding())
					if (pBld->Type->Bunker) allow = false;
				if (!allow || pTechno->IsSinking || pTechno->IsCrashing || !pTechno->IsAlive || pTechno->Health <= 0 || !pTechno->IsOnMap || pTechno->Transporter || pTechno->Absorbed || !pFoot->CanBeRecruited(pHouse))
					continue;
				++ownedRecruitables[pTechno->GetTechnoType()];
			}
		}

		HouseClass* targetHouse = nullptr;
		if (pHouse->EnemyHouseIndex >= 0)
			targetHouse = HouseClass::Array->GetItem(pHouse->EnemyHouseIndex);
		bool onlyCheckImportantTriggers = false;

		for (auto const pTrigger : *AITriggerTypeClass::Array)
		{
			if (!pTrigger || ScenarioClass::Instance->IgnoreGlobalAITriggers == pTrigger->IsGlobal || !pTrigger->Team1)
				continue;
			if (onlyPickDefensiveTeams && !pTrigger->IsForBaseDefense)
				continue;
			int triggerHouse = pTrigger->HouseIndex;
			int triggerSide = pTrigger->SideIndex;
			if (pTrigger->IsEnabled)
			{
				if (pTrigger->Team1->TechLevel > pHouse->StaticData.TechLevel)
					continue;
				if ((int)houseDifficulty == 0 && !pTrigger->Enabled_Hard
					|| (int)houseDifficulty == 1 && !pTrigger->Enabled_Normal
					|| (int)houseDifficulty == 2 && !pTrigger->Enabled_Easy)
					continue;
				if ((triggerHouse == -1 || houseTypeIdx == triggerHouse) && (triggerSide == 0 || sideTypeIdx == triggerSide))
				{
					if ((int)pTrigger->ConditionType >= 0)
					{
						switch ((int)pTrigger->ConditionType)
						{
						case 0: if (!pTrigger->ConditionObject || !EnemyOwns(pTrigger, pHouse, targetHouse, true, pTrigger->ConditionObject)) continue; break;
						case 1: if (!pTrigger->ConditionObject || !HouseOwns(pTrigger, pHouse, false, pTrigger->ConditionObject)) continue; break;
						case 7: if (!pTrigger->ConditionObject || !NeutralOwns(pTrigger, pTrigger->ConditionObject)) continue; break;
						case 8: if (!pTrigger->ConditionObject || !EnemyOwns(pTrigger, pHouse, nullptr, false, pTrigger->ConditionObject)) continue; break;
						case 9: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !EnemyOwns(pTrigger, pHouse, targetHouse, false, RulesExtData::Instance()->AITargetTypesLists[pTrigger->Conditions[3].ComparatorOperand])) continue; break;
						case 10: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !HouseOwns(pTrigger, pHouse, false, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 11: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !NeutralOwns(pTrigger, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 12: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !EnemyOwns(pTrigger, pHouse, nullptr, false, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 13: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !HouseOwns(pTrigger, pHouse, true, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 14: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !EnemyOwnsAll(pTrigger, pHouse, targetHouse, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 15: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !HouseOwnsAll(pTrigger, pHouse, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 16: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !NeutralOwnsAll(pTrigger, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 17: if ((size_t)pTrigger->Conditions[3].ComparatorOperand < RulesExtData::Instance()->AITargetTypesLists.size() && !EnemyOwnsAll(pTrigger, pHouse, nullptr, RulesExtData::Instance()->AITargetTypesLists[(pTrigger->Conditions[3].ComparatorOperand)])) continue; break;
						case 18: if (!CountConditionMet(pTrigger, destroyedBridgesCount)) continue; break;
						case 19: if (!CountConditionMet(pTrigger, undamagedBridgesCount)) continue; break;
						default: if (!pTrigger->ConditionMet(pHouse, targetHouse, hasReachedMaxDefensiveTeamsLimit)) continue; break;
						}
					}
					if (onlyCheckImportantTriggers && pTrigger->Weight_Current < 5000) continue;
					auto pTriggerTeam1Type = pTrigger->Team1;
					if (!pTriggerTeam1Type) continue;
					if (pTriggerTeam1Type->IsBaseDefense && hasReachedMaxDefensiveTeamsLimit) continue;
					int count = 0;
					for (auto team : activeTeamsList) if (team->Type == pTriggerTeam1Type) count++;
					if (count >= pTriggerTeam1Type->Max) continue;
					TeamCategory teamIsCategory = TeamCategory::None;
					if (splitTriggersByCategory)
					{
						for (int i = 0; i < 6; i++)
						{
							auto entry = pTriggerTeam1Type->TaskForce->Entries[i];
							TeamCategory entryIsCategory = TeamCategory::Ground;
							if (entry.Amount > 0)
							{
								if (!entry.Type) continue;
								if (entry.Type->WhatAmI() == AbstractType::AircraftType || entry.Type->ConsideredAircraft) entryIsCategory = TeamCategory::Air;
								else
								{
									auto pTechnoTypeExt = TechnoTypeExtContainer::Instance.Find(entry.Type);
									if (pTechnoTypeExt->ConsideredNaval || (entry.Type->Naval && (entry.Type->MovementZone != MovementZone::Amphibious && entry.Type->MovementZone != MovementZone::AmphibiousDestroyer && entry.Type->MovementZone != MovementZone::AmphibiousCrusher))) entryIsCategory = TeamCategory::Naval;
									if (pTechnoTypeExt->ConsideredVehicle || (entryIsCategory != TeamCategory::Naval && entryIsCategory != TeamCategory::Air)) entryIsCategory = TeamCategory::Ground;
								}
								teamIsCategory = teamIsCategory == TeamCategory::None || teamIsCategory == entryIsCategory ? entryIsCategory : TeamCategory::Unclassified;
								if (teamIsCategory == TeamCategory::Unclassified) break;
							}
							else break;
						}
						if (teamIsCategory == TeamCategory::Unclassified && mergeUnclassifiedCategoryWith >= 0) teamIsCategory = (TeamCategory)mergeUnclassifiedCategoryWith;
					}
					bool allObjectsCanBeBuiltOrRecruited = true;
					if (pTriggerTeam1Type->Autocreate)
					{
						for (const auto& entry : pTriggerTeam1Type->TaskForce->Entries)
						{
							if (entry.Amount > 0)
							{
								if (!entry.Type) continue;
								bool canBeBuilt = HouseExtData::PrerequisitesMet(pHouse, entry.Type);
								if (!canBeBuilt) { allObjectsCanBeBuiltOrRecruited = false; break; }
							}
							else break;
						}
					}
					else allObjectsCanBeBuiltOrRecruited = false;
					if (!allObjectsCanBeBuiltOrRecruited && pTriggerTeam1Type->Recruiter)
					{
						allObjectsCanBeBuiltOrRecruited = true;
						for (const auto& entry : pTriggerTeam1Type->TaskForce->Entries)
						{
							if (allObjectsCanBeBuiltOrRecruited && entry.Type && entry.Amount > 0)
							{
								auto iter = ownedRecruitables.get_key_iterator(entry.Type);
								if (iter != ownedRecruitables.end())
								{
									if ((iter->second) < entry.Amount) { allObjectsCanBeBuiltOrRecruited = false; break; }
								}
							}
						}
					}
					if (!allObjectsCanBeBuiltOrRecruited) continue;
					if (pTrigger->Weight_Current >= 5000 && !onlyCheckImportantTriggers)
					{
						if (validTriggerCandidates.size() > 0)
						{
							for (auto& v : validTriggerCandidatesByCategory) v.clear();
							validTriggerCandidates.clear();
							validCategory = TeamCategory::None;
						}
						onlyCheckImportantTriggers = true;
						totalWeight = 0.0;
						for (auto& w : totalWeightByCategory) w = 0.0;
						splitTriggersByCategory = false;
					}
					double weight = pTrigger->Weight_Current < 1.0 ? 1.0 : pTrigger->Weight_Current;
					totalWeight += weight;
					validTriggerCandidates.emplace_back(totalWeight, pTrigger, teamIsCategory);
					if (splitTriggersByCategory)
					{
						totalWeightByCategory[(int)teamIsCategory] += weight;
						validTriggerCandidatesByCategory[(int)teamIsCategory].emplace_back(totalWeightByCategory[(int)teamIsCategory], pTrigger, teamIsCategory);
					}
				}
			}
		}

		if (splitTriggersByCategory)
		{
			switch (validCategory)
			{
			case TeamCategory::Ground: Debug::LogInfo("DEBUG: This time only will be picked GROUND teams."); break;
			case TeamCategory::Unclassified: Debug::LogInfo("DEBUG: This time only will be picked MIXED teams."); break;
			case TeamCategory::Naval: Debug::LogInfo("DEBUG: This time only will be picked NAVAL teams."); break;
			case TeamCategory::Air: Debug::LogInfo("DEBUG: This time only will be picked AIR teams."); break;
			default: Debug::LogInfo("DEBUG: This time teams categories are DISABLED."); break;
			}
		}

		if (validTriggerCandidates.empty())
		{
			Debug::LogInfo("DEBUG: [{}] (idx: {}) No valid triggers for now. A new attempt will be done later...", pHouse->Type->ID, pHouse->ArrayIndex);
			return true;
		}

		if (splitTriggersByCategory && validCategory != TeamCategory::None && validTriggerCandidatesByCategory[(int)validCategory].empty())
		{
			Debug::LogInfo("DEBUG: [{}] (idx: {}) No valid triggers of this category. A new attempt should be done later...", pHouse->Type->ID, pHouse->ArrayIndex);
			if (!isFallbackEnabled) return true;
			Debug::LogInfo("... but fallback mode is enabled so now will be checked all available triggers.");
			validCategory = TeamCategory::None;
		}

		AITriggerTypeClass* selectedTrigger = nullptr;
		double weightDice = 0.0;
		bool found = false;
		const HelperedVector<TriggerElementWeight>* candidates = &validTriggerCandidates;
		double totalW = totalWeight;
		if (splitTriggersByCategory && validCategory != TeamCategory::None)
		{
			candidates = &validTriggerCandidatesByCategory[(int)validCategory];
			totalW = totalWeightByCategory[(int)validCategory];
		}
		weightDice = ScenarioClass::Instance->Random.RandomRanged(0, (int)totalW) * 1.0;
		for (const auto& element : *candidates)
		{
			if (weightDice < element.Weight && !found)
			{
				selectedTrigger = element.Trigger;
				found = true;
				break;
			}
		}
		if (!selectedTrigger)
		{
			Debug::LogInfo("AI Team Selector: House [{}] (idx: {}) failed to select Trigger. A new attempt Will be done later...", pHouse->Type->ID, pHouse->ArrayIndex);
			return true;
		}
		if (selectedTrigger->Weight_Current >= 5000.0 && selectedTrigger->Weight_Minimum <= 4999.0)
		{
			selectedTrigger->Weight_Current = 4999.0;
		}
		Debug::LogInfo("AI Team Selector: House [{}] (idx: {}) selected trigger [{}].", pHouse->Type->ID, pHouse->ArrayIndex, selectedTrigger->ID);
		if (auto pTriggerTeam1Type = selectedTrigger->Team1)
		{
			int count = 0;
			for (const auto& team : activeTeamsList) if (team->Type == pTriggerTeam1Type) count++;
			if (count < pTriggerTeam1Type->Max)
			{
				if (TeamClass* newTeam1 = pTriggerTeam1Type->CreateTeam(pHouse)) newTeam1->NeedsToDisappear = false;
			}
		}
		if (auto pTriggerTeam2Type = selectedTrigger->Team2)
		{
			int count = 0;
			for (const auto& team : activeTeamsList) if (team->Type == pTriggerTeam2Type) count++;
			if (count < pTriggerTeam2Type->Max)
			{
				if (TeamClass* newTeam2 = pTriggerTeam2Type->CreateTeam(pHouse)) newTeam2->NeedsToDisappear = false;
			}
		}
	}
	return true;
}

ASMJIT_PATCH(0x4F8A27, TeamTypeClass_SuggestedNewTeam_NewTeamsSelector, 0x5)
{
	enum { UseOriginalSelector = 0x4F8A63, SkipCode = 0x4F8B08 };
	GET(HouseClass*, pHouse, ESI);

	bool houseIsHuman = pHouse->IsHumanPlayer;
	if (SessionClass::IsCampaign())
		houseIsHuman = pHouse->IsHumanPlayer || pHouse->IsInPlayerControl;

	if (houseIsHuman || pHouse->Type->MultiplayPassive)
		return SkipCode;

	return UpdateTeam(pHouse) ? SkipCode : UseOriginalSelector;
}

#include <ExtraHeaders/StackVector.h>

ASMJIT_PATCH(0x687C9B, ReadScenarioINI_AITeamSelector_PreloadValidTriggers, 0x7)
{
	// For each house save a list with only AI Triggers that can be used
	for (HouseClass* pHouse : *HouseClass::Array)
	{
		StackVector<int, 256> list {};
		const int houseIdx = pHouse->ArrayIndex;
		const int sideIdx = pHouse->SideIndex + 1;

		for (int i = 0; i < AITriggerTypeClass::Array->Count; i++)
		{
			if (auto pTrigger = AITriggerTypeClass::Array->Items[i])
			{
				if (ScenarioClass::Instance->IgnoreGlobalAITriggers == pTrigger->IsGlobal || !pTrigger->Team1)
					continue;

				const int triggerHouse = pTrigger->HouseIndex;
				const int triggerSide = pTrigger->SideIndex;

				// The trigger must be compatible with the owner
				if ((triggerHouse == -1 || houseIdx == triggerHouse) && (triggerSide == 0 || sideIdx == triggerSide))
					list->push_back(i);
			}
		}

		Debug::LogInfo("House {} [{}] could use {} AI triggers in this map.", pHouse->ArrayIndex, pHouse->Type->ID, list->size());
	}

	return 0;
}