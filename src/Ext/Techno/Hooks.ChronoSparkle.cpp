#include "Body.h"

#include <Ext/Anim/Body.h>
#include <Ext/BuildingType/Body.h>

#include <TacticalClass.h>

void PlayChronoSparkleAnim(TechnoClass* pTechno, CoordStruct* pLoc, int X_Offs = 120, int nDelay = 24, bool bHidden = false, int ZAdjust = 0)
{
	if (bHidden || (Unsorted::CurrentFrame % nDelay))
		return;

	const auto pSparkle = RulesClass::Instance->ChronoSparkle1;
	if (!pSparkle)
		return;

	CoordStruct nLoc { pLoc->X + X_Offs , pLoc->Y + X_Offs  , pLoc->Z };
	auto pAnim = GameCreate<AnimClass>(pSparkle, nLoc, 0, 1, AnimFlag(0x600), false, false);
	pAnim->ZAdjust = ZAdjust;
	HouseClass* pOwner = nullptr;
	HouseClass* pVictim = pTechno->GetOwningHouse();
	TechnoClass* pTInvoker = nullptr;

	if (const auto pInvoker = pTechno->TemporalTargetingMe)
	{
		if (VTable::Get(pInvoker) == TemporalClass::vtable)
		{
			// we want the owner exist so the temporal will be cleaned up
			// if these were nullptr and the warp remaining is less than 0
			// the target unit will be stuck in the temporal limbo
			if (!pInvoker->Target && pInvoker->WarpRemaining <= 0)
				pInvoker->Target = pTechno;

			if (auto pOwnerOfTemp = pInvoker->Owner)
			{
				pOwner = pOwnerOfTemp->GetOwningHouse();
				pTInvoker = pOwnerOfTemp;
			}
		}
		else
		{
			pTechno->TemporalTargetingMe = nullptr;
		}
	}

	AnimExtData::SetAnimOwnerHouseKind(pAnim, pOwner, pVictim, pTInvoker, false, false);
}

ASMJIT_PATCH(0x73622F, UnitClass_AI_ChronoSparkle, 0x5)
{
	GET(TechnoClass*, pThis, ESI);
	PlayChronoSparkleAnim(pThis, &pThis->Location, 120, RulesExtData::Instance()->ChronoSparkleDisplayDelay);
	return 0x7362A7;
}

ASMJIT_PATCH(0x51BAF6, InfantryClass_AI_ChronoSparkle, 0x5)
{
	GET(TechnoClass*, pThis, ESI);
	PlayChronoSparkleAnim(pThis, &pThis->Location, 120, RulesExtData::Instance()->ChronoSparkleDisplayDelay);
	return 0x51BB6E;
}

ASMJIT_PATCH(0x414C06, AircraftClass_AI_ChronoSparkle, 0x5)
{
	GET(TechnoClass*, pThis, ESI);
	PlayChronoSparkleAnim(pThis, &pThis->Location, 0, RulesExtData::Instance()->ChronoSparkleDisplayDelay);
	return 0x414C78;
}

ASMJIT_PATCH(0x4403D4, BuildingClass_AI_ChronoSparkle, 0x6)
{
	enum { SkipGameCode = 0x44055D };

	GET(BuildingClass*, pThis, ESI);

	if (RulesClass::Instance->ChronoSparkle1)
	{
		auto const displayPositions = RulesExtData::Instance()->ChronoSparkleBuildingDisplayPositions;
		auto const pType = pThis->Type;
		const bool displayOnBuilding = (displayPositions & ChronoSparkleDisplayPosition::Building) != ChronoSparkleDisplayPosition::None;
		const bool displayOnSlots = (displayPositions & ChronoSparkleDisplayPosition::OccupantSlots) != ChronoSparkleDisplayPosition::None;
		const bool displayOnOccupants = (displayPositions & ChronoSparkleDisplayPosition::Occupants) != ChronoSparkleDisplayPosition::None;
		const int occupantCount = displayOnSlots ? pType->MaxNumberOccupants : pThis->GetOccupantCount();
		const bool showOccupy = occupantCount && (displayOnOccupants || displayOnSlots);

		if (showOccupy)
		{
			for (int i = 0; i < occupantCount; i++)
			{
				if (!((Unsorted::CurrentFrame + i) % RulesExtData::Instance()->ChronoSparkleDisplayDelay))
				{
					const auto offset = TacticalClass::Instance->ApplyMatrix_Pixel(
						(pType->MaxNumberOccupants <= 10 ?
							pType->MuzzleFlash[i] :
							BuildingTypeExtContainer::Instance.Find(pType)->OccupierMuzzleFlashes[i])
					);

					auto coords = pThis->GetRenderCoords();
					coords.X += offset.X;
					coords.Y += offset.Y;

					auto const pAnim = GameCreate<AnimClass>(RulesClass::Instance->ChronoSparkle1, coords);
					pAnim->ZAdjust = -200;
					HouseClass* pOwner = nullptr;
					HouseClass* pVictim = pThis->GetOwningHouse();
					TechnoClass* pTInvoker = nullptr;
					if (const auto pInvoker = pThis->TemporalTargetingMe)
					{
						if (auto pOwnerOfTemp = pInvoker->Owner)
						{
							pOwner = pOwnerOfTemp->GetOwningHouse();
							pTInvoker = pOwnerOfTemp;
						}
					}

					AnimExtData::SetAnimOwnerHouseKind(pAnim, pOwner, pVictim, pTInvoker, false, false);
				}
			}
		}

		if ((!showOccupy || displayOnBuilding))
		{
			auto nLoc = pThis->GetCenterCoords();
			PlayChronoSparkleAnim(pThis, &nLoc, 0);
		}
	}

	return SkipGameCode;
}