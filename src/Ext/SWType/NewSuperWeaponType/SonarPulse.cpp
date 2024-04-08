#include "SonarPulse.h"

#include <Utilities/Helpers.h>
#include <Ext/Techno/Body.h>

std::vector<const char*> SW_SonarPulse::GetTypeString() const
{
	return { "SonarPulse" };
}

SuperWeaponFlags SW_SonarPulse::Flags(const SWTypeExtData* pData) const
{
	return (this->GetRange(pData).WidthOrRange > 0) ? SuperWeaponFlags::None : SuperWeaponFlags::NoEvent;
}

bool SW_SonarPulse::Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer)
{
	auto const pType = pThis->Type;
	auto const pData = SWTypeExtContainer::Instance.Find(pType);

	auto Detect = [pThis, pData](TechnoClass* const pTechno) -> bool
		{
			if (!pData->IsHouseAffected(pThis->Owner, pTechno->Owner) || !pData->IsTechnoAffected(pTechno))
			{
				return true;
			}

			auto& nTime = TechnoExtContainer::Instance.Find(pTechno)->CloakSkipTimer;

			nTime.Start(std::max(nTime.GetTimeLeft(), pData->Sonar_Delay.Get()));

			if (pTechno->CloakState != CloakState::Uncloaked)
			{
				pTechno->Uncloak(true);
				pTechno->NeedsRedraw = true;
			}

			return true;
		};

	auto const range = this->GetRange(pData);

	if (range.WidthOrRange < 0)
	{
		for (auto const pTechno : *TechnoClass::Array)
		{
			Detect(pTechno);
		}
	}
	else
	{
		Helpers::Alex::DistinctCollector<TechnoClass*> items;
		Helpers::Alex::for_each_in_rect_or_range<TechnoClass>(Coords, range.WidthOrRange, range.Height, items);
		items.apply_function_for_each(Detect);

		if (pData->SW_RadarEvent)
		{
			RadarEventClass::Create(RadarEventType::SuperweaponActivated, Coords);
		}
	}

	return true;
}

void SW_SonarPulse::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action(AresNewActionType::SuperWeaponAllowed);
	pData->SW_RadarEvent = false;
	pData->Sonar_Delay = 60;
	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::Stealth;
	pData->SW_AffectsHouse = AffectedHouse::Enemies;
	pData->SW_AffectsTarget = SuperWeaponTarget::Water;
	pData->SW_RequiresTarget = SuperWeaponTarget::Water;
	pData->SW_AIRequiresTarget = SuperWeaponTarget::Water;
}

void SW_SonarPulse::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->get_ID();
	pData->Sonar_Delay = pINI->ReadInteger(section, "SonarPulse.Delay", pData->Sonar_Delay);
	pData->AttachedToObject->Action = (GetRange(pData).WidthOrRange < 0) ? Action::None : Action(AresNewActionType::SuperWeaponAllowed);
}

bool SW_SonarPulse::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!this->IsLaunchsiteAlive(pBuilding))
		return false;

	return (!pData->SW_Lauchsites.empty() && pData->SW_Lauchsites.Contains(pBuilding->Type)) || this->IsSWTypeAttachedToThis(pData, pBuilding);
}

SWRange SW_SonarPulse::GetRange(const SWTypeExtData* pData) const
{
	return pData->SW_Range->empty() ? SWRange { 10 } : pData->SW_Range.Get();
}