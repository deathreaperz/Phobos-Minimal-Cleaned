#include "Body.h"

#include <Ext/BulletType/Body.h>

#include <Utilities/Macro.h>

// This broke aircraft targeting , disable it for now - Otama

// Ares reimplements the bullet obstacle logic so need to get creative to add any new functionality for that in Phobos.
// Not named PhobosTrajectoryHelper to avoid confusion with actual custom trajectory logic.
class BulletObstacleHelper
{
public:

	static CellClass* GetObstacle(CellClass* pSourceCell, CellClass* pTargetCell, CellClass* pCurrentCell, CoordStruct currentCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, BulletTypeClass* pBulletType, bool isTargetingCheck = false)
	{
		CellClass* pObstacleCell = nullptr;

		if (BulletObstacleHelper::SubjectToObstacles(pBulletType))
		{
			if (BulletObstacleHelper::SubjectToTerrain(pCurrentCell, pBulletType, isTargetingCheck))
				pObstacleCell = pCurrentCell;
		}

		return pObstacleCell;
	}

	static CellClass* FindFirstObstacle(CoordStruct const& pSourceCoords, CoordStruct const& pTargetCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, BulletTypeClass* pBulletType, bool isTargetingCheck = false)
	{
		if (BulletObstacleHelper::SubjectToObstacles(pBulletType))
		{
			auto sourceCell = CellClass::Coord2Cell(pSourceCoords);
			auto const pSourceCell = MapClass::Instance->GetCellAt(sourceCell);
			auto targetCell = CellClass::Coord2Cell(pTargetCoords);
			auto const pTargetCell = MapClass::Instance->GetCellAt(targetCell);

			auto const sub = sourceCell - targetCell;
			CellStruct const delta { (short)std::abs(sub.X), (short)std::abs(sub.Y) };
			auto const maxDelta = static_cast<size_t>(MaxImpl(delta.X, delta.Y));
			auto const step = !maxDelta ? CoordStruct::Empty : (pTargetCoords - pSourceCoords) * (1.0 / maxDelta);
			CoordStruct crdCur = pSourceCoords;
			auto pCellCur = pSourceCell;

			for (size_t i = 0; i < (maxDelta + (size_t)isTargetingCheck); ++i)
			{
				if (auto const pCell = BulletObstacleHelper::GetObstacle(pSourceCell, pTargetCell, pCellCur, crdCur, pSource, pTarget, pOwner, pBulletType, isTargetingCheck))
					return pCell;

				crdCur += step;
				pCellCur = MapClass::Instance->GetCellAt(crdCur);
			}
		}

		return nullptr;
	}

	static CellClass* FindFirstImpenetrableObstacle(CoordStruct const& pSourceCoords, CoordStruct const& pTargetCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, WeaponTypeClass* pWeapon, bool isTargetingCheck = false)
	{
		// Does not currently need further checks.
		return BulletObstacleHelper::FindFirstObstacle(pSourceCoords, pTargetCoords, pSource, pTarget, pOwner, pWeapon->Projectile, isTargetingCheck);
	}

	static bool SubjectToObstacles(BulletTypeClass* pBulletType)
	{
		const auto pBulletTypeExt = BulletTypeExtContainer::Instance.Find(pBulletType);
		const bool subjectToTerrain = pBulletTypeExt->SubjectToLand.isset() || pBulletTypeExt->SubjectToWater.isset();

		return subjectToTerrain ? true : pBulletType->Level;
	}

