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

//
// skip old logic's way to determine the cursor
// Was 7
DEFINE_OVERRIDE_SKIP_HOOK(0x51E5BB, InfantryClass_GetActionOnObject_MultiEngineerA, 0x5 ,51E5D9)

DEFINE_OVERRIDE_HOOK(0x51F628, InfantryClass_Guard_Doggie, 0x5)
{
	GET(InfantryClass*, pThis, ESI);
	GET(int, res, EAX);

	if (res != -1)
	{
		return 0x51F634;
	}

	// doggie sit down on tiberium handling
	if (pThis->Type->Doggie && !pThis->Crawling && !pThis->Target && !pThis->Destination)
	{
		if (!pThis->PrimaryFacing.Is_Rotating() && pThis->GetCell()->LandType == LandType::Tiberium)
		{
			if (pThis->PrimaryFacing.Current().Get_Dir() == DirType::East)
			{
				// correct facing, sit down
				pThis->PlayAnim(DoType::Down);
			}
			else
			{
				// turn to correct facing
				DirStruct dir(3, DirType::East);
				pThis->Locomotor->Do_Turn(dir);
			}
		}
	}

	return 0x51F62D;
}

DEFINE_OVERRIDE_HOOK(0x518CB3, InfantryClass_ReceiveDamage_Doggie, 0x6)
{
	GET(InfantryClass*, pThis, ESI);

	// hurt doggie gets more panic
	if (pThis->Type->Doggie && pThis->IsRedHP()) {
		R->EDI(RulesExt::Global()->DoggiePanicMax);
	}

	return 0;
}

DEFINE_OVERRIDE_HOOK(0x51ABD7, InfantryClass_SetDestination_Doggie, 0x6)
{
	GET(InfantryClass*, pThis, EBP);
	GET(AbstractClass*, pTarget, EBX);

	// doggie cannot crawl; has to stand up and run
	bool doggieStandUp = pTarget && pThis->Crawling && pThis->Type->Doggie;

	return doggieStandUp ? 0x51AC16 : 0;
}

DEFINE_OVERRIDE_HOOK(0x5200C1, InfantryClass_UpdatePanic_Doggie, 0x6)
{
	GET(InfantryClass*, pThis, ESI);
	auto pType = pThis->Type;

	if (!pType->Doggie)
	{
		return 0;
	}

	// if panicking badly, lay down on tiberium
	if (pThis->PanicDurationLeft >= RulesExt::Global()->DoggiePanicMax)
	{
		if (!pThis->Destination && !pThis->Locomotor->Is_Moving())
		{
			if (pThis->GetCell()->LandType == LandType::Tiberium)
			{
				// is on tiberium. just lay down
				pThis->PlayAnim(DoType::Down);
			}
			else if (!pThis->InLimbo)
			{
				// search tiberium and abort current mission
				pThis->MoveToTiberium(16, false);
				if (pThis->Destination)
				{
					pThis->SetTarget(nullptr);
					pThis->QueueMission(Mission::Move, false);
					pThis->NextMission();
				}
			}
		}
	}

	if (!pType->Fearless)
	{
		--pThis->PanicDurationLeft;
	}

	return 0x52025A;
}

// #1008047: the C4 did not work correctly in YR, because some ability checks were missing
DEFINE_OVERRIDE_HOOK(0x51C325, InfantryClass_IsCellOccupied_C4Ability, 0x6)
{
	GET(InfantryClass*, pThis, EBP);

	return (pThis->Type->C4 || pThis->HasAbility(AbilityType::C4)) ?
		0x51C37D : 0x51C335;
}

DEFINE_OVERRIDE_HOOK(0x51A4D2, InfantryClass_UpdatePosition_C4Ability, 0x6)
{
	GET(InfantryClass*, pThis, ESI);

	return (!pThis->Type->C4 && !pThis->HasAbility(AbilityType::C4)) ?
		0x51A7F4 : 0x51A4E6;
}

// do not prone in water
DEFINE_OVERRIDE_HOOK(0x5201CC, InfantryClass_UpdatePanic_ProneWater, 0x6)
{
	GET(InfantryClass*, pThis, ESI);
	auto pCell = pThis->GetCell();
	auto landType = pCell->LandType;
	return (landType == LandType::Beach || landType == LandType::Water) &&
		!pCell->ContainsBridge() ? 0x5201DC : 0x0;
}

DEFINE_OVERRIDE_HOOK(0x51F716, InfantryClass_Mi_Unload_Undeploy, 0x5)
{
	GET(InfantryTypeClass*, pThisType, ECX);
	GET(InfantryClass*, pThis, ESI);

	if (pThisType->UndeployDelay < 0)
		pThis->PlayAnim(DoType::Undeploy, true, false);

	R->EBX(1);
	return 0x51F7C9;
}

