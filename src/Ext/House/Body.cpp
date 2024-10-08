#include "Body.h"

#include <Ext/TechnoType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/Side/Body.h>
#include <Ext/HouseType/Body.h>
#include <Ext/SWType/Body.h>
#include <Ext/Super/Body.h>

#include <ScenarioClass.h>

#include <New/Type/GenericPrerequisite.h>
#include <New/Type/CrateTypeClass.h>

std::vector<int> HouseExtData::AIProduction_CreationFrames;
std::vector<int> HouseExtData::AIProduction_Values;
std::vector<int> HouseExtData::AIProduction_BestChoices;
std::vector<int> HouseExtData::AIProduction_BestChoicesNaval;

int HouseExtData::LastGrindingBlanceUnit = 0;
int HouseExtData::LastGrindingBlanceInf = 0;
int HouseExtData::LastHarvesterBalance = 0;
int HouseExtData::LastSlaveBalance = 0;

bool HouseExtData::IsAnyFirestormActive = false;

CDTimerClass HouseExtData::CloakEVASpeak;
CDTimerClass HouseExtData::SubTerraneanEVASpeak;
void HouseExtData::InitializeConstant()
{
	//BuiltAircraftTypes.PopulateCounts(10000);
	//BuiltInfantryTypes.PopulateCounts(10000);
	//BuiltUnitTypes.PopulateCounts(10000);
	//BuiltBuildingTypes.PopulateCounts(10000);
	//KilledAircraftTypes.PopulateCounts(10000);
	//KilledInfantryTypes.PopulateCounts(10000);
	//KilledUnitTypes.PopulateCounts(10000);
	//KilledBuildingTypes.PopulateCounts(10000);
	//CapturedBuildings.PopulateCounts(10000);
	//CollectedCrates.PopulateCounts(10000);

	//Debug::Log("Initilizing Tiberium storage for [%s] with [%d] count !\n", this->AttachedToObject->Type->ID, TiberiumClass::Array->Count);
	TiberiumStorage.m_values.resize(TiberiumClass::Array->Count);
}

void HouseExtData::InitializeTrackers(HouseClass* pHouse)
{
	//auto pExt = HouseExtContainer::Instance.Find(pHouse);
	//pExt->BuiltAircraftTypes.PopulateCounts(AircraftTypeClass::Array->Count);
	//pExt->BuiltInfantryTypes.PopulateCounts(InfantryTypeClass::Array->Count);
	//pExt->BuiltUnitTypes.PopulateCounts(UnitTypeClass::Array->Count);
	//pExt->BuiltBuildingTypes.PopulateCounts(BuildingTypeClass::Array->Count);
	//pExt->KilledAircraftTypes.PopulateCounts(AircraftTypeClass::Array->Count);
	//pExt->KilledInfantryTypes.PopulateCounts(InfantryTypeClass::Array->Count);
	//pExt->KilledUnitTypes.PopulateCounts(UnitTypeClass::Array->Count);
	//pExt->KilledBuildingTypes.PopulateCounts(BuildingTypeClass::Array->Count);
	//pExt->CapturedBuildings.PopulateCounts(BuildingTypeClass::Array->Count);
	//pExt->CollectedCrates.PopulateCounts(CrateTypeClass::Array.size());
}

bool HouseExtData::IsMutualAllies(HouseClass const* pThis, HouseClass const* pHouse)
{
	return pHouse == pThis
		|| (pThis->Allies.Contains(pHouse->ArrayIndex)
			&& pHouse->Allies.Contains(pThis->ArrayIndex));
}

float HouseExtData::GetRestrictedFactoryPlantMult(TechnoTypeClass* pTechnoType) const
{
	float mult = 1.0;
	auto const pTechnoTypeExt = TechnoTypeExtContainer::Instance.Find(pTechnoType);

	for (auto const pBuilding : this->RestrictedFactoryPlants)
	{
		auto const pTypeExt = BuildingTypeExtContainer::Instance.Find(pBuilding->Type);

		if (pTypeExt->FactoryPlant_AllowTypes.size() > 0 && !pTypeExt->FactoryPlant_AllowTypes.Contains(pTechnoType))
			continue;

		if (pTypeExt->FactoryPlant_DisallowTypes.size() > 0 && pTypeExt->FactoryPlant_DisallowTypes.Contains(pTechnoType))
			continue;

		float currentMult = 1.0f;

		switch (pTechnoType->WhatAmI())
		{
		case AbstractType::BuildingType:
			if (((BuildingTypeClass*)pTechnoType)->BuildCat == BuildCat::Combat)
				currentMult -= pBuilding->Type->DefensesCostBonus;
			else
				currentMult -= pBuilding->Type->BuildingsCostBonus;
			break;
		case AbstractType::AircraftType:
			currentMult -= pBuilding->Type->AircraftCostBonus;
			break;
		case AbstractType::InfantryType:
			currentMult -= pBuilding->Type->InfantryCostBonus;
			break;
		case AbstractType::UnitType:
			currentMult -= pBuilding->Type->UnitsCostBonus;
			break;
		default:
			break;
		}

		mult *= float(1.0f - currentMult * pTechnoTypeExt->FactoryPlant_Multiplier);
	}

	return mult;
}

RequirementStatus HouseExtData::RequirementsMet(
	HouseClass* pHouse, TechnoTypeClass* pItem)
{
	const auto pData = TechnoTypeExtContainer::Instance.Find(pItem);
	const auto pHouseExt = HouseExtContainer::Instance.Find(pHouse);
	const bool IsHuman = pHouse->IsControlledByHuman();
	const bool IsUnbuildable = pItem->Unbuildable || (IsHuman && pData->HumanUnbuildable);

	if (!IsUnbuildable)
	{
		if ((pData->Prerequisite_RequiredTheaters & (1 << static_cast<int>(ScenarioClass::Instance->Theater))) != 0)
		{
			if (!Prereqs::HouseOwnsAny(pHouse, pData->Prerequisite_Negative.data(), pData->Prerequisite_Negative.size()))
			{
				if (pHouseExt->Reversed.contains(pItem))
				{
					return RequirementStatus::Overridden;
				}

				if (pData->RequiredStolenTech.any())
				{
					if ((pHouseExt->StolenTech & pData->RequiredStolenTech) != pData->RequiredStolenTech)
					{
						return RequirementStatus::Incomplete;
					}
				}

				if (Prereqs::HouseOwnsAny(pHouse, pItem->PrerequisiteOverride))
				{
					return RequirementStatus::Overridden;
				}

				if (pHouse->HasFromSecretLab(pItem))
				{
					return RequirementStatus::Overridden;
				}

				if (IsHuman && pItem->TechLevel == -1)
				{
					return RequirementStatus::Incomplete;
				}

				if (!pHouse->HasAllStolenTech(pItem))
				{
					return RequirementStatus::Incomplete;
				}

				if (!pHouse->InRequiredHouses(pItem) || pHouse->InForbiddenHouses(pItem))
				{
					return RequirementStatus::Forbidden;
				}

				if (!HouseExtData::CheckFactoryOwners(pHouse, pItem))
				{
					return RequirementStatus::Incomplete;
				}

				if (auto const pBldType = specific_cast<BuildingTypeClass const*>(pItem))
				{
					if (HouseExtData::IsDisabledFromShell(pHouse, pBldType))
					{
						return RequirementStatus::Forbidden;
					}
				}

				if (pData->Prerequisite_Power.isset())
				{
					if (pData->Prerequisite_Power <= 0)
					{
						if (-pData->Prerequisite_Power > pHouse->PowerOutput)
						{
							return RequirementStatus::Incomplete;
						}
					}
					else if (pData->Prerequisite_Power > pHouse->PowerOutput - pHouse->PowerDrain)
					{
						return RequirementStatus::Incomplete;
					}
				}

				return (pHouse->StaticData.TechLevel >= pItem->TechLevel) ?
					RequirementStatus::Complete : RequirementStatus::Incomplete;
			}
		}
	}

	return RequirementStatus::Unbuildable;
}

