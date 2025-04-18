#include "Body.h"
#include <Utilities/Macro.h>
#include <Ext/Anim/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Infantry/Body.h>
#include <Ext/AnimType/Body.h>

#include <Misc/Hooks.Otamaa.h>

ASMJIT_PATCH(0x4519A2, BuildingClass_UpdateAnim_SetParentBuilding, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(FakeAnimClass*, pAnim, EBP);
	pAnim->_GetExtData()->ParentBuilding = pThis;
	return 0;
}

ASMJIT_PATCH(0x424CF1, AnimClass_Start_DetachedReport, 0x6)
{
	GET(FakeAnimClass*, pThis, ESI);

	auto const pTypeExt = pThis->_GetTypeExtData();

	if (pTypeExt->DetachedReport.isset())
		VocClass::PlayAt(pTypeExt->DetachedReport.Get(), pThis->GetCoords());

	return 0;
}

// Deferred creation of attached particle systems for debris anims.
ASMJIT_PATCH(0x423939, AnimClass_BounceAI_AttachedSystem, 0x6)
{
	GET(FakeAnimClass*, pThis, EBP);
	pThis->_GetExtData()->CreateAttachedSystem();
	return 0;
}

ASMJIT_PATCH(0x4232E2, AnimClass_DrawIt_AltPalette, 0x6)
{
	enum { SkipGameCode = 0x4232EA, SetAltPaletteLightConvert = 0x4232F0 };

	GET(FakeAnimClass*, pThis, ESI);

	const auto pTypeExt = pThis->_GetTypeExtData();
	int schemeIndex = RulesExtData::Instance()->AnimRemapDefaultColorScheme;

	if (((pTypeExt->CreateUnit && pTypeExt->CreateUnit_RemapAnim.Get(pTypeExt->RemapAnim)) || pTypeExt->RemapAnim) && pThis->Owner)
	{
		schemeIndex = pThis->Owner->ColorSchemeIndex - 1;
	}

	schemeIndex += pTypeExt->AltPalette_ApplyLighting ? 1 : 0;
	R->ECX(ColorScheme::Array->Items[schemeIndex]);

	return SkipGameCode;
}

ASMJIT_PATCH(0x422CAB, AnimClass_DrawIt_XDrawOffset, 0x5)
{
	GET(FakeAnimClass* const, pThis, ECX);
	GET_STACK(Point2D*, pCoord, STACK_OFFS(0x100, -0x4));

	if (pThis->Type)
	{
		pCoord->X += pThis->_GetTypeExtData()->XDrawOffset;
	}

	return 0;
}

ASMJIT_PATCH(0x423B95, AnimClass_AI_HideIfNoOre_Threshold, 0x6)
{
	GET(AnimClass* const, pThis, ESI);
	GET(FakeAnimTypeClass* const, pType, EDX);

	if (pType && pType->HideIfNoOre)
	{
		auto const pCell = pThis->GetCell();

		pThis->Invisible = !pCell || pCell->GetContainedTiberiumValue()
			<= Math::abs(pType->_GetExtData()->HideIfNoOre_Threshold.Get());

		return 0x423BBF;
	}

	return 0x0;
} //was 8

//DEFINE_FUNCTION_JUMP(VTABLE, 0x7E33CC, GET_OFFSET(AnimExtData::GetLayer_patch));

//ASMJIT_PATCH(0x424CB0, AnimClass_InWhichLayer_Override, 0x6) //was 5
//{
//	GET(AnimClass*, pThis, ECX);
//
//	enum
//	{
//		RetLayerGround = 0x424CBA,
//		RetLayerAir = 0x0424CD1,
//		RetTypeLayer = 0x424CCA,
//		ReturnSetManualResult = 0x424CD6
//	};
//
//	if (pThis->Type) {
//		if (pThis->OwnerObject) {
//
//			const auto pExt = AnimTypeExtContainer::Instance.Find(pThis->Type);
//
//			if (!pExt->Layer_UseObjectLayer.isset() || !pThis->OwnerObject->IsAlive) {
//				return RetLayerGround;
//			}
//
//			if (pExt->Layer_UseObjectLayer.Get()) {
//
//				Layer nRes = Layer::Ground;
//
//				if (auto const pFoot = generic_cast<FootClass*>(pThis->OwnerObject)) {
//
//					if(pFoot->IsCrashing ||  pFoot->IsSinking)
//						return RetLayerGround;
//
//					if (auto const pLocomotor = pFoot->Locomotor.GetInterfacePtr())
//						nRes = pLocomotor->In_Which_Layer();
//				}
//				else if (auto const pBullet = specific_cast<BulletClass*>(pThis->OwnerObject)) {
//					nRes = pBullet->InWhichLayer();
//				}
//				else {
//					nRes = pThis->OwnerObject->ObjectClass::InWhichLayer();
//				}
//
//				R->EAX(nRes);
//				return ReturnSetManualResult;
//			}
//		}
//		else {
//			R->EAX(pThis->Type->Layer);
//			return ReturnSetManualResult;
//		}
//	}
//
//	return RetLayerAir;
//}

