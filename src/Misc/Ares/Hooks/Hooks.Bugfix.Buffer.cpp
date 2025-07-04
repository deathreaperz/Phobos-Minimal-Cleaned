#include <Utilities/Macro.h>
#include <Helpers/Macro.h>
#include <Base/Always.h>

#include <Utilities/TemplateDefB.h>

#include <WeaponTypeClass.h>
#include <CCINIClass.h>
#include <AnimTypeClass.h>
#include <TerrainTypeClass.h>
#include <SmudgeTypeClass.h>
#include <TriggerTypeClass.h>
#include <TEventClass.h>
#include <TActionClass.h>

ASMJIT_PATCH(0x72749B, TriggerTypeClass_LoadFromINI_Buffers, 0x8)
{
	GET(TriggerTypeClass*, pThis, EBP);
	GET_STACK(CCINIClass*, pINI, 0x248);

	if (pINI->ReadString(GameStrings::Events(), pThis->ID, Phobos::readDefval, Phobos::readBuffer, Phobos::readLength))
	{
		if (auto v16 = CRT::atoi(CRT::strtok(Phobos::readBuffer, Phobos::readDelims)))
		{
			do
			{
				auto pEvent = GameCreate<TEventClass>();
				pEvent->LoadFromINI();
				--v16;
				pEvent->NextEvent = pThis->FirstEvent;
				pThis->FirstEvent = pEvent;
			}
			while (v16);
		}
	}

	if (pINI->ReadString(GameStrings::Actions(), pThis->ID, Phobos::readDefval, Phobos::readBuffer, Phobos::readLength))
	{
		pThis->FirstAction = nullptr;
		TActionClass* pTemp = nullptr;

		if (auto v16 = CRT::atoi(CRT::strtok(Phobos::readBuffer, Phobos::readDelims)))
		{
			do
			{
				auto pAction = GameCreate<TActionClass>();
				pAction->LoadFromINI();
				if (pThis->FirstAction)
				{
					pTemp->NextAction = pAction;
				}
				else
				{
					pThis->FirstAction = pAction;
				}
				--v16;
				pTemp = pAction;
			}
			while (v16);
		}
	}

	return 0x7275A5;
}

ASMJIT_PATCH(0x4750D0, CCINIClass_ReadHouseTypeList_Buffers, 0xA)
{
	GET(CCINIClass*, pThis, ECX);
	GET_STACK(const char*, pSection, 0x4);
	GET_STACK(const char*, pKey, 0x8);
	GET_STACK(DWORD, _default, 0xC);

	if (pThis->ReadString(pSection, pKey, Phobos::readDefval, Phobos::readBuffer, Phobos::readLength))
	{
		_default = 0u;
		for (auto i = strtok(Phobos::readBuffer, Phobos::readDelims); i; i = strtok(0, Phobos::readDelims))
		{
			_default |= HouseTypeClass::FindIndexOfNameShiftToTheRightOnce(i);
		}
	}

	R->EAX(_default);
	return 0x475140;
}

ASMJIT_PATCH(0x475260, CCINIClass_ReadAlly_Buffers, 0xA)
{
	GET(CCINIClass*, pThis, ECX);
	GET_STACK(const char*, pSection, 0x4);
	GET_STACK(const char*, pKey, 0x8);
	GET_STACK(DWORD, _default, 0xC);

	if (pThis->ReadString(pSection, pKey, Phobos::readDefval, Phobos::readBuffer, Phobos::readLength))
	{
		_default = 0u;
		for (auto i = strtok(Phobos::readBuffer, Phobos::readDelims); i; i = strtok(0, Phobos::readDelims))
		{
			_default |= (1 << HouseClass::FindIndexByName(i));
		}
	}

	R->EAX(_default);
	return 0x4752D9;
}

