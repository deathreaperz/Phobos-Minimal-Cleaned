#include <AbstractClass.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <Utilities/Macro.h>
#include <Helpers/Macro.h>
#include <Base/Always.h>

#include <HouseClass.h>
#include <Utilities/Debug.h>

#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/VoxelAnim/Body.h>
#include <Ext/Anim/Body.h>

#include "Header.h"

#include <SpawnManagerClass.h>

ASMJIT_PATCH(0x415085, AircraftClass_Update_DamageSmoke, 7)
{
	GET(AircraftClass*, pThis, ESI);

	const auto pExt = TechnoTypeExtContainer::Instance.Find(pThis->Type);

	AnimTypeClass* pType = pExt->SmokeAnim.Get(RulesExtData::Instance()->DefaultAircraftDamagedSmoke);
	if (!pType)
		return 0x41512C;

	const int chance = (pThis->Health > 0) ? pExt->SmokeChanceRed.Get(10) : pExt->SmokeChanceDead.Get(80);

	if (chance <= 0)
		return 0x41512C;

	if (pThis->GetHealthPercentage() < RulesClass::Instance->ConditionRed)
	{
		if (pThis->GetHeight() > 0)
		{
			if (ScenarioClass::Instance->Random.RandomFromMax(99) < chance)
			{
				AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(pType, pThis->Location),
					pThis->Owner,
					nullptr,
					nullptr,
					false, false
				);
			}
		}
	}

	return 0x41512C;
}

ASMJIT_PATCH(0x417D75, AircraftClass_GetActionOnObject_CanTote, 5)
{
	GET(AircraftClass*, pCarryall, ESI);
	GET(UnitClass*, pTarget, EDI);

	return (TechnoTypeExt_ExtData::CarryallCanLift(pCarryall->Type, pTarget))
		? 0u
		: 0x417DF6u
		;
}

ASMJIT_PATCH(0x416E37, AircraftClass_Mi_MoveCarryall_CanTote, 5)
{
	GET(AircraftClass*, pCarryall, ESI);
	GET(UnitClass*, pTarget, EDI);

	return (TechnoTypeExt_ExtData::CarryallCanLift(pCarryall->Type, pTarget))
		? 0u
		: 0x416EC9u
		;
}

ASMJIT_PATCH(0x41949F, AircraftClass_ReceivedRadioCommand_SpecificPassengers, 6)
{
	GET(AircraftClass* const, pThis, ESI);
	GET_STACK(TechnoClass const* const, pSender, 0x14);

	enum { Allowed = 0x41945Fu, Disallowed = 0x41951Fu };

	auto const pType = pThis->Type;

	if (pThis->Passengers.NumPassengers >= pType->Passengers)
	{
		return Disallowed;
	}

	auto const pSenderType = pSender->GetTechnoType();

	return TechnoTypeExtData::PassangersAllowed(pType, pSenderType) ? Allowed : Disallowed;
}

ASMJIT_PATCH(0x41946B, AircraftClass_ReceivedRadioCommand_QueryEnterAsPassenger_KillDriver, 6)
{
	// prevent units from getting the enter cursor on transports
	// with killed drivers.
	GET(AircraftClass*, pThis, ESI);
	return (TechnoExtContainer::Instance.Find(pThis)->Is_DriverKilled ? 0x4190DDu : 0u);
}

ASMJIT_PATCH(0x416CF4, AircraftClass_Carryall_Unload_Guard, 0x5)
{
	GET(FootClass*, pCargo, ESI);

	pCargo->Transporter = 0;
	pCargo->QueueMission(Mission::Guard, true);

	if (auto pTeam = pCargo->Team)
		pTeam->AddMember(pCargo, false);

	return 0;
}

ASMJIT_PATCH(0x416C94, AircraftClass_Carryall_Unload_UpdateCargo, 0x6)
{
	GET(UnitClass*, pCargo, ESI);

	pCargo->UpdatePosition(PCPType::End);

	if (pCargo->Deactivated && pCargo->Locomotor.GetInterfacePtr()->Is_Powered())
	{
		pCargo->Locomotor.GetInterfacePtr()->Power_Off();
	}

	return 0;
}

// skip the check for UnitRepair, as it does not play well with UnitReload and
// Factory=AircraftType at all. in fact, it's prohibited, and thus docking to
// other structures was never allowed.
ASMJIT_PATCH(0x417E16, AircraftClass_GetActionOnObject_Dock, 0x6)
{
	// target is known to be a building
	GET(AircraftClass* const, pThis, ESI);
	GET(BuildingClass* const, pBuilding, EDI);

	// enter and no-enter cursors only if aircraft can dock
	// or show select cursor
	return pThis->Type->Dock.Contains(pBuilding->Type) ? 0x417E4B : 0x417E7D;
}

