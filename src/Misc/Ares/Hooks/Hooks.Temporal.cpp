#include <AbstractClass.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <Utilities/Macro.h>
#include <Helpers/Macro.h>
#include <Base/Always.h>

#include <HouseClass.h>
#include <Utilities/Debug.h>

#include <HoverLocomotionClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/AnimType/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/VoxelAnim/Body.h>
#include <Ext/Terrain/Body.h>
#include <Ext/Techno/Body.h>

#include <TerrainTypeClass.h>
#include <New/Type/ArmorTypeClass.h>
#include <Misc/AresData.h>
#include <Notifications.h>

// TODO :
#define Ares_TemporalWeapon(var) (*(BYTE*)(((char*)GetAresTechnoExt(var)) + 0xA))
#define Ares_AboutToChronoshift(var) (*(bool*)(((char*)GetAresBuildingExt(var)) + 0xD))

// TODO : still not correct !
//DEFINE_HOOK(718279, TeleportLocomotionClass_MakeRoom, 5)
//{
//	GET(CoordStruct*, pCoord, EAX);
//	GET(TeleportLocomotionClass*, pLoco, EBP);
//
//	auto const pLinked = pLoco->LinkedTo;
//	auto const pLinkedIsInf = pLinked->WhatAmI() == AbstractType::Infantry;
//	auto const pCell = Map.TryGetCellAt(*pCoord);
//
//	R->Stack(0x48, false);
//	R->EBX(pCell->OverlayTypeIndex);
//	R->EDI(0);
//
//	for (NextObject obj(pCell->GetContent()); obj; ++obj)
//	{
//
//		auto const pObj = (*obj);
//		auto const bIsObjFoot = pObj->AbstractFlags & AbstractFlags::Foot;
//		auto const pObjIsInf = pObj->WhatAmI() == AbstractType::Infantry;
//		auto bIsObjectInvicible = pObj->IsIronCurtained();
//		auto const pType = pObj->GetTechnoType();
//		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
//
//		if (pType && !pTypeExt->Chronoshift_Crushable)
//			bIsObjectInvicible = true;
//
//		if (!bIsObjectInvicible && pObjIsInf && pLinkedIsInf)
//		{
//			auto const bEligible = pLinked->Owner && !pLinked->Owner->IsAlliedWith(pObj);
//			auto const pAttackerHouse = bEligible ? pLinked->Owner : nullptr;
//			auto const pAttackerTechno = bEligible ? pLinked : nullptr;
//
//			auto nCoord = pObj->GetCoords();
//			if (nCoord == *pCoord)
//			{
//				auto nDamage = pObj->Health;
//				pObj->ReceiveDamage(&nDamage, 0, RulesClass::Instance->C4Warhead, pAttackerTechno, true, false, pAttackerHouse);
//			}
//		}
//		else if (bIsObjectInvicible || !bIsObjFoot)
//		{
//			if (bIsObjectInvicible)
//			{
//				auto const pObjHouse = pObj->GetOwningHouse();
//				auto const pAttackerHouse = pObjHouse && !pObjHouse->IsAlliedWith(pObj) ? pObjHouse : nullptr;
//				auto const pAttackerTechno = reinterpret_cast<TechnoClass*>(pObj);
//
//				auto nDamage = pLinked->Health;
//				pLinked->ReceiveDamage(&nDamage, 0, RulesClass::Instance->C4Warhead, pAttackerTechno, true, false, pAttackerHouse);
//			}
//			else if (!bIsObjFoot)
//			{
//				R->Stack(0x48, true);
//			}
//		}
//		else
//		{
//
//			auto const bEligible = pLinked->Owner && !pLinked->Owner->IsAlliedWith(pObj);
//			auto const pAttackerHouse = bEligible ? pLinked->Owner : nullptr;
//			auto const pAttackerTechno = bEligible ? pLinked : nullptr;
//			auto nDamage = pObj->Health;
//			pObj->ReceiveDamage(&nDamage, 0, RulesClass::Instance->C4Warhead, pAttackerTechno, true, false, pAttackerHouse);
//		}
//	}
//
//	auto const nFlag300 = CellFlags::Bridge | CellFlags::Unknown_200;
//	if ((pCell->Flags & nFlag300) == CellFlags::Bridge)
//		R->Stack(0x48, true);
//
//	R->Stack(0x20, pLinked->GetMapCoords());
//	R->EAX(true);
//
//	return 0x7184CE;
//}

//TODO :
// This fuckery need more than this 
// need to port Bulletclass::Detonate stuffs 
// otherwise it will cause some inconsistency
//DEFINE_HOOK(71AAAC, TemporalClass_Update_Abductor, 6)
//{
//	GET(TemporalClass*, pThis, ESI);
//
//	const auto pOwner = pThis->Owner;
//	const auto pTempExt = TemporalExt::ExtMap.Find(pThis);
//	const auto pWeapon = pTempExt->GetWeapon();
//
//	return (WeaponTypeExt::ExtMap.Find(pWeapon)->conductAbduction(pOwner, pThis->Target))
//		? 0x71AAD5 : 0x0;
//}

