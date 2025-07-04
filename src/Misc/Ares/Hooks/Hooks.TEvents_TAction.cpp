#include <AbstractClass.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <UnitClass.h>
#include <Utilities/Macro.h>
#include <Helpers/Macro.h>
#include <Base/Always.h>

#include <HouseClass.h>
#include <Utilities/Debug.h>

#include <Ext/Anim/Body.h>
#include <Ext/AnimType/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/VoxelAnim/Body.h>
#include <Ext/Terrain/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/Side/Body.h>

#include <TerrainTypeClass.h>
#include <New/Type/ArmorTypeClass.h>
//#include <Lib/gcem/gcem.hpp>

#include <Notifications.h>

#include "Header.h"

#include <TEventClass.h>
#include <TActionClass.h>

ASMJIT_PATCH(0x6E20D8, TActionClass_DestroyAttached_Loop, 0x5)
{
	GET(int, nLoopVal, EAX);
	return nLoopVal < 4 ? 0x6E20E0 : 0x0;
}

ASMJIT_PATCH(0x41E893, AITriggerTypeClass_ConditionMet_SideIndex, 0xA)
{
	GET(HouseClass*, House, EDI);
	GET(int, triggerSide, EAX);

	enum { Eligible = 0x41E8D7, NotEligible = 0x41E8A1 };

	if (!triggerSide)
	{
		return Eligible;
	}

	return((triggerSide - 1) == House->SideIndex)
		? Eligible
		: NotEligible
		;
}

#include <Ext/TEvent/Body.h>

ASMJIT_PATCH(0x6E3EE0, TActionClass_GetFlags, 5)
{
	GET(AresNewTriggerAction, nAction, ECX);

	std::pair<TriggerAttachType, bool> _result = AresTActionExt::GetFlag(nAction);

	if (_result.second)
	{
		R->EAX(_result.first);
		return 0x6E3EFE;
	}

	_result = TEventExtData::GetFlag((PhobosTriggerEvent)nAction);

	if (_result.second)
	{
		R->EAX(_result.first);
		return 0x6E3EFE;
	}

	return 0;
}

ASMJIT_PATCH(0x6E3B60, TActionClass_GetMode, 8)
{
	GET(AresNewTriggerAction, nAction, ECX);

	std::pair<LogicNeedType, bool> _result = AresTActionExt::GetMode(nAction);

	if (_result.second)
	{
		R->EAX(_result.first);
		return 0x6E3C4B;
	}

	_result = TEventExtData::GetMode((PhobosTriggerEvent)nAction);

	if (_result.second)
	{
		R->EAX(_result.first);
		return 0x6E3C4B;
	}

	R->EAX(((int)nAction) - 1);
	return ((int)nAction) > 0x8F ? 0x6E3C49 : 0x6E3B6E;
}

ASMJIT_PATCH(0x71F9C0, TEventClass_Persistable_AresNewTriggerEvents, 6)
{
	GET(TEventClass*, pThis, ECX);
	auto const& [Flag, Handled] =
		AresTEventExt::GetPersistableFlag((AresTriggerEvents)pThis->EventKind);
	if (!Handled)
		return 0x0;

	R->EAX(Flag);
	return 0x71F9DF;
}

ASMJIT_PATCH(0x71F39B, TEventClass_SaveToINI, 5)
{
	GET(AresTriggerEvents, nAction, EDX);
	const auto& [Logic, handled] = AresTEventExt::GetLogicNeed(nAction);

	if (!handled)
		return (int)nAction > 61 ? 0x71F3FC : 0x71F3A0;

	R->EAX(Logic);
	return 0x71F3FE;
}

ASMJIT_PATCH(0x71f683, TEventClass_GetFlags_Ares, 5)
{
	GET(AresTriggerEvents, nAction, ECX);

	const auto& [handled, result] = AresTEventExt::GetAttachFlags(nAction);
	if (handled)
	{
		R->EAX(result);
		return 0x71F6F6;
	}

	return (int)nAction > 59 ? 0x71F69C : 0x71F688;
}

// the general events requiring a house
ASMJIT_PATCH(0x71F06C, EventClass_HasOccured_PlayerAtX1, 5)
{
	GET(int const, param, ECX);

	auto const pHouse = AresTEventExt::ResolveHouseParam(param);
	R->EAX(pHouse);

	// continue normally if a house was found or this isn't Player@X logic,
	// otherwise return false directly so events don't fire for non-existing
	// players.
	return (pHouse || !HouseClass::Index_IsMP(param)) ? 0x71F071u : 0x71F0D5u;
}

// validation for Spy as House, the Entered/Overflown Bys and the Crossed V/H Lines

ASMJIT_PATCH(0x71ED01, EventClass_HasOccured_PlayerAtX2, 5)
{
	GET(int const, param, ECX);
	R->EAX(AresTEventExt::ResolveHouseParam(param));
	return R->Origin() + 5;
}ASMJIT_PATCH_AGAIN(0x71ED33, EventClass_HasOccured_PlayerAtX2, 5)
ASMJIT_PATCH_AGAIN(0x71F1C9, EventClass_HasOccured_PlayerAtX2, 5)
ASMJIT_PATCH_AGAIN(0x71F1ED, EventClass_HasOccured_PlayerAtX2, 5)

// param for Attacked by House is the array index
ASMJIT_PATCH(0x71EE79, EventClass_HasOccured_PlayerAtX3, 9)
{
	GET(int, param, EAX);
	GET(HouseClass* const, pHouse, EDX);

	// convert Player @ X to real index
	if (HouseClass::Index_IsMP(param))
	{
		auto const pPlayer = AresTEventExt::ResolveHouseParam(param);
		param = pPlayer ? pPlayer->ArrayIndex : -1;
	}

	return (pHouse->ArrayIndex == param) ? 0x71EE82u : 0x71F163u;
}

ASMJIT_PATCH(0x71E949, TEventClass_HasOccured_Ares, 7)
{
	GET(TEventClass*, pThis, EBP);
	REF_STACK(EventArgs, args, (0x2C + 0x4));
	enum { return_true = 0x71F1B1, return_false = 0x71F163 };

	//const char* name = "Unknown";
	//if(args.EventType < TriggerEvent::count)
	//	name =TriggerEventsName[(int)args.EventType];

	bool result = false;
	//Debug::LogInfo("Event [%d - %s] IsOccured " , (int)args.EventType , name);
	if (AresTEventExt::HasOccured(pThis, args, result))
	{
		//Debug::LogInfo("Event [%d - %s] AresEventHas Occured " , (int)args.EventType , name);
		return result ? return_true : return_false;
	}
	//Debug::LogInfo("Event [%d - %s] AresEventNot Occured " , (int)args.EventType , name);

	return 0;
}