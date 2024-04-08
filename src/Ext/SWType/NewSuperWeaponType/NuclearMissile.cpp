#include "NuclearMissile.h"
#include <Ext/WeaponType/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Building/Body.h>
#include <New/Type/CursorTypeClass.h>

SuperWeaponTypeClass* SW_NuclearMissile::CurrentNukeType = nullptr;

std::vector<const char*> SW_NuclearMissile::GetTypeString() const
{
	return { "NewNuke" };
}

bool SW_NuclearMissile::HandleThisType(SuperWeaponType type) const
{
	return (type == SuperWeaponType::Nuke);
}

SuperWeaponFlags SW_NuclearMissile::Flags(const SWTypeExtData* pData) const
{
	return SuperWeaponFlags::NoEvent;
}

bool SW_NuclearMissile::Activate(SuperClass* const pThis, const CellStruct& Coords, bool const IsPlayer)
{
	if (pThis->IsCharged)
	{
		auto pType = pThis->Type;
		auto pData = SWTypeExtContainer::Instance.Find(pType);

		auto pCell = MapClass::Instance->GetCellAt(Coords);
		auto target = pCell->GetCoordsWithBridge();

		BuildingClass* pSilo = nullptr;

		if ((!pThis->Granted || !pThis->OneTime) && pData->Nuke_SiloLaunch)
		{
			pSilo = specific_cast<BuildingClass*>(this->GetFirer(pThis, Coords, false));
		}

		bool fired = false;
		if (pSilo)
		{
			pSilo->FiringSWType = pType->ArrayIndex;
			TechnoExtContainer::Instance.Find(pSilo)->LinkedSW = pThis;
			TechnoExtContainer::Instance.Find(pSilo)->SuperTarget = Coords;
			pThis->Owner->NukeTarget = Coords;

			pSilo->QueueMission(Mission::Missile, false);
			pSilo->NextMission();
			fired = true;
		}

		if (!fired && pData->Nuke_Payload)
		{
			fired = SW_NuclearMissile::DropNukeAt(pType, target, this->GetAlternateLauchSite(pData, pThis), pThis->Owner, pData->Nuke_Payload);
		}

		if (fired)
		{
			if (pData->SW_RadarEvent && pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer))
			{
				RadarEventClass::Create(RadarEventType::SuperweaponActivated, Coords);
			}

			VocClass::PlayAt(pData->SW_ActivationSound.Get(RulesClass::Instance->DigSound), target, nullptr);
			pThis->Owner->RecheckTechTree = true;
			return true;
		}
	}

	return false;
}

void SW_NuclearMissile::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action::Nuke;
	pData->Nuke_Payload = WeaponTypeClass::FindOrAllocate(GameStrings::NukePayload);
	pData->Nuke_PsiWarning = AnimTypeClass::Find(GameStrings::PSIWARN);

	pData->EVA_Detected = VoxClass::FindIndexById(GameStrings::EVA_NuclearSiloDetected());
	pData->EVA_Ready = VoxClass::FindIndexById(GameStrings::EVA_NuclearMissileReady);
	pData->EVA_Activated = VoxClass::FindIndexById(GameStrings::EVA_NuclearMissileLaunched());

	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::Nuke;
	pData->CursorType = static_cast<int>(MouseCursorType::Nuke);

	pData->CrateGoodies = true;
}

void SW_NuclearMissile::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->AttachedToObject->ID;

	INI_EX exINI(pINI);

	pData->Nuke_Payload.Read(exINI, section, "Nuke.Payload");
	pData->Nuke_TakeOff.Read(exINI, section, "Nuke.TakeOff");
	pData->Nuke_PsiWarning.Read(exINI, section, "Nuke.PsiWarning");
	pData->Nuke_SiloLaunch.Read(exINI, section, "Nuke.SiloLaunch");
}

bool SW_NuclearMissile::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	const auto pBldExt = BuildingExtContainer::Instance.Find(pBuilding);
	if (pBldExt->LimboID != -1 || !this->IsLaunchsiteAlive(pBuilding))
	{
		return false;
	}

	return pBuilding->Type->NukeSilo && this->IsSWTypeAttachedToThis(pData, pBuilding);
}

WarheadTypeClass* SW_NuclearMissile::GetWarhead(const SWTypeExtData* pData) const
{
	return pData->SW_Warhead.Get(nullptr) ? pData->SW_Warhead : (pData->Nuke_Payload ? pData->Nuke_Payload->Warhead : nullptr);
}

int SW_NuclearMissile::GetDamage(const SWTypeExtData* pData) const
{
	return (pData->SW_Damage.Get(-1) < 0) ? (pData->Nuke_Payload ? pData->Nuke_Payload->Damage : 0) : pData->SW_Damage.Get(-1);
}

BuildingClass* SW_NuclearMissile::GetAlternateLauchSite(const SWTypeExtData* pData, SuperClass* pThis) const
{
	for (auto& pBuilding : pThis->Owner->Buildings)
	{
		if (this->IsLaunchsiteAlive(pBuilding) && this->IsSWTypeAttachedToThis(pData, pBuilding))
		{
			return pBuilding;
		}
	}

	return nullptr;
}

bool SW_NuclearMissile::DropNukeAt(SuperWeaponTypeClass* pSuper, const CoordStruct& to, TechnoClass* Owner, HouseClass* OwnerHouse, WeaponTypeClass* pPayload)
{
	if (!pPayload->Projectile)
	{
		return false;
	}

	const auto pCell = MapClass::Instance->GetCellAt(to);
	auto pBullet = GameCreate<BulletClass>();

	pBullet->Construct(pPayload->Projectile, pCell, Owner, 0, nullptr, pPayload->Speed, false);
	pBullet->SetWeaponType(pPayload);

	int Damage = pPayload->Damage;
	WarheadTypeClass* pWarhead = pPayload->Warhead;

	if (pSuper)
	{
		auto pData = SWTypeExtContainer::Instance.Find(pSuper);

		BulletClass::CreateDamagingBulletAnim(OwnerHouse, pCell, pBullet, pData->Nuke_PsiWarning);

		auto pNewType = NewSWType::GetNewSWType(pData);
		Damage = pNewType->GetDamage(pData);
		pWarhead = pNewType->GetWarhead(pData);

		BulletExtContainer::Instance.Find(pBullet)->NukeSW = pSuper;
	}

	pBullet->Health = Damage;
	pBullet->WH = pWarhead;
	pBullet->Bright = pPayload->Bright || pWarhead->Bright;
	pBullet->Range = WeaponTypeExtContainer::Instance.Find(pPayload)->GetProjectileRange();

	if (!Owner)
	{
		BulletExtContainer::Instance.Find(pBullet)->Owner = OwnerHouse;
	}

	return pBullet->MoveTo(to + CoordStruct { 0, 0, pPayload->Projectile->DetonationAltitude }, { 0.0, 0.0, -100.0 });
}