// == WeaponType ==
ASMJIT_PATCH(0x772462, WeaponTypeClass_LoadFromINI_ListLength, 0x9)
{
	GET(WeaponTypeClass*, pThis, ESI);
	GET(const char*, pSection, EBX);
	GET(CCINIClass*, pINI, EDI);

	INI_EX exINI(pINI);
	detail::ParseVector<AnimTypeClass*>(pThis->Anim, exINI, pSection, GameStrings::Anim(), "Expect valid AnimType");

	return 0x77255F;
}

// == WarheadType ==
ASMJIT_PATCH(0x75D660, WarheadTypeClass_LoadFromINI_ListLength, 9)
{
	GET(WarheadTypeClass*, pThis, ESI);
	GET(const char*, pSection, EBP);
	GET(CCINIClass*, pINI, EDI);

	INI_EX exINI(pINI);
	detail::ParseVector<AnimTypeClass*>(pThis->AnimList, exINI, pSection, GameStrings::AnimList, "Expect valid AnimType");
	detail::ParseVector(pThis->DebrisMaximums, exINI, pSection, GameStrings::DebrisMaximums, "Expect valid number");
	detail::ParseVector<VoxelAnimTypeClass*>(pThis->DebrisTypes, exINI, pSection, GameStrings::DebrisTypes, "Expect valid VoxelAnimType");

	return 0x75D75D;
}

//WarheadTypeClass_LoadFromINI_SkipLists
DEFINE_JUMP(LJMP, 0x75DAE6, 0x75DDCC);

ASMJIT_PATCH(0x713171, TechnoTypeClass_LoadFromINI_SkipLists1, 9)
{
	GET(TechnoTypeClass*, pThis, EBP);
	GET(Category, category, EAX);
	pThis->Category = category;
	return 0x713264;
}

ASMJIT_PATCH(0x713C10, TechnoTypeClass_LoadFromINI_SkipLists2, 7)
{
	GET(TechnoTypeClass*, pThis, EBP);
	GET(const CoordStruct*, pResult, EAX);
	pThis->NaturalParticleSystemLocation = *pResult;
	return 0x713E1A;
}

// == TechnoType ==
ASMJIT_PATCH(0x7125DF, TechnoTypeClass_LoadFromINI_ListLength, 7)
{
	GET(TechnoTypeClass*, pThis, EBP);
	GET(const char*, pSection, EBX);
	GET(CCINIClass*, pINI, ESI);

	INI_EX exINI(pINI);

	detail::ParseVector<ParticleSystemTypeClass*>(pThis->DamageParticleSystems, exINI, pSection, GameStrings::DamageParticleSystems, "Expect valid ParticleSystemType");
	detail::ParseVector<ParticleSystemTypeClass*>(pThis->DestroyParticleSystems, exINI, pSection, GameStrings::DestroyParticleSystems, "Expect valid ParticleSystemType");

	detail::ParseVector<BuildingTypeClass*>(pThis->Dock, exINI, pSection, GameStrings::Dock, "Expect valid BuildingType");

	detail::ParseVector(pThis->DebrisMaximums, exINI, pSection, GameStrings::DebrisMaximums, "Expect valid number");
	detail::ParseVector<VoxelAnimTypeClass*>(pThis->DebrisTypes, exINI, pSection, GameStrings::DebrisTypes, "Expect valid VoxelAnimType");
	detail::ParseVector<AnimTypeClass*>(pThis->DebrisAnims, exINI, pSection, GameStrings::DebrisAnims, "Expect valid AnimType");

	return 0x712830;
}

// ============= [AI] =============

