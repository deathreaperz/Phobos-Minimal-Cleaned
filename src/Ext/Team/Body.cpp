#include "Body.h"
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>

#include <AITriggerTypeClass.h>

void TeamExtData::InvalidatePointer(AbstractClass* ptr, bool bRemoved)
{
	AnnounceInvalidPointer(TeamLeader, ptr, bRemoved);
	AnnounceInvalidPointer(LastFoundSW, ptr);
	AnnounceInvalidPointer(PreviousScript, ptr);
}

bool TeamExtData::HouseOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, bool allies, const Iterator<TechnoTypeClass*>& list)
{
	int counter = 0;

	for (const auto& pItem : list)
	{
		for (auto pObject : *TechnoClass::Array)
		{
			if (!TechnoExtData::IsAlive(pObject))
				continue;

			if (((!allies && pObject->Owner == pHouse) || (allies && pHouse != pObject->Owner && pHouse->IsAlliedWith(pObject->Owner)))
				&& !pObject->Owner->Type->MultiplayPassive
				&& pObject->GetTechnoType() == pItem)
			{
				++counter;
			}
		}
	}

	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	bool result = false;
	switch (operand)
	{
	case 0: result = counter < type; break;
	case 1: result = counter <= type; break;
	case 2: result = counter == type; break;
	case 3: result = counter >= type; break;
	case 4: result = counter > type; break;
	case 5: result = counter != type; break;
	default: break;
	}
	return result;
}

bool TeamExtData::EnemyOwns(AITriggerTypeClass* pThis, HouseClass* pHouse, HouseClass* pEnemy, bool onlySelectedEnemy, const Iterator<TechnoTypeClass*>& list)
{
	int counter = 0;
	if (pEnemy && pHouse->IsAlliedWith(pEnemy) && !onlySelectedEnemy)
		pEnemy = nullptr;

	for (const auto& pItem : list)
	{
		for (auto pObject : *TechnoClass::Array)
		{
			if (!TechnoExtData::IsAlive(pObject))
				continue;

			if (pObject->Owner != pHouse
				&& (!pEnemy || (pEnemy && !pHouse->IsAlliedWith(pEnemy)))
				&& !pObject->Owner->Type->MultiplayPassive
				&& pObject->GetTechnoType() == pItem)
			{
				++counter;
			}
		}
	}

	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	bool result = false;
	switch (operand)
	{
	case 0: result = counter < type; break;
	case 1: result = counter <= type; break;
	case 2: result = counter == type; break;
	case 3: result = counter >= type; break;
	case 4: result = counter > type; break;
	case 5: result = counter != type; break;
	default: break;
	}
	return result;
}

bool TeamExtData::NeutralOwns(AITriggerTypeClass* pThis, const Iterator<TechnoTypeClass*>& list)
{
	int counter = 0;
	for (auto pHouse : *HouseClass::Array)
	{
		if (IS_SAME_STR_(SideClass::Array->Items[pHouse->Type->SideIndex]->Name, GameStrings::Civilian()))
			continue;
		for (const auto& pItem : list)
		{
			for (auto pObject : *TechnoClass::Array)
			{
				if (!TechnoExtData::IsAlive(pObject))
					continue;
				if (pObject->Owner == pHouse && pObject->GetTechnoType() == pItem)
					++counter;
			}
		}
	}
	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	bool result = false;
	switch (operand)
	{
	case 0: result = counter < type; break;
	case 1: result = counter <= type; break;
	case 2: result = counter == type; break;
	case 3: result = counter >= type; break;
	case 4: result = counter > type; break;
	case 5: result = counter != type; break;
	default: break;
	}
	return result;
}

