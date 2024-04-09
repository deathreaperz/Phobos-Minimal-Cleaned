#include "EMPulse.h"

#include <Utilities/Helpers.h>

#include <Ext/Building/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/BulletType/Body.h>

std::vector<const char*> SW_EMPulse::GetTypeString() const
{
	return { "EMPulse" , "ChemLauncher" , "MultiLauncher" , "FireAt" };
}

bool SW_EMPulse::Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer)
{
	auto pData = SWTypeExtContainer::Instance.Find(pThis->Type);
	pThis->Owner->EMPTarget = Coords;

	const auto Count = static_cast<size_t>(pData->SW_MaxCount);

	const bool ignoreRange = pData->EMPulse_Linked || pData->EMPulse_TargetSelf;

	auto IsEligible = [=](BuildingClass* pBld)
	{
		return this->IsLaunchSiteEligible(pData, Coords, pBld, ignoreRange);
	};

	Helpers::Alex::for_each_if_n(pThis->Owner->Buildings.begin(), pThis->Owner->Buildings.end(),
		Count, IsEligible, [=](BuildingClass* pBld)
		{
			if (!pData->EMPulse_TargetSelf)
			{
				auto pBldExt = TechnoExtContainer::Instance.Find(pBld);
				pBldExt->SuperTarget = Coords;
				pBldExt->LinkedSW = pThis;
				pBld->FiringSWType = pThis->Type->ArrayIndex;
				pBld->QueueMission(Mission::Missile, false);
				pBld->NextMission();
			}
			else
			{
				auto pWeapon = pBld->GetWeapon(0)->WeaponType;
				if (pWeapon)
				{
					auto pBulletTypeExt = BulletTypeExtContainer::Instance.Find(pWeapon->Projectile);
					auto pBullet = pBulletTypeExt->CreateBullet(pBld, pBld, pWeapon, false, true);
					if (pBullet)
					{
						pBullet->Limbo();
						pBullet->Detonate(BuildingExtData::GetCenterCoords(pBld));
						pBullet->Release();
					}
				}
			}
		});

	return true;
}

void SW_EMPulse::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action(AresNewActionType::SuperWeaponAllowed);
	pData->SW_RangeMaximum = -1.0;
	pData->SW_RangeMinimum = 0.0;
	pData->SW_MaxCount = 1;

	pData->EMPulse_Linked = false;
	pData->EMPulse_TargetSelf = false;

	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::None;
	pData->CursorType = static_cast<int>(MouseCursorType::Attack);
	pData->NoCursorType = static_cast<int>(MouseCursorType::AttackOutOfRange);
}

void SW_EMPulse::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->get_ID();

	INI_EX exINI(pINI);

	pData->EMPulse_Linked.Read(exINI, section, "EMPulse.Linked");
	pData->EMPulse_TargetSelf.Read(exINI, section, "EMPulse.TargetSelf");
	pData->EMPulse_PulseDelay.Read(exINI, section, "EMPulse.PulseDelay");
	pData->EMPulse_PulseBall.Read(exINI, section, "EMPulse.PulseBall");
	pData->EMPulse_Cannons.Read(exINI, section, "EMPulse.Cannons");

	pData->AttachedToObject->Action = pData->EMPulse_TargetSelf ? Action::None : (Action)AresNewActionType::SuperWeaponAllowed;
}

bool SW_EMPulse::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	const auto pBldExt = BuildingExtContainer::Instance.Find(pBuilding);
	if (pBldExt->LimboID != -1)
		return false;

	if (!this->IsLaunchsiteAlive(pBuilding))
		return false;

	if (!pData->EMPulse_Cannons.empty() && pData->EMPulse_Cannons.Contains(pBuilding->Type))
	{
		return true;
	}

	return pBuilding->Type->EMPulseCannon && this->IsSWTypeAttachedToThis(pData, pBuilding);
}

std::pair<double, double> SW_EMPulse::GetLaunchSiteRange(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (pData->EMPulse_TargetSelf)
	{
		return { -1.0, -1.0 };
	}

	if (!pBuilding)
	{
		return { 0.0, 0.0 };
	}

	auto pWeap = pBuilding->GetWeapon(0)->WeaponType;
	if (pWeap)
	{
		double maxRange = pData->SW_RangeMaximum;
		if (maxRange < 0.0)
		{
			maxRange = pWeap->Range / 256.0;
		}

		double minRange = pData->SW_RangeMinimum;
		if (minRange < 0.0)
		{
			minRange = pWeap->MinimumRange / 256.0;
		}

		return { minRange, maxRange };
	}

	return NewSWType::GetLaunchSiteRange(pData, pBuilding);
}