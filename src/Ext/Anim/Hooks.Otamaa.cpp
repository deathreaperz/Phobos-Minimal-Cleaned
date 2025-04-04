#include "Body.h"
#include <Utilities/Macro.h>

#include <Ext/TechnoType/Body.h>
#include <Ext/Tiberium/Body.h>
#include <Ext/AnimType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/AnimHelpers.h>
#include <Utilities/Helpers.h>
#include <SmudgeClass.h>
#include <SmudgeTypeClass.h>

#include <Memory.h>

void ApplyVeinsDamage(AnimClass* pThis, int VeinDamage, WarheadTypeClass* VeinWarhead)
{
	//causing game to lock up atm
	if (pThis->Type->IsVeins && VeinWarhead && RulesExtData::Instance()->VeinsAttack_interval)
	{
		auto coord = pThis->GetCoords();
		auto pCoorCell = MapClass::Instance->GetCellAt(coord);
		auto pFirst = pCoorCell->FirstObject;

		if (!pFirst
			|| pFirst->GetHeight() > 0
			|| pCoorCell->OverlayTypeIndex != 126
			|| pCoorCell->OverlayData < 0x30u
			|| pCoorCell->SlopeIndex
			)
		{
			pThis->__ToDelete_197 = true; // wut
		}

		if (Unsorted::CurrentFrame % RulesExtData::Instance()->VeinsAttack_interval == 0)
		{
			while (pFirst != nullptr)
			{
				ObjectClass* pNext = pFirst->NextObject;

				if (auto pTechno = flag_cast_to<TechnoClass*, false>(pFirst))
				{
					const auto pType = pTechno->GetTechnoType();
					if (!TechnoTypeExtContainer::Instance.Find(pType)->IsDummy && pTechno->IsAlive && pTechno->Health > 0 && !pTechno->InLimbo)
					{
						if (pTechno->WhatAmI() != UnitClass::AbsID || ((UnitClass*)pTechno)->DeathFrameCounter <= 0)
						{
							if ((!RulesExtData::Instance()->VeinsDamagingWeightTreshold.isset() || pType->Weight >= RulesExtData::Instance()->VeinsDamagingWeightTreshold)
								&& !pType->ImmuneToVeins
								&& !pTechno->HasAbility(AbilityType::VeinProof)
								&& pTechno->GetHeight() <= 5
								)
							{
								int dmg = VeinDamage;
								pFirst->ReceiveDamage(&dmg, 0, VeinWarhead, nullptr, false, false, nullptr);
							}
						}
					}
				}

				pFirst = pNext;
			}
		}
	}
}

ASMJIT_PATCH(0x424cfb, AnimClass_Init_Additionals, 6)
{
	GET(FakeAnimClass*, pThis, ESI);

	auto const pTypeExt = pThis->_GetTypeExtData();

	if (pTypeExt->AltReport.isset())
	{
		VocClass::PlayIndexAtPos(pTypeExt->AltReport, pThis->GetCoords(), nullptr);
	}

	if (pTypeExt->AdditionalHeight > 0)
		pThis->Location.Z += pTypeExt->AdditionalHeight;

	return 0;
}

ASMJIT_PATCH(0x4243BC, AnimClass_AI_Veins, 0x6)
{
	enum
	{
		ContinueDrawTiberium = 0x4243CC,
		ContinueNotTiberium = 0x42442E
	};

	GET(AnimClass*, pThis, ESI);
	ApplyVeinsDamage(pThis, RulesClass::Instance->VeinDamage, RulesExtData::Instance()->Veinhole_Warhead);
	return pThis->Type->IsAnimatedTiberium ?
		ContinueDrawTiberium : ContinueNotTiberium;
}

#include <Ext/Cell/Body.h>

ASMJIT_PATCH(0x685078, Generate_OreTwinkle_Anims, 0x7)
{
	GET(FakeCellClass* const, location, ESI);

	const int tib_idx = location->_GetTiberiumType();
	const int value = tib_idx == -1 ? 0 : TiberiumClass::Array->Items[tib_idx]->Value;

	if (value > 0)
	{
		const auto pTibExt = ((FakeTiberiumClass*)TiberiumClass::Array->Items[tib_idx])->_GetExtData();

		if (!ScenarioClass::Instance->Random.RandomFromMax(pTibExt->GetTwinkleChance() - 1))
		{
			if (auto pAnimtype = pTibExt->GetTwinkleAnim())
			{
				GameCreate<AnimClass>(pAnimtype, location->GetCoords(), 1);
			}
		}
	}

	return 0x6850E5;
}