bool TeamExtData::HouseOwnsAll(AITriggerTypeClass* pThis, HouseClass* pHouse, const Iterator<TechnoTypeClass*>& list)
{
	if (list.empty())
		return false;
	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	for (const auto& pItem : list)
	{
		int counter = 0;
		for (auto pObject : *TechnoClass::Array)
		{
			if (!TechnoExtData::IsAlive(pObject))
				continue;
			if (pObject->Owner == pHouse && pObject->GetTechnoType() == pItem)
				++counter;
		}
		bool result = false;
		switch (operand)
		{
		case 0: result = counter < type; break;
		case 1: result = counter <= type; break;
		case 2: result = counter == type; break;
		case 3: result = counter >= type; break;
		case 4: result = counter > type; break;
		case 5: result = counter != type; break;
		default: break;
		}
		if (!result)
			return false;
	}
	return true;
}

bool TeamExtData::EnemyOwnsAll(AITriggerTypeClass* pThis, HouseClass* pHouse, HouseClass* pEnemy, const Iterator<TechnoTypeClass*>& list)
{
	if (pEnemy && pHouse->IsAlliedWith(pEnemy))
		pEnemy = nullptr;
	if (list.empty())
		return false;
	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	for (const auto& pItem : list)
	{
		int counter = 0;
		for (auto pObject : *TechnoClass::Array)
		{
			if (!TechnoExtData::IsAlive(pObject) || !pObject->Owner)
				continue;
			if (pObject->Owner != pHouse
				&& (!pEnemy || (pEnemy && !pHouse->IsAlliedWith(pEnemy)))
				&& !pObject->Owner->Type->MultiplayPassive
				&& pObject->GetTechnoType() == pItem)
			{
				++counter;
			}
		}
		bool result = false;
		switch (operand)
		{
		case 0: result = counter < type; break;
		case 1: result = counter <= type; break;
		case 2: result = counter == type; break;
		case 3: result = counter >= type; break;
		case 4: result = counter > type; break;
		case 5: result = counter != type; break;
		default: break;
		}
		if (!result)
			return false;
	}
	return true;
}

bool TeamExtData::NeutralOwnsAll(AITriggerTypeClass* pThis, const Iterator<TechnoTypeClass*>& list)
{
	if (list.empty())
		return false;
	const int operand = pThis->Conditions[0].ComparatorOperand;
	const int type = pThis->Conditions[0].ComparatorType;
	for (auto pHouse : *HouseClass::Array)
	{
		if (IS_SAME_STR_(SideClass::Array->Items[pHouse->Type->SideIndex]->Name, GameStrings::Civilian()))
			continue;
		bool foundAll = true;
		for (const auto& pItem : list)
		{
			int counter = 0;
			for (auto pObject : *TechnoClass::Array)
			{
				if (!TechnoExtData::IsAlive(pObject))
					continue;
				if (pObject->Owner == pHouse && pObject->GetTechnoType() == pItem)
					++counter;
			}
			bool result = false;
			switch (operand)
			{
			case 0: result = counter < type; break;
			case 1: result = counter <= type; break;
			case 2: result = counter == type; break;
			case 3: result = counter >= type; break;
			case 4: result = counter > type; break;
			case 5: result = counter != type; break;
			default: break;
			}
			if (!result)
			{
				foundAll = false;
				break;
			}
		}
		if (!foundAll)
			return false;
	}
	return true;
}

bool TeamExtData::GroupAllowed(TechnoTypeClass* pThis, TechnoTypeClass* pThat)
{
	const auto pThatTechExt = TechnoTypeExtContainer::Instance.TryFind(pThat);
	const auto pThisTechExt = TechnoTypeExtContainer::Instance.TryFind(pThis);

	if (!pThatTechExt || !pThisTechExt)
		return false;

	if (GeneralUtils::IsValidString(pThatTechExt->GroupAs.c_str())) return IS_SAME_STR_(pThis->ID, pThatTechExt->GroupAs.c_str());
	else if (GeneralUtils::IsValidString(pThisTechExt->GroupAs.c_str())) return  IS_SAME_STR_(pThat->ID, pThisTechExt->GroupAs.c_str());

	return false;
}

