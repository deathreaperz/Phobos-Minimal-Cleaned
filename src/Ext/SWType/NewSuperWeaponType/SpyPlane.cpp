#include "SpyPlane.h"

#include <Ext/Techno/Body.h>

std::vector<const char*> SW_SpyPlane::GetTypeString() const
{
	return { "Airstrike" };
}

bool SW_SpyPlane::HandleThisType(SuperWeaponType type) const
{
	return (type == SuperWeaponType::SpyPlane);
}

bool SW_SpyPlane::Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer)
{
	SuperWeaponTypeClass* pSW = pThis->Type;
	SWTypeExtData* pData = SWTypeExtContainer::Instance.Find(pSW);

	if (!pThis->IsCharged)
	{
		return false;
	}

	CellClass* pTarget = MapClass::Instance->GetCellAt(Coords);

	if (!pTarget)
	{
		Debug::Log("SpyPlane [%s] SW Invalid Target!\n", pThis->get_ID());
		return false;
	}

	const auto Default = HouseExtData::GetSpyPlane(pThis->Owner);

	const auto& PlaneIdxes = pData->SpyPlanes_TypeIndex;
	const auto& PlaneCounts = pData->SpyPlanes_Count;
	const auto& PlaneMissions = pData->SpyPlanes_Mission;
	const auto& PlaneRank = pData->SpyPlanes_Rank;

	const size_t nSize = PlaneIdxes.size();

	for (size_t idx = 0; idx < nSize; idx++)
	{
		const int Amount = (idx < PlaneCounts.size()) ? PlaneCounts[idx] : 1;
		const Mission Mission = (idx < PlaneMissions.size()) ? PlaneMissions[idx] : Mission::SpyplaneApproach;
		const Rank Rank = (idx < PlaneRank.size()) ? PlaneRank[idx] : Rank::Rookie;
		const auto Plane = (idx < PlaneIdxes.size()) ? PlaneIdxes[idx] : Default;

		if (Plane && Plane->Strength != 0)
		{
			TechnoExtData::SendPlane(Plane, Amount, pThis->Owner, Rank, Mission, pTarget, nullptr);
		}
	}

	return true;
}

void SW_SpyPlane::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action::SpyPlane;
	pData->SW_RadarEvent = false;
	pData->EVA_Ready = VoxClass::FindIndexById(GameStrings::EVA_SpyPlaneReady);
	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::ParaDrop;
	pData->CursorType = static_cast<int>(MouseCursorType::SpyPlane);
}

void SW_SpyPlane::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->AttachedToObject->ID;

	INI_EX exINI(pINI);

	pData->SpyPlanes_TypeIndex.Read(exINI, section, "SpyPlane.Type");
	pData->SpyPlanes_Count.Read(exINI, section, "SpyPlane.Count");
	pData->SpyPlanes_Mission.Read(exINI, section, "SpyPlane.Mission");
	pData->SpyPlanes_Rank.Read(exINI, section, "SpyPlane.Rank");
}

bool SW_SpyPlane::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!this->IsLaunchsiteAlive(pBuilding))
	{
		return false;
	}

	if (!pData->SW_Lauchsites.empty() && pData->SW_Lauchsites.Contains(pBuilding->Type))
	{
		return true;
	}

	return this->IsSWTypeAttachedToThis(pData, pBuilding);
}