std::pair<NewFactoryState, BuildingClass*> HouseExtData::HasFactory(
	HouseClass* pHouse,
	TechnoTypeClass* pType,
	bool bSkipAircraft,
	bool requirePower,
	bool bCheckCanBuild,
	bool b7)
{
	if (bCheckCanBuild && pHouse->CanBuild(pType, true, true) <= CanBuildResult::Unbuildable)
	{
		return { NewFactoryState::NoFactory  , nullptr };
	}

	auto const nWhat = pType->WhatAmI();
	auto const bitsOwners = pType->GetOwners();
	auto const isNaval = pType->Naval;
	auto const pExt = TechnoTypeExtContainer::Instance.Find(pType);
	BuildingClass* pNonPrimaryBuilding = nullptr;
	BuildingClass* pOfflineBuilding = nullptr;

	for (auto const& pBld : pHouse->Buildings)
	{
		if (pBld->InLimbo
			|| pBld->GetCurrentMission() == Mission::Selling
			|| pBld->QueuedMission == Mission::Selling
		)
		{
			continue;
		}

		auto const pBType = pBld->Type;

		if ((nWhat != pBType->Factory) || !pType->InOwners(bitsOwners))
			continue;

		if (!bSkipAircraft && (nWhat == AbstractType::AircraftType) && pBld->HasAnyLink())
		{
			if (!pBld->HasFreeLink())
			{
				continue;
			}
		}
		else if ((nWhat == AbstractType::UnitType) && (pBType->Naval != isNaval))
		{
			continue;
		}

		if (TechnoTypeExtData::CanBeBuiltAt(pType, pBType))
		{
			if (requirePower && (!pBld->HasPower || pBld->Deactivated))
			{
				pOfflineBuilding = pBld;
			}
			else
			{
				pNonPrimaryBuilding = pBld;
				if (pBld->IsPrimaryFactory)
					return { NewFactoryState::Available_Primary , pNonPrimaryBuilding };

				//do only single loop and use the pOfflineBuildingResult
				if (b7)
				{
					pOfflineBuilding = pBld;
					break;
				}
			}
		}
	}

	if (pNonPrimaryBuilding)
	{
		return { NewFactoryState::Available_Alternative  , pNonPrimaryBuilding };
	}
	else if (pOfflineBuilding)
	{
		return { NewFactoryState::Unpowered  , pOfflineBuilding };
	}

	return { NewFactoryState::NotFound , nullptr };
}

int HouseExtData::BuildBuildingLimitRemaining(HouseClass* pHouse, BuildingTypeClass* pItem)
{
	auto const BuildLimit = pItem->BuildLimit;

	if (BuildLimit >= 0)
		return BuildLimit - BuildingTypeExtData::GetUpgradesAmount(const_cast<BuildingTypeClass*>(pItem), const_cast<HouseClass*>(pHouse));
	else
		return -BuildLimit - pHouse->CountOwnedEver(pItem);
}

int HouseExtData::CheckBuildingBuildLimit(HouseClass* pHouse, BuildingTypeClass* pItem, bool const includeQueued)
{
	enum { NotReached = 1, ReachedPermanently = -1, ReachedTemporarily = 0 };

	int BuildLimit = pItem->BuildLimit;
	int Remaining = HouseExtData::BuildBuildingLimitRemaining(pHouse, pItem);

	if (BuildLimit >= 0 && Remaining <= 0)
		return (includeQueued && FactoryClass::FindByOwnerAndProduct(pHouse, pItem)) ? NotReached : ReachedPermanently;

	return Remaining > 0 ? NotReached : ReachedTemporarily;
}

CanBuildResult HouseExtData::PrereqValidate(
	HouseClass* pHouse, TechnoTypeClass* pItem,
	bool buildLimitOnly, bool includeQueued)
{
	//	auto canBuiltresult = CanBuildResult::Unbuildable;
	//	bool resultRetrieved = false;
	//	std::string TemporarilyResult {};

	const bool IsHuman = pHouse->IsControlledByHuman();
	//const bool debug = CRT::strcmpi(pItem->ID, "GAOREP") == 0;

	if (!buildLimitOnly)
	{
		const RequirementStatus ReqsMet = HouseExtData::RequirementsMet(pHouse, pItem);
		const auto pItemExt = TechnoTypeExtContainer::Instance.Find(pItem);

		if (ReqsMet <= RequirementStatus::Incomplete)
		{
			//if (ReqsMet == RequirementStatus::Incomplete  &&
			//	!pItemExt->Prerequisite_Display.empty())
			//{
			//	if (Prereqs::HouseOwnsAll(pHouse,
			//		pItemExt->Prerequisite_Display.data(),
			//		pItemExt->Prerequisite_Display.size())
			//	)
			//	{
			//		return CanBuildResult::TemporarilyUnbuildable;
			//	}
			//}

			return CanBuildResult::Unbuildable;
		}

		if (IsHuman && ReqsMet == RequirementStatus::Complete)
		{
			if (!HouseExtData::PrerequisitesMet(pHouse, pItem))
			{
				return CanBuildResult::Unbuildable;
			}
		}

		const auto factoryresult = HouseExtData::HasFactory(pHouse, pItem, true, true, false, true).first;
		if (factoryresult == NewFactoryState::NotFound || factoryresult == NewFactoryState::NoFactory)
			return CanBuildResult::Unbuildable;

		if (factoryresult == NewFactoryState::Unpowered)
			return CanBuildResult::TemporarilyUnbuildable;
	}

	if (!IsHuman && RulesExtData::Instance()->AllowBypassBuildLimit[pHouse->GetAIDifficultyIndex()])
	{
		return CanBuildResult::Buildable;
	}

	const auto builtLimitResult = static_cast<CanBuildResult>(HouseExtData::CheckBuildLimit(pHouse, pItem, includeQueued));

	if (builtLimitResult == CanBuildResult::Buildable && pItem->WhatAmI() == BuildingTypeClass::AbsID && !BuildingTypeExtContainer::Instance.Find((BuildingTypeClass*)pItem)->PowersUp_Buildings.empty())
	{
		return static_cast<CanBuildResult>(HouseExtData::CheckBuildingBuildLimit(pHouse, (BuildingTypeClass*)pItem, includeQueued));
	}

	return builtLimitResult;
}

// true : continue check
// false : RequirementStatus::Incomplete
bool HouseExtData::CheckFactoryOwners(HouseClass* pHouse, TechnoTypeClass* pItem)
{
	auto const pExt = TechnoTypeExtContainer::Instance.Find(pItem);
	auto const pHouseExt = HouseExtContainer::Instance.Find(pHouse);

	if (!pExt->FactoryOwners.empty() || !pExt->FactoryOwners_Forbidden.empty())
	{
		for (auto& gather : pHouseExt->FactoryOwners_GatheredPlansOf)
		{
			auto FactoryOwners_begin = pExt->FactoryOwners.begin();
			const auto FactoryOwners_end = pExt->FactoryOwners.end();

			if (FactoryOwners_begin != FactoryOwners_end)
			{
				while (*FactoryOwners_begin != gather)
				{
					if (++FactoryOwners_begin == FactoryOwners_end)
						continue;
				}
			}

			if (pExt->FactoryOwners_Forbidden.empty())
				return true;//will skip the buildings check

			auto FactoryForbiddenOwners_begin = pExt->FactoryOwners_Forbidden.begin();
			const auto FactoryForbidden_end = pExt->FactoryOwners_Forbidden.end();

			while (*FactoryForbiddenOwners_begin != gather)
			{
				if (++FactoryForbiddenOwners_begin == FactoryForbidden_end)
					return true; //will skip the buildings check
			}
		}
	}

	const auto whatItem = pItem->WhatAmI();
	for (auto const& pBld : pHouse->Buildings)
	{
		auto pBldExt = TechnoExtContainer::Instance.Find(pBld);

		auto FactoryOwners_begin = pExt->FactoryOwners.begin();
		const auto FactoryOwners_end = pExt->FactoryOwners.end();

		if (FactoryOwners_begin != FactoryOwners_end)
		{
			while (*FactoryOwners_begin != pBldExt->OriginalHouseType)
			{
				if (++FactoryOwners_begin == FactoryOwners_end)
					continue;
			}
		}

		if (pExt->FactoryOwners_Forbidden.empty() || !pExt->FactoryOwners_Forbidden.Contains(pBldExt->OriginalHouseType))
		{
			if (pBld->Type->Factory == whatItem || BuildingTypeExtContainer::Instance.Find(pBld->Type)->Type->FactoryOwners_HasAllPlans)
			{
				return true;
			}
		}
	}

	return false;
}

void HouseExtData::UpdateAcademy(HouseClass* pHouse, BuildingClass* pAcademy, bool added)
{
	HouseExtContainer::Instance.Find(pHouse)->UpdateAcademy(pAcademy, added);
}

void HouseExtData::UpdateAcademy(BuildingClass* pAcademy, bool added)
{
	// check if added and there already, or removed and not there
	auto it = this->Academies.find(pAcademy);
	if (added == (it != this->Academies.end()))
	{
		return;
	}

	// now this can be unconditional
	if (added)
	{
		//using `emplace` because it already check above,..
		this->Academies.emplace(pAcademy);
	}
	else
	{
		this->Academies.erase(it);
	}
}

void HouseExtData::ApplyAcademy(HouseClass* pHouse, TechnoClass* pTechno, AbstractType considerAs)
{
	HouseExtContainer::Instance.Find(pHouse)->ApplyAcademy(pTechno, considerAs);
}