	static bool SubjectToTerrain(CellClass* pCurrentCell, BulletTypeClass* pBulletType, bool isTargetingCheck)
	{
		const bool isCellWater = (pCurrentCell->LandType == LandType::Water || pCurrentCell->LandType == LandType::Beach) && pCurrentCell->ContainsBridge();
		const bool isLevel = pBulletType->Level ? pCurrentCell->IsOnFloor() : false;
		const auto pBulletTypeExt = BulletTypeExtContainer::Instance.Find(pBulletType);

		if (!isTargetingCheck && isLevel && !pBulletTypeExt->SubjectToLand.isset() && !pBulletTypeExt->SubjectToWater.isset())
			return true;
		else if (!isCellWater && pBulletTypeExt->SubjectToLand.Get(false))
			return !isTargetingCheck ? pBulletTypeExt->SubjectToLand_Detonate : true;
		else if (isCellWater && pBulletTypeExt->SubjectToWater.Get(false))
			return !isTargetingCheck ? pBulletTypeExt->SubjectToWater_Detonate : true;

		return false;
	}
};

// Hooks

DEFINE_HOOK(0x4688A9, BulletClass_Unlimbo_Obstacles, 0x6)
{
	enum { SkipGameCode = 0x468A3F, Continue = 0x4688BD };

	GET(BulletClass*, pThis, EBX);
	GET(CoordStruct const* const, sourceCoords, EDI);
	REF_STACK(CoordStruct const, targetCoords, STACK_OFFSET(0x54, -0x10));

	if (pThis->Type->Inviso)
	{
		auto const pOwner = pThis->Owner ? pThis->Owner->Owner : BulletExtContainer::Instance.Find(pThis)->Owner;
		const auto pObstacleCell = BulletObstacleHelper::FindFirstObstacle(*sourceCoords, targetCoords, pThis->Owner, pThis->Target, pOwner, pThis->Type, false);

		if (pObstacleCell)
		{
			pThis->SetLocation(pObstacleCell->GetCoords());
			pThis->Speed = 0;
			pThis->Velocity = { 0,0,0 };

			return SkipGameCode;
		}

		return Continue;
	}

	return 0;
}

DEFINE_HOOK(0x468C86, BulletClass_ShouldExplode_Obstacles, 0xA)
{
	enum { SkipGameCode = 0x468C90, Explode = 0x468C9F };

	GET(BulletClass*, pThis, ESI);

	if (BulletObstacleHelper::SubjectToObstacles(pThis->Type))
	{
		auto const pCellSource = MapClass::Instance->GetCellAt(pThis->SourceCoords);
		auto const pCellTarget = MapClass::Instance->GetCellAt(pThis->TargetCoords);
		auto const pCellCurrent = MapClass::Instance->GetCellAt(pThis->LastMapCoords);
		auto const pOwner = pThis->Owner ? pThis->Owner->Owner : BulletExtContainer::Instance.Find(pThis)->Owner;
		const auto pObstacleCell = BulletObstacleHelper::GetObstacle(pCellSource, pCellTarget, pCellCurrent, pThis->Location, pThis->Owner, pThis->Target, pOwner, pThis->Type, false);

		if (pObstacleCell)
			return Explode;
	}

	// Restore overridden instructions.
	R->EAX(pThis->GetHeight());
	return SkipGameCode;
}

#include <Ext/TechnoType/Body.h>
#include <Ext/WeaponType/Body.h>

DEFINE_HOOK(0x6F7248, TechnoClass_InRange_Additionals, 0x6)
{
	enum { ContinueCheck = 0x6F72E3, RetTrue = 0x6F7256, RetFalse = 0x6F7655 };

	GET(TechnoClass*, pThis, ESI);
	GET(AbstractClass*, pTarget, ECX);
	GET(WeaponTypeClass*, pWeapon, EBX);

	if (!pTarget || !pWeapon)
		return RetFalse;

	int range = WeaponTypeExtData::GetRangeWithModifiers(pWeapon, pThis);

	if (range == -512)
		return RetTrue;

	const auto pThisTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());

	if (pThisTypeExt->NavalRangeBonus.isset())
	{
		if (auto const pFoot = abstract_cast<FootClass* const>(pTarget))
		{
			if (pFoot->GetTechnoType()->Naval)
			{
				const auto pFootCell = pFoot->GetCell();
				if (pFootCell->LandType == LandType::Water && !pFootCell->ContainsBridge())
					range += pThisTypeExt->NavalRangeBonus.Get();
			}
		}
	}

	if (pTarget->IsInAir())
		range += pThisTypeExt->AttachedToObject->AirRangeBonus;

	if (pThis->BunkerLinkedItem && pThis->BunkerLinkedItem->WhatAmI() != AbstractType::Building)
		range += RulesClass::Instance->BunkerWeaponRangeBonus * Unsorted::LeptonsPerCell;

	if (pThis->Transporter)
	{
		range += TechnoTypeExtContainer::Instance.Find(pThis->Transporter->GetTechnoType())
			->OpenTopped_RangeBonus.Get(RulesClass::Instance->OpenToppedRangeBonus) * Unsorted::LeptonsPerCell;
	}

	R->EDI(range);
	return ContinueCheck;
}

