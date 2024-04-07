#include "Body.h"

DEFINE_HOOK(0x44AAD3, BuildingClass_Mi_Selling_Upgrades, 9)
{
	GET(BuildingTypeClass*, pUpgrades, ECX);
	GET(BuildingClass*, pThis, EBP);

	if (pUpgrades)
	{
		pThis->Owner->UnitsSelfHeal -= pUpgrades->UnitsGainSelfHeal;
		pThis->Owner->InfantrySelfHeal -= pUpgrades->InfantryGainSelfHeal;

		pThis->Owner->UnitsSelfHeal = (pThis->Owner->UnitsSelfHeal < 0) ? 0 : pThis->Owner->UnitsSelfHeal;
		pThis->Owner->InfantrySelfHeal = (pThis->Owner->InfantrySelfHeal < 0) ? 0 : pThis->Owner->InfantrySelfHeal;
	}

	return 0;
}

DEFINE_HOOK(0x445A9F, BuildingClass_Remove_Upgrades, 0x8)
{
	GET(BuildingClass*, pThis, ESI);

	for (int i = 0; i < 3; ++i)
	{
		auto const upgrade = pThis->Upgrades[i];

		if (!upgrade)
			continue;

		auto const pTargetHouse = pThis->Owner;
		pTargetHouse->InfantrySelfHeal -= upgrade->InfantryGainSelfHeal;
		pTargetHouse->UnitsSelfHeal -= upgrade->UnitsGainSelfHeal;

		pTargetHouse->InfantrySelfHeal = (pTargetHouse->InfantrySelfHeal < 0) ? 0 : pTargetHouse->InfantrySelfHeal;
		pTargetHouse->UnitsSelfHeal = (pTargetHouse->UnitsSelfHeal < 0) ? 0 : pTargetHouse->UnitsSelfHeal;

		if (upgrade->IsThreatRatingNode)
		{
			Debug::Log("Removing Upgrade [%d][%s] With IsThreatRatingNode = true!\n", i, upgrade->get_ID());
		}
	}

	R->Stack(0x13, true);
	return 0x445AC6;
}

DEFINE_HOOK(0x4516B1, BuildingClass_RemoveUpgrades_Add, 0x7)
{
	GET(BuildingTypeClass*, pUpgrades, EAX);
	GET(BuildingClass*, pThis, ESI);

	if (pThis->Owner)
	{
		pThis->Owner->InfantrySelfHeal -= pUpgrades->InfantryGainSelfHeal;
		pThis->Owner->UnitsSelfHeal -= pUpgrades->UnitsGainSelfHeal;

		pThis->Owner->InfantrySelfHeal = (pThis->Owner->InfantrySelfHeal < 0) ? 0 : pThis->Owner->InfantrySelfHeal;
		pThis->Owner->UnitsSelfHeal = (pThis->Owner->UnitsSelfHeal < 0) ? 0 : pThis->Owner->UnitsSelfHeal;
	}

	return 0;
}

DEFINE_HOOK(0x4492D7, BuildingClass_SetOwningHouse_Upgrades, 0x5)
{
	GET(BuildingClass*, pThis, ESI);
	GET(HouseClass*, pOld, EBX);
	GET(HouseClass*, pNew, EBP);

	BuildingExtContainer::Instance.Find(pThis)->AccumulatedIncome = 0;

	for (auto const& upgrade : pThis->Upgrades)
	{
		if (upgrade)
		{
			pOld->InfantrySelfHeal -= upgrade->InfantryGainSelfHeal;
			pOld->UnitsSelfHeal -= upgrade->UnitsGainSelfHeal;

			pOld->InfantrySelfHeal = (pOld->InfantrySelfHeal < 0) ? 0 : pOld->InfantrySelfHeal;
			pOld->UnitsSelfHeal = (pOld->UnitsSelfHeal < 0) ? 0 : pOld->UnitsSelfHeal;

			if (!pNew->Type->MultiplayPassive)
			{
				pNew->InfantrySelfHeal += upgrade->InfantryGainSelfHeal;
				pNew->UnitsSelfHeal += upgrade->UnitsGainSelfHeal;
			}
		}
	}

	return 0;
}