ASMJIT_PATCH(0x423CC1, AnimClass_AI_HasExtras_Expired, 0x6)
{
	enum { SkipGameCode = 0x423EFD };

	GET(AnimClass* const, pThis, ESI);
	GET8(bool, LandIsWater, BL);
	GET8(bool, EligibleHeight, AL);

	//overriden instruction !
	R->Stack(STACK_OFFS(0x8C, 0x78), R->AL());

	return AnimExtData::OnExpired(pThis, LandIsWater, EligibleHeight) ?
		SkipGameCode : 0x0;
}

/// replae this entirely since the function using lea for getting int and seems broke everyone else stacks
void FakeAnimClass::_Middle()
{
	auto center = this->GetCoords();
	auto centercell = MapClass::Instance->GetCellAt(center);
	int width = 30;
	int height = 30;
	if (auto pSHP = this->Type->GetImage())
	{
		RectangleStruct bounds {};
		if (this->Type->MiddleFrameWidth == -1 || this->Type->MiddleFrameHeight == -1)
			bounds = pSHP->GetFrameBounds(this->Type->MiddleFrameIndex);

		if (this->Type->MiddleFrameWidth == -1)
		{
			this->Type->MiddleFrameWidth = bounds.Width;
		}

		if (this->Type->MiddleFrameHeight == -1)
		{
			this->Type->MiddleFrameHeight = bounds.Height;
		}

		width = this->Type->MiddleFrameWidth;
		height = this->Type->MiddleFrameHeight;
	}

	AnimExtData::OnMiddle(this);
	AnimExtData::OnMiddle_SpawnSmudge(this, centercell, { width ,height });
}

DEFINE_FUNCTION_JUMP(CALL, 0x424D5A, FakeAnimClass::_Middle);
DEFINE_FUNCTION_JUMP(CALL, 0x424687, FakeAnimClass::_Middle);
DEFINE_FUNCTION_JUMP(LJMP, 0x424F00, FakeAnimClass::_Middle);

ASMJIT_PATCH(0x42264D, AnimClass_Init, 0x5)
{
	GET(AnimClass*, pThis, ESI);
	GET_BASE(CoordStruct*, pCoord, 0xC);

	AnimExtData::OnInit(pThis, pCoord);

	return 0x0;
}

#ifdef PerformanceHoggers
//ASMJIT_PATCH_AGAIN(0x42429E, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x42437E, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x4243A6, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x424567, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x4246DC, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x424B42, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x4247EB, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x42492A, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH_AGAIN(0x424B29, AnimClass_UpdateEnd, 0x6)
//ASMJIT_PATCH(0x424B1B, AnimClass_UpdateEnd, 0x6)
//{
//	GET(AnimClass*, pThis, ESI);
//	AnimExtContainer::Instance.Find(pThis)->SpawnsStatusData.OnUpdate(pThis);
//	return 0;
//}
//
//ASMJIT_PATCH(0x424785, AnimClass_Loop, 0x6)
//{
//	GET(AnimClass*, pThis, ESI);
//	AnimExtContainer::Instance.Find(pThis)->SpawnsStatusData.OnLoop(pThis);
//	return 0;
//}
//
//ASMJIT_PATCH_AGAIN(0x4247F3, AnimClass_Done, 0x6)
//ASMJIT_PATCH(0x424298, AnimClass_Done, 0x6)
//{
//	GET(AnimClass*, pThis, ESI);
//	AnimExtContainer::Instance.Find(pThis)->SpawnsStatusData.OnDone(pThis);
//	return 0;
//}
//
//ASMJIT_PATCH(0x424801, AnimClass_Next, 0x6)
//{
//	GET(AnimClass*, pThis, ESI);
//	GET(AnimTypeClass*, pNextAnimType, ECX);
//	AnimExtContainer::Instance.Find(pThis)->SpawnsStatusData.OnNext(pThis , pNextAnimType);
//	return 0x0;
//}
#endif

#ifdef ENABLE_NEWHOOKS
TODO : retest for desync

ASMJIT_PATCH(0x4242BA, AnimClass_AI_SetCoord, 0x6)
{
	GET(AnimClass*, pThis, ESI);

	if (auto const pExt = AnimExtData::ExtMap.Find(pThis))
	{
		if (pExt->Something)
		{
			auto nCoord = pThis->GetCoords() + pExt->Something;
			pThis->SetLocation(nCoord);
		}
	}
	return 0x0;
}

ASMJIT_PATCH(0x422CC6, AnimClass_DrawIT_SpecialDraw, 0xA)
{
	GET(AnimClass* const, pThis, ESI);

	if (auto const pTypeExt = AnimTypeExtContainer::Instance.TryFind(pThis->Type))
	{
		R->AL(pTypeExt->SpecialDraw.Get());
		return 0x422CD0;
	}

	return 0x0;
}

#endif