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

#include <Ext/TechnoType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/VoxelAnim/Body.h>

// TODO : Some hook causing desyncs

DEFINE_OVERRIDE_SKIP_HOOK(0x50928C, HouseClass_Update_Factories_Queues_SkipBrokenDTOR, 0x5, 5092A3)

// reject negative indexes. if the index is the result of the function above, this
// catches all invalid cells. otherwise, the game can write of of bounds, which can
// set a field that is supposed to be a pointer, and crash when calling a virtual
// method on it. in worst case, this goes unnoticed.
DEFINE_OVERRIDE_HOOK(0x4FA2E0, HouseClass_SetThreat_Bounds, 0x7)
{
	//GET(HouseClass*, pThis, ESI);
	GET_STACK(int, index, 0x4);
	//GET_STACK(int, threat, 0x8);

	return index < 0 ? 0x4FA347u : 0;
}

DEFINE_OVERRIDE_HOOK(0x504796, HouseClass_AddAnger_MultiplayPassive, 0x6)
{
	GET_STACK(HouseClass*, pOtherHouse, 0x10);
	GET(HouseClass*, pThis, ECX);

	R->ECX(SessionClass::Instance->GameMode != GameMode::Campaign
		&& pOtherHouse->Type->MultiplayPassive ? 0x0 : pThis->AngerNodes.Count);

	return 0x50479C;
}

DEFINE_OVERRIDE_HOOK(0x509303, HouseClass_AllyWith_unused, 0x6)
{
	GET(HouseClass*, pThis, ESI);
	GET(HouseClass*, pThat, EAX);

	pThis->RadarVisibleTo.Add(pThat);
	return 0x509319;
}

// don't crash if you can't find a base unit
// I imagine we'll have a pile of hooks like this sooner or later
DEFINE_OVERRIDE_HOOK(0x4F65BF, HouseClass_CanAffordBase, 0x6)
{
	GET(UnitTypeClass*, pBaseUnit, ECX);

	if (pBaseUnit)
	{
		return 0;
	}
	//GET(HouseClass *, pHouse, ESI);
	//Debug::Log("AI House of country [%s] cannot build anything from [General]BaseUnit=.\n", pHouse->Type->ID);
	return 0x4F65DA;
}

DEFINE_OVERRIDE_HOOK(0x50067C, HouseClass_ClearFactoryCreatedManually, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	pThis->InfantryType_53D1 = false;
	return 0x5006C0;
}

DEFINE_OVERRIDE_HOOK(0x5005CC, HouseClass_SetFactoryCreatedManually, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	pThis->InfantryType_53D1 = true;
	return 0x500612;
}

DEFINE_OVERRIDE_HOOK(0x5007BE, HouseClass_SetFactoryCreatedManually2, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	pThis->InfantryType_53D1 = R->DL();
	return 0x50080D;
}

DEFINE_OVERRIDE_HOOK(0x455E4C, HouseClass_FindRepairBay, 0x9)
{
	GET(UnitClass* const, pUnit, ECX);
	GET(BuildingClass* const, pBay, ESI);

	auto const pType = pBay->Type;
	auto const pUnitType = pUnit->Type;

	const bool isNotAcceptable = (pUnitType->BalloonHover
		|| pType->Naval != pUnitType->Naval
		|| pType->Factory == AbstractType::AircraftType
		|| pType->Helipad
		|| pType->HoverPad && !RulesClass::Instance->SeparateAircraft);

	if (isNotAcceptable)
	{
		return 0x455EDE;
	}

	auto const response = pUnit->SendCommand(
		RadioCommand::QueryCanEnter, pBay);

	// original game accepted any valid answer as a positive one
	return response != RadioCommand::AnswerPositive ? 0x455EDEu : 0x455E5Du;
}

// fixes SWs not being available in campaigns if they have been turned off in a
// multiplayer mode
DEFINE_OVERRIDE_HOOK(0x5055D8, HouseClass_GenerateAIBuildList_SWAllowed, 0x5)
{
	auto const allowed = SessionClass::Instance->GameMode == GameMode::Campaign
		|| GameModeOptionsClass::Instance->SWAllowed;

	R->EAX(allowed);
	return 0x5055DD;
}

// #917 - stupid copying logic
/**
 * v2[0] = v1[0];
 * v2[1] = v1[1];
 * v2[2] = v1[2];
 * for(int i = 3; i < v1.Count; ++i) {
 *  v2[i] = v1[i];
 * }
 * care to guess what happens when v1.Count is < 3?
 *
 * fixed old fix, which was quite broken itself...
 */

DEFINE_OVERRIDE_HOOK(0x505B58, HouseClass_GenerateAIBuildList_SkipManualCopy, 0x6)
{
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase1, STACK_OFFS(0xA4, 0x90));
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase2, STACK_OFFS(0xA4, 0x78));
	PlannedBase2.SetCapacity(PlannedBase1.Capacity, nullptr);
	return 0x505C2C;
}

DEFINE_OVERRIDE_HOOK(0x505C34, HouseClass_GenerateAIBuildList_FullAutoCopy, 0x5)
{
	R->EDI(0);
	return 0x505C39;
}

// I am crying all inside
DEFINE_OVERRIDE_HOOK(0x505CF1, HouseClass_GenerateAIBuildList_PadWithN1, 0x5)
{
	REF_STACK(DynamicVectorClass<BuildingTypeClass*>, PlannedBase2, STACK_OFFS(0xA4, 0x78));
	GET(int, DefenseCount, EAX);
	while (PlannedBase2.Count <= 3)
	{
		PlannedBase2.AddItem(reinterpret_cast<BuildingTypeClass*>(-1));
		--DefenseCount;
	}
	R->EDI(DefenseCount);
	R->EBX(-1);
	return (DefenseCount > 0) ? 0x505CF6u : 0x505D8Du;
}

// #1369308: if still charged it hasn't fired.
// more efficient place would be 4FAEC9, but this is global
DEFINE_OVERRIDE_HOOK(0x4FAF2A, HouseClass_SWDefendAgainst_Aborted, 0x8)
{
	GET(SuperClass*, pSW, EAX);
	return (pSW && !pSW->IsCharged) ? 0x4FAF32 : 0x4FB0CF;
}