ASMJIT_PATCH(0x672B0E, Buf_AI, 6)
{
	GET(RulesClass*, pRules, ESI);
	GET(CCINIClass*, pINI, EDI);

	INI_EX exINI(pINI);
	const char* section = GameStrings::AI;
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildConst, exINI, section, GameStrings::BuildConst, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildPower, exINI, section, GameStrings::BuildPower, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildRefinery, exINI, section, GameStrings::BuildRefinery, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildBarracks, exINI, section, GameStrings::BuildBarracks, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildTech, exINI, section, GameStrings::BuildTech, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildWeapons, exINI, section, GameStrings::BuildWeapons, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->AlliedBaseDefenses, exINI, section, GameStrings::AlliedBaseDefenses, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->SovietBaseDefenses, exINI, section, GameStrings::SovietBaseDefenses, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->ThirdBaseDefenses, exINI, section, GameStrings::ThirdBaseDefenses, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildDefense, exINI, section, GameStrings::BuildDefense, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildPDefense, exINI, section, GameStrings::BuildPDefense, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildAA, exINI, section, GameStrings::BuildAA, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildHelipad, exINI, section, GameStrings::BuildHelipad, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildRadar, exINI, section, GameStrings::BuildRadar, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->ConcreteWalls, exINI, section, GameStrings::ConcreteWalls, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->NSGates, exINI, section, GameStrings::NSGates, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->EWGates, exINI, section, GameStrings::EWGates, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildNavalYard, exINI, section, GameStrings::BuildNavalYard, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->BuildDummy, exINI, section, GameStrings::BuildDummy, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->NeutralTechBuildings, exINI, section, GameStrings::NeutralTechBuildings, "Expect valid BuildingType");

	detail::ParseVector(pRules->AIForcePredictionFudge, exINI, section, GameStrings::AIForcePredictionFudge, "Expect valid number");

	return 0x673950;
}

// ============= [CombatDamage] =============
ASMJIT_PATCH(0x66BC71, Buf_CombatDamage, 9)
{
	GET(RulesClass*, pRules, ESI);
	GET(CCINIClass*, pINI, EDI);

	pRules->TiberiumStrength = R->EAX<int>();

	INI_EX exINI(pINI);
	detail::ParseVector<SmudgeTypeClass*>(pRules->Scorches, exINI, GameStrings::CombatDamage(), GameStrings::Scorches, "Expect valid SmudgeType");
	detail::ParseVector<SmudgeTypeClass*>(pRules->Scorches1, exINI, GameStrings::CombatDamage(), GameStrings::Scorches1, "Expect valid SmudgeType");
	detail::ParseVector<SmudgeTypeClass*>(pRules->Scorches2, exINI, GameStrings::CombatDamage(), GameStrings::Scorches2, "Expect valid SmudgeType");
	detail::ParseVector<SmudgeTypeClass*>(pRules->Scorches3, exINI, GameStrings::CombatDamage(), GameStrings::Scorches3, "Expect valid SmudgeType");
	detail::ParseVector<SmudgeTypeClass*>(pRules->Scorches4, exINI, GameStrings::CombatDamage(), GameStrings::Scorches4, "Expect valid SmudgeType");

	detail::ParseVector<AnimTypeClass*>(pRules->SplashList, exINI, GameStrings::CombatDamage(), GameStrings::SplashList, "Expect valid AnimType");
	return 0x66C287;
}

template<typename T, bool Allocate = false, bool Unique = false>
OPTIONALINLINE void ParseVector_loc(DynamicVectorClass<T>& List, INI_EX& IniEx, const char* section, const char* key, const char* message = nullptr)
{
	if (IniEx.ReadString(section, key))
	{
		List.Reset();
		char* context = nullptr;

		using BaseType = std::remove_pointer_t<T>;
		Debug::LogInfo("Parsing [{}] form [{}] result {}", key, section, IniEx.value());

		for (char* cur = strtok_s(IniEx.value(), Phobos::readDelims, &context); cur;
			 cur = strtok_s(nullptr, Phobos::readDelims, &context))
		{
			BaseType* buffer = nullptr;
			if COMPILETIMEEVAL(Allocate)
			{
				buffer = BaseType::FindOrAllocate(cur);
			}
			else
			{
				buffer = BaseType::Find(cur);
			}

			if (buffer)
			{
				if COMPILETIMEEVAL(!Unique)
				{
					List.AddItem(buffer);
				}
				else
				{
					List.AddUnique(buffer);
				}
			}
			else if (!GameStrings::IsBlank(cur))
			{
				Debug::INIParseFailed(section, key, cur, message);
			}
		}
		Debug::LogInfo("count : {}", List.Count);
	}
};
// ============= [General] =============
std::optional<int> ATOI_Count;