void HouseExtData::ApplyAcademy(
	TechnoClass* const pTechno, AbstractType const considerAs) const
{
	// mutex in effect, ignore academies to fix preplaced order issues.
	// also triggered in game for certain "conversions" like deploy
	if (Unsorted::ScenarioInit)
	{
		return;
	}

	auto const pType = pTechno->GetTechnoType();
	if (pType->Trainable)
	{
		// get the academy data for this type
		Valueable<double> BuildingTypeExtData::* pmBonus = nullptr;
		switch (considerAs)
		{
		case AbstractType::Infantry:
			pmBonus = &BuildingTypeExtData::AcademyInfantry;
			break;
		case AbstractType::Aircraft:
			pmBonus = &BuildingTypeExtData::AcademyAircraft;
			break;
		case AbstractType::Unit:
			pmBonus = &BuildingTypeExtData::AcademyVehicle;
			break;
		default:
			pmBonus = &BuildingTypeExtData::AcademyBuilding;
			break;
		}

		auto veterancyBonus = 0.0;

		// aggregate the bonuses
		for (auto const& pBld : this->Academies)
		{
			if (!pBld)
				continue;

			auto const pExt = BuildingTypeExtContainer::Instance.Find(pBld->Type);

			auto const isWhitelisted = pExt->AcademyWhitelist.empty()
				|| pExt->AcademyWhitelist.Contains(pType);

			if (isWhitelisted && !pExt->AcademyBlacklist.Contains(pType))
			{
				const auto& data = pExt->*pmBonus;
				veterancyBonus = MaxImpl(veterancyBonus, data.Get());
			}
		}

		// apply the bonus
		auto& value = pTechno->Veterancy.Veterancy;
		if (veterancyBonus > value)
		{
			value = static_cast<float>(MinImpl(
				veterancyBonus, RulesClass::Instance->VeteranCap));
		}
	}
}

void HouseExtData::UpdateFactoryPlans(BuildingClass* pBld)
{
	auto Types = pBld->GetTypes();
	auto Types_c = Types.begin();

	while (!*Types_c || !TechnoTypeExtContainer::Instance.Find(*Types_c)->FactoryOwners_HaveAllPlans)
	{
		if (++Types_c == Types.end())
			return;
	}

	HouseExtContainer::Instance.Find(pBld->Owner)->FactoryOwners_GatheredPlansOf
		.insert(TechnoExtContainer::Instance.Find(pBld)->OriginalHouseType);
}

bool HouseExtData::PrerequisitesMet(HouseClass* const pThis, TechnoTypeClass* const pItem)
{
	for (auto& prereq : TechnoTypeExtContainer::Instance.Find(pItem)->Prerequisites)
	{
		if (Prereqs::HouseOwnsAll(pThis, prereq.data(), prereq.size()))
		{
			return true;
		}
	}

	return false;
}

bool HouseExtData::PrerequisitesMet(HouseClass* pThis, int* items, int size)
{
	return Prereqs::HouseOwnsAll(pThis, items, size);
}

void HouseExtData::LoadFromINIFile(CCINIClass* pINI, bool parseFailAddr)
{
	const char* pSection = this->AttachedToObject->PlainName;

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);
	exINI.Read3Bool(pSection, "RepairBaseNodes", this->RepairBaseNodes);

	this->Degrades.Read(exINI, pSection, "Degrades");
}

TunnelData* HouseExtData::GetTunnelVector(HouseClass* pHouse, size_t nTunnelIdx)
{
	if (!pHouse || nTunnelIdx >= TunnelTypeClass::Array.size())
		return nullptr;

	auto pHouseExt = HouseExtContainer::Instance.Find(pHouse);

	while (pHouseExt->Tunnels.size() < TunnelTypeClass::Array.size())
	{
		pHouseExt->Tunnels.emplace_back().MaxCap = TunnelTypeClass::Array[nTunnelIdx]->Passengers;
	}

	return pHouseExt->Tunnels.data() + nTunnelIdx;
}

TunnelData* HouseExtData::GetTunnelVector(BuildingTypeClass* pBld, HouseClass* pHouse)
{
	return HouseExtData::GetTunnelVector(pHouse, BuildingTypeExtContainer::Instance.Find(pBld)->TunnelType);
}

void HouseExtData::UpdateShotCount(SuperWeaponTypeClass* pFor)
{
	this->LaunchDatas.resize(SuperWeaponTypeClass::Array->Count);
	this->LaunchDatas[pFor->ArrayIndex].Update();
}

void HouseExtData::UpdateShotCountB(SuperWeaponTypeClass* pFor)
{
	this->LaunchDatas.resize(SuperWeaponTypeClass::Array->Count);

	auto& nData = this->LaunchDatas[pFor->ArrayIndex];

	if ((nData.LastFrame & 0x80000000) != 0)
		nData.LastFrame = Unsorted::CurrentFrame();
}

SuperClass* HouseExtData::IsSuperAvail(int nIdx, HouseClass* pHouse)
{
	if (const auto pSW = pHouse->Supers.GetItemOrDefault(nIdx))
	{
		if (SWTypeExtContainer::Instance.Find(pSW->Type)->IsAvailable(pHouse))
		{
			return pSW;
		}
	}

	return nullptr;
}

int HouseExtData::GetSurvivorDivisor(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	if (pTypeExt && (pTypeExt->SurvivorDivisor.Get() > 0))
		return pTypeExt->SurvivorDivisor;

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetSurvivorDivisor();
	}

	return 0;
}

InfantryTypeClass* HouseExtData::GetCrew(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	if (pTypeExt && pTypeExt->Crew)
		return pTypeExt->Crew;

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetCrew();
	}

	return RulesClass::Instance->Technician;
}

InfantryTypeClass* HouseExtData::GetEngineer(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	if (pTypeExt && pTypeExt->Engineer)
		return pTypeExt->Engineer;

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetEngineer();
	}

	return RulesClass::Instance->Engineer;
}

InfantryTypeClass* HouseExtData::GetTechnician(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	if (pTypeExt && pTypeExt->Technician)
		return pTypeExt->Technician;

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetTechnician();
	}

	return RulesClass::Instance->Technician;
}

InfantryTypeClass* HouseExtData::GetDisguise(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	if (pTypeExt && pTypeExt->Disguise)
		return pTypeExt->Disguise;

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetDisguise();
	}

	return nullptr;
}

AircraftTypeClass* HouseExtData::GetParadropPlane(HouseClass* pHouse)
{
	// tries to get the house's default plane and falls back to
	// the sides default plane.
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);
	AircraftTypeClass* pRest = nullptr;

	if (pTypeExt && pTypeExt->ParaDropPlane)
	{
		pRest = pTypeExt->ParaDropPlane;
	}

	if (!pRest)
	{
		int iPlane = -1;
		if (const auto pSide = HouseExtData::GetSide(pHouse))
		{
			iPlane = SideExtContainer::Instance.Find(pSide)->ParaDropPlane;
		}

		// didn't help. default to the PDPlane like the game does.

		pRest =
			AircraftTypeClass::Array->GetItemOrDefault(iPlane, RulesExtData::Instance()->DefaultParaPlane);
	}

	if (!pRest)
		Debug::FatalError("Invalid Paradrop Plane");

	return pRest;
}

AircraftTypeClass* HouseExtData::GetSpyPlane(HouseClass* pHouse)
{
	AircraftTypeClass* pRest = nullptr;

	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);
	if (pTypeExt && pTypeExt->SpyPlane)
	{
		pRest = pTypeExt->SpyPlane;
	}

	if (!pRest)
	{
		if (const auto pSide = HouseExtData::GetSide(pHouse))
		{
			pRest = SideExtContainer::Instance.Find(pSide)->SpyPlane;
		}
	}

	if (!pRest)
		pRest = AircraftTypeClass::Find(GameStrings::SPYP);

	if (pRest && pRest->Strength == 0)
		Debug::FatalError("Invalid Spy Plane[%s]", pRest->ID);
	else if (!pRest)
		Debug::FatalError("Invalid Spy Plane");

	return pRest;
}

UnitTypeClass* HouseExtData::GetHunterSeeker(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);
	if (pTypeExt && pTypeExt->HunterSeeker)
	{
		return pTypeExt->HunterSeeker;
	}

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		return SideExtContainer::Instance.Find(pSide)->GetHunterSeeker();
	}

	return nullptr;
}

AnimTypeClass* HouseExtData::GetParachuteAnim(HouseClass* pHouse)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);
	if (pTypeExt && pTypeExt->ParachuteAnim)
	{
		return pTypeExt->ParachuteAnim;
	}

	if (const auto pSide = HouseExtData::GetSide(pHouse))
	{
		if (auto pAnim = SideExtContainer::Instance.Find(pSide)->ParachuteAnim.Get(RulesClass::Instance->Parachute))
			return pAnim;

		Debug::Log(
			"[GetParachuteAnim] House %s and its side have no valid parachute defined. Rules fallback failed.\n",
			pHouse->get_ID());
	}

	return AnimTypeClass::Find("PARACH");
}