DEFINE_HOOK(0x6FC3A1, TechnoClass_CanFire_InBunkerRangeCheck, 0x5)
{
	enum { ContinueChecks = 0x6FC3C5, CannotFire = 0x6FC86A };

	GET(TechnoClass*, pTarget, EBP);
	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);

	if (pTarget->WhatAmI() == AbstractType::Unit && WeaponTypeExtData::GetRangeWithModifiers(pWeapon, pThis) < 384.0)
		return CannotFire;

	return ContinueChecks;
}

DEFINE_HOOK(0x70CF6F, TechnoClass_ThreatCoefficients_WeaponRange, 0x6)
{
	enum { SkipGameCode = 0x70CF75 };

	GET(TechnoClass*, pThis, EDI);
	GET(WeaponTypeClass*, pWeapon, EBX);

	R->EAX(WeaponTypeExtData::GetRangeWithModifiers(pWeapon, pThis));

	return SkipGameCode;
}

DEFINE_HOOK(0x41810F, AircraftClass_MissionAttack_WeaponRangeCheck1, 0x6)
{
	enum { WithinDistance = 0x418117, NotWithinDistance = 0x418131 };

	GET(AircraftClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EDI);
	GET(int, distance, EAX);

	int range = WeaponTypeExtData::GetRangeWithModifiers(pWeapon, pThis);

	if (distance < range)
		return WithinDistance;

	return NotWithinDistance;
}

DEFINE_HOOK(0x418BA8, AircraftClass_MissionAttack_WeaponRangeCheck2, 0x6)
{
	enum { SkipGameCode = 0x418BAE };

	GET(AircraftClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EAX);

	R->EAX(WeaponTypeExtData::GetRangeWithModifiers(pWeapon, pThis));

	return SkipGameCode;
}

DEFINE_HOOK(0x6F7647, TechnoClass_InRange_Obstacles, 0x5)
{
	GET_STACK(TechnoClass*, pThis, 0xC);
	GET_BASE(WeaponTypeClass*, pWeapon, 0x10);
	GET(CoordStruct const* const, pSourceCoords, ESI);
	REF_STACK(CoordStruct const, targetCoords, STACK_OFFSET(0x3C, -0x1C));
	GET_BASE(AbstractClass* const, pTarget, 0xC);
	GET(CellClass*, pResult, EAX);

	if (!pResult) // disable it for in air stuffs for now , broke some Aircraft targeting
		pResult = BulletObstacleHelper::FindFirstImpenetrableObstacle(*pSourceCoords, targetCoords, pThis, pTarget, pThis->Owner, pWeapon, true);

	R->EAX(pResult);
	return 0;
}

// Skip a forced detonation check for Level=true projectiles that is now handled in Hooks.Obstacles.cpp.
DEFINE_JUMP(LJMP, 0x468D08, 0x468D2F);
//DEFINE_SKIP_HOOK(0x468D08 , BulletClass_IsForceToExplode_SkipLevelCheck , 0x6 , 468D2F);