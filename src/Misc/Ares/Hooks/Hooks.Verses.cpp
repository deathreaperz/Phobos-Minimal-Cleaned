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

#include <Ext/Anim/Body.h>
#include <Ext/AnimType/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/VoxelAnim/Body.h>

#include <New/Type/ArmorTypeClass.h>

#include <Notifications.h>

DEFINE_OVERRIDE_HOOK(0x75DDCC, WarheadTypeClass_GetVerses_Skipvanilla, 0x7)
{
	// should really be doing something smarter due to westwood's weirdass code, but cannot be bothered atm
	// will fix if it is reported to actually break things
	// this breaks 51E33D which stops infantry with verses (heavy=0 & steel=0) from targeting non-infantry at all
	// (whoever wrote that code must have quite a few gears missing in his head)

	return 0x75DE98;
}

void Debug(ObjectClass* pTarget, int nArmor, VersesData* pData, WarheadTypeClass* pWH, const char* pAdd)
{
	//if (IS_SAME_STR_(pTarget->get_ID(), "CAOILD"))
	//{
	//	auto const pArmor = ArmorTypeClass::FindFromIndex(nArmor);

	//	if (IS_SAME_STR_(pArmor->Name.data(), "oild"))
	//	{
	//		Debug::Log("[%s] WH[%s] against oild Flag :  FF %d , PA %d , RR %d [%fl] \n",
	//			pAdd,
	//			pWH->get_ID(),
	//			pData->Flags.ForceFire,
	//			pData->Flags.PassiveAcquire,
	//			pData->Flags.Retaliate,
	//			pData->Verses);
	//	}
	//}
}

//DEFINE_HOOK(0x6FF349, TechnoClass_FireAt_ReportSound, 0x6)
//{
//	GET(TechnoClass*, pThis, ESI);
//	GET(WeaponTypeClass*, pWeapon, EBX);
//
//	if (IS_SAME_STR_(pThis->get_ID(), "MPLN") && pWeapon->Report.Count)
//	{
//		auto nSound = pThis->weapon_sound_randomnumber_3C8;
//		Debug::Log("MPLN TechnoClass FireAt , ReporSound Result [%d] Idx[%d] count [%d] \n"
//			, pWeapon->Report[nSound % pWeapon->Report.Count] 
//			, nSound % pWeapon->Report.Count
//			, pWeapon->Report.Count);
//	}
//
//	return 0x0;
//}

DEFINE_OVERRIDE_HOOK(0x489235, GetTotalDamage_Verses, 0x8)
{
	GET(WarheadTypeClass*, pWH, EDI);
	GET(int, nArmor, EDX);
	GET(int, nDamage, ECX);

	const auto pExt = WarheadTypeExt::ExtMap.Find(pWH);
	const auto vsData = &pExt->Verses[nArmor];

	R->EAX(static_cast<int>((nDamage * vsData->Verses)));
	return 0x489249;
}

DEFINE_OVERRIDE_HOOK(0x6F7D3D, TechnoClass_CanAutoTargetObject_Verses, 0x7)
{
	enum { ReturnFalse = 0x6F894F, ContinueCheck = 0x6F7D55, };

	GET(ObjectClass*, pTarget, ESI);
	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, nArmor, EAX);

	const auto pData = WarheadTypeExt::ExtMap.Find(pWH);
	const auto vsData = &pData->Verses[nArmor];

	Debug(pTarget, nArmor, vsData, pWH, __FUNCTION__);

	return vsData->Flags.PassiveAcquire  //|| !(vsData->Verses <= 0.02)
		? ContinueCheck
		: ReturnFalse
		;
}

DEFINE_OVERRIDE_HOOK(0x6FCB6A, TechnoClass_CanFire_Verses, 0x7)
{
	enum { FireIllegal = 0x6FCB7E, ContinueCheck = 0x6FCB8D, };

	GET(ObjectClass*, pTarget, EBP);
	GET(WarheadTypeClass*, pWH, EDI);
	GET(int, nArmor, EAX);

	const auto pData = WarheadTypeExt::ExtMap.Find(pWH);
	const auto vsData = &pData->Verses[nArmor];

	Debug(pTarget, nArmor, vsData, pWH, __FUNCTION__);

	return vsData->Flags.ForceFire || vsData->Verses != 0.0
		? ContinueCheck
		: FireIllegal
		;
}

