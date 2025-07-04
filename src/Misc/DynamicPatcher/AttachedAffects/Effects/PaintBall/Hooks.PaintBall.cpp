#include "PaintBall.h"

#include <Ext/Techno/Body.h>
#include <Ext/Anim/Body.h>
#include <Ext/Building/Body.h>

#include <New/PhobosAttachedAffect/PhobosAttachEffectTypeClass.h>
#include <Misc/PhobosGlobal.h>

#include <AirstrikeClass.h>
#include <InfantryClass.h>

// Gets tint colors for invulnerability, airstrike laser target and berserk, depending on parameters.
COMPILETIMEEVAL void InitializeColors() {
	auto g_instance = PhobosGlobal::Instance();

	if (!g_instance->ColorDatas.Initialized) {
		g_instance->ColorDatas.Initialized = true;
		g_instance->ColorDatas.Forceshield_Color = GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->ForceShieldColor);
		g_instance->ColorDatas.IronCurtain_Color = GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->IronCurtainColor);
		g_instance->ColorDatas.LaserTarget_Color = GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->LaserTargetColor);
		g_instance->ColorDatas.Berserk_Color = GeneralUtils::GetColorFromColorAdd(RulesClass::Instance->BerserkColor);
	}
}

int ApplyTintColor(TechnoClass* pThis, bool invulnerability, bool airstrike, bool berserk)
{
	int tintColor = 0;
	auto g_instance = PhobosGlobal::Instance();
	InitializeColors();

	if (invulnerability && pThis->IsIronCurtained())
		tintColor |= pThis->ProtectType == ProtectTypes::ForceShield ? g_instance->ColorDatas.Forceshield_Color : g_instance->ColorDatas.IronCurtain_Color;
	if (airstrike && TechnoExtContainer::Instance.Find(pThis)->AirstrikeTargetingMe) {
		auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(TechnoExtContainer::Instance.Find(pThis)->AirstrikeTargetingMe->Owner->GetTechnoType());
		if (pTypeExt->LaserTargetColor.isset())
			tintColor |= GeneralUtils::GetColorFromColorAdd(pTypeExt->LaserTargetColor);
		else
			tintColor |= g_instance->ColorDatas.LaserTarget_Color;
	}

	if (berserk && pThis->Berzerk)
		tintColor |= g_instance->ColorDatas.Berserk_Color;

	return tintColor;
}

