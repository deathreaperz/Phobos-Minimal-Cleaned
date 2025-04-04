#include "Body.h"

#include <Ext/TechnoType/Body.h>

#include <InfantryClass.h>

ASMJIT_PATCH(0x6F3AF9, TechnoClass_GetFLH_GetAlternateFLH, 0x5)
{
	GET(TechnoClass*, pThis, EBX);
	GET(int, weaponIdx, ESI);

	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());
	const auto conpy_weaponIdx = (-weaponIdx);

	if (conpy_weaponIdx <= 5 || pTypeExt->AlternateFLHs.empty())
		return 0x0;

	const auto conpy_weaponIdx_B = Math::abs(5 + weaponIdx);
	Debug::LogInfo("[{}] Trying to get Additional AlternateFLH at [original {} vs changed {}] !", pTypeExt->AttachedToObject->ID, conpy_weaponIdx, conpy_weaponIdx_B);

	if ((size_t)conpy_weaponIdx_B < pTypeExt->AlternateFLHs.size())
	{
		const CoordStruct& flh = pTypeExt->AlternateFLHs[conpy_weaponIdx_B];

		R->ECX(flh.X);
		R->EBP(flh.Y);
		R->EAX(flh.Z);

		return 0x6F3B37;
	}

	return 0x0;
}

ASMJIT_PATCH(0x6F3B37, TechnoClass_Transform_6F3AD0_BurstFLH_1, 0x7)
{
	GET(TechnoClass*, pThis, EBX);
	GET_STACK(int, weaponIndex, STACK_OFFS(0xD8, -0x8));

	auto pExt = TechnoExtContainer::Instance.Find(pThis);

	std::pair<bool, CoordStruct> nResult =
		!pExt->CustomFiringOffset.has_value() ?
		TechnoExtData::GetBurstFLH(pThis, weaponIndex) :
		std::make_pair(true, pExt->CustomFiringOffset.value());

	if (!nResult.first && pThis->WhatAmI() == InfantryClass::AbsID)
	{
		nResult = TechnoExtData::GetInfantryFLH(reinterpret_cast<InfantryClass*>(pThis), weaponIndex);
	}

	if (nResult.first)
	{
		R->ECX(nResult.second.X);
		R->EBP(nResult.second.Y);
		R->EAX(nResult.second.Z);
		pExt->FlhChanged = true;
	}

	return 0;
}

ASMJIT_PATCH(0x6F3C88, TechnoClass_Transform_6F3AD0_BurstFLH_2, 0x6)
{
	GET(TechnoClass*, pThis, EBX);
	//GET_STACK(int, weaponIndex, STACK_OFFS(0xD8, -0x8));

	auto pExt = TechnoExtContainer::Instance.Find(pThis);
	if (pExt->FlhChanged)
	{
		pExt->FlhChanged = false;
		R->EAX(0); //clear the angle ?
	}

	return 0;
}