ASMJIT_PATCH(0x66D55E, Buf_General, 6)
{
	GET(RulesClass*, pRules, ESI);
	GET(CCINIClass*, pINI, EDI);

	INI_EX exINI(pINI);
	const char* section = GameStrings::General();
	detail::ParseVector<InfantryTypeClass*>(pRules->AmerParaDropInf, exINI, section, GameStrings::AmerParaDropInf, "Expect valid InfantryType");
	detail::ParseVector<InfantryTypeClass*>(pRules->AllyParaDropInf, exINI, section, GameStrings::AllyParaDropInf, "Expect valid InfantryType");
	detail::ParseVector<InfantryTypeClass*>(pRules->SovParaDropInf, exINI, section, GameStrings::SovParaDropInf, "Expect valid InfantryType");
	detail::ParseVector<InfantryTypeClass*>(pRules->YuriParaDropInf, exINI, section, GameStrings::YuriParaDropInf, "Expect valid InfantryType");

	detail::ParseVector(pRules->AmerParaDropNum, exINI, section, GameStrings::AmerParaDropNum, "Expect valid number");
	detail::ParseVector(pRules->AllyParaDropNum, exINI, section, GameStrings::AllyParaDropNum, "Expect valid number");
	detail::ParseVector(pRules->SovParaDropNum, exINI, section, GameStrings::SovParaDropNum, "Expect valid number");
	detail::ParseVector(pRules->YuriParaDropNum, exINI, section, GameStrings::YuriParaDropNum, "Expect valid number");

	detail::ParseVector<InfantryTypeClass*, true>(pRules->AnimToInfantry, exINI, section, GameStrings::AnimToInfantry, "Expect valid InfantryType");

	//if (!ATOI_Count.has_value() || !ATOI_Count.value())
	//	ATOI_Count = pRules->AnimToInfantry.Count;
	//else if(pRules->AnimToInfantry.Count != ATOI_Count.value()) {
	//	Debug::FatalError("ATOI Array missmatch was %d cur %d", ATOI_Count.value(), pRules->AnimToInfantry.Count);
	//}

	detail::ParseVector<InfantryTypeClass*>(pRules->SecretInfantry, exINI, section, GameStrings::SecretInfantry, "Expect valid InfantryType");
	detail::ParseVector<UnitTypeClass*>(pRules->SecretUnits, exINI, section, GameStrings::SecretUnits, "Expect valid UnitType");
	detail::ParseVector<BuildingTypeClass*>(pRules->SecretBuildings, exINI, section, GameStrings::SecretBuildings, "Expect valid BuildingType");

	pRules->SecretSum = pRules->SecretInfantry.Count
		+ pRules->SecretUnits.Count
		+ pRules->SecretBuildings.Count;

	detail::ParseVector<UnitTypeClass*>(pRules->HarvesterUnit, exINI, section, GameStrings::HarvesterUnit, "Expect valid UnitType");
	detail::ParseVector<UnitTypeClass*>(pRules->BaseUnit, exINI, section, GameStrings::BaseUnit, "Expect valid UnitType");
	detail::ParseVector<AircraftTypeClass*>(pRules->PadAircraft, exINI, section, GameStrings::PadAircraft, "Expect valid AircraftType");

	detail::ParseVector<BuildingTypeClass*>(pRules->Shipyard, exINI, section, GameStrings::Shipyard, "Expect valid BuildingType");
	detail::ParseVector<BuildingTypeClass*>(pRules->RepairBay, exINI, section, GameStrings::RepairBay, "Expect valid BuildingType");

	detail::ParseVector<AnimTypeClass*>(pRules->WeatherConClouds, exINI, section, GameStrings::WeatherConClouds, "Expect valid AnimType");
	detail::ParseVector<AnimTypeClass*>(pRules->WeatherConBolts, exINI, section, GameStrings::WeatherConBolts, "Expect valid AnimType");
	detail::ParseVector<AnimTypeClass*>(pRules->BridgeExplosions, exINI, section, GameStrings::BridgeExplosions, "Expect valid AnimType");

	detail::ParseVector<TerrainTypeClass*>(pRules->DefaultMirageDisguises, exINI, section, GameStrings::DefaultMirageDisguises, "Expect valid TerrainType");

	if (pINI->ReadString(section, GameStrings::WallTower, nullptr, Phobos::readBuffer) > 0)
	{
		if (const auto pBuilding = BuildingTypeClass::FindOrAllocate(Phobos::readBuffer))
		{
			pRules->WallTower = pBuilding;
		}
		else
		{
			Debug::LogInfo("WallTower Building readed as [{}] but it is nullptr ! ", Phobos::readBuffer);
		}
	}

	return 0;
}