void ApplyCustomTint(TechnoClass* pThis, int* tintColor, int* intensity)
{
	const auto pExt = TechnoExtContainer::Instance.Find(pThis);
	bool calculateIntensity = intensity != nullptr;
	const bool calculateTintColor = tintColor != nullptr;
	const bool curretlyObserve = HouseClass::IsCurrentPlayerObserver();

	for (auto& paint : TechnoExtContainer::Instance.Find(pThis)->PaintBallStates) {
		if (paint.second.timer.GetTimeLeft() && paint.second.AllowDraw(pThis)) {
			if (calculateTintColor)
				*tintColor |= paint.second.Color;

			if (calculateIntensity && paint.second.Data->BrightMultiplier != 0.0)
				*intensity = (int)(*intensity * paint.second.Data->BrightMultiplier);
		}
	}

	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());

	if (calculateIntensity)
	{
		BuildingClass* pBld = cast_to<BuildingClass*, false>(pThis);

		if (pBld) {
			if ((pBld->CurrentMission == Mission::Construction)
				&& pBld->BState == BStateType::Construction && pBld->Type->Buildup) {
				if (BuildingTypeExtContainer::Instance.Find(pBld->Type)->BuildUp_UseNormalLIght.Get())
				{
					*intensity = 1000;
				}
			}
		}

		const bool bInf = pThis->WhatAmI() == InfantryClass::AbsID;
		bool needRedraw = false;

		// EMP
		if (pThis->IsUnderEMP())
		{
			if (!bInf || pTypeExt->Infantry_DimWhenEMPEd.Get(((InfantryTypeClass*)(pTypeExt->AttachedToObject))->Cyborg))
			{
				*intensity /= 2;
				needRedraw = true;
			}
		}
		else if (pThis->IsDeactivated())
		{
			if (!bInf || pTypeExt->Infantry_DimWhenDisabled.Get(((InfantryTypeClass*)(pTypeExt->AttachedToObject))->Cyborg))
			{
				*intensity /= 2;
				needRedraw = true;
			}
		}

		if (pBld && needRedraw)
			BuildingExtContainer::Instance.Find(pBld)->LighningNeedUpdate = true;
	}

	if ((pTypeExt->Tint_Color.isset() || pTypeExt->Tint_Intensity != 0.0) && EnumFunctions::CanTargetHouse(pTypeExt->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
	{
		if (calculateTintColor)
			*tintColor |= Drawing::RGB_To_Int(pTypeExt->Tint_Color);

		if (calculateIntensity)
			*intensity += static_cast<int>(pTypeExt->Tint_Intensity * 1000);
	}

	if (pExt->AE.HasTint) {
		for (auto const& attachEffect : pExt->PhobosAE) {
			if (!attachEffect)
				continue;

			auto const type = attachEffect->GetType();

			if (!attachEffect->IsActive() || !type->HasTint())
				continue;

			if (!curretlyObserve && !EnumFunctions::CanTargetHouse(type->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
				continue;

			if (calculateTintColor && type->Tint_Color.isset())
				*tintColor |= Drawing::RGB_To_Int(type->Tint_Color);

			if (calculateIntensity)
				*intensity += static_cast<int>(type->Tint_Intensity * 1000);
		}
	}

	if (pExt->Shield && pExt->Shield->IsActive() && pExt->Shield->HasTint())
	{
		auto const pShieldType = pExt->Shield->GetType();

		if (!curretlyObserve && !EnumFunctions::CanTargetHouse(pShieldType->Tint_VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
			return;

		if (calculateTintColor && pShieldType->Tint_Color.isset())
			*tintColor |= Drawing::RGB_To_Int(pShieldType->Tint_Color);

		if (calculateIntensity && pShieldType->Tint_Intensity != 0.0)
			*intensity += static_cast<int>(pShieldType->Tint_Intensity * 1000);
	}
}

ASMJIT_PATCH(0x43FA19, BuildingClass_Mark_TintIntensity, 0x7)
{
	GET(BuildingClass*, pThis, EDI);
	GET(int, intensity, ESI);

	ApplyCustomTint(pThis, nullptr, &intensity);
	R->ESI(intensity);

	return 0;
}

ASMJIT_PATCH(0x43D386, BuildingClass_Draw_TintColor, 0x6)
{
	enum { SkipGameCode = 0x43D4EB };

	GET(BuildingClass*, pThis, ESI);

	int color = ApplyTintColor(pThis, true, true, false);
	ApplyCustomTint(pThis, &color, nullptr);
	R->EDI(color);

	return SkipGameCode;
}

ASMJIT_PATCH(0x43DC1C, BuildingClass_Draw2_TintColor, 0x6)
{
	enum { SkipGameCode = 0x43DD8E };

	GET(BuildingClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x12C, -0x110));

	color = ApplyTintColor(pThis, true, true, false);
	ApplyCustomTint(pThis, &color, nullptr);

	return SkipGameCode;
}

ASMJIT_PATCH(0x70632E, TechnoClass_DrawShape_GetTintIntensity, 0x6)
{
	enum { SkipGameCode = 0x706389 };

	GET(TechnoClass*, pThis, ESI);
	GET(int, intensity, EAX);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	const auto pExt = TechnoExtContainer::Instance.Find(pThis);

	if (pExt->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	R->EBP(intensity);
	return SkipGameCode;
}

ASMJIT_PATCH(0x706786, TechnoClass_DrawVoxel_TintColor, 0x5)
{
	enum { SkipTint = 0x7067E4 };

	GET(TechnoClass*, pThis, EBP);

	auto const rtti = pThis->WhatAmI();

	// Vehicles already have had tint intensity as well as custom tints applied, no need to do it twice.
	if (rtti == AbstractType::Unit)
		return SkipTint;

	GET(int, intensity, EAX);
	REF_STACK(int, color, STACK_OFFSET(0x50, 0x24));

	if (rtti == AbstractType::Aircraft)
		color = ApplyTintColor(pThis, true, false, false);

	// Non-aircraft voxels do not need custom tint color applied again, discard that component for them.
	ApplyCustomTint(pThis, rtti == AbstractType::Aircraft ? &color : nullptr, &intensity);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	const auto pExt = TechnoExtContainer::Instance.Find(pThis);

	if (pExt->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	R->EDI(intensity);
	return SkipTint;
}

ASMJIT_PATCH(0x706389, TechnoClass_DrawObject_TintColor, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(int, intensity, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x54, 0x2C));

	const auto what = pThis->WhatAmI();
	const bool isVehicle = what == AbstractType::Unit;
	const bool isAircraft = what == AbstractType::Aircraft;

	if (isVehicle || isAircraft)
	{
		color |= ApplyTintColor(pThis, true, true, !isAircraft);
	}

	ApplyCustomTint(pThis, &color, &intensity);

	R->EBP(intensity);

	return 0;
}

ASMJIT_PATCH(0x423420, AnimClass_Draw_ParentBuildingCheck, 0x6)
{
	GET(FakeAnimClass*, pThis, ESI);
	GET(BuildingClass*, pBuilding, EAX);
	REF_STACK(int, color, STACK_OFFSET(0x110, -0xF4));
	REF_STACK(int, intensity, STACK_OFFSET(0x110, -0xD8));

	enum { SkipGameCode = 0x4235D3 };

	if (!pBuilding)
		pBuilding = (pThis->_GetExtData()->ParentBuilding);

	if (pBuilding)
	{
		bool UseNormalLight = pThis->Type->UseNormalLight;

		if ((pBuilding->CurrentMission == Mission::Construction)
			&& pBuilding->BState == BStateType::Construction && pBuilding->Type->Buildup)
			if (BuildingTypeExtContainer::Instance.Find(pBuilding->Type)->BuildUp_UseNormalLIght.Get())
				UseNormalLight = true;

		ApplyTintColor(pBuilding, true, true, false);
		ApplyCustomTint(pBuilding, &color, !UseNormalLight ? &intensity : nullptr);
	}

	R->EBP(color);
	return SkipGameCode;
}

ASMJIT_PATCH(0x51946D, InfantryClass_Draw_TintIntensity, 0x6)
{
	GET(InfantryClass*, pThis, EBP);
	GET(int, intensity, ESI);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	if (TechnoExtContainer::Instance.Find(pThis)->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	ApplyCustomTint(pThis, nullptr, &intensity);
	R->ESI(intensity);

	return 0;
}

ASMJIT_PATCH(0x518FC8, InfantryClass_Draw_TintColor, 0x6)
{
	enum { SkipGameCode = 0x519082 };

	GET(InfantryClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x54, -0x40));

	color = ApplyTintColor(pThis, true, true, true);
	ApplyCustomTint(pThis, &color, nullptr);

	return SkipGameCode;
}

ASMJIT_PATCH(0x73BF95, UnitClass_DrawAsVoxel_Tint, 0x7)
{
	enum { SkipGameCode = 0x73C141 };

	GET(UnitClass*, pThis, EBP);
	GET(int, flashIntensity, ESI);
	REF_STACK(int, intensity, STACK_OFFSET(0x1D0, 0x10));

	intensity = flashIntensity;

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	if (TechnoExtContainer::Instance.Find(pThis)->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	int color = ApplyTintColor(pThis, true, true, true);
	ApplyCustomTint(pThis, &color, &intensity);

	R->ESI(color);
	return SkipGameCode;
}

/*
ASMJIT_PATCH(0x43D442, BuildingClass_Draw_ForceShieldICColor, 0x7)
{
	enum { SkipGameCode = 0x43D45B };

	GET(BuildingClass*, pThis, ESI);

	RulesClass* rules = RulesClass::Instance;

	R->ECX(rules);
	R->EAX(pThis->ProtectType == ProtectTypes::ForceShield ?
		rules->ForceShieldColor : rules->IronCurtainColor);

	return SkipGameCode;
}

ASMJIT_PATCH(0x43D396, BuildingClass_Draw_LaserTargetColor, 0x6) {
	GET(BuildingClass*, pThis, ESI);
	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->Airstrike->Owner->GetTechnoType());
	const ColorStruct clr = GeneralUtils::GetColorStructFromColorAdd(pTypeExt->LaserTargetColor.isset() ? pTypeExt->LaserTargetColor : RulesClass::Instance->LaserTargetColor);

	R->BL(clr.G);
	R->Stack8(0x11 , clr.R);
	R->Stack8(0x12, clr.B);
	return 0x43D3CA;
}

ASMJIT_PATCH(0x42343C, AnimClass_Draw_Airstrike, 0x6)
{
	GET(BuildingClass*, pBld, ECX);
	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pBld->Airstrike->Owner->GetTechnoType());
	const ColorStruct clr = GeneralUtils::GetColorStructFromColorAdd(pTypeExt->LaserTargetColor.isset() ? pTypeExt->LaserTargetColor : RulesClass::Instance->LaserTargetColor);

	R->Stack8(0x1B, clr.R);
	R->Stack8(0x1A, clr.G);
	R->Stack8(0x13, clr.B);
	return 0x42347B;
}

ASMJIT_PATCH(0x43DC30, BuildingClass_DrawFogged_LaserTargetColor, 0x6)
{
	enum { SkipGameCode = 0x43DC3C };

	GET(BuildingClass*, pThis, EBP);
	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->Airstrike->Owner->GetTechnoType());
	const ColorStruct clr = GeneralUtils::GetColorStructFromColorAdd(pTypeExt->LaserTargetColor.isset() ? pTypeExt->LaserTargetColor : RulesClass::Instance->LaserTargetColor);

	R->BL(clr.G);
	R->Stack8(0x13, clr.R);
	R->Stack8(0x12, clr.B);
	return 0x43DC64;
}

ASMJIT_PATCH(0x43DCE1, BuildingClass_Draw2_ForceShieldICColor, 0x7)
{
	enum { SkipGameCode = 0x43DCFA };

	GET(BuildingClass*, pThis, EBP);

	RulesClass* rules = RulesClass::Instance;

	R->ECX(rules);
	R->EAX(pThis->ProtectType == ProtectTypes::ForceShield ?
		rules->ForceShieldColor : rules->IronCurtainColor);

	return SkipGameCode;
}

ASMJIT_PATCH(0x43D4EB, BuildingClass_Draw_TintColor, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(int, color, EDI);

	ApplyCustomTint(pThis, &color, nullptr);

	R->EDI(color);

	return 0;
}

ASMJIT_PATCH(0x43DD8E, BuildingClass_Draw2_TintColor, 0xA)
{
	GET(BuildingClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x12C, -0x110));

	ApplyCustomTint(pThis, &color, nullptr);

	return 0;
}

ASMJIT_PATCH(0x519082, InfantryClass_Draw_TintColor, 0x7)
{
	GET(InfantryClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x54, -0x40));

	color |= ApplyTintColor(pThis, true, false, false);
	ApplyCustomTint(pThis, &color, nullptr);

	return 0;
}

ASMJIT_PATCH(0x73BFBF, UnitClass_DrawAsVoxel_ForceShieldICColor, 0x6)
{
	enum { SkipGameCode = 0x73BFC5 };

	GET(UnitClass*, pThis, EBP);

	RulesClass* rules = RulesClass::Instance;

	R->EAX(pThis->ProtectType == ProtectTypes::ForceShield ?
		rules->ForceShieldColor : rules->IronCurtainColor);

	return SkipGameCode;
}

ASMJIT_PATCH(0x73C083, UnitClass_DrawAsVoxel_TintColor, 0x6)
{
	GET(UnitClass*, pThis, EBP);
	GET(int, color, ESI);
	REF_STACK(int, intensity, STACK_OFFSET(0x1D0, 0x10));

	ApplyCustomTint(pThis, &color, &intensity);

	R->ESI(color);

	return 0;
}

ASMJIT_PATCH(0x42350C, AnimClass_Draw_ForceShieldICColor, 0x7)
{
	enum { SkipGameCode = 0x423525 };

	GET(BuildingClass*, pBuilding, ECX);

	RulesClass* rules = RulesClass::Instance;

	R->ECX(rules);
	R->EAX(pBuilding->ProtectType == ProtectTypes::ForceShield ?
		rules->ForceShieldColor : rules->IronCurtainColor);

	return SkipGameCode;
}

ASMJIT_PATCH(0x4235D3, AnimClass_Draw_TintColor, 0x6)
{
	GET(FakeAnimClass*, pThis, ESI);
	GET(int, color, EBP);
	REF_STACK(int, intensity, STACK_OFFSET(0x110, -0xD8));

	auto const pBuilding = pThis->_GetExtData()->ParentBuilding;

	if (!pBuilding || !pBuilding->IsAlive || pBuilding->InLimbo)
		return 0;

	ApplyCustomTint(pBuilding, &color, pThis->Type->UseNormalLight ? &intensity : nullptr);

	R->EBP(color);

	return 0;
}*/
/*
*
* ASMJIT_PATCH(0x73C15F, UnitClass_DrawVXL_Colour, 0x7)
{
	GET(UnitClass* const, pOwnerObject, EBP);

	if (auto& pPaintBall = TechnoExtContainer::Instance.Find(pOwnerObject)->PaintBallState)
		pPaintBall->DrawVXL_Paintball(pOwnerObject, R, false);

	return 0;
}
//case VISUAL_NORMAL
ASMJIT_PATCH(0x7063FF, TechnoClass_DrawSHP_Colour, 0x7)
{
	GET(TechnoClass* const, pOwnerObject, ESI);

	if (auto& pPaintBall = TechnoExtContainer::Instance.Find(pOwnerObject)->PaintBallState)
		pPaintBall->DrawSHP_Paintball(pOwnerObject, R);

	return 0;
}

ASMJIT_PATCH(0x706640, TechnoClass_DrawVXL_Colour, 0x5)
{
	GET(TechnoClass* const, pOwnerObject, ECX);

	if (pOwnerObject->WhatAmI() == BuildingClass::AbsID)
	{
		if (auto& pPaintBall = TechnoExtContainer::Instance.Find(pOwnerObject)->PaintBallState)
			pPaintBall->DrawVXL_Paintball(pOwnerObject, R, true);
	}

	return 0;
}

ASMJIT_PATCH(0x423630, AnimClass_Draw_It, 0x6)
{
	GET(AnimClass*, pAnim, ESI);
	GET(CellClass*, pCell, EAX);

	if (pAnim && pAnim->IsBuildingAnim)
	{
		auto const pBuilding = AnimExtContainer::Instance.Find(pAnim)->ParentBuilding;

		if (pBuilding && pBuilding->IsAlive && !pBuilding->Type->Invisible) {
			if (auto& pPaintBall = TechnoExtContainer::Instance.Find(pBuilding)->PaintBallStates) {
				pPaintBall->DrawSHP_Paintball_BuildAnim(pBuilding, R);
			}
		}
	}

	return 0;
}
*/