// should correct issue #743
DEFINE_OVERRIDE_HOOK(0x51D799, InfantryClass_PlayAnim_WaterSound, 0x7)
{
	enum
	{
		Play = 0x51D7A6,
		SkipPlay = 0x51D8BF
	};

	GET(InfantryClass*, I, ESI);

	return (I->Transporter || I->Type->MovementZone != MovementZone::AmphibiousDestroyer)
		? SkipPlay
		: Play
		;
}

DEFINE_OVERRIDE_HOOK(0x520731, InfantryClass_UpdateFiringState_Heal, 0x5)
{
	GET(InfantryClass*, pThis, EBP);

	auto const pTargetTechno = generic_cast<TechnoClass*>(pThis->Target);

	if (!pTargetTechno || RulesClass::Instance->ConditionGreen <= pTargetTechno->GetHealthPercentage_())
		pThis->SetTarget(nullptr);

	return 0x52094C;
}

// do not infiltrate buildings of allies
DEFINE_OVERRIDE_HOOK(0x519FF8, InfantryClass_UpdatePosition_PreInfiltrate, 0x6)
{
	GET(InfantryClass*, pThis, ESI);
	GET(BuildingClass*, pBld, EDI);

	return (!pThis->Type->Agent || pThis->Owner->IsAlliedWith(pBld))
		? 0x51A03E : 0x51A002;
}

// #895584: ships not taking damage when repaired in a shipyard. bug
// was that the logic that prevented units from being damaged when
// exiting a war factory applied here, too. added the Naval check.
DEFINE_OVERRIDE_HOOK(0x737CE4, UnitClass_ReceiveDamage_ShipyardRepair, 0x6)
{
	GET(BuildingTypeClass*, pType, ECX);
	return (pType->WeaponsFactory && !pType->Naval) ?
		0x737CEE : 0x737D31;
}

// actual game code: if(auto B = specific_cast<BuildingClass *>(T)) { if(T->currentAmmo > 1) { return 1; } }
// if the object being queried doesn't have a weapon (Armory/Hospital), it'll return 1 anyway
DEFINE_OVERRIDE_SKIP_HOOK(0x6FCFA4, TechnoClass_GetROF_BuildingHack, 0x5, 6FCFC1)

DEFINE_OVERRIDE_HOOK(0x51BCB2, InfantryClass_Update_Reload, 0x6)
{
	GET(InfantryClass*, I, ESI);

	if (I->InLimbo)
	{
		return 0x51BDCF;
	}

	I->Reload();
	return 0x51BCC0;
}

DEFINE_OVERRIDE_SKIP_HOOK(0x51F1D8, InfantryClass_ActionOnObject_IvanBombs, 0x6, 51F1EA)

DEFINE_OVERRIDE_HOOK(0x52070F, InfantryClass_UpdateFiringState_Uncloak, 0x5)
{
	GET(InfantryClass*, pThis, EBP);
	GET_STACK(int, idxWeapon, STACK_OFFS(0x34, 0x24));

	if (pThis->IsCloseEnough(pThis->Target, idxWeapon))
	{
		pThis->Uncloak(false);
	}

	return 0x52094C;
}

// issues 896173 and 1433804: the teleport locomotor keeps a copy of the last
// coordinates, and unmarks the occupation bits of that place instead of the
// ones the unit was added to after putting it back on the map. that left the
// actual cell blocked. this fix resets the last coords, so the actual position
// is unmarked.
DEFINE_OVERRIDE_HOOK(0x51DF27, InfantryClass_Remove_Teleport, 0x6)
{
	GET(InfantryClass* const, pThis, ECX);

	if (pThis->Type->Teleporter)
	{
		auto const pLoco = static_cast<LocomotionClass*>(
			pThis->Locomotor.get());

		if ((((DWORD*)pLoco)[0] == TeleportLocomotionClass::vtable))
		{
			auto const pTele = static_cast<TeleportLocomotionClass*>(pLoco);
			pTele->LastCoords = CoordStruct::Empty;
		}
	}

	return 0;
}

DEFINE_HOOK(0x5243E3, InfantryTypeClass_AllowDamageSparks, 0xB)
{
	GET(InfantryTypeClass*, pThis, ESI)
		auto bIsCyborg = R->AL();
	GET(INIClass*, pINI, EBP);

	pThis->DamageSparks = pINI->ReadBool(pThis->ID, "AllowDamageSparks", bIsCyborg);

	return 0x5243EE;
}

DEFINE_OVERRIDE_HOOK(0x51E3B0, InfantryClass_GetActionOnObject_EMP, 0x7)
{
	GET(InfantryClass*, pInfantry, ECX);
	GET_STACK(TechnoClass*, pTarget, 0x4);

	// infantry should really not be able to deploy then EMP'd.
	if ((pInfantry == pTarget) && pInfantry->Type->Deployer && pInfantry->IsUnderEMP())
	{
		R->EAX(Action::NoDeploy);
		return 0x51F187;
	}

	return 0;
}

DEFINE_OVERRIDE_SKIP_HOOK(0x5200D7, InfantryClass_UpdatePanic_DontReload, 0x6, 52010B)