bool HouseExtData::GetParadropContent(HouseClass* pHouse, Iterator<TechnoTypeClass*>& Types, Iterator<int>& Num)
{
	const auto pTypeExt = HouseTypeExtContainer::Instance.TryFind(pHouse->Type);

	// tries to get the house's default contents and falls back to
	// the sides default contents.
	if (pTypeExt && !pTypeExt->ParaDropTypes.empty())
	{
		Types = pTypeExt->ParaDropTypes;
		Num = pTypeExt->ParaDropNum;
	}

	// fall back to side specific para drop
	if (!Types)
	{
		if (const auto pSide = HouseExtData::GetSide(pHouse))
		{
			SideExtData* pData = SideExtContainer::Instance.Find(pSide);

			Types = pData->GetParaDropTypes();
			Num = pData->GetParaDropNum();
		}
	}

	return (Types && Num);
}

bool HouseExtData::InvalidateIgnorable(AbstractClass* ptr)
{
	switch (VTable::Get(ptr))
	{
	case BuildingClass::vtable:
	case InfantryClass::vtable:
	case UnitClass::vtable:
	case AircraftClass::vtable:
	case TeamClass::vtable:
	case SuperClass::vtable:
		return false;
	}

	return true;
}

TechTreeTypeClass* HouseExtData::GetTechTreeType()
{
	if (!this->SideTechTree.isset())
	{
		TechTreeTypeClass* ret = nullptr;

		for (const auto& pType : TechTreeTypeClass::Array)
		{
			if (pType->SideIndex == this->AttachedToObject->SideIndex)
			{
				ret = pType.get();
			}
		}

		if (!ret)
		{
			Debug::Log("TechTreeTypeClass::GetForSide: Could not find tech tree for side %d, returning tech tree 0: %s",
				this->AttachedToObject->SideIndex, TechTreeTypeClass::Array[0]->Name.data());
			ret = TechTreeTypeClass::Array[0].get();
		}

		this->SideTechTree = ret;
	}

	return this->SideTechTree.get();
}

void HouseExtData::InvalidatePointer(AbstractClass* ptr, bool bRemoved)
{
	if (ptr == nullptr)
		return;

	//AnnounceInvalidPointer(OwnedTechno, ptr , bRemoved);
	AnnounceInvalidPointer(Factory_BuildingType, ptr, bRemoved);
	AnnounceInvalidPointer(Factory_InfantryType, ptr, bRemoved);
	AnnounceInvalidPointer(Factory_VehicleType, ptr, bRemoved);
	AnnounceInvalidPointer(Factory_NavyType, ptr, bRemoved);
	AnnounceInvalidPointer(Factory_AircraftType, ptr, bRemoved);
	AnnounceInvalidPointer<TechnoClass*>(LimboTechno, ptr, bRemoved);
	AnnounceInvalidPointer<BuildingClass*>(Academies, ptr, bRemoved);
	AnnounceInvalidPointer<BuildingClass*>(RestrictedFactoryPlants, ptr, bRemoved);

	if (bRemoved)
		AutoDeathObjects.erase((TechnoClass*)ptr);

	for (auto& nTun : Tunnels)
		AnnounceInvalidPointer(nTun.Vector, ptr, bRemoved);

	AnnounceInvalidPointer<SuperClass*>(Batteries, ptr);
}

int HouseExtData::ActiveHarvesterCount(HouseClass* pThis)
{
	if (!pThis || !pThis->IsCurrentPlayer()) return 0;

	int result =
		std::count_if(TechnoClass::Array->begin(), TechnoClass::Array->end(),
		[pThis](TechnoClass* techno)
		{
			if (!techno->IsAlive || techno->Health <= 0 || techno->IsCrashing || techno->IsSinking || techno->Owner != pThis)
				return false;

			if (techno->WhatAmI() == UnitClass::AbsID && (static_cast<UnitClass*>(techno)->DeathFrameCounter > 0))
				return false;

			return TechnoTypeExtContainer::Instance.Find(techno->GetTechnoType())->IsCountedAsHarvester() && TechnoExtData::IsHarvesting(techno);
		});

	return result;
}

int HouseExtData::TotalHarvesterCount(HouseClass* pThis)
{
	if (!pThis || !pThis->IsCurrentPlayer() || pThis->Defeated) return 0;

	int result = 0;

	TechnoTypeClass::Array->for_each([&result, pThis](TechnoTypeClass* techno)
 {
	 if (TechnoTypeExtContainer::Instance.Find(techno)->IsCountedAsHarvester())
	 {
		 result += pThis->CountOwnedAndPresent(techno);
	 }
	});

	return result;
}

// This basically gets same cell that AI script action 53 Gather at Enemy Base uses, and code for that (0x6EF700) was used as reference here.
CellClass* HouseExtData::GetEnemyBaseGatherCell(HouseClass* pTargetHouse, HouseClass* pCurrentHouse, const CoordStruct& defaultCurrentCoords, SpeedType speedTypeZone, int extraDistance)
{
	if (!pTargetHouse || !pCurrentHouse)
		return nullptr;

	const auto targetBaseCoords = CellClass::Cell2Coord(pTargetHouse->GetBaseCenter());

	if (targetBaseCoords == CoordStruct::Empty)
		return nullptr;

	auto currentCoords = CellClass::Cell2Coord(pCurrentHouse->GetBaseCenter());

	if (currentCoords == CoordStruct::Empty)
		currentCoords = defaultCurrentCoords;

	const int deltaX = currentCoords.X - targetBaseCoords.X;
	const int deltaY = targetBaseCoords.Y - currentCoords.Y;
	const int distance = (RulesClass::Instance->AISafeDistance + extraDistance) * Unsorted::LeptonsPerCell;
	auto newCoords = GeneralUtils::CalculateCoordsFromDistance(currentCoords, targetBaseCoords, distance);
	auto cellStruct = CellClass::Coord2Cell(newCoords);
	cellStruct = MapClass::Instance->NearByLocation(cellStruct, speedTypeZone, -1, MovementZone::Normal, false, 3, 3, false, false, false, true, cellStruct, false, false);

	return MapClass::Instance->TryGetCellAt(cellStruct);
}

HouseClass* HouseExtContainer::Civilian = nullptr;
HouseClass* HouseExtContainer::Special = nullptr;
HouseClass* HouseExtContainer::Neutral = nullptr;

HouseClass* HouseExtData::FindFirstCivilianHouse()
{
	if (!HouseExtContainer::Civilian)
	{
		HouseExtContainer::Civilian = HouseClass::FindBySideIndex(RulesExtData::Instance()->CivilianSideIndex);
	}

	return HouseExtContainer::Civilian;
}

HouseClass* HouseExtData::FindSpecial()
{
	if (!HouseExtContainer::Special)
	{
		HouseExtContainer::Special = HouseClass::FindByCountryIndex(RulesExtData::Instance()->SpecialCountryIndex);
	}

	return HouseExtContainer::Special;
}

HouseClass* HouseExtData::FindNeutral()
{
	if (!HouseExtContainer::Neutral)
	{
		HouseExtContainer::Neutral = HouseClass::FindByCountryIndex(RulesExtData::Instance()->NeutralCountryIndex);
	}

	return HouseExtContainer::Neutral;
}

void HouseExtData::ForceOnlyTargetHouseEnemy(HouseClass* pThis, int mode = -1)
{
	const auto pHouseExt = HouseExtContainer::Instance.Find(pThis);

	if (mode < 0 || mode > 2)
		mode = -1;

	enum { ForceFalse = 0, ForceTrue = 1, ForceRandom = 2, UseDefault = -1 };

	pHouseExt->ForceOnlyTargetHouseEnemyMode = mode;

	switch (mode)
	{
	case ForceFalse:
		pHouseExt->m_ForceOnlyTargetHouseEnemy = false;
		break;

	case ForceTrue:
		pHouseExt->m_ForceOnlyTargetHouseEnemy = true;
		break;

	case ForceRandom:
		pHouseExt->m_ForceOnlyTargetHouseEnemy = ScenarioClass::Instance->Random.RandomBool();
		break;

	default:
		pHouseExt->m_ForceOnlyTargetHouseEnemy = false;
		break;
	}
}

// Ares
HouseClass* HouseExtData::GetHouseKind(OwnerHouseKind const& kind, bool const allowRandom, HouseClass* const pDefault, HouseClass* const pInvoker, HouseClass* const pVictim)
{
	switch (kind)
	{
	case OwnerHouseKind::Invoker:
	case OwnerHouseKind::Killer:
		return pInvoker ? pInvoker : pDefault;
	case OwnerHouseKind::Victim:
		return pVictim ? pVictim : pDefault;
	case OwnerHouseKind::Civilian:
		return HouseExtData::FindFirstCivilianHouse();// HouseClass::FindFirstCivilianHouse();
	case OwnerHouseKind::Special:
		return HouseExtData::FindSpecial();//  HouseClass::FindSpecial();
	case OwnerHouseKind::Neutral:
		return HouseExtData::FindNeutral();//  HouseClass::FindNeutral();
	case OwnerHouseKind::Random:
		if (allowRandom)
		{
			auto& Random = ScenarioClass::Instance->Random;
			return HouseClass::Array->Items[
				Random.RandomFromMax(HouseClass::Array->Count - 1)];
		}
		else
		{
			return pDefault;
		}
	case OwnerHouseKind::Default:
	default:
		return pDefault;
	}
}