//TODO :
// temporal per-slot
// HAHA AE and Jammer stuffs go brrr
//DEFINE_HOOK(71A84E, TemporalClass_UpdateA, 5)
//{
//	GET(TemporalClass* const, pThis, ESI);
//
//	// it's not guaranteed that there is a target
//	if (auto const pTarget = pThis->Target)
//	{
//		auto const pExt = TechnoExt::ExtMap.Find(pTarget);
//		// Temporal should disable RadarJammers
//		pExt->RadarJam = nullptr;
//
//		//AttachEffect handling under Temporal
//		if (!pExt->AttachEffects_NeedTo_RecreateAnims)
//		{
//			for (auto& Item : pExt->AttachedEffects)
//			{
//				if (Item.Type->TemporalHidesAnim)
//				{
//					Item.KillAnim();
//				}
//			}
//			pExt->AttachEffects_NeedTo_RecreateAnims = true;
//		}
//	}
//
//	pThis->WarpRemaining -= pThis->GetWarpPerStep(0);
//
//	R->EAX(pThis->WarpRemaining);
//	return 0x71A88D;
//}

// TODO :
// Prism fuckery 
//DEFINE_HOOK(71AF76, TemporalClass_Fire_PrismForwardAndWarpable, 9)
//{
//	GET(TechnoClass* const, pThis, EDI);
//
//	// bugfix #874 B: Temporal warheads affect Warpable=no units
//	// it has been checked: this is warpable. free captured and destroy spawned units.
//	if (pThis->SpawnManager)
//	{
//		pThis->SpawnManager->KillNodes();
//	}
//
//	if (pThis->CaptureManager)
//	{
//		pThis->CaptureManager->FreeAll();
//	}
//
//	// prism forward
//	if (pThis->WhatAmI() == AbstractType::Building)
//	{
//		auto const pData = BuildingExt::ExtMap.Find(reinterpret_cast<BuildingClass*>(pThis));
//		pData->PrismForwarding.RemoveFromNetwork(true);
//	}
//
//	return 0;
//}

// issue #1437: crash when warping out buildings infantry wants to garrison
DEFINE_OVERRIDE_HOOK(0x71AA52, TemporalClass_Update_AnnounceInvalidPointer, 0x8)
{
	GET(TechnoClass*, pVictim, ECX);
	pVictim->IsAlive = false;
	return 0;
}

// issue 472: deglob WarpAway
DEFINE_OVERRIDE_HOOK(0x71A900, TemporalClass_Update_WarpAway, 6)
{
	GET(TemporalClass*, pThis, ESI);
	const auto nWeaponIDx = Ares_TemporalWeapon(pThis->Owner);
	auto const pWeapon = pThis->Owner->GetWeapon(nWeaponIDx)->WeaponType;
	R->EDX<AnimTypeClass*>(WarheadTypeExt::ExtMap.Find(pWeapon->Warhead)->Temporal_WarpAway.Get(RulesClass::Global()->WarpAway));
	return 0x71A906;
}

// bugfix #379: Temporal friendly kills give veterancy
// bugfix #1266: Temporal kills gain double experience
// 
DEFINE_OVERRIDE_HOOK(0x71A917, TemporalClass_Update_Erase, 5)
{
	GET(TemporalClass*, pThis, ESI);

	auto pOwner = pThis->Owner;
	auto const pWeapon = pThis->Owner->GetWeapon(Ares_TemporalWeapon(pThis->Owner))->WeaponType;
	auto pOwnerExt = TechnoExt::ExtMap.Find(pOwner);
	auto pWarheadExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);

	if (pWarheadExt->Supress_LostEva)
		pOwnerExt->SupressEVALost = true;

	return 0x71A97D;
}

int GetWarpPerStep(TemporalClass* pThis, int nStep)
{
	int nAddStep = 0;
	int nStepR = 0;

	for (; pThis;)
	{
		if (nStep > 50)
			break;

		++nStep;
		auto const pWeapon = pThis->Owner->GetWeapon(Ares_TemporalWeapon(pThis->Owner))->WeaponType;

		if (auto const pTarget = pThis->Target)
			nStepR = MapClass::GetTotalDamage(pWeapon->Damage, pWeapon->Warhead, pTarget->GetTechnoType()->Armor, 0);
		else
			nStepR = pWeapon->Damage;

		nAddStep += nStepR;
		pThis->WarpPerStep = nStepR;
		pThis = pThis->PrevTemporal;
	}

	return nAddStep;
}

DEFINE_OVERRIDE_HOOK(0x71AB10, TemporalClass_GetWarpPerStep, 6)
{
	GET_STACK(int, nStep, 0x4);
	GET(TemporalClass*, pThis, ESI);
	R->EAX(GetWarpPerStep(pThis, nStep));
	return 0x71AB57;
}

// bugfix #874 A: Temporal warheads affect Warpable=no units
// skip freeing captured and destroying spawned units,
// as it is not clear here if this is warpable at all.
DEFINE_OVERRIDE_SKIP_HOOK(0x71AF2B, TemporalClass_Fire_UnwarpableA, 0xA, 71AF4D)

