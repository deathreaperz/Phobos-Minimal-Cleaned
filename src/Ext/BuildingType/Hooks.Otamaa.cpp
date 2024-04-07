#include "Body.h"
#include <TacticalClass.h>
#include <BuildingClass.h>
#include <HouseClass.h>
#include <Ext/Rules/Body.h>
#include <Utilities/Macro.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/GeneralUtils.h>
#include <Utilities/Cast.h>

DEFINE_HOOK(0x6FE3F1, TechnoClass_FireAt_OccupyDamageBonus, 0x9)
{
	GET(TechnoClass* const, pThis, ESI);

	if (auto const Building = specific_cast<BuildingClass*>(pThis))
	{
		GET_STACK(int, nDamage, 0x2C);
		R->EAX(int(nDamage * BuildingTypeExtContainer::Instance.Find(Building->Type)->BuildingOccupyDamageMult.Get(RulesClass::Instance->OccupyDamageMultiplier)));
		return 0x6FE405;
	}

	return 0;
}

DEFINE_HOOK(0x6FE421, TechnoClass_FireAt_BunkerDamageBonus, 0x9)
{
	GET(TechnoClass* const, pThis, ESI);

	if (auto const Building = specific_cast<BuildingClass*>(pThis->BunkerLinkedItem))
	{
		GET_STACK(int, nDamage, 0x2C);
		R->EAX(int(nDamage * BuildingTypeExtContainer::Instance.Find(Building->Type)->BuildingBunkerDamageMult.Get(RulesClass::Instance->OccupyDamageMultiplier)));
		return 0x6FE435;
	}

	return 0;
}

DEFINE_HOOK(0x6FD183, TechnoClass_RearmDelay_BuildingOccupyROFMult, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if (auto const Building = specific_cast<BuildingClass*>(pThis))
	{
		auto const nMult = BuildingTypeExtContainer::Instance.Find(Building->Type)->BuildingOccupyROFMult.Get(RulesClass::Instance->OccupyROFMultiplier);

		if (nMult != 0.0f)
		{
			GET_STACK(int, nROF, STACK_OFFS(0x10, -0x4));
			R->EAX(int(((double)nROF) / nMult));
			return 0x6FD1AB;
		}

		return 0x6FD1B1;
	}

	return 0;
}

#pragma region BunkerSounds
DEFINE_HOOK(0x45933D, BuildingClass_BunkerWallSound, 0x5)
{
	GET(BuildingClass* const, pThis, ESI);
	const auto pTypeExt = BuildingTypeExtContainer::Instance.Find(pThis->Type);

	if (pTypeExt)
	{
		const auto nSound = pTypeExt->BunkerWallsUpSound.Get(RulesClass::Instance->BunkerWallsUpSound);
		VocClass::PlayIndexAtPos(nSound, pThis->Location);
	}

	return 0x459374;
}

DEFINE_HOOK(0x4595D9, BuildingClass_BunkerDownSound_1, 0x5)
{
	GET(BuildingClass* const, pThis, EDI);
	const auto pTypeExt = BuildingTypeExtContainer::Instance.Find(pThis->Type);

	if (pTypeExt)
	{
		const auto nSound = pTypeExt->BunkerWallsDownSound.Get(RulesClass::Instance->BunkerWallsDownSound);
		VocClass::PlayIndexAtPos(nSound, pThis->Location);
	}

	return 0x459612;
}

DEFINE_HOOK(0x459494, BuildingClass_BunkerDownSound_2, 0x5)
{
	GET(BuildingClass* const, pThis, ESI);
	const auto pTypeExt = BuildingTypeExtContainer::Instance.Find(pThis->Type);

	if (pTypeExt)
	{
		const auto nSound = pTypeExt->BunkerWallsDownSound.Get(RulesClass::Instance->BunkerWallsDownSound);
		VocClass::PlayIndexAtPos(nSound, pThis->Location);
	}

	return 0x4594CD;
}
#pragma endregion

DEFINE_HOOK(0x450821, BuildingClass_Repair_AI_Step, 0x5)
{
	GET(BuildingClass* const, pThis, ESI);

	R->EAX(int(BuildingTypeExtContainer::Instance.Find(pThis->Type)->RepairRate.Get(RulesClass::Instance->RepairRate) * 900.0));

	return 0x450837;
}

DEFINE_HOOK(0x712125, TechnoTypeClass_GetRepairStep_Building, 0x6)
{
	GET(TechnoTypeClass*, pThis, ECX);
	GET(RulesClass*, pRules, EAX);

	auto nStep = pRules->RepairStep;
	if (auto const pBuildingType = type_cast<BuildingTypeClass*>(pThis))
		nStep = BuildingTypeExtContainer::Instance.Find(pBuildingType)->RepairStep.Get(nStep);

	R->EAX(nStep);

	return 0x71212B;
}

DEFINE_HOOK(0x7120D0, TechnoTypeClass_GetRepairCost_Building, 0x7)
{
	GET(TechnoTypeClass*, pThis, ECX);

	int cost = pThis->GetCost();
	if (!cost || !pThis->Strength)
	{
		R->EAX(1);
		return 0x712119;
	}

	int nStep = RulesClass::Instance->RepairStep;
	if (auto const pBuildingType = type_cast<BuildingTypeClass*>(pThis))
	{
		if (BuildingTypeExtContainer::Instance.Find(pBuildingType)->RepairStep.isset())
			nStep = BuildingTypeExtContainer::Instance.Find(pBuildingType)->RepairStep;
	}

	if (!nStep)
	{
		R->EAX(1);
		return 0x712119;
	}

	const auto nCalc = int(((double)cost / int((double)pThis->Strength / nStep)) * RulesClass::Instance->RepairPercent);
	R->EAX(MaxImpl(nCalc, 1));
	return 0x712119;
}

DEFINE_HOOK(0x505F6C, HouseClass_GenerateAIBuildList_AIBuildInstead, 0x6)
{
	GET(HouseClass*, pHouse, ESI);

	if (!pHouse->IsControlledByHuman() && !pHouse->IsNeutral())
	{
		for (auto& nNodes : pHouse->Base.BaseNodes)
		{
			auto nIdx = nNodes.BuildingTypeIndex;
			if (nIdx >= 0)
			{
				const auto pBldTypeExt = BuildingTypeExtContainer::Instance.Find(BuildingTypeClass::Array->Items[nIdx]);

				if (!pBldTypeExt->AIBuildInsteadPerDiff.empty() && pBldTypeExt->AIBuildInsteadPerDiff[pHouse->GetCorrectAIDifficultyIndex()] != -1)
					nIdx = pBldTypeExt->AIBuildInsteadPerDiff[pHouse->GetCorrectAIDifficultyIndex()];

				nNodes.BuildingTypeIndex = nIdx;
			}
		}
	}

	return 0;
}