HouseClass* HouseExtData::GetSlaveHouse(SlaveReturnTo const& kind, HouseClass* const pKiller, HouseClass* const pVictim)
{
	switch (kind)
	{
	case SlaveReturnTo::Killer:
		return pKiller;
	case SlaveReturnTo::Master:
		return pVictim;
	case SlaveReturnTo::Civilian:
		return HouseExtData::FindFirstCivilianHouse();
	case SlaveReturnTo::Special:
		return HouseExtData::FindSpecial();
	case SlaveReturnTo::Neutral:
		return HouseExtData::FindNeutral();
	case SlaveReturnTo::Random:
		auto& Random = ScenarioClass::Instance->Random;
		return HouseClass::Array->Items[
			Random.RandomFromMax(HouseClass::Array->Count - 1)];
	}

	return pKiller;
}

bool HouseExtData::IsObserverPlayer()
{
	auto const pCur = HouseClass::CurrentPlayer();

	if (!pCur)
		return false;

	if (pCur == HouseClass::Observer)
		return true;

	return false;
}

bool HouseExtData::IsObserverPlayer(HouseClass* pCur)
{
	if (!pCur)
		return false;

	if (pCur == HouseClass::Observer())
		return true;

	return false;
}

int HouseExtData::GetHouseIndex(int param, TeamClass* pTeam = nullptr, TActionClass* pTAction = nullptr)
{
	// Special case that returns the house index of the TeamClass object or the Trigger Action
	if (param == 8997)
	{
		if (pTAction)
			return (pTeam ? pTeam->Owner->ArrayIndex : pTAction->TeamType->Owner->ArrayIndex);
		else
			return -1;
	}

	if (param < 0)
	{
		std::vector<HouseClass*> housesListIdx {};

		switch (param)
		{
		case -1:
		{
			// Random non-neutral
			for (auto pHouse : *HouseClass::Array)
			{
				if (!pHouse->Defeated
					&& !HouseExtData::IsObserverPlayer(pHouse)
					&& !pHouse->Type->MultiplayPassive)
				{
					housesListIdx.push_back(pHouse);
				}
			}

			return housesListIdx.empty() ?
				-1 :
				housesListIdx[ScenarioClass::Instance->Random.RandomFromMax(housesListIdx.size() - 1)]->ArrayIndex;
		}
		case -2:
		{
			// Find first Neutral
			for (auto pHouseNeutral : *HouseClass::Array)
			{
				if (pHouseNeutral->IsNeutral())
				{
					return pHouseNeutral->ArrayIndex;
				}
			}

			return -1;
		}
		case -3:
		{
			// Random Human Player
			for (auto pHouse : *HouseClass::Array)
			{
				if (pHouse->IsControlledByHuman()
					&& !pHouse->Defeated
					&& !HouseExtData::IsObserverPlayer(pHouse))
				{
					housesListIdx.push_back(pHouse);
				}
			}

			return housesListIdx.empty() ?
				-1 :
				housesListIdx[(ScenarioClass::Instance->Random.RandomFromMax(housesListIdx.size() - 1))]
				->ArrayIndex;
		}
		default:
			return -1;
		}
	}

	// Transtale the Multiplayer index into a valid index for the HouseClass array
	if (HouseClass::Index_IsMP(param))
	{
		if (HouseClass* pHouse = HouseClass::FindByIndex(param))
		{
			if (!pHouse->Defeated
				&& !pHouse->IsObserver()
				&& !pHouse->Type->MultiplayPassive)
			{
				return pHouse->ArrayIndex;
			}
		}

		return -1;
	}

	// Positive index values check. Includes any kind of House
	if (HouseClass* pHouse = HouseClass::FindByCountryIndex(param))
	{
		if (!pHouse->Defeated && !pHouse->IsObserver())
		{
			return pHouse->ArrayIndex;
		}

		return -1;
	}

	return -1;
}

bool HouseExtData::UpdateHarvesterProduction()
{
	auto pThis = this->AttachedToObject;
	const auto AIDifficulty = static_cast<int>(pThis->GetAIDifficultyIndex());
	const auto idxParentCountry = pThis->Type->FindParentCountryIndex();
	const auto pHarvesterUnit = HouseExtData::FindOwned(pThis, idxParentCountry, make_iterator(RulesClass::Instance->HarvesterUnit));

	if (pHarvesterUnit)
	{
		const auto harvesters = pThis->CountResourceGatherers;
		const auto maxHarvesters = HouseExtData::FindBuildable(
			pThis, idxParentCountry, make_iterator(RulesClass::Instance->BuildRefinery))
			? RulesClass::Instance->HarvestersPerRefinery[AIDifficulty] * pThis->CountResourceDestinations
			: RulesClass::Instance->AISlaveMinerNumber[AIDifficulty];

		if (pThis->IQLevel2 >= RulesClass::Instance->Harvester && !pThis->IsTiberiumShort
			&& !pThis->IsControlledByHuman() && harvesters < maxHarvesters
			&& pThis->StaticData.TechLevel >= pHarvesterUnit->TechLevel)
		{
			pThis->ProducingUnitTypeIndex = pHarvesterUnit->ArrayIndex;
			return true;
		}
	}
	else
	{
		if (pThis->CountResourceGatherers < RulesClass::Instance->AISlaveMinerNumber[AIDifficulty])
		{
			if (const auto pRefinery = HouseExtData::FindBuildable(
				pThis, idxParentCountry, make_iterator(RulesClass::Instance->BuildRefinery)))
			{
				if (auto const pSlaveMiner = pRefinery->UndeploysInto)
				{
					if (pSlaveMiner->ResourceDestination)
					{
						pThis->ProducingUnitTypeIndex = pSlaveMiner->ArrayIndex;
						return true;
					}
				}
			}
		}
	}

	return false;
}

size_t HouseExtData::FindOwnedIndex(
	HouseClass*, int const idxParentCountry,
	Iterator<TechnoTypeClass const*> const items, size_t const start)
{
	auto const bitOwner = 1u << idxParentCountry;

	for (size_t i = start; i < items.size(); ++i)
	{
		if (items[i]->InOwners(bitOwner))
		{
			return i;
		}
	}

	return items.size();
}

bool HouseExtData::IsDisabledFromShell(
	HouseClass* pHouse, BuildingTypeClass const* const pItem)
{
	// SWAllowed does not apply to campaigns any more
	if (SessionClass::Instance->GameMode == GameMode::Campaign
		|| GameModeOptionsClass::Instance->SWAllowed)
	{
		return false;
	}

	if (pItem->SuperWeapon != -1)
	{
		// allow SWs only if not disableable from shell
		if (!RulesClass::Instance->BuildTech.Contains(const_cast<BuildingTypeClass*>(pItem)))
		{
			if (pHouse->Supers[pItem->SuperWeapon]->Type->DisableableFromShell)
			{
				return true;
			}
		}
	}

	return false;
}

void HouseExtData::UpdateAutoDeathObjects()
{
	if (this->AutoDeathObjects.empty())
		return;

	for (const auto& [pThis, nMethod] : this->AutoDeathObjects)
	{
		if (pThis->IsInLogic || !pThis->IsAlive || nMethod == KillMethod::None)
			continue;

		auto const pExt = TechnoExtContainer::Instance.TryFind(pThis);
		if (!pExt)
		{
			Debug::Log("HouseExtData::UpdateAutoDeathObject -  Killing Techno Failed , No Extptr [%x - %s] ! \n", pThis, pThis->get_ID());
			continue;
		}

		if (!pExt->Death_Countdown.Completed())
			continue;

		Debug::Log("HouseExtData::UpdateAutoDeathObject -  Killing Techno[%x - %s] ! \n", pThis, pThis->get_ID());
		if (auto const pBuilding = specific_cast<BuildingClass*>(pThis))
		{
			if (BuildingExtContainer::Instance.Find(pBuilding)->LimboID != -1)
			{
				//this->RemoveFromLimboTracking(pBuilding->Type);
				BuildingExtData::LimboKill(pBuilding);
				continue;
			}
		}

		TechnoExtData::KillSelf(pThis, nMethod, true, TechnoTypeExtContainer::Instance.Find(pExt->Type)->AutoDeath_VanishAnimation);
	}
}