DEFINE_HOOK(0x71AC50, TemporalClass_LetItGo_ExpireEffect, 0x5)
{
	GET(TemporalClass* const, pThis, ESI);

	if (auto const pTarget = pThis->Target)
	{
		pTarget->UpdatePlacement(PlacementType::Redraw);

		auto nTotal = pThis->GetWarpPerStep();
		if (nTotal)
		{
			auto const pWeapon = pThis->Owner->GetWeapon(Ares_TemporalWeapon(pThis->Owner))->WeaponType;

			if (auto const Warhead = pWeapon->Warhead)
			{

				auto const pTempOwner = pThis->Owner;
				auto const peWHext = WarheadTypeExt::ExtMap.Find(Warhead);

				if (auto pExpireAnim = peWHext->TemporalExpiredAnim.Get())
				{

					auto nCoord = pTarget->GetCenterCoords();

					if (auto const pAnim = GameCreate<AnimClass>(pExpireAnim, nCoord))
					{
						pAnim->ZAdjust = pTarget->GetZAdjustment() - 3;
						AnimExt::SetAnimOwnerHouseKind(pAnim, pTempOwner->GetOwningHouse()
							, pTarget->GetOwningHouse(), pThis->Owner, false);
					}
				}

				if (peWHext->TemporalExpiredApplyDamage.Get())
				{
					auto const pTargetStreght = pTarget->GetTechnoType()->Strength;

					if (pThis->WarpRemaining > 0)
					{

						auto damage = int((pTargetStreght * ((1.0 - pThis->WarpRemaining / 10.0 / pTargetStreght)
							* (pWeapon->Damage * peWHext->TemporalDetachDamageFactor.Get()) / 100)));

						if (pTarget->IsAlive && !pTarget->IsSinking && !pTarget->IsCrashing)
							pTarget->ReceiveDamage(&damage, pTempOwner->DistanceFrom(pTarget), Warhead, pTempOwner, false, ScenarioClass::Instance->Random.RandomBool(), pTempOwner->Owner);
					}
				}
			}
		}
	}

	return 0x71AC5D;
}

DEFINE_OVERRIDE_HOOK(0x71AFB2, TemporalClass_Fire_HealthFactor, 5)
{
	GET(TechnoClass*, pTarget, ECX);
	GET(TemporalClass*, pThis, ESI);
	GET(int, nStreght, EAX);

	auto const pWeapon = pThis->Owner->GetWeapon(Ares_TemporalWeapon(pThis->Owner))->WeaponType;;
	const auto pWarhead = pWeapon->Warhead;
	const auto pWarheadExt = WarheadTypeExt::ExtMap.Find(pWarhead);
	const double nCalc = (1.0 - pTarget->Health / pTarget->GetTechnoType()->Strength) * pWarheadExt->Temporal_HealthFactor.Get();
	const double nCalc_b = (1.0 - nCalc) * (10 * nStreght) + nCalc * 0.0;

	R->EAX(nCalc_b <= 1.0 ? 1 : static_cast<int>(nCalc_b));
	return 0x71AFB7;
}

bool Warpable(TechnoClass* pTarget)
{
	if (!pTarget || pTarget->IsSinking || pTarget->IsCrashing || pTarget->IsIronCurtained())
		return false;

	if (TechnoExt::HasAbility(pTarget, PhobosAbilityType::Unwarpable))
		return false;

	if (Is_Building(pTarget))
	{

		if (Ares_AboutToChronoshift(pTarget))
		{
			return false;
		}
	}
	else
	{

		if (Is_Unit(pTarget) && !TechnoExt::IsInWarfactory(pTarget))
			return false;

		if (TechnoExt::IsChronoDelayDamageImmune(static_cast<FootClass*>(pTarget)))
			return false;
	}

	return true;
}

DEFINE_OVERRIDE_HOOK(0x71AE50, TemporalClass_CanWarpTarget, 8)
{
	GET_STACK(TechnoClass*, pTarget, 0x4);
	R->EAX(Warpable(pTarget));
	return 0x71AF19;
}

DEFINE_OVERRIDE_HOOK(0x71944E, TeleportLocomotionClass_ILocomotion_Process, 6)
{
	GET(FootClass*, pObject, ECX);
	GET(CoordStruct*, XYZ, EDX);
	*XYZ = pObject->GetCoords();
	R->EAX<CoordStruct*>(XYZ);

	if (auto pType = pObject->GetTechnoType())
	{
		if (const auto pImage = pType->AlphaImage)
		{
			Point2D xy;
			TacticalClass::Instance->CoordsToClient(XYZ, &xy);
			RectangleStruct ScreenArea = TacticalClass::Instance->VisibleArea();
			Point2D off = { ScreenArea.X - (pImage->Width / 2), ScreenArea.Y - (pImage->Height / 2) };
			xy += off;
			RectangleStruct Dirty =
			{ xy.X - ScreenArea.X, xy.Y - ScreenArea.Y,
			  pImage->Width, pImage->Height };
			TacticalClass::Instance->RegisterDirtyArea(Dirty, true);
		}
	}

	return 0x719454;
}