ASMJIT_PATCH(0x413FA3, AircraftClass_Init_Cloakable, 0x5)
{
	GET(AircraftClass*, Item, ESI);

	if (Item->Type->Cloakable)
	{
		Item->Cloakable = true;
	}

	return 0;
}

ASMJIT_PATCH(0x415533, AircraftClass_Mi_Unload_Blocked, 0x5)
{
	GET(AircraftClass*, pThis, ESI);
	GET(FootClass*, pCargo, EDI);

	if (pThis->KickOutUnit(pCargo, CellStruct::Empty) != KickOutResult::Failed)
	{
		pCargo->Transporter = nullptr;
		pCargo->IsOnCarryall = false;
	}
	else
	{
		pThis->AddPassenger(pCargo);
		pThis->EnterIdleMode(false, 1);
	}

	return 0x41554F;
}

// #1232: fix for dropping units out of flying Carryalls
ASMJIT_PATCH(0x415DF6, AircraftClass_Paradrop_Carryall, 0x6)
{
	GET(FootClass*, pTechno, ESI);
	pTechno->Transporter = nullptr;
	pTechno->IsOnCarryall = false;
	return 0;
}

// fix for vehicle paradrop alignment
ASMJIT_PATCH(0x415CA6, AircraftClass_Paradrop_Units, 0x6)
{
	GET(AircraftClass*, A, EDI);
	GET(FootClass*, P, ESI);

	if (P->WhatAmI() == UnitClass::AbsID)
	{
		const CoordStruct SrcXYZ = A->GetCoords();
		LEA_STACK(CoordStruct*, XYZ, 0x20);
		XYZ->X = (SrcXYZ.X & ~0xFF) + 0x80;
		XYZ->Y = (SrcXYZ.Y & ~0xFF) + 0x80;
		XYZ->Z = SrcXYZ.Z - 1;
		R->ECX(XYZ);
		return 0x415DE3;
	}

	return 0;
}

// flying aircraft carriers
// allow spawned units to spawn above ground
ASMJIT_PATCH(0x414338, AircraftClass_Put_SpawnHigh, 0x6)
{
	GET(AircraftClass*, pThis, ESI);
	GET(AircraftTypeClass*, pType, ECX);

	R->EAX(pType->MissileSpawn || pThis->SpawnOwner);
	return 0x41433E;
}

// aim for the cell for flying carriers
ASMJIT_PATCH(0x6B7838, SpawnManagerClass_Update_SpawnHigh, 0x5)
{
	GET(SpawnManagerClass*, pThis, ESI);
	GET(TechnoClass*, pSpawnee, EDI);

	pSpawnee->SetDestination(pThis->Owner->GetHeight() > 0
		? (AbstractClass*)pThis->Owner->GetCell() : (AbstractClass*)pThis->Owner, true);

	return 0x6B7848;
}

/* #1354 - Aircraft and empty SovParaDropInf list */
ASMJIT_PATCH(0x41D887, AirstrikeClass_Fire, 0x6)
{
	if (!RulesClass::Instance->SovParaDropInf.Count)
	{
		R->ECX(-1);
		return 0x41D895;
	}

	return 0x0;
}

ASMJIT_PATCH(0x416C4D, AircraftClass_Carryall_Unload_DestroyCargo, 0x5)
{
	GET(AircraftClass*, pCarryall, EDI);
	GET(UnitClass*, pCargo, ESI);

	int Damage = pCargo->Type->Strength;
	pCargo->ReceiveDamage(&Damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);

	Damage = pCarryall->Type->Strength;
	pCarryall->ReceiveDamage(&Damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);

	return 0x416C53;
}

ASMJIT_PATCH(0x416C3A, AircraftClass_Carryall_Unload_Facing, 0x5)
{
	enum
	{
		RetFailed = 0x416C49,
		RetSucceeded = 0x416C5A
	};

	GET(FootClass*, pCargo, ESI);
	GET(CoordStruct*, pCoord, ECX);
	GET(AircraftClass*, pThis, EDI);

	const auto nFacing = pThis->TurretFacing();

	if (!pCargo->Unlimbo(*pCoord, (DirType)(((nFacing.Raw >> 7) + 1) >> 1)))
		return RetFailed;

	const auto pCargoType = pCargo->GetTechnoType();
	const auto pCorgoTypeExt = TechnoTypeExtContainer::Instance.Find(pCargoType);
	const auto nRot = pCargoType->ROT;

	pCargo->PrimaryFacing.Set_ROT(nRot);
	pCargo->SecondaryFacing.Set_ROT(pCorgoTypeExt->TurretRot.Get(nRot));
	return RetSucceeded;
}