int HouseExtData::CountOwnedIncludeDeploy(const HouseClass* pThis, const TechnoTypeClass* pItem)
{
	int count = pThis->CountOwnedNow(pItem);
	count += pItem->DeploysInto ? pThis->CountOwnedNow(pItem->DeploysInto) : 0;
	count += pItem->UndeploysInto ? pThis->CountOwnedNow(pItem->UndeploysInto) : 0;
	return count;
}

std::vector<int> HouseExtData::GetBuildLimitGroupLimits(HouseClass* pHouse, TechnoTypeClass* pType)
{
	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
	std::vector<int> limits = pTypeExt->BuildLimitGroup_Nums;

	if (!pTypeExt->BuildLimitGroup_ExtraLimit_Types.empty() && !pTypeExt->BuildLimitGroup_ExtraLimit_Nums.empty())
	{
		for (size_t i = 0; i < pTypeExt->BuildLimitGroup_ExtraLimit_Types.size(); i++)
		{
			int count = 0;
			auto pTmpType = pTypeExt->BuildLimitGroup_ExtraLimit_Types[i];
			auto const pBuildingType = specific_cast<BuildingTypeClass*>(pTmpType);

			if (pBuildingType &&
				(BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
				count = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pHouse));
			else
				count = pHouse->CountOwnedNow(pTmpType);

			if (i < pTypeExt->BuildLimitGroup_ExtraLimit_MaxCount.size() && pTypeExt->BuildLimitGroup_ExtraLimit_MaxCount[i] > 0)
				count = MinImpl(count, pTypeExt->BuildLimitGroup_ExtraLimit_MaxCount[i]);

			for (auto& limit : limits)
			{
				if (i < pTypeExt->BuildLimitGroup_ExtraLimit_Nums.size() && pTypeExt->BuildLimitGroup_ExtraLimit_Nums[i] > 0)
					limit += count * pTypeExt->BuildLimitGroup_ExtraLimit_Nums[i];

				if (pTypeExt->BuildLimitGroup_ExtraLimit_MaxNum > 0)
					limit = MinImpl(limit, pTypeExt->BuildLimitGroup_ExtraLimit_MaxNum);
			}
		}
	}

	return limits;
}

int HouseExtData::QueuedNum(const HouseClass* pHouse, const TechnoTypeClass* pType)
{
	const AbstractType absType = pType->WhatAmI();

	int queued = 0;

	if (const FactoryClass* pFactory = pHouse->GetPrimaryFactory(absType, pType->Naval, BuildCat::DontCare))
	{
		queued = pFactory->CountTotal(pType);

		if (const auto pObject = pFactory->Object)
		{
			if (pObject->GetType() == pType)
				--queued;
		}
	}

	return queued;
}

void HouseExtData::RemoveProduction(const HouseClass* pHouse, const TechnoTypeClass* pType, int num)
{
	const AbstractType absType = pType->WhatAmI();

	if (FactoryClass* pFactory = pHouse->GetPrimaryFactory(absType, pType->Naval, BuildCat::DontCare))
	{
		int queued = pFactory->CountTotal(pType);
		if (num >= 0)
			queued = MinImpl(num, queued);

		for (int i = 0; i < queued; i++)
		{
			pFactory->RemoveOneFromQueue(pType);
		}
	}
}

bool HouseExtData::ReachedBuildLimit(HouseClass* pHouse, TechnoTypeClass* pType, bool ignoreQueued)
{
	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);

	if (pTypeExt->BuildLimitGroup_Types.empty() || pTypeExt->BuildLimitGroup_Nums.empty())
		return false;

	const std::vector<int> limits = HouseExtData::GetBuildLimitGroupLimits(pHouse, pType);

	if (limits.size() == 1)
	{
		int count = 0;
		int queued = 0;
		bool inside = false;

		for (auto& pTmpType : pTypeExt->BuildLimitGroup_Types)
		{
			const auto pTmpTypeExt = TechnoTypeExtContainer::Instance.Find(pTmpType);

			if (!ignoreQueued)
				queued += QueuedNum(pHouse, pTmpType) * pTmpTypeExt->BuildLimitGroup_Factor;

			int owned = 0;
			const auto pBuildingType = specific_cast<BuildingTypeClass*>(pTmpType);

			if (pBuildingType && (BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
				owned = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pHouse));
			else
				owned = pHouse->CountOwnedNow(pTmpType);

			count += owned * pTmpTypeExt->BuildLimitGroup_Factor;

			if (pTmpType == pType)
				inside = true;
		}

		int num = count - limits.back();

		if (num + queued >= 1 - pTypeExt->BuildLimitGroup_Factor)
		{
			if (inside)
				RemoveProduction(pHouse, pType, (num + queued + pTypeExt->BuildLimitGroup_Factor - 1) / pTypeExt->BuildLimitGroup_Factor);
			else if (num >= 1 - pTypeExt->BuildLimitGroup_Factor || pTypeExt->BuildLimitGroup_NotBuildableIfQueueMatch)
				RemoveProduction(pHouse, pType, -1);

			return true;
		}
	}
	else
	{
		bool reached = true;
		bool realReached = true;

		for (size_t i = 0; i < limits.size(); i++)
		{
			TechnoTypeClass* pTmpType = pTypeExt->BuildLimitGroup_Types[i];
			const auto pTmpTypeExt = TechnoTypeExtContainer::Instance.Find(pTmpType);
			int queued = ignoreQueued ? 0 : QueuedNum(pHouse, pTmpType) * pTmpTypeExt->BuildLimitGroup_Factor;
			int num = 0;
			const auto pBuildingType = specific_cast<BuildingTypeClass*>(pTmpType);

			if (pBuildingType && (BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
				num = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pHouse));
			else
				num = pHouse->CountOwnedNow(pTmpType);

			num *= pTmpTypeExt->BuildLimitGroup_Factor - limits[i];

			if (pType == pTmpType && num + queued >= 1 - pTypeExt->BuildLimitGroup_Factor)
			{
				if (pTypeExt->BuildLimitGroup_ContentIfAnyMatch)
				{
					if (num >= 1 - pTypeExt->BuildLimitGroup_Factor || pTypeExt->BuildLimitGroup_NotBuildableIfQueueMatch)
						RemoveProduction(pHouse, pType, (num + queued + pTypeExt->BuildLimitGroup_Factor - 1) / pTypeExt->BuildLimitGroup_Factor);

					return true;
				}
				else if (num < 1 - pTypeExt->BuildLimitGroup_Factor)
				{
					realReached = false;
				}
			}
			else if (pType != pTmpType && num + queued >= 0)
			{
				if (pTypeExt->BuildLimitGroup_ContentIfAnyMatch)
				{
					if (num >= 0 || pTypeExt->BuildLimitGroup_NotBuildableIfQueueMatch)
						RemoveProduction(pHouse, pType, -1);

					return true;
				}
				else if (num < 0)
				{
					realReached = false;
				}
			}
			else
			{
				reached = false;
			}
		}

		if (reached)
		{
			if (realReached || pTypeExt->BuildLimitGroup_NotBuildableIfQueueMatch)
				RemoveProduction(pHouse, pType, -1);

			return true;
		}
	}

	return false;
}

bool HouseExtData::ShouldDisableCameo(HouseClass* pThis, TechnoTypeClass* pType)
{
	auto ret = false;
	if (pType)
	{
		const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);

		// there is some another stupid bug
		// where if the building already queueed and paused
		// then you remove the prereq thing , the timer wont restart
		// it make it odd
		// also doing this check every frame is kind a weird ,..
		//if(!pTypeExt->Prerequisite_Display.empty()
		//	&& HouseExtData::PrereqValidate(pThis, pType , false , false ) == CanBuildResult::TemporarilyUnbuildable)
		//{
		//	R->EAX(true);
		//	return 0x50B669;
		//}

		auto const abs = pType->WhatAmI();
		auto const pFactory = pThis->GetPrimaryFactory(
			abs, pType->Naval, BuildCat::DontCare);

		// special logic for AirportBound
		if (abs == AbstractType::AircraftType)
		{
			auto const pAType = static_cast<AircraftTypeClass const*>(pType);
			if (pAType->AirportBound)
			{
				auto ownedAircraft = 0;
				auto queuedAircraft = 0;

				for (auto const& pAircraft : RulesClass::Instance->PadAircraft)
				{
					ownedAircraft += pThis->CountOwnedAndPresent(pAircraft);
					if (pFactory)
					{
						queuedAircraft += pFactory->CountTotal(pAircraft);
					}
				}

				// #896082: also check BuildLimit, and not always return the
				// result of this comparison directly. originally, it would
				// return false here, too, allowing more units than the
				// BuildLimit permitted.
				if (ownedAircraft + queuedAircraft >= pThis->AirportDocks)
				{
					return true;
				}
			}
		}

		auto queued = 0;
		if (pFactory)
		{
			queued = pFactory->CountTotal(pType);

			// #1286800: build limit > 1 and queues
			// the object in production is counted twice: it appears in this
			// factory queue, and it is already counted in the house's counters.
			// this only affects positive build limits, for negative ones
			// players could queue up one more than BuildLimit.
			if (auto const pObject = pFactory->Object)
			{
				if (pObject->GetType() == pType && pType->BuildLimit > 0)
				{
					--queued;
				}
			}
		}

		// #1521738: to stay consistent, use the new method to calculate this
		if (HouseExtData::BuildLimitRemaining(pThis, pType) - queued <= 0)
		{ ret = true; }
		else
		{
			const auto state = HouseExtData::HasFactory(pThis, pType, true, true, false, true);
			ret = (state.first < NewFactoryState::Available_Alternative);
		}
	}

	return ret;
}