// =============================
// load / save

template <typename T>
void TeamExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->Initialized)

		.Process(this->WaitNoTargetAttempts)
		.Process(this->NextSuccessWeightAward)
		.Process(this->IdxSelectedObjectFromAIList)
		.Process(this->CloseEnough)
		.Process(this->Countdown_RegroupAtLeader)
		.Process(this->MoveMissionEndMode)
		.Process(this->WaitNoTargetTimer)
		.Process(this->ForceJump_Countdown)
		.Process(this->ForceJump_InitialCountdown)
		.Process(this->ForceJump_RepeatMode)
		.Process(this->TeamLeader, true)

		.Process(this->LastFoundSW, true)

		.Process(this->ConditionalJump_Evaluation)
		.Process(this->ConditionalJump_ComparatorMode)
		.Process(this->ConditionalJump_ComparatorValue)
		.Process(this->ConditionalJump_Counter)
		.Process(this->ConditionalJump_Index)
		.Process(this->AbortActionAfterKilling)
		.Process(this->ConditionalJump_EnabledKillsCount)
		.Process(this->ConditionalJump_ResetVariablesIfJump)

		.Process(this->TriggersSideIdx)
		.Process(this->TriggersHouseIdx)

		.Process(this->AngerNodeModifier)
		.Process(this->OnlyTargetHouseEnemy)
		.Process(this->OnlyTargetHouseEnemyMode)

		.Process(this->PreviousScript)
		.Process(this->BridgeRepairHuts)
		;
}

// =============================
// container
TeamExtContainer TeamExtContainer::Instance;

// =============================
// container hooks

//Everything InitEd
ASMJIT_PATCH(0x6E8D05, TeamClass_CTOR, 0x5)
{
	GET(TeamClass*, pThis, ESI);
	TeamExtContainer::Instance.Allocate(pThis);
	return 0;
}

ASMJIT_PATCH(0x6E8ECB, TeamClass_DTOR, 0x7)
{
	GET(TeamClass*, pThis, ESI);
	TeamExtContainer::Instance.Remove(pThis);
	return 0;
}

#include <Misc/Hooks.Otamaa.h>

HRESULT __stdcall FakeTeamClass::_Load(IStream* pStm)
{
	TeamExtContainer::Instance.PrepareStream(this, pStm);
	HRESULT res = this->TeamClass::Load(pStm);

	if (SUCCEEDED(res))
		TeamExtContainer::Instance.LoadStatic();

	return res;
}

HRESULT __stdcall FakeTeamClass::_Save(IStream* pStm, bool clearDirty)
{
	TeamExtContainer::Instance.PrepareStream(this, pStm);
	HRESULT res = this->TeamClass::Save(pStm, clearDirty);

	if (SUCCEEDED(res))
		TeamExtContainer::Instance.SaveStatic();

	return res;
}

DEFINE_FUNCTION_JUMP(VTABLE, 0x7F4744, FakeTeamClass::_Load)
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F4748, FakeTeamClass::_Save)

ASMJIT_PATCH(0x6EAE60, TeamClass_Detach, 0x7)
{
	GET(TeamClass*, pThis, ECX);
	GET_STACK(AbstractClass*, target, 0x4);
	GET_STACK(bool, all, 0x8);

	TeamExtContainer::Instance.InvalidatePointerFor(pThis, target, all);

	//return pThis->Target == target ? 0x6EAECC : 0x6EAECF;
	return 0x0;
}

//void __fastcall TeamClass_Detach_Wrapper(TeamClass* pThis ,DWORD , AbstractClass* target , bool all)\
//{
//	TeamExtContainer::Instance.InvalidatePointerFor(pThis , target , all);
//	pThis->TeamClass::PointerExpired(target , all);
//}
//DEFINE_FUNCTION_JUMP(VTABLE, 0x7F4758, GET_OFFSET(TeamClass_Detach_Wrapper))