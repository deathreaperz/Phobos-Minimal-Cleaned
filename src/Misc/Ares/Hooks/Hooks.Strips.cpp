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

#include <CCToolTip.h>

DEFINE_OVERRIDE_HOOK(0x6A7EEE, sub_6A7D70_Strip1, 0x6)
{
	GET(SidebarClass*, pThis, ESI);
	pThis->Tabs[pThis->ActiveTabIndex].Func_6A93F0_GScreenAddButton();
	return 0x6A7F9F;
}

DEFINE_OVERRIDE_HOOK(0x6A801C, sub_6A7D70_Strip2, 0x6)
{
	GET(SidebarClass*, pThis, ESI);
	pThis->Tabs[pThis->ActiveTabIndex].Deactivate();
	return 0x6A8061;
}

DEFINE_OVERRIDE_HOOK(0x6A64C9, SidebarClass_AddCameo_Strip, 6)
{
	GET(SidebarClass*, pThis, EBX);
	GET(int, nStrip, EDI);
	pThis->ChangeTab(nStrip);
	return 0x6A65D6;
}

DEFINE_OVERRIDE_HOOK(0x6A75B9, SidebarClass_SetActiveTab_Strip1, 6)
{
	GET(SidebarClass*, pThis, EBP);
	pThis->Tabs[pThis->ActiveTabIndex].Func_6A94B0_GScreenRemoveButton();
	return 0x6A7602;
}

DEFINE_OVERRIDE_HOOK(0x6A7619, SidebarClass_SetActiveTab_Strip2, 6)
{
	GET(SidebarClass*, pThis, EBP);
	pThis->Tabs[pThis->ActiveTabIndex].Func_6A93F0_GScreenAddButton();
	return 0x6A76CA;
}

DEFINE_OVERRIDE_HOOK(0x6A793F, SidebarClass_Update_Strip1, 6)
{
	GET(SidebarClass*, pThis, ESI);
	pThis->Tabs[pThis->ActiveTabIndex].Func_6A94B0_GScreenRemoveButton();
	return 0x6A7988;
}

DEFINE_OVERRIDE_HOOK(0x6A79A0, SidebarClass_Update_Strip2, 6)
{
	GET(SidebarClass*, pThis, ESI);
	pThis->Tabs[pThis->ActiveTabIndex].Func_6A93F0_GScreenAddButton();
	return 0x6A7A51;
}

DEFINE_OVERRIDE_HOOK(0x6A93F0, StripClass_Activate, 6)
{
	GET(StripClass*, pThis, ECX);
	pThis->AllowedToDraw = true;
	pThis->Activate();
	return 0x6A94A0;
}

DEFINE_OVERRIDE_HOOK(0x6A94B0, StripClass_Deactivate, 6)
{
	GET(StripClass*, pThis, ECX);
	pThis->AllowedToDraw = false;
	pThis->Deactivate();
	return 0x6A94E9;
}

//void NOINLINE Strip3(size_t nCurIdx, int Offset)
//{
//	auto selects = SelectClass::Buttons()[0][0];
//	for (size_t a = 0; a < nCurIdx; ++a)
//	{
//
//		ToolTip nToolTip {
//			a + Offset ,
//			selects->Rect,
//			nullptr,
//			true
//		};
//
//		CCToolTip::Instance->Add(nToolTip);
//		++selects;
//	}
//}
//
// Doest work for some reason 
// the compiled code producing exacly same result like ares one
//DEFINE_OVERRIDE_HOOK(0x6AC02F, sub_6ABD30_Strip3, 0x8)
//{
//	GET_STACK(size_t, nCurIdx, 0x14);
//	int Offset = 0x3E8;
//
//	for (int i = 0; i < 0xF0; ++i)
//		CCToolTip::Instance->Remove(i + Offset);
//
//	if (nCurIdx > 0)
//		Strip3(nCurIdx, Offset);
//	
//	return 0x6AC0A7;
//}
//
//DEFINE_OVERRIDE_HOOK(0x6ABFB2, sub_6ABD30_Strip2, 0x6)
//{
//	enum
//	{
//		ContinueLoop = 0x6ABF66,
//		BreakLoop = 0x6ABFC4,
//	};
//
//	GET(DWORD, pPtr, ESI);
//	const auto pCur = (pPtr + 0x3480);
//	R->ESI(pCur);
//	R->Stack(0x10, pCur);
//	return pCur < SelectClass::Buttons_endPtr.getAddrs() ?
//		ContinueLoop : BreakLoop;
//}
//
//DEFINE_OVERRIDE_HOOK(0x6ABF44, sub_6ABD30_Strip1, 0x5)
//{
//	R->ESI(SelectClass::ButtonsPtr());
//	return 0x6ABF49;
//}