//Buf_SecretBoons
DEFINE_JUMP(LJMP, 0x66FA13, 0x66FAD6);
//Buf_Shipyard
DEFINE_JUMP(LJMP, 0x66f589, 0x66F68C);
//Buf_AnimToInf_Paradrop
DEFINE_JUMP(LJMP, 0x67062F, 0x6707FE);

ASMJIT_PATCH(0x66F7C0, Buf_PPA, 9)
{
	GET(RulesClass*, Rules, ESI);
	GET(UnitTypeClass*, Pt, EAX); // recreating overwritten bits
	Rules->PrerequisiteProcAlternate = Pt;
	return 0x66F9FA;
}

ASMJIT_PATCH(0x66F34B, Buf_RepairBay, 5)
{
	GET(RulesClass*, Rules, ESI);
	Rules->NoParachuteMaxFallRate = R->EAX<int>();
	return 0x66F450;
}

// Buf_WeatherArt
DEFINE_JUMP(LJMP, 0x66DD13, 0x66DF19);
//Buf_BridgeExplosions
DEFINE_JUMP(LJMP, 0x66DB93, 0x66DC96);

ASMJIT_PATCH(0x511D16, HouseTypeClass_LoadFromINI_Buffer_CountryVeteran, 9)
{
	GET(HouseTypeClass*, pHouseType, EBX);
	GET(CCINIClass*, pINI, ESI);

	INI_EX exINI(pINI);
	detail::ParseVector<InfantryTypeClass*>(pHouseType->VeteranInfantry, exINI, pHouseType->ID, GameStrings::VeteranInfantry, "Expect valid InfantryType");
	detail::ParseVector<UnitTypeClass*>(pHouseType->VeteranUnits, exINI, pHouseType->ID, GameStrings::VeteranUnits, "Expect valid UnitType");
	detail::ParseVector<AircraftTypeClass*>(pHouseType->VeteranAircraft, exINI, pHouseType->ID, GameStrings::VeteranAircraft, "Expect valid AircraftType");

	return 0x51208C;
}

// HTExt_Unlimit1
DEFINE_JUMP(LJMP, 0x4E3792, 0x4E37AD);

//HTExt_Unlimit2
DEFINE_JUMP(LJMP, 0x4E3A9C, 0x4E3AA1);

//HTExt_Unlimit3
DEFINE_JUMP(LJMP, 0x4E3F31, 0x4E3F4C);

//HTExt_Unlimit4
DEFINE_JUMP(LJMP, 0x4E412C, 0x4E4147);

//HTExt_Unlimit5
DEFINE_JUMP(LJMP, 0x4E41A7, 0x4E41C3);

//OptionsDlg_WndProc_RemoveResLimit
DEFINE_JUMP(LJMP, 0x56017A, 0x560183);

//OptionsDlg_WndProc_RemoveHiResCheck
DEFINE_JUMP(LJMP, 0x5601E3, 0x5601FC);