CanBuildResult HouseExtData::BuildLimitGroupCheck(HouseClass* pThis, TechnoTypeClass* pItem, bool buildLimitOnly, bool includeQueued)
{
	auto pItemExt = TechnoTypeExtContainer::Instance.Find(pItem);

	if (pItemExt->BuildLimitGroup_Types.empty())
	{
		return CanBuildResult::Buildable;
	}

	const std::vector<int> limits = HouseExtData::GetBuildLimitGroupLimits(pThis, pItem);

	if (pItemExt->BuildLimitGroup_ContentIfAnyMatch.Get())
	{
		bool reachedLimit = false;

		for (size_t i = 0; i < MinImpl(pItemExt->BuildLimitGroup_Types.size(), pItemExt->BuildLimitGroup_Nums.size()); i++)
		{
			TechnoTypeClass* pType = pItemExt->BuildLimitGroup_Types[i];
			const auto pBuildingType = specific_cast<BuildingTypeClass*>(pType);
			const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
			int ownedNow = 0;

			if (pBuildingType && (BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
				ownedNow = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pThis));
			else
				ownedNow = CountOwnedIncludeDeploy(pThis, pType);

			if (ownedNow >= limits[i] + 1 - pItemExt->BuildLimitGroup_Factor)
				reachedLimit |= !(includeQueued && FactoryClass::FindByOwnerAndProduct(pThis, pType));
		}

		return reachedLimit ? CanBuildResult::TemporarilyUnbuildable : CanBuildResult::Buildable;
	}
	else
	{
		if (limits.size() == 1U)
		{
			int sum = 0;
			bool reachedLimit = false;

			for (auto& pType : pItemExt->BuildLimitGroup_Types)
			{
				const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
				const auto pBuildingType = specific_cast<BuildingTypeClass*>(pType);
				int owned = 0;

				if (pBuildingType && (BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
					owned = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pThis));
				else
					owned = CountOwnedIncludeDeploy(pThis, pType);

				sum += owned * pTypeExt->BuildLimitGroup_Factor;
			}

			if (sum >= limits[0] + 1 - pItemExt->BuildLimitGroup_Factor)
			{
				for (auto& pType : pItemExt->BuildLimitGroup_Types)
				{
					reachedLimit |= !(includeQueued && FactoryClass::FindByOwnerAndProduct(pThis, pType));
				}
			}

			return reachedLimit ? CanBuildResult::TemporarilyUnbuildable : CanBuildResult::Buildable;
		}
		else
		{
			for (size_t i = 0; i < MinImpl(pItemExt->BuildLimitGroup_Types.size(), limits.size()); i++)
			{
				TechnoTypeClass* pType = pItemExt->BuildLimitGroup_Types[i];
				const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
				const auto pBuildingType = specific_cast<BuildingTypeClass*>(pType);
				int ownedNow = 0;

				if (pBuildingType && (BuildingTypeExtContainer::Instance.Find(pBuildingType)->PowersUp_Buildings.size() > 0)
				|| BuildingTypeClass::Find(pBuildingType->PowersUpBuilding))
					ownedNow = BuildingTypeExtData::GetUpgradesAmount(pBuildingType, const_cast<HouseClass*>(pThis));
				else
					ownedNow = CountOwnedIncludeDeploy(pThis, pType);

				ownedNow *= pTypeExt->BuildLimitGroup_Factor;

				if ((pItem == pType && ownedNow < limits[i] + 1 - pItemExt->BuildLimitGroup_Factor) || includeQueued && FactoryClass::FindByOwnerAndProduct(pThis, pType))
					return CanBuildResult::Buildable;

				if ((pItem != pType && ownedNow < limits[i]) || includeQueued && FactoryClass::FindByOwnerAndProduct(pThis, pType))
					return CanBuildResult::Buildable;
			}

			return CanBuildResult::TemporarilyUnbuildable;
		}
	}
}

BuildLimitStatus HouseExtData::CheckBuildLimit(
	HouseClass const* const pHouse, TechnoTypeClass* pItem,
	bool const includeQueued)
{
	int BuildLimit = pItem->BuildLimit;
	int remaining = HouseExtData::BuildLimitRemaining(pHouse, pItem);

	if (BuildLimit > 0 && remaining <= 0)
	{
		return !includeQueued || !pHouse->GetFactoryProducing(pItem)
			? BuildLimitStatus::ReachedPermanently
			: BuildLimitStatus::NotReached;
	}

	return (remaining > 0)
		? BuildLimitStatus::NotReached
		: BuildLimitStatus::ReachedTemporarily
		;
}

signed int HouseExtData::BuildLimitRemaining(
	HouseClass const* const pHouse, TechnoTypeClass* pItem)
{
	int BuildLimit = pItem->BuildLimit;

	if (BuildLimit < 0)
	{
		return -(BuildLimit + pHouse->CountOwnedEver(pItem));
	}
	else
	{
		const auto cur = HouseExtData::CountOwnedNowTotal(pHouse, pItem);

		if (cur < 0)
			Debug::FatalError("%s for [%s - %x] CountOwned return less than 0 when counted\n", pItem->ID, pHouse->Type->ID, pHouse);

		return BuildLimit - cur;
	}
}

int HouseExtData::CountOwnedNowTotal(
	HouseClass const* const pHouse, TechnoTypeClass* pItem)
{
	switch (pItem->WhatAmI())
	{
	case AbstractType::BuildingType:
	{
		const BuildingTypeClass* pBType = static_cast<BuildingTypeClass const*>(pItem);
		const char* pPowersUp = pBType->PowersUpBuilding;
		int sum = 0;

		if (pPowersUp[0])
		{
			if (auto const pTPowersUp = BuildingTypeClass::Find(pPowersUp))
			{
				for (auto const& pBld : pHouse->Buildings)
				{
					if (pBld->Type == pTPowersUp)
					{
						for (auto const& pUpgrade : pBld->Upgrades)
						{
							if (pUpgrade == pBType)
							{
								++sum;
							}
						}
					}
				}
			}
		}
		else
		{
			sum = pHouse->CountOwnedNow(pBType);
			if (auto const pUndeploy = pBType->UndeploysInto)
			{
				sum += pHouse->CountOwnedNow(pUndeploy);
			}
		}

		return sum;
	}
	case AbstractType::UnitType:
	{
		const UnitTypeClass* pUType = static_cast<UnitTypeClass const*>(pItem);
		auto sum = pHouse->CountOwnedNow(pUType);

		if (auto const pDeploy = pUType->DeploysInto)
		{
			sum += pHouse->CountOwnedNow(pDeploy);
		}

		return sum;
	}
	case AbstractType::InfantryType:
	{
		const InfantryTypeClass* pIType = static_cast<InfantryTypeClass const*>(pItem);

		auto sum = pHouse->CountOwnedNow(pIType);

		if (pIType->VehicleThief)
		{
			auto index = pIType->ArrayIndex;

			for (auto pUnit : *UnitClass::Array)
			{
				if (pUnit->HijackerInfantryType == index
					&& pUnit->Owner == pHouse)
				{
					++sum;
				}
			}
		}

		return sum;
	}

	case AbstractType::AircraftType:
	{
		return  pHouse->CountOwnedNow(
			static_cast<AircraftTypeClass const*>(pItem));
	}
	default:
		break;
	}

	return 0;
}

void HouseExtData::UpdateTransportReloaders()
{
	for (auto& pTech : this->LimboTechno)
	{
		if (pTech->IsAlive
			&& pTech->WhatAmI() != AircraftClass::AbsID
			&& pTech->WhatAmI() != BuildingClass::AbsID
			&& pTech->Transporter && pTech->Transporter->IsInLogic)
		{
			const auto pType = pTech->GetTechnoType();
			if (pType->Ammo > 0 && TechnoTypeExtContainer::Instance.Find(pTech->GetTechnoType())->ReloadInTransport)
			{
				pTech->Reload();
			}
		}
	}
}

