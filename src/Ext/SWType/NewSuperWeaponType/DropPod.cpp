#include "DropPod.h"

#include <Ext/Techno/Body.h>
#include <Ext/Rules/Body.h>

std::vector<const char*> SW_DropPod::GetTypeString() const
{
	return { "DropPod" };
}

bool SW_DropPod::Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer)
{
	SuperWeaponTypeClass* pSW = pThis->Type;
	auto pData = SWTypeExtContainer::Instance.Find(pSW);

	const auto nDeferment = pData->SW_Deferment.Get(-1);

	if (nDeferment <= 0)
		DroppodStateMachine::SendDroppods(pThis, pData, this, Coords);
	else
		this->newStateMachine(nDeferment, Coords, pThis);

	return true;
}

void SW_DropPod::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action(AresNewActionType::SuperWeaponAllowed);

	pData->EVA_Detected = VoxClass::FindIndexById("EVA_DropPodDetected");
	pData->EVA_Ready = VoxClass::FindIndexById("EVA_DropPodReady");
	pData->EVA_Activated = VoxClass::FindIndexById("EVA_DropPodActivated");

	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::DropPod;
	pData->CursorType = static_cast<int>(MouseCursorType::ParaDrop);

	pData->DroppodProp.Initialize();
}

void SW_DropPod::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	const char* section = pData->AttachedToObject->ID;

	INI_EX exINI(pINI);
	pData->DropPod_Minimum.Read(exINI, section, "DropPod.Minimum");
	pData->DropPod_Maximum.Read(exINI, section, "DropPod.Maximum");
	pData->DropPod_Veterancy.Read(exINI, section, "DropPod.Veterancy");
	pData->DropPod_Types.Read(exINI, section, "DropPod.Types");

	pData->Droppod_RetryCount.Read(exINI, section, "DropPod.RetryCount");

	pData->DroppodProp.Read(exINI, section);
}

bool SW_DropPod::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!IsLaunchsiteAlive(pBuilding))
		return false;

	if (!pData->SW_Lauchsites.empty() && pData->SW_Lauchsites.Contains(pBuilding->Type))
		return true;

	return IsSWTypeAttachedToThis(pData, pBuilding);
}

void DroppodStateMachine::Update()
{
	if (Finished())
	{
		SendDroppods(Super, GetTypeExtData(), Type, Coords);
	}
}

void DroppodStateMachine::SendDroppods(SuperClass* pSuper, SWTypeExtData* pData, NewSWType* pNewType, const CellStruct& loc)
{
	pData->PrintMessage(pData->Message_Activate, pSuper->Owner);

	const auto sound = pData->SW_ActivationSound.Get(-1);
	if (sound != -1)
	{
		VocClass::PlayGlobal(sound, Panning::Center, 1.0);
	}

	const auto& Types = !pData->DropPod_Types.empty()
		? pData->DropPod_Types
		: RulesExtData::Instance()->DropPodTypes;

	if (Types.empty())
	{
		return;
	}

	const int cMin = pData->DropPod_Minimum.Get(RulesExtData::Instance()->DropPodMinimum);
	const int cMax = pData->DropPod_Maximum.Get(RulesExtData::Instance()->DropPodMaximum);

	PlaceUnits(pSuper, pData->DropPod_Veterancy.Get(), Types, cMin, cMax, loc, false);
}

void DroppodStateMachine::PlaceUnits(SuperClass* pSuper, double veterancy, Iterator<TechnoTypeClass*> const Types, int cMin, int cMax, const CellStruct& Coords, bool retries)
{
	const auto pData = SWTypeExtContainer::Instance.Find(pSuper->Type);
	const int count = ScenarioClass::Instance->Random.RandomRanged(cMin, cMax);
	CellStruct cell = Coords;
	std::vector<std::pair<bool, int>> Succeededs(count, { false , pData->Droppod_RetryCount });
	const bool needRandom = Types.size() > 1;

	for (auto& [status, retrycount] : Succeededs)
	{
		if (!status && retrycount)
		{
			TechnoTypeClass* pType = Types[needRandom ? ScenarioClass::Instance->Random.RandomFromMax(Types.size() - 1) : 0];

			if (!pType || pType->Strength <= 0 || pType->WhatAmI() == BuildingTypeClass::AbsID)
			{
				continue;
			}

			FootClass* pFoot = static_cast<FootClass*>(pType->CreateObject(pSuper->Owner));
			if (!pFoot)
				continue;

			if (veterancy > pFoot->Veterancy.Veterancy)
			{
				pFoot->Veterancy.Add(veterancy);
			}

			CellStruct tmpCell = MapClass::Instance->NearByLocation(cell, pType->SpeedType, -1,
				pType->MovementZone, false, 1, 1, false, false, false, false, CellStruct::Empty, false, false);

			CoordStruct crd = CellClass::Cell2Coord(tmpCell);

			TechnoExtContainer::Instance.Find(pFoot)->LinkedSW = pSuper;
			if (TechnoExtData::CreateWithDroppod(pFoot, crd))
			{
				status = true;
			}
			else
			{
				--retrycount;
			}

			CellClass* pCell = MapClass::Instance->GetCellAt(tmpCell);
			int rnd = ScenarioClass::Instance->Random.RandomFromMax(7);

			for (int j = 0; j < 8; ++j)
			{
				FacingType dir = FacingType(((j + rnd) % 8) & 7);

				CellClass* pNeighbour = pCell->GetNeighbourCell(dir);
				if (pFoot->IsCellOccupied(pNeighbour, FacingType::None, -1, nullptr, true) == Move::OK)
				{
					cell = pNeighbour->MapCoords;
					break;
				}
			}

			if (pFoot->InLimbo)
			{
				pFoot->UnInit();
			}
		}
	}
}