DEFINE_OVERRIDE_HOOK(0x70CEA0, TechnoClass_EvalThreatRating_TargetWeaponWarhead_Verses, 0x6)
{
	GET(TechnoClass*, pThis, EDI);
	GET(TechnoClass*, pTarget, ESI);
	GET(WarheadTypeClass*, pTargetWH, EAX);
	GET_STACK(double, mult, 0x18);
	GET(TechnoTypeClass*, pThisType, EBX);

	const auto pData = WarheadTypeExt::ExtMap.Find(pTargetWH);
	const auto vsData = &pData->Verses[(int)pThisType->Armor];

	double nMult = 0.0;

	Debug(pThis, (int)pThisType->Armor, vsData, pTargetWH, __FUNCTION__);

	if (pTarget->Target == pThis)
		nMult = -(mult * vsData->Verses);
	else
		nMult = mult * vsData->Verses;

	R->Stack(0x10, nMult);
	return 0x70CED2;
}

DEFINE_OVERRIDE_HOOK(0x70CF45, TechnoClass_EvalThreatRating_ThisWeaponWarhead_Verses, 0xB)
{
	GET(ObjectClass*, pTarget, ESI);
	//GET(WeaponTypeClass*, pWeapon, EBX);
	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, nArmor, EAX);
	GET_STACK(double, dmult, 0x10);
	GET_STACK(double, dCoeff, 0x30);

	const auto pData = WarheadTypeExt::ExtMap.Find(pWH);
	const auto vsData = &pData->Verses[nArmor];

	Debug(pTarget, nArmor, vsData, pWH, __FUNCTION__);
	R->Stack(0x10, dCoeff * vsData->Verses + dmult);
	return 0x70CF58;
}

DEFINE_OVERRIDE_HOOK(0x6F36E3, TechnoClass_SelectWeapon_Verses, 0x5)
{
	enum
	{
		UseSecondary = 0x6F3745,
		ContinueCheck = 0x6F3754,
		UsePrimary = 0x6F37AD,
	};

	GET(TechnoClass*, pTarget, EBP);
	GET_STACK(WeaponTypeClass*, pWeapon_A, 0x10);
	GET_STACK(WeaponTypeClass*, pWeapon_B, 0x14);

	const int nArmor = (int)pTarget->GetTechnoType()->Armor;
	const auto vsData_A = &WarheadTypeExt::ExtMap.Find(pWeapon_A->Warhead)->Verses[nArmor];

	if (vsData_A->Verses == 0.0)
		return UsePrimary;

	const auto vsData_B = &WarheadTypeExt::ExtMap.Find(pWeapon_B->Warhead)->Verses[nArmor];

	return vsData_B->Verses != 0.0 ? ContinueCheck : UseSecondary;
}

DEFINE_OVERRIDE_HOOK(0x708AF7, TechnoClass_ShouldRetaliate_Verses, 0x7)
{
	enum { Retaliate = 0x708B0B, DoNotRetaliate = 0x708B17 };

	GET(WarheadTypeClass*, pWH, ECX);
	GET(int, nArmor, EAX);

	const auto pData = WarheadTypeExt::ExtMap.Find(pWH);
	const auto vsData = &pData->Verses[nArmor];

	return vsData->Flags.Retaliate //|| !(vsData->Verses <= 0.0099999998)
		? Retaliate
		: DoNotRetaliate
		;
}

DEFINE_OVERRIDE_HOOK(0x4753F0, ArmorType_FindIndex, 0xA)
{
	GET(CCINIClass*, pINI, ECX);

	ArmorTypeClass::AddDefaults();

	GET_STACK(const char*, Section, 0x4);
	GET_STACK(const char*, Key, 0x8);
	GET_STACK(int, fallback, 0xC);

	int nResult = fallback;
	char buf[0x64];
	if (pINI->ReadString(Section, Key, Phobos::readDefval, buf)) {
		nResult = ArmorTypeClass::FindIndexById(buf);

		if (nResult < 0) {
			nResult = fallback; //always
			Debug::INIParseFailed(Section, Key, buf);
		}
	}

	R->EAX(nResult);

	return 0x475430;
}