//void HouseExtData::AddToLimboTracking(TechnoTypeClass* pTechnoType)
//{
//	if (pTechnoType)
//	{
//		int arrayIndex = pTechnoType->GetArrayIndex();
//
//		switch (pTechnoType->WhatAmI())
//			// I doubt those in LimboDelete being really necessary, they're gonna be updated either next frame or after uninit anyway
//		{
//		case AbstractType::AircraftType:
//			this->LimboAircraft.Increment(arrayIndex);
//			break;
//		case AbstractType::BuildingType:
//			this->LimboBuildings.Increment(arrayIndex);
//			break;
//		case AbstractType::InfantryType:
//			this->LimboInfantry.Increment(arrayIndex);
//			break;
//		case AbstractType::UnitType:
//			this->LimboVehicles.Increment(arrayIndex);
//			break;
//		default:
//			break;
//		}
//	}
//}
//
//void HouseExtData::RemoveFromLimboTracking(TechnoTypeClass* pTechnoType)
//{
//	if (pTechnoType)
//	{
//		int arrayIndex = pTechnoType->GetArrayIndex();
//
//		switch (pTechnoType->WhatAmI())
//		{
//		case AbstractType::AircraftType:
//			this->LimboAircraft.Decrement(arrayIndex);
//			break;
//		case AbstractType::BuildingType:
//			this->LimboBuildings.Decrement(arrayIndex);
//			break;
//		case AbstractType::InfantryType:
//			this->LimboInfantry.Decrement(arrayIndex);
//			break;
//		case AbstractType::UnitType:
//			this->LimboVehicles.Decrement(arrayIndex);
//			break;
//		default:
//			break;
//		}
//	}
//}
//
//int HouseExtData::CountOwnedPresentAndLimboed(TechnoTypeClass* pTechnoType)
//{
//	int count = this->OwnerObject()->CountOwnedAndPresent(pTechnoType);
//	int arrayIndex = pTechnoType->GetArrayIndex();
//
//	switch (pTechnoType->WhatAmI())
//	{
//	case AbstractType::AircraftType:
//		count += this->LimboAircraft.GetItemCount(arrayIndex);
//		break;
//	case AbstractType::BuildingType:
//		count += this->LimboBuildings.GetItemCount(arrayIndex);
//		break;
//	case AbstractType::InfantryType:
//		count += this->LimboInfantry.GetItemCount(arrayIndex);
//		break;
//	case AbstractType::UnitType:
//		count += this->LimboVehicles.GetItemCount(arrayIndex);
//		break;
//	default:
//		break;
//	}
//
//	return count;
//}

// =============================
// load / save

template <typename T>
void HouseExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->Initialized)
		.Process(this->Degrades)
		.Process(this->PowerPlantEnhancerBuildings)
		.Process(this->Building_BuildSpeedBonusCounter)
		.Process(this->Building_OrePurifiersCounter)
		.Process(this->m_ForceOnlyTargetHouseEnemy)
		.Process(this->ForceOnlyTargetHouseEnemyMode)
		//.Process(this->RandomNumber)
		.Process(this->Factory_BuildingType)
		.Process(this->Factory_InfantryType)
		.Process(this->Factory_VehicleType)
		.Process(this->Factory_NavyType)
		.Process(this->Factory_AircraftType)
		.Process(this->AllRepairEventTriggered)
		.Process(this->LastBuildingTypeArrayIdx)
		.Process(this->RepairBaseNodes)
		.Process(this->LastBuiltNavalVehicleType)
		.Process(this->ProducingNavalUnitTypeIndex)

		.Process(this->AutoDeathObjects)
		.Process(this->LaunchDatas)
		.Process(this->CaptureObjectExecuted)
		.Process(this->DiscoverEvaDelay)

		.Process(this->Tunnels)
		.Process(this->Seed)

		.Process(this->SWLastIndex)
		.Process(this->Batteries)
		.Process(this->LimboTechno)
		.Process(this->AvaibleDocks)

		.Process(this->StolenTech)
		.Process(this->RadarPersist)
		.Process(this->FactoryOwners_GatheredPlansOf)
		.Process(this->Academies, true)
		.Process(this->Reversed)

		.Process(this->Is_NavalYardSpied)
		.Process(this->Is_AirfieldSpied)
		.Process(this->Is_ConstructionYardSpied)
		.Process(this->AuxPower)
		.Process(this->KeepAliveCount)
		.Process(this->KeepAliveBuildingCount)
		.Process(this->TiberiumStorage)

		.Process(this->SideTechTree)
		.Process(this->CombatAlertTimer)
		.Process(this->EMPulseWeaponIndex)
		.Process(this->RestrictedFactoryPlants)
		.Process(this->AISellAllDelayTimer)
		//.Process(this->BuiltAircraftTypes)
		//.Process(this->BuiltInfantryTypes)
		//.Process(this->BuiltUnitTypes)
		//.Process(this->BuiltBuildingTypes)
		//.Process(this->KilledAircraftTypes)
		//.Process(this->KilledInfantryTypes)
		//.Process(this->KilledUnitTypes)
		//.Process(this->KilledBuildingTypes)
		//.Process(this->CapturedBuildings)
		//.Process(this->CollectedCrates)

		.Process(this->OwnedDeployingUnits)
		.Process(this->BaseNormalCells)
		.Process(this->CurrentBuildingType)
		.Process(this->CurrentBuildingTopLeft)
		.Process(this->CurrentBuildingTimer)
		.Process(this->CurrentBuildingTimes)
		;
}

bool HouseExtContainer::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Process(HouseExtData::LastGrindingBlanceUnit)
		.Process(HouseExtData::LastGrindingBlanceInf)
		.Process(HouseExtData::LastHarvesterBalance)
		.Process(HouseExtData::LastSlaveBalance)
		.Process(HouseExtData::IsAnyFirestormActive)
		.Success();
}

bool HouseExtContainer::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Process(HouseExtData::LastGrindingBlanceUnit)
		.Process(HouseExtData::LastGrindingBlanceInf)
		.Process(HouseExtData::LastHarvesterBalance)
		.Process(HouseExtData::LastSlaveBalance)
		.Process(HouseExtData::IsAnyFirestormActive)
		.Success();
}

// =============================
// container

HouseExtContainer HouseExtContainer::Instance;

void HouseExtContainer::Clear()
{
	HouseExtData::AIProduction_CreationFrames.clear();
	HouseExtData::AIProduction_Values.clear();
	HouseExtData::AIProduction_BestChoices.clear();
	HouseExtData::AIProduction_BestChoicesNaval.clear();

	HouseExtData::LastGrindingBlanceUnit = 0;
	HouseExtData::LastGrindingBlanceInf = 0;
	HouseExtData::LastHarvesterBalance = 0;
	HouseExtData::LastSlaveBalance = 0;

	HouseExtData::CloakEVASpeak.Stop();
	HouseExtData::SubTerraneanEVASpeak.Stop();

	Civilian = 0;
	Special = 0;
	Neutral = 0;
}
// =============================
// container hooks

DEFINE_HOOK(0x4F6532, HouseClass_CTOR, 0x5)
{
	GET(HouseClass*, pItem, EAX);
	HouseExtContainer::Instance.Allocate(pItem);
	return 0;
}

DEFINE_HOOK(0x4F7186, HouseClass_DTOR, 0x8)
{
	GET(HouseClass*, pItem, ESI);
	HouseExtContainer::Instance.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x504080, HouseClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x503040, HouseClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(HouseClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);
	HouseExtContainer::Instance.PrepareStream(pItem, pStm);
	return 0;
}

DEFINE_HOOK(0x504069, HouseClass_Load_Suffix, 0x7)
{
	HouseExtContainer::Instance.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x5046DE, HouseClass_Save_Suffix, 0x7)
{
	HouseExtContainer::Instance.SaveStatic();
	return 0;
}

DEFINE_HOOK(0x50114D, HouseClass_InitFromINI, 0x5)
{
	GET(HouseClass* const, pThis, EBX);
	GET(CCINIClass* const, pINI, ESI);

	HouseExtContainer::Instance.LoadFromINI(pThis, pINI, false);

	return 0;
}

//DEFINE_HOOK(0x4FB9B7, HouseClass_Detach, 0xA)
//{
//	GET(HouseClass*, pThis, ECX);
//	GET_STACK(void*, target, STACK_OFFSET(0xC, 0x4));
//	GET_STACK(bool, all, STACK_OFFSET(0xC, 0x8));
//
//	HouseExtContainer::Instance.InvalidatePointerFor(pThis, target, all);
//
//	R->ESI(pThis);
//	R->EBX(0);
//	return pThis->ToCapture == target ?
//		0x4FB9C3 : 0x4FB9C9;
//}

void __fastcall HouseClass_Detach_Wrapper(HouseClass* pThis, DWORD, AbstractClass* target, bool all)
{
	HouseExtContainer::Instance.InvalidatePointerFor(pThis, target, all);
	pThis->HouseClass::PointerExpired(target, all);
}
DEFINE_JUMP(VTABLE, 0x7EA8C8, GET_OFFSET(HouseClass_Detach_Wrapper))