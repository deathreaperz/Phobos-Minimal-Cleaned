#include <Locomotor/DriveLocomotionClass.h>
#include <Locomotor/ShipLocomotionClass.h>
#include <UnitClass.h>

#include <Ext/TechnoType/Body.h>
#include <Utilities/Macro.h>
#include <Utilities/TemplateDef.h>

ASMJIT_PATCH(0x073B05B, UnitClass_PerCellProcess_TiltWhenCrushes, 0x6)
{
	enum { SkipGameCode = 0x73B074 };

	GET(UnitClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->Type);

	if (!pTypeExt->TiltsWhenCrushes_Overlays.Get(pThis->Type->TiltsWhenCrushes))
		return SkipGameCode;

	if (!pTypeExt->CrushOverlayExtraForwardTilt.isset())
		return 0x0;

	pThis->RockingForwardsPerFrame += static_cast<float>(pTypeExt->CrushOverlayExtraForwardTilt);

	return SkipGameCode;
}

ASMJIT_PATCH(0x0741941, UnitClass_OverrunSquare_TiltWhenCrushes, 0x6)
{
	enum { SkipGameCode = 0x74195E };

	GET(UnitClass*, pThis, EDI);

	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->Type);

	if (!pTypeExt->TiltsWhenCrushes_Vehicles.Get(pThis->Type->TiltsWhenCrushes))
		return SkipGameCode;

	if (!pTypeExt->CrushForwardTiltPerFrame.isset())
		return 0x0;

	if (pThis->RockingForwardsPerFrame == 0.0)
		pThis->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame);

	return SkipGameCode;
}

ASMJIT_PATCH(0x4B1150, DriveLocomotionClass_WhileMoving_CrushSlowdown, 0x9)
{
	enum { SkipGameCode = 0x4B116B };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->LinkedTo->GetTechnoType());
	auto slowdownCoefficient = pThis->movementspeed_50;

	if (slowdownCoefficient > pTypeExt->CrushSlowdownMultiplier)
		slowdownCoefficient = pTypeExt->CrushSlowdownMultiplier;

	__asm { fld slowdownCoefficient };

	return SkipGameCode;
}

ASMJIT_PATCH(0x4B19F7, DriveLocomotionClass_WhileMoving_CrushTilt, 0xD)
{
	enum { SkipGameCode1 = 0x4B1A04, SkipGameCode2 = 0x4B1A58 };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pLinkedTo = pThis->LinkedTo;
	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pLinkedTo->GetTechnoType());

	if (pTypeExt->CrushForwardTiltPerFrame.isset())
	{
		pLinkedTo->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get());
		return R->Origin() == 0x4B19F7 ? SkipGameCode1 : SkipGameCode2;
	}

	return 0x0;
}
ASMJIT_PATCH_AGAIN(0x4B1A4B, DriveLocomotionClass_WhileMoving_CrushTilt, 0xD)

ASMJIT_PATCH(0x6A0813, ShipLocomotionClass_WhileMoving_CrushSlowdown, 0x9)
{
	enum { SkipGameCode = 0x6A082E };

	GET(ShipLocomotionClass*, pThis, EBP);

	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->LinkedTo->GetTechnoType());
	auto slowdownCoefficient = pThis->movementspeed_50;

	if (slowdownCoefficient > pTypeExt->CrushSlowdownMultiplier)
		slowdownCoefficient = pTypeExt->CrushSlowdownMultiplier;

	__asm { fld slowdownCoefficient };

	return SkipGameCode;
}

ASMJIT_PATCH(0x6A108D, ShipLocomotionClass_WhileMoving_CrushTilt, 0xD)
{
	enum { SkipGameCode = 0x6A109A };

	GET(DriveLocomotionClass*, pThis, EBP);

	auto const pLinkedTo = pThis->LinkedTo;
	auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pLinkedTo->GetTechnoType());

	if (pTypeExt->CrushForwardTiltPerFrame.isset())
	{
		pLinkedTo->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get());
		return SkipGameCode;
	}

	return 0x0;
}