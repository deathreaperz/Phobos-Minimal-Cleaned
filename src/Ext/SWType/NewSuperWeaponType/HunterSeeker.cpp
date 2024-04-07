#include "HunterSeeker.h"

#include <Ext/Techno/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/Rules/Body.h>

std::vector<const char*> SW_HunterSeeker::GetTypeString() const
{
	return { "HunterSeeker" };
}

bool SW_HunterSeeker::Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer)
{
	HouseClass* pOwner = pThis->Owner;
	auto pExt = SWTypeExtContainer::Instance.Find(pThis->Type);

	UnitTypeClass* pType = pExt->HunterSeeker_Type.Get(HouseExtData::GetHunterSeeker(pOwner));

	if (!pType)
	{
		Debug::Log("HunterSeeker super weapon \"%s\" could not be launched. "
				   "No HunterSeeker unit type set for house \"%ls\".\n", pThis->Type->ID, pOwner->UIName);
		return false;
	}

	const size_t Count = (pExt->SW_MaxCount >= 0) ? static_cast<size_t>(pExt->SW_MaxCount) : std::numeric_limits<size_t>::max();

	size_t Success = 0;
	for (auto& pBld : pOwner->Buildings)
	{
		if (Success >= Count)
			break;

		if (!IsLaunchSite_HS(pExt, pBld))
			continue;

		CellStruct cell = GetLaunchCell(pExt, pBld, pType);

		if (cell == CellStruct::Empty)
			continue;

		if (auto pHunter = static_cast<UnitClass*>(pType->CreateObject(pOwner)))
		{
			TechnoExtContainer::Instance.Find(pHunter)->LinkedSW = pThis;

			if (pHunter->Unlimbo(CellClass::Cell2Coord(cell), DirType::East))
			{
				pHunter->Locomotor->Acquire_Hunter_Seeker_Target();
				pHunter->QueueMission((pHunter->Type->Harvester || pHunter->Type->ResourceGatherer) ? Mission::Area_Guard : Mission::Attack, false);
				pHunter->NextMission();
				++Success;
			}
			else
			{
				GameDelete<true, false>(pHunter);
			}
		}
	}

	if (!Success)
	{
		Debug::Log("HunterSeeker super weapon \"%s\" could not be launched. House \"%ls\" "
				   "does not own any HSBuilding or No Buildings attached with this HSType Superweapon.\n", pThis->Type->ID, pOwner->UIName);
	}

	return Success != 0;
}

void SW_HunterSeeker::Initialize(SWTypeExtData* pData)
{	// Defaults to HunterSeeker values
	pData->SW_MaxCount = 1;

	pData->EVA_Detected = VoxClass::FindIndexById("EVA_HunterSeekerDetected");
	pData->EVA_Ready = VoxClass::FindIndexById("EVA_HunterSeekerReady");
	pData->EVA_Activated = VoxClass::FindIndexById("EVA_HunterSeekerLaunched");

	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::HunterSeeker;
	pData->SW_AffectsHouse = AffectedHouse::Enemies;

	pData->Text_Ready = CSFText("TXT_RELEASE");

	// hardcoded
	pData->AttachedToObject->Action = Action::None;
	pData->SW_RadarEvent = false;
}

void SW_HunterSeeker::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->get_ID();

	INI_EX exINI(pINI);

	pData->HunterSeeker_Type.Read(exINI, section, "HunterSeeker.Type");
	pData->HunterSeeker_RandomOnly.Read(exINI, section, "HunterSeeker.RandomOnly");
	pData->HunterSeeker_Buildings.Read(exINI, section, "HunterSeeker.Buildings");
	pData->HunterSeeker_AllowAttachedBuildingAsFallback.Read(exINI, section, "HunterSeeker.AllowAttachedBuildingAsFallback");

	// hardcoded
	pData->AttachedToObject->Action = Action::None;
	pData->SW_RadarEvent = false;
}

bool SW_HunterSeeker::IsLaunchSite_HS(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!pData->HunterSeeker_Buildings.empty())
	{
		for (BuildingTypeClass* bldType : pData->HunterSeeker_Buildings)
		{
			if (bldType == pBuilding->Type)
			{
				return true;
			}
		}
		return false;
	}

	if (pData->HunterSeeker_AllowAttachedBuildingAsFallback)
	{
		return IsSWTypeAttachedToThis(pData, pBuilding);
	}

	return false;
}

bool SW_HunterSeeker::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!this->IsLaunchsiteAlive(pBuilding))
		return false;

	return this->IsLaunchSite_HS(pData, pBuilding);
}

CellStruct SW_HunterSeeker::GetLaunchCell(SWTypeExtData* pSWType, BuildingClass* pBuilding, UnitTypeClass* pHunter) const
{
	CellStruct cell;

	if (BuildingExtContainer::Instance.Find(pBuilding)->LimboID != -1)
	{
		const auto edge = pBuilding->Owner->GetHouseEdge();
		cell = MapClass::Instance->PickCellOnEdge(edge, CellStruct::Empty, CellStruct::Empty, SpeedType::Foot, true, MovementZone::Normal);
	}
	else
	{
		auto position = CellClass::Coord2Cell(pBuilding->GetCoords());
		cell = MapClass::Instance->NearByLocation(position, SpeedType::Foot, -1, MovementZone::Normal, false, 1, 1, false, false, false, true, CellStruct::Empty, false, false);
	}

	return MapClass::Instance->IsWithinUsableArea(cell, true) ? cell : CellStruct::Empty;
}