#include <Ext/Building/Body.h>

#include <SpecificStructures.h>
#include <ScenarioClass.h>
#include <WarheadTypeClass.h>

#include <Ext/BuildingType/Body.h>
#include <Ext/Anim/Body.h>
#include <Utilities/Macro.h>

#include <TacticalClass.h>

#ifndef ENABLE_NEWHOOKS
// Brief :
// The AnimClass* memory is owned by the vector , dont delete it
// just un-init it and replace it with nullptr is enough
namespace DamageFireAnims
{
	void FORCEDINLINE HandleRemoveAsExt(BuildingExtData* pExt)
	{
		for (auto& nFires : pExt->DamageFireAnims)
		{
			if (nFires && nFires->Type)
			{
				//GameDelete<true,false>(nFires);
				nFires->TimeToDie = true;
				nFires->UnInit();
				nFires = nullptr;
			}
		}
	}

	void FORCEDINLINE HandleRemove(FakeBuildingClass* pThis)
	{
		HandleRemoveAsExt(pThis->_GetExtData());
	}

	void FORCEDINLINE HandleInvalidPtr(FakeBuildingClass* pThis, void* ptr)
	{
		for (auto& nFires : pThis->_GetExtData()->DamageFireAnims)
		{
			if (nFires == ptr)
			{
				nFires = nullptr;
			}
		}
	}

	void Construct(BuildingClass* pThis)
	{
		const auto pType = pThis->Type;
		const auto pExt = BuildingExtContainer::Instance.Find(pThis);
		const auto pTypeext = pExt->Type;

		HandleRemoveAsExt(pExt);

		auto const& pFire = pTypeext->DamageFireTypes.GetElements(RulesClass::Instance->DamageFireTypes);

		if (!pFire.empty() &&
			!pTypeext->DamageFire_Offs.empty())
		{
			pExt->DamageFireAnims.resize(pTypeext->DamageFire_Offs.size());
			const auto render_coords = pThis->GetRenderCoords();
			const auto nBuildingHeight = pType->GetFoundationHeight(false);
			const auto nWidth = pType->GetFoundationWidth();

			for (int i = 0; i < (int)pTypeext->DamageFire_Offs.size(); ++i)
			{
				const auto& nFireOffs = pTypeext->DamageFire_Offs[i];
				const auto& [nPiX, nPiY] = TacticalClass::Instance->ApplyOffsetPixel(nFireOffs);

				CoordStruct nPixCoord { nPiX, nPiY, 0 };
				nPixCoord += render_coords;

				if (const auto pFireType = pFire[pFire.size() == 1 ?
					 0 : ScenarioClass::Instance->Random.RandomFromMax(pFire.size() - 1)])
				{
					auto pAnim = GameCreate<AnimClass>(pFireType, nPixCoord);
					const auto nAdjust = ((3 * (nFireOffs.Y - 15 * nWidth + (-15) * nBuildingHeight)) >> 1) - 10;
					pAnim->ZAdjust = nAdjust > 0 ? 0 : nAdjust; //ZAdjust always negative
					if (pAnim->Type->End > 0)
						pAnim->Animation.Value = ScenarioClass::Instance->Random.RandomFromMax(pAnim->Type->End - 1);

					pAnim->Owner = pThis->Owner;
					pExt->DamageFireAnims[i] = pAnim;
				}
			}
		}
	}
};

DEFINE_HOOK(0x43FC90, BuildingClass_CreateDamageFireAnims, 0x7)
{
	GET(BuildingClass*, pThis, ESI);
	DamageFireAnims::Construct(pThis);
	return 0x43FC97;
}

//DEFINE_JUMP(CALL, 0x43FC92, GET_OFFSET(DamageFireAnims::Construct));
//DEFINE_HOOK(0x46038A , BuildingTypeClass_ReadINI_SkipDamageFireAnims, 0x6) { return 0x46048E; }
DEFINE_JUMP(LJMP, 0x46038A, 0x46048E);
DEFINE_JUMP(LJMP, 0x43C0D0, 0x43C29B);
//DEFINE_JUMP(LJMP,0x43BA72, 0x43BA7F); //remove memset for buildingFireAnims

#define HANDLEREMOVE_HOOKS(addr ,reg ,name, size ,ret) \
DEFINE_HOOK(addr , BuildingClass_##name##_DamageFireAnims , size ) { \
	GET(FakeBuildingClass*, pThis, reg);\
	DamageFireAnims::HandleRemove(pThis);\
	return ret;\
}

HANDLEREMOVE_HOOKS(0x43BDD5, ESI, DTOR, 0x6, 0x43BDF6)
HANDLEREMOVE_HOOKS(0x44AB87, EBP, Fire1, 0x6, 0x44ABAC)
HANDLEREMOVE_HOOKS(0x440076, ESI, Fire2, 0x6, 0x44009B)
HANDLEREMOVE_HOOKS(0x43FC99, ESI, Fire3, 0x6, 0x43FCBE)
HANDLEREMOVE_HOOKS(0x4458E4, ESI, Fire4, 0x6, 0x445905)

#undef HANDLEREMOVE_HOOKS

DEFINE_HOOK(0x4415F9, BuildingClass_handleRemoveFire5, 0x5)
{
	GET(FakeBuildingClass*, pThis, ESI);
	DamageFireAnims::HandleRemove(pThis);
	R->EBX(0);
	return 0x44161C;
}

DEFINE_HOOK(0x43C2A0, BuildingClass_RemoveFire_handle, 0x8) //was 5
{
	GET(FakeBuildingClass*, pThis, ECX);
	DamageFireAnims::HandleRemove(pThis);
	return 0x43C2C9;
}

//DEFINE_JUMP(LJMP, 0x44EA1C, 0x44EA2F);
DEFINE_HOOK(0x44EA1C, BuildingClass_DetachOrInvalidPtr_handle, 0x6)
{
	GET(FakeBuildingClass*, pThis, ESI);
	GET(void*, ptr, EBP);
	DamageFireAnims::HandleInvalidPtr(pThis, ptr);
	return 0x44EA2F;
}

//remove it from load
DEFINE_JUMP(LJMP, 0x454154, 0x454170);
//DEFINE_HOOK(0x454154 , BuildingClass_LoadGame_DamageFireAnims , 0x6) {
//	return 0x454170;
//}
#endif