ASMJIT_PATCH(0x424CB0, AnimClass_InWhichLayer_AttachedObjectLayer, 0x6)
{
	enum { ReturnValue = 0x424CBF };

	GET(FakeAnimClass*, pThis, ECX);

	if (!pThis->Type)
		return 0x0;

	auto pExt = pThis->_GetTypeExtData();

	if (pThis->OwnerObject && pExt->Layer_UseObjectLayer.isset())
	{
		Layer layer = pThis->Type->Layer;

		if (pExt->Layer_UseObjectLayer.Get())
			layer = pThis->OwnerObject->InWhichLayer();

		R->EAX(layer);

		return ReturnValue;
	}

	return 0;
}

ASMJIT_PATCH(0x424C3D, AnimClass_AttachTo_BuildingCoords, 0x6)
{
	GET(FakeAnimClass*, pThis, ESI);
	GET(ObjectClass*, pObject, EDI);
	LEA_STACK(CoordStruct*, pCoords, STACK_OFFS(0x34, 0xC));

	if (pThis->Type)
	{
		if (pThis->_GetTypeExtData()->UseCenterCoordsIfAttached)
		{
			pObject->GetRenderCoords(pCoords);

			//save original coords because centering it broke damage
			pThis->_GetExtData()->BackupCoords = pObject->Location;

			pCoords->X += 128;
			pCoords->Y += 128;
			R->EAX(pCoords);
			return 0x424C49;
		}
	}

	return 0x0;
}

ASMJIT_PATCH(0x424807, AnimClass_AI_Next, 0x6) //was 8
{
	GET(FakeAnimClass*, pThis, ESI);

	if (pThis->Type)
	{
		const auto pExt = pThis->_GetExtData();
		const auto pTypeExt = pThis->_GetTypeExtData();

		if (pExt->AttachedSystem && pExt->AttachedSystem->Type != pTypeExt->AttachedSystem.Get())
			pExt->AttachedSystem = nullptr;

		if (!pExt->AttachedSystem && pTypeExt->AttachedSystem)
			pExt->CreateAttachedSystem();
	}

	return 0x0;
}

ASMJIT_PATCH(0x424AEC, AnimClass_AI_SetMission, 0x6)
{
	GET(FakeAnimClass*, pThis, ESI);
	GET(InfantryClass*, pInf, EDI);

	const auto pTypeExt = pThis->_GetTypeExtData();
	const auto Is_AI = !pInf->Owner->IsControlledByHuman();

	if (!pTypeExt->ScatterAnimToInfantry(Is_AI))
		pInf->QueueMission(pTypeExt->GetAnimToInfantryMission(Is_AI), false);
	else
		pInf->Scatter(CoordStruct::Empty, true, false);

	return 0x0;
}

//the stack is change , so i need to replace everything if i want just use normal hook
//this make it unnessesary
//replace the vtable call

DEFINE_FUNCTION_JUMP(CALL6, 0x424B04, FakeInfantryClass::_Dummy);

ASMJIT_PATCH(0x423365, AnimClass_DrawIt_ExtraShadow, 0x8)
{
	enum { DrawShadow = 0x42336D, SkipDrawShadow = 0x4233EE };

	GET(FakeAnimClass*, pThis, ESI);

	if (!pThis->Type->Shadow)
		return SkipDrawShadow;

	const bool hasExtra = R->AL();

	return hasExtra && pThis->_GetTypeExtData()->ExtraShadow ?
		DrawShadow : SkipDrawShadow;
}

ASMJIT_PATCH(0x425060, AnimClass_Expire_ScorchFlamer, 0x6)
{
	GET(AnimClass*, pThis, ESI);

	auto const pType = pThis->Type;

	if (!pType->Flamer && !pType->Scorch)
		return 0;

	AnimExtData::SpawnFireAnims(pThis);

	return 0;
}