#include "Body.h"

#include <Ext/Building/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/SWType/Body.h>

#include <Utilities/GeneralUtils.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/Macro.h>

std::vector<std::string> BuildingTypeExtData::trenchKinds;
const DirStruct  BuildingTypeExtData::DefaultJuggerFacing = DirStruct { 0x7FFF };
const CellStruct BuildingTypeExtData::FoundationEndMarker = { 0x7FFF, 0x7FFF };

#include <Locomotor/Cast.h>
#include <ExtraHeaders/StackVector.h>

#include <EventClass.h>

bool FakeBuildingTypeClass::_CanUseWaypoint()
{
	return RulesExtData::Instance()->BuildingWaypoint;
}

bool BuildingTypeExtData::IsSameBuildingType(BuildingTypeClass* pType1, BuildingTypeClass* pType2)
{
	return ((pType1->BuildCat != BuildCat::Combat) == (pType2->BuildCat != BuildCat::Combat));
}

// Check whether can call the occupiers leave
bool BuildingTypeExtData::CheckOccupierCanLeave(HouseClass* pBuildingHouse, HouseClass* pOccupierHouse)
{
	if (!pOccupierHouse || !pBuildingHouse)
		return false;
	else if (pBuildingHouse == pOccupierHouse)
		return true;
	else if (SessionClass::Instance->GameMode == GameMode::Campaign && pOccupierHouse->IsInPlayerControl)
		return true;
	else if (!pOccupierHouse->IsControlledByHuman() && pOccupierHouse->IsAlliedWith(pBuildingHouse))
		return true;

	return false;
}

// Force occupiers leave, return: whether it should stop right now
bool BuildingTypeExtData::CleanUpBuildingSpace(BuildingTypeClass* pBuildingType, CellStruct topLeftCell, HouseClass* pHouse, TechnoClass* pExceptTechno)
{
	// Step 1: Find the technos inside of the building place grid.
	auto infantryCount = CellStruct::Empty;
	StackVector<TechnoClass*, 24> checkedTechnos {};
	StackVector<CellClass*, 24> checkedCells {};

	for (auto pFoundation = pBuildingType->GetFoundationData(false); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		auto currentCell = topLeftCell + *pFoundation;

		if (const auto pCell = MapClass::Instance->GetCellAt(currentCell))
		{
			auto pObject = pCell->FirstObject;

			while (pObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
				{
					const auto pCellTechno = static_cast<TechnoClass*>(pObject);
					const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pCellTechno->GetTechnoType());

					if (!pTypeExt->CanBeBuiltOn && pCellTechno != pExceptTechno) // No need to check house
					{
						const auto pFoot = static_cast<FootClass*>(pCellTechno);

						if (pFoot->GetCurrentSpeed() <= 0 || !pFoot->Locomotor->Is_Moving())
						{
							if (absType == AbstractType::Infantry)
								++infantryCount.X;

							checkedTechnos->push_back(pCellTechno);
						}
					}
				}

				pObject = pObject->NextObject;
			}

			checkedCells->push_back(pCell);
		}
	}

	if (checkedTechnos->empty()) // All in moving
		return false;

	// Step 2: Find the cells around the building.
	StackVector<CellClass*, 24> optionalCells {};

	for (auto pFoundation = pBuildingType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		auto searchCell = topLeftCell + *pFoundation;

		if (const auto pSearchCell = MapClass::Instance->GetCellAt(searchCell))
		{
			if (std::find(checkedCells->begin(), checkedCells->end(), pSearchCell) == checkedCells->end() // TODO If there is a cellflag (or CellExt) that can be used …
				&& !pSearchCell->GetBuilding()
				&& pSearchCell->IsClearToMove(SpeedType::Amphibious, true, true, ZoneType::None, MovementZone::Amphibious, -1, false))
			{
				optionalCells->push_back(pSearchCell);
			}
		}
	}

	if (optionalCells->empty()) // There is no place for scattering
		return true;

	// Step 3: Sort the technos by the distance out of the foundation.
	std::sort(&checkedTechnos[0], &checkedTechnos[checkedTechnos->size()], [optionalCells](TechnoClass* pTechnoA, TechnoClass* pTechnoB)
	{
		int minA = INT_MAX;
		int minB = INT_MAX;

		for (const auto& pOptionalCell : optionalCells.container()) // If there are many valid cells at start, it means most of occupiers will near to the edge
		{
			if (minA > 65536) // If distance squared is lower or equal to 256^2, then no need to calculate any more because it is on the edge
			{
				auto curA = static_cast<int>(pTechnoA->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curA < minA)
					minA = curA;
			}

			if (minB > 65536)
			{
				auto curB = static_cast<int>(pTechnoB->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curB < minB)
					minB = curB;
			}
		}

		return minA > minB;
	});

	// Step 4: Core, successively find the farthest techno and its closest valid destination.
	StackVector<TechnoClass*, 50> reCheckedTechnos {};

	struct InfantryCountInCell // Temporary struct
	{
		CellClass* position;
		int count;
	};
	StackVector<InfantryCountInCell, 25> infantryCells {};

	struct TechnoWithDestination // Also temporary struct
	{
		TechnoClass* techno;
		CellClass* destination;
	};
	StackVector<TechnoWithDestination, 25> finalOrder {};

	do
	{
		// Step 4.1: Push the technos discovered just now back to the vector.
		for (const auto& pRecheckedTechno : reCheckedTechnos.container())
		{
			if (pRecheckedTechno->WhatAmI() == AbstractType::Infantry)
				++infantryCount.X;

			checkedTechnos->push_back(pRecheckedTechno);
		}

		reCheckedTechnos->clear();

		// Step 4.2: Check the techno vector.
		for (const auto& pCheckedTechno : checkedTechnos.container())
		{
			CellClass* pDestinationCell = nullptr;

			// Step 4.2.1: Search the closest valid cell to be the destination.
			do
			{
				const auto location = pCheckedTechno->GetMapCoords();
				const bool isInfantry = pCheckedTechno->WhatAmI() == AbstractType::Infantry;
				const auto pCheckedType = pCheckedTechno->GetTechnoType();

				if (isInfantry) // Try to maximizing cells utilization
				{
					if (!infantryCells->empty() && infantryCount.Y >= (infantryCount.X / 3 + (infantryCount.X % 3 ? 1 : 0)))
					{
						std::sort(&infantryCells[0], &infantryCells[infantryCells->size()], [location](InfantryCountInCell cellA, InfantryCountInCell cellB)
{
	return cellA.position->MapCoords.DistanceFromSquared(location) < cellB.position->MapCoords.DistanceFromSquared(location);
						});

						for (auto& infantryCell : infantryCells.container())
						{
							if (static_cast<InfantryClass*>(pCheckedTechno)->Destination == infantryCell.position)
							{
								infantryCell.count = 3;
							}
							else if (infantryCell.count < 3 && infantryCell.position->IsClearToMove(pCheckedType->SpeedType, true, true, ZoneType::None, pCheckedType->MovementZone, -1, false))
							{
								pDestinationCell = infantryCell.position;
								++infantryCell.count;

								break;
							}
						}

						if (pDestinationCell)
							break; // Complete
					}
				}

				std::sort(&optionalCells[0], &optionalCells[optionalCells->size()], [location](CellClass* pCellA, CellClass* pCellB)
{
	return pCellA->MapCoords.DistanceFromSquared(location) < pCellB->MapCoords.DistanceFromSquared(location);
				});
				const auto minDistanceSquared = optionalCells[0]->MapCoords.DistanceFromSquared(location);

				for (const auto& pOptionalCell : optionalCells.container()) // Prioritize selecting empty cells
				{
					if (!pOptionalCell->FirstObject && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, ZoneType::None, pCheckedType->MovementZone, -1, false))
					{
						if (isInfantry) // Not need to remove it now
						{
							infantryCells->emplace_back(pOptionalCell, 1);
							++infantryCount.Y;
						}

						if (pOptionalCell->MapCoords.DistanceFromSquared(location) < (minDistanceSquared * 4)) // Empty cell is not too far
							pDestinationCell = pOptionalCell;

						break;
					}
				}

				if (!pDestinationCell)
				{
					StackVector<CellClass*, 10> deleteCells {};

					for (const auto& pOptionalCell : optionalCells.container())
					{
						auto pCurObject = pOptionalCell->FirstObject;
						StackVector<TechnoClass*, 4> optionalTechnos {};
						bool valid = true;

						while (pCurObject)
						{
							const auto absType = pCurObject->WhatAmI();

							if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
							{
								const auto pCurTechno = static_cast<TechnoClass*>(pCurObject);

								if (!BuildingTypeExtData::CheckOccupierCanLeave(pHouse, pCurTechno->Owner)) // Means invalid for all
								{
									deleteCells->push_back(pOptionalCell);
									valid = false;
									break;
								}

								optionalTechnos->push_back(pCurTechno);
							}

							pCurObject = pCurObject->NextObject;
						}

						if (valid && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, ZoneType::None, pCheckedType->MovementZone, -1, false))
						{
							for (const auto& pOptionalTechno : optionalTechnos.container())
							{
								reCheckedTechnos->push_back(pOptionalTechno);
							}

							if (isInfantry) // Not need to remove it now
							{
								infantryCells->emplace_back(pOptionalCell, 1);
								++infantryCount.Y;
							}

							pDestinationCell = pOptionalCell;
							break;
						}
					}

					for (const auto& pDeleteCell : deleteCells.container()) // Mark the invalid cells
					{
						checkedCells->push_back(pDeleteCell);
						optionalCells->erase(std::remove(optionalCells->begin(), optionalCells->end(), pDeleteCell), optionalCells->end());
					}
				}
			}
			while (false);

			// Step 4.2.2: Mark the cell and push back its surrounded cells, then prepare for the command.
			if (pDestinationCell)
			{
				if (std::find(checkedCells->begin(), checkedCells->end(), pDestinationCell) == checkedCells->end())
					checkedCells->push_back(pDestinationCell);

				if (std::find(optionalCells->begin(), optionalCells->end(), pDestinationCell) != optionalCells->end())
				{
					optionalCells->erase(std::remove(optionalCells->begin(), optionalCells->end(), pDestinationCell), optionalCells->end());
					auto searchCell = pDestinationCell->MapCoords - CellStruct { 1, 1 };

					for (int i = 0; i < 4; ++i)
					{
						for (int j = 0; j < 2; ++j)
						{
							if (const auto pSearchCell = MapClass::Instance->TryGetCellAt(searchCell))
							{
								if (std::find(checkedCells->begin(), checkedCells->end(), pSearchCell) == checkedCells->end()
									&& std::find(optionalCells->begin(), optionalCells->end(), pSearchCell) == optionalCells->end()
									&& !pSearchCell->GetBuilding()
									&& pSearchCell->IsClearToMove(SpeedType::Amphibious, true, true, ZoneType::None, MovementZone::Amphibious, -1, false))
								{
									optionalCells->push_back(pSearchCell);
								}
							}

							if (i % 2)
								searchCell.Y += static_cast<short>((i / 2) ? -1 : 1);
							else
								searchCell.X += static_cast<short>((i / 2) ? -1 : 1);
						}
					}
				}

				finalOrder->emplace_back(pCheckedTechno, pDestinationCell);
			}
			else // Can not build
			{
				return true;
			}
		}

		checkedTechnos->clear();
	}
	while (reCheckedTechnos->size());

	// Step 5: Confirm command execution.
	for (const auto& pThisOrder : finalOrder.container())
	{
		pThisOrder.techno->ClickedMission(Mission::Move, nullptr, pThisOrder.destination, pThisOrder.destination);
	}

	return false;
}

bool BuildingTypeExtData::AutoPlaceBuilding(BuildingClass* pBuilding)
{
	const auto pType = pBuilding->Type;
	const auto pTypeExt = BuildingTypeExtContainer::Instance.Find(pType);

	if (!pTypeExt->AutoBuilding.Get(RulesExtData::Instance()->AutoBuilding) || pType->LaserFence || pType->Gate || pType->ToTile)
		return false;

	const auto pHouse = pBuilding->Owner;

	if (pHouse->Buildings.Empty())
		return false;

	const auto foundation = pType->GetFoundationData(true);

	auto canBuildHere = [&pType, &pHouse, &foundation](CellStruct cell)
		{
			return DisplayClass::Instance->CanBuildHere(pType, pHouse->ArrayIndex, foundation, &cell) // Adjacent
				&& DisplayClass::Instance->ProximityCheck2(pType, pHouse->ArrayIndex, foundation, &cell); // NoShroud
		};

	const auto pHouseExt = HouseExtContainer::Instance.Find(pHouse);

	auto getMapCell = [&pHouseExt](BuildingClass* pBuilding)
		{
			if (!pBuilding->IsAlive || pBuilding->Health <= 0 || !pBuilding->IsOnMap || pBuilding->InLimbo || (BuildingExtContainer::Instance.Find(pBuilding)->LimboID != -1))
				return CellStruct::Empty;

			return pBuilding->GetMapCoords();
		};

	auto addPlaceEvent = [&pType, &pHouse](CellStruct cell)
		{
			const EventClass event
			(
				HouseClass::CurrentPlayer->ArrayIndex,
				EventType::PLACE,
				AbstractType::Building,
				pType->GetArrayIndex(),
				pType->Naval,
				cell
			);

			EventClass::AddEvent(&event);
		};

	if (pType->LaserFencePost || pType->Wall)
	{
		for (const auto& pOwned : pHouse->Buildings)
		{
			const auto pOwnedType = pOwned->Type;

			if (!pOwnedType->ProtectWithWall)
				continue;

			const auto baseCell = getMapCell(pOwned);

			if (!baseCell.IsValid())
				continue;

			const auto width = pOwnedType->GetFoundationWidth();
			const auto height = pOwnedType->GetFoundationHeight(true);
			auto cell = CellStruct::Empty;
			int index = 0, check = width + 1, count = 0;

			for (auto pFoundation = pOwnedType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
			{
				if (++index != check)
					continue;

				check += (++count & 1) ? 1 : (height * 2 + width + 1);
				const auto outsideCell = baseCell + *pFoundation;
				const auto pCell = MapClass::Instance->TryGetCellAt(outsideCell);

				if (pCell && pCell->CanThisExistHere(pOwnedType->SpeedType, pOwnedType, pHouse) && canBuildHere(outsideCell))
				{
					addPlaceEvent(outsideCell);
					return true;
				}
			}

			for (auto pFoundation = pOwnedType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
			{
				const auto outsideCell = baseCell + *pFoundation;
				const auto pCell = MapClass::Instance->TryGetCellAt(outsideCell);

				if (pCell && pCell->CanThisExistHere(pOwnedType->SpeedType, pOwnedType, pHouse) && canBuildHere(outsideCell))
					cell = outsideCell;
			}

			if (!cell.IsValid())
				continue;

			addPlaceEvent(cell);
			return true;
		}

		return false;
	}
	else if (pType->PowersUpBuilding[0])
	{
		for (const auto& pOwned : pHouse->Buildings)
		{
			if (!pOwned->CanUpgrade(pType, pHouse))
				continue;

			const auto cell = getMapCell(pOwned);

			if (cell == CellStruct::Empty || pOwned->CurrentMission == Mission::Selling || !canBuildHere(cell))
				continue;

			addPlaceEvent(cell);
			return true;
		}

		return false;
	}
	else if (pType->PlaceAnywhere)
	{
		for (const auto& pOwned : pHouse->Buildings)
		{
			if (!pOwned->Type->BaseNormal)
				continue;

			const auto cell = getMapCell(pOwned);

			if (cell == CellStruct::Empty || !canBuildHere(cell))
				continue;

			addPlaceEvent(cell);
			return true;
		}

		return false;
	}

	const auto buildGap = static_cast<short>(pTypeExt->AutoBuilding_Gap + pType->ProtectWithWall ? 1 : 0);
	const auto doubleGap = buildGap * 2;
	const auto width = pType->GetFoundationWidth() + doubleGap;
	const auto height = pType->GetFoundationHeight(true) + doubleGap;
	const auto speedType = pType->SpeedType == SpeedType::Float ? SpeedType::Float : SpeedType::Track;
	const auto buildable = speedType != SpeedType::Float;

	auto tryBuildAt = [&](DynamicVectorClass<BuildingClass*>& vector, bool baseNormal)
		{
			for (const auto& pBase : vector)
			{
				if (baseNormal && !pBase->Type->BaseNormal)
					continue;

				const auto baseCell = getMapCell(pBase);

				if (baseCell == CellStruct::Empty)
					continue;

				// TODO The construction area does not actually need to be so large, the surrounding space should be able to be occupied by other things
				// TODO It would be better if the Buildable check could be fit with ExtendedBuildingPlacing within this function.
				// TODO Similarly, it would be better if the following Adjacent & NoShroud check could be made within this function.
				auto cell = pType->PlaceAnywhere ? baseCell : MapClass::Instance->NearByLocation(baseCell, speedType, ZoneType::None, MovementZone::Normal, false,
					width, height, false, false, false, false, CellStruct::Empty, false, buildable);

				if (cell == CellStruct::Empty)
					return false;

				cell += CellStruct { buildGap, buildGap };

				if (!canBuildHere(cell))
					continue;

				addPlaceEvent(cell);
				return true;
			}

			return false;
		};

	if (pHouse->ConYards.Count > 0 && tryBuildAt(pHouse->ConYards, false))
		return true;

	return tryBuildAt(pHouse->Buildings, true);
}

bool BuildingTypeExtData::BuildLimboBuilding(BuildingClass* pBuilding)
{
	const auto pBuildingType = pBuilding->Type;

	if (BuildingTypeExtContainer::Instance.Find(pBuildingType)->LimboBuild)
	{
		const EventClass event
		(
			pBuilding->Owner->ArrayIndex,
			EventType::PLACE,
			AbstractType::Building,
			pBuildingType->GetArrayIndex(),
			pBuildingType->Naval,
			CellStruct { 1, 1 }
		);

		EventClass::AddEvent(&event);

		return true;
	}

	return false;
}

void BuildingTypeExtData::CreateLimboBuilding(BuildingClass* pBuilding, BuildingTypeClass* pType, HouseClass* pOwner, int ID)
{
	if (pBuilding || (pBuilding = static_cast<BuildingClass*>(pType->CreateObject(pOwner)), pBuilding))
	{
		// All of these are mandatory
		pBuilding->InLimbo = false;
		pBuilding->IsAlive = true;
		pBuilding->IsOnMap = true;

		// For reasons beyond my comprehension, the discovery logic is checked for certain logics like power drain/output in campaign only.
		// Normally on unlimbo the buildings are revealed to current player if unshrouded or if game is a campaign and to non-player houses always.
		// Because of the unique nature of LimboDelivered buildings, this has been adjusted to always reveal to the current player in singleplayer
		// and to the owner of the building regardless, removing the shroud check from the equation since they don't physically exist - Starkku
		if (SessionClass::IsCampaign())
			pBuilding->DiscoveredBy(HouseClass::CurrentPlayer);

		pBuilding->DiscoveredBy(pOwner);

		pOwner->RegisterGain(pBuilding, false);
		pOwner->UpdatePower();
		pOwner->RecheckTechTree = true;
		pOwner->RecheckPower = true;
		pOwner->RecheckRadar = true;
		pOwner->Buildings.AddItem(pBuilding);

		pOwner->ActiveBuildingTypes.Increment(pBuilding->Type->ArrayIndex);
		pOwner->UpdateSuperWeaponsUnavailable();

		auto const pBuildingExt = BuildingExtContainer::Instance.Find(pBuilding);

		HouseExtData::UpdateFactoryPlans(pBuilding);

		if (BuildingTypeExtContainer::Instance.Find(pType)->Academy)
			HouseExtContainer::Instance.Find(pOwner)->UpdateAcademy(pBuilding, true);

		if (pType->SecretLab)
		{
			pOwner->SecretLabs.AddItem(pBuilding);
			BuildingExtData::UpdateSecretLab(pBuilding);
		}

		pBuildingExt->LimboID = ID;
		pBuildingExt->TechnoExt->Shield.release();
		pBuildingExt->TechnoExt->Trails.clear();
		pBuildingExt->TechnoExt->RevengeWeapons.clear();
		pBuildingExt->TechnoExt->DamageSelfState.release();
		pBuildingExt->TechnoExt->MyGiftBox.release();
		pBuildingExt->TechnoExt->PaintBallStates.clear();
		pBuildingExt->TechnoExt->ExtraWeaponTimers.clear();
		pBuildingExt->TechnoExt->MyWeaponManager.Clear();
		pBuildingExt->TechnoExt->MyWeaponManager.CWeaponManager.Clear();

		if (!HouseExtData::AutoDeathObjects.contains(pBuilding))
		{
			KillMethod nMethod = pBuildingExt->Type->Type->Death_Method.Get();

			if (nMethod != KillMethod::None
				&& pBuildingExt->Type->Type->Death_Countdown > 0
				&& !pBuildingExt->TechnoExt->Death_Countdown.HasStarted())
			{
				pBuildingExt->TechnoExt->Death_Countdown.Start(pBuildingExt->Type->Type->Death_Countdown);
				HouseExtData::AutoDeathObjects.emplace_unchecked(pBuilding, nMethod);
			}
		}
	}
}

int BuildingTypeExtData::CountOwnedNowWithDeployOrUpgrade(BuildingTypeClass* pBuilding, HouseClass* pHouse)
{
	const auto upgrades = BuildingTypeExtData::GetUpgradesAmount(pBuilding, pHouse);

	if (upgrades != -1)
		return upgrades;

	return pHouse->CountOwnedNow(pBuilding) + (pBuilding->UndeploysInto ? pHouse->CountOwnedNow(pBuilding->UndeploysInto) : 0);
}

bool BuildingTypeExtData::IsAcademy() const
{
	if (this->Academy.empty())
	{
		this->Academy = this->AcademyInfantry > 0.0
			|| this->AcademyAircraft > 0.0
			|| this->AcademyVehicle > 0.0
			|| this->AcademyBuilding > 0.0;
	}

	return this->Academy;
}

void BuildingTypeExtData::UpdateFoundationRadarShape()
{
	this->FoundationRadarShape.Clear();

	if (this->IsCustom)
	{
		auto pType = this->AttachedToObject;
		auto pRadar = RadarClass::Global();

		int width = pType->GetFoundationWidth();
		int height = pType->GetFoundationHeight(false);

		// transform between cell length and pixels on radar
		auto Transform = [](int length, double factor) -> int
			{
				double dblLength = length * factor + 0.5;
				double minLength = (length == 1) ? 1.0 : 2.0;

				if (dblLength < minLength)
				{
					dblLength = minLength;
				}

				return int(dblLength);
			};

		// the transformed lengths
		int pixelsX = Transform(width, pRadar->RadarSizeFactor);
		int pixelsY = Transform(height, pRadar->RadarSizeFactor);

		// heigth of the foundation tilted by 45�
		int rows = pixelsX + pixelsY - 1;

		// this draws a rectangle standing on an edge, getting
		// wider for each line drawn. the start and end values
		// are special-cased to not draw the pixels outside the
		// foundation.

		if (rows > 0)
		{
			int increment = 0;
			int start = 0;
			int end = 0;
			int YPixStart = 0;
			do
			{
				if (increment >= pixelsY)
				{
					start = increment - 2 * pixelsY + 2;
				}
				else
				{
					start = YPixStart;
				}

				if (increment >= pixelsX)
				{
					end = 2 * pixelsX - increment - 2;
				}
				else
				{
					end = increment;
				}

				if (start <= end)
				{
					do
					{
						this->FoundationRadarShape.AddItem(Point2D(start, increment));
						++start;
					}
					while (start <= end);
				}
				++increment;
				--YPixStart;
			}
			while (increment < rows);
		}
	}
}

void BuildingTypeExtData::UpdateBuildupFrames(BuildingTypeClass* pThis)
{
	auto pExt = BuildingTypeExtContainer::Instance.Find(pThis);

	if (const auto pShp = pThis->Buildup)
	{
		const auto frames = pThis->Gate ?
			pThis->GateStages + 1 : pShp->Frames / 2;

		const auto duration_build = pExt->BuildupTime.Get(RulesClass::Instance->BuildupTime);
		const auto duration_sell = pExt->SellTime.Get(duration_build);

		pExt->SellFrames = frames > 0 ? (int)(duration_sell / (double)frames * 900.0) : 1;
		pThis->BuildingAnimFrame[0].dwUnknown = 0;
		pThis->BuildingAnimFrame[0].FrameCount = frames;
		pThis->BuildingAnimFrame[0].FrameDuration = frames > 0 ? (int)(duration_build / (double)frames * 900.0) : 1;
	}
}

void BuildingTypeExtData::CompleteInitialization()
{
	auto const pThis = this->AttachedToObject;

	// enforce same foundations for rubble/intact building pairs
	if (this->RubbleDestroyed &&
		!BuildingTypeExtData::IsFoundationEqual(pThis, this->RubbleDestroyed))
	{
		Debug::FatalErrorAndExit(
			"BuildingType %s and its %s %s don't have the same foundation.",
			pThis->ID, "Rubble.Destroyed", this->RubbleDestroyed->ID);
	}

	if (this->RubbleIntact &&
		!BuildingTypeExtData::IsFoundationEqual(pThis, this->RubbleIntact))
	{
		Debug::FatalErrorAndExit(
			"BuildingType %s and its %s %s don't have the same foundation.",
			pThis->ID, "Rubble.Intact", this->RubbleIntact->ID);
	}

	BuildingTypeExtData::UpdateBuildupFrames(pThis);
}

bool BuildingTypeExtData::IsFoundationEqual(BuildingTypeClass* pType1, BuildingTypeClass* pType2)
{
	// both types must be set and must have same foundation id
	if (!pType1 || !pType2 || pType1->Foundation != pType2->Foundation)
	{
		return false;
	}

	// non-custom foundations need no special handling
	if (pType1->Foundation != BuildingTypeExtData::CustomFoundation)
	{
		return true;
	}

	// custom foundation
	auto const pExt1 = BuildingTypeExtContainer::Instance.Find(pType1);
	auto const pExt2 = BuildingTypeExtContainer::Instance.Find(pType2);
	const auto& data1 = pExt1->CustomData;
	const auto& data2 = pExt2->CustomData;

	// this works for any two foundations. it's linear with sorted ones
	return pExt1->CustomWidth == pExt2->CustomWidth
		&& pExt1->CustomHeight == pExt2->CustomHeight
		&& std::is_permutation(
			data1.begin(), data1.end(), data2.begin(), data2.end());
}

void BuildingTypeExtData::Initialize()
{
	this->Type = TechnoTypeExtContainer::Instance.Find(this->AttachedToObject);
	this->LostEvaEvent = VoxClass::FindIndexById(GameStrings::EVA_TechBuildingLost());
	this->PrismForwarding.Initialize(this->AttachedToObject);
	this->EVA_Online = VoxClass::FindIndexById(GameStrings::EVA_BuildingOnLine());
	this->EVA_Offline = VoxClass::FindIndexById(GameStrings::EVA_BuildingOffLine());
	this->NextBuilding_CurrentHeapId = this->AttachedToObject->ArrayIndex;
}

bool BuildingTypeExtData::CanBeOccupiedBy(InfantryClass* whom) const
{
	// if CanBeOccupiedBy isn't empty, we have to check if this soldier is allowed in
	if (!this->DisallowedOccupiers.empty() && this->DisallowedOccupiers.Contains(whom->Type))
		return false;

	if (!this->AllowedOccupiers.empty() && !this->AllowedOccupiers.Contains(whom->Type))
		return false;

	return true;
}

void BuildingTypeExtData::DisplayPlacementPreview()
{
	const auto pBuilding = cast_to<BuildingClass*>(DisplayClass::Instance->CurrentBuilding);

	if (!pBuilding)
		return;

	const auto pType = pBuilding->Type;
	const auto pTypeExt = BuildingTypeExtContainer::Instance.Find(pType);
	const bool bShow = pTypeExt->PlacementPreview_Show.Get(RulesExtData::Instance()->Building_PlacementPreview);

	if (!bShow)
		return;

	const auto pCell = MapClass::Instance->TryGetCellAt(Unsorted::Display_ZoneCell() + Unsorted::Display_ZoneOffset());
	if (!pCell)
		return;

	if (!MapClass::Instance->IsWithinUsableArea(pCell->GetCoords()))
		return;

	SHPStruct* Selected = nullptr;
	int nDecidedFrame = 0;

	if (!pTypeExt->PlacementPreview_Shape.isset())
	{
		if (const auto pBuildup = pType->LoadBuildup())
		{
			nDecidedFrame = ((pBuildup->Frames / 2) - 1);
			Selected = pBuildup;
		}
		else
		{
			Selected = pType->GetImage();
		}
	}
	else
	{
		Selected = pTypeExt->PlacementPreview_Shape.Get(nullptr);
	}

	if (!Selected)
		return;

	const auto& [nOffsetX, nOffsetY, nOffsetZ] = pTypeExt->PlacementPreview_Offset.Get();
	const auto nHeight = pCell->GetFloorHeight({ 0,0 });

	auto [nPoint, _result] = TacticalClass::Instance->GetCoordsToClientSituation(CellClass::Cell2Coord(pCell->MapCoords, nHeight + nOffsetZ));

	if (!_result)
		return;

	const auto nFrame = std::clamp(pTypeExt->PlacementPreview_ShapeFrame.Get(nDecidedFrame), 0, static_cast<int>(Selected->Frames));
	nPoint.X += nOffsetX;
	nPoint.Y += nOffsetY;
	const auto nFlag = BlitterFlags::Centered | BlitterFlags::Nonzero | BlitterFlags::MultiPass | EnumFunctions::GetTranslucentLevel(pTypeExt->PlacementPreview_TranslucentLevel.Get(RulesExtData::Instance()->BuildingPlacementPreview_TranslucentLevel));
	auto nREct = DSurface::Temp()->Get_Rect_WithoutBottomBar();

	ConvertClass* pDecidedPal = FileSystem::UNITx_PAL();

	if (!pTypeExt->PlacementPreview_Remap.Get())
	{
		if (const auto pCustom = pTypeExt->PlacementPreview_Palette.GetConvert())
		{
			pDecidedPal = pCustom;
		}
	}
	else
	{
		if (auto pVecPal = pTypeExt->PlacementPreview_Palette.ColorschemeDataVector)
		{
			pDecidedPal = pVecPal->Items[pBuilding->Owner->ColorSchemeIndex]->LightConvert;
		}
		else
		{
			pDecidedPal = pBuilding->GetRemapColour();
		}
	}

	DSurface::Temp()->DrawSHP(pDecidedPal, Selected, nFrame, &nPoint, &nREct, nFlag, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
}

bool BuildingTypeExtData::CanUpgrade(BuildingClass* pBuilding, BuildingTypeClass* pUpgradeType, HouseClass* pUpgradeOwner)
{
	const auto pUpgradeExt = BuildingTypeExtContainer::Instance.Find(pUpgradeType);
	if (EnumFunctions::CanTargetHouse(pUpgradeExt->PowersUp_Owner, pUpgradeOwner, pBuilding->Owner))
	{
		// PowersUpBuilding
		if (_stricmp(pBuilding->Type->ID, pUpgradeType->PowersUpBuilding) == 0)
			return true;

		// PowersUp.Buildings
		for (auto& pPowerUpBuilding : pUpgradeExt->PowersUp_Buildings)
		{
			if (_stricmp(pBuilding->Type->ID, pPowerUpBuilding->ID) == 0)
				return true;
		}
	}

	return false;
}

SuperClass* BuildingTypeExtData::GetSuperWeaponByIndex(int index, HouseClass* pHouse) const
{
	if (auto pSuper = pHouse->Supers.GetItemOrDefault(this->GetSuperWeaponIndex(index)))
	{
		if (SWTypeExtContainer::Instance.Find(pSuper->Type)->IsAvailable(pHouse))
		{
			return pSuper;
		}
	}

	return nullptr;
}

int BuildingTypeExtData::GetSuperWeaponIndex(const int index, HouseClass* pHouse) const
{
	const size_t idxSW = this->GetSuperWeaponIndex(index);

	if (idxSW < (size_t)pHouse->Supers.Count)
	{
		if (!SWTypeExtContainer::Instance.Find(pHouse->Supers[idxSW]->Type)->IsAvailable(pHouse))
		{
			return -1;
		}
	}

	return (int)idxSW;
}

int BuildingTypeExtData::GetSuperWeaponIndex(const int index) const
{
	const auto pThis = this->AttachedToObject;

	if (index < 2)
	{
		return !index ? pThis->SuperWeapon : pThis->SuperWeapon2;
	}
	else if (index - 2 < (int)this->SuperWeapons.size())
	{
		return this->SuperWeapons[index - 2];
	}

	return -1;
}

int BuildingTypeExtData::GetBuildingAnimTypeIndex(BuildingClass* pThis, const BuildingAnimSlot& nSlot, const char* pDefault)
{
	//pthis check is just in  case
	if (pThis->IsAlive
	   && (pThis->Occupants.Count > 0)
	   && pThis->Occupants[0]->IsAlive
	   && pThis->Occupants[0]->Owner
	   && pThis->Occupants[0]->Owner->Type
	   )
	{
		const auto pBuildingExt = BuildingTypeExtContainer::Instance.Find(pThis->Type);

		{
			const auto nIndex = pThis->Occupants[0]->Owner->Type->ArrayIndex;
			if (nIndex != -1)
			{
				AnimTypeClass* pDecidedAnim = nullptr;

				switch (nSlot)
				{
				case BuildingAnimSlot::Active:
					pDecidedAnim = pBuildingExt->GarrisonAnim_ActiveOne[nIndex];
					break;
				case BuildingAnimSlot::ActiveTwo:
					pDecidedAnim = pBuildingExt->GarrisonAnim_ActiveTwo[nIndex];
					break;
				case BuildingAnimSlot::ActiveThree:
					pDecidedAnim = pBuildingExt->GarrisonAnim_ActiveThree[nIndex];
					break;
				case BuildingAnimSlot::ActiveFour:
					pDecidedAnim = pBuildingExt->GarrisonAnim_ActiveFour[nIndex];
					break;
				case BuildingAnimSlot::Idle:
					pDecidedAnim = pBuildingExt->GarrisonAnim_idle[nIndex];
					break;
				default:
					break;
				}

				if (pDecidedAnim)
				{
					return pDecidedAnim->ArrayIndex;
				}
			}
		}
	}

	return AnimTypeClass::FindIndexById(pDefault);
}

void __fastcall BuildingTypeExtData::DrawPlacementGrid(Surface* Surface, ConvertClass* Pal, SHPStruct* SHP, int FrameIndex, const Point2D* const Position, const RectangleStruct* const Bounds, BlitterFlags Flags, int Remap, int ZAdjust, ZGradient ZGradientDescIndex, int Brightness, int TintColor, SHPStruct* ZShape, int ZShapeFrame, int XOffset, int YOffset)
{
	const auto nFlag = Flags | EnumFunctions::GetTranslucentLevel(RulesExtData::Instance()->PlacementGrid_TranslucentLevel.Get());

	CC_Draw_Shape(Surface, Pal, SHP, FrameIndex, Position, Bounds, nFlag, Remap, ZAdjust,
		ZGradientDescIndex, Brightness, TintColor, ZShape, ZShapeFrame, XOffset, YOffset);
}

bool BuildingTypeExtData::IsLinkable(BuildingTypeClass* pThis)
{
	const auto pExt = BuildingTypeExtContainer::Instance.Find(pThis);

	return pExt->Firestorm_Wall || pExt->IsTrench >= 0;
}

int BuildingTypeExtData::GetEnhancedPower(BuildingClass* pBuilding, HouseClass* pHouse)
{
	int nAmount = 0;
	float fFactor = 1.0f;

	auto const pHouseExt = HouseExtContainer::Instance.Find(pHouse);
	for (const auto& [pBldType, nCount] : pHouseExt->PowerPlantEnhancerBuildings)
	{
		const auto pExt = BuildingTypeExtContainer::Instance.Find(pBldType);
		if (pExt->PowerPlantEnhancer_Buildings.empty() || !pExt->PowerPlantEnhancer_Buildings.Contains(pBuilding->Type))
			continue;

		fFactor *= std::powf(pExt->PowerPlantEnhancer_Factor, static_cast<float>(nCount));
		nAmount += pExt->PowerPlantEnhancer_Amount * nCount;
	}

	return static_cast<int>(std::round(pBuilding->GetPowerOutput() * fFactor)) + nAmount;
}

float BuildingTypeExtData::GetPurifierBonusses(HouseClass* pHouse)
{
	/*removing the counter reference
	 Unit unload , 73E437 done
	 inf storage Ai , 522DD9 done
	 limbo 1 , 44591F done
	 Grand open , 44637C done
	 Captured1 , 448AC2 done
	 Captured2 , 4491EB done*/

	float fFactor = 0.00f;

	if (!pHouse || pHouse->Defeated || pHouse->IsNeutral() || HouseExtData::IsObserverPlayer(pHouse))
		return 0.00f;

	auto pHouseExt = HouseExtContainer::Instance.Find(pHouse);

	// AI VirtualPurifiers only applicable outside campaign
	// the bonus is using default rules value
	//virtual purifier using rules value
	const bool Eligible = !pHouse->IsHumanPlayer && SessionClass::Instance->GameMode != GameMode::Campaign;
	const int bonusCount = !Eligible ? 0 : RulesClass::Instance->AIVirtualPurifiers[pHouse->GetAIDifficultyIndex()];
	const float bonusAI = RulesClass::Instance->PurifierBonus * bonusCount;

	for (const auto& [pBldType, nCount] : pHouseExt->Building_OrePurifiersCounter)
	{
		const auto pExt = BuildingTypeExtContainer::Instance.Find(pBldType);
		const auto bonusses = pExt->PurifierBonus.Get(RulesClass::Instance->PurifierBonus);

		if (bonusses > 0.00f)
		{
			fFactor += (bonusses * nCount);
		}
	}

	return (fFactor > 0.00f) ? fFactor + bonusAI : bonusAI;
}

double BuildingTypeExtData::GetExternalFactorySpeedBonus(TechnoClass* pWhat, HouseClass* pOwner)
{
	double fFactor = 0.0;

	if (!pWhat || !pOwner || pOwner->Defeated || pOwner->IsNeutral() || HouseExtData::IsObserverPlayer(pOwner))
		return fFactor;

	const auto pType = pWhat->GetTechnoType();
	if (!pType)
		return fFactor;

	auto pHouseExt = HouseExtContainer::Instance.Find(pOwner);
	if (pHouseExt->Building_BuildSpeedBonusCounter.empty())
		return fFactor;

	for (const auto& [pBldType, nCount] : pHouseExt->Building_BuildSpeedBonusCounter)
	{
		auto const pExt = BuildingTypeExtContainer::Instance.Find(pBldType);
		{
			if (!pExt->SpeedBonus.AffectedType.empty() && !pExt->SpeedBonus.AffectedType.Contains(pType))
				continue;

			auto nBonus = 0.000;
			switch ((((DWORD*)pWhat)[0]))
			{
			case AircraftTypeClass::vtable:
				nBonus = pExt->SpeedBonus.SpeedBonus_Aircraft;
				break;
			case BuildingTypeClass::vtable:
				nBonus = pExt->SpeedBonus.SpeedBonus_Building;
				break;
			case UnitTypeClass::vtable:
				nBonus = pExt->SpeedBonus.SpeedBonus_Unit;
				break;
			case InfantryTypeClass::vtable:
				nBonus = pExt->SpeedBonus.SpeedBonus_Infantry;
				break;
			default:
				continue;
				break;
			}

			if (nBonus == 0.000)
				continue;

			fFactor *= std::pow(nBonus, nCount);
		}
	}

	return fFactor;
}

double BuildingTypeExtData::GetExternalFactorySpeedBonus(TechnoTypeClass* pWhat, HouseClass* pOwner)
{
	double fFactor = 1.0;
	if (!pWhat || !pOwner || pOwner->Defeated || pOwner->IsNeutral() || HouseExtData::IsObserverPlayer(pOwner))
		return fFactor;

	auto pHouseExt = HouseExtContainer::Instance.Find(pOwner);
	if (pHouseExt->Building_BuildSpeedBonusCounter.empty())
		return fFactor;

	const auto what = pWhat->WhatAmI();
	for (const auto& [pBldType, nCount] : pHouseExt->Building_BuildSpeedBonusCounter)
	{
		//if (pBldType->PowerDrain && pOwner->HasLowPower())
		//	continue;

		if (auto const pExt = BuildingTypeExtContainer::Instance.TryFind(pBldType))
		{
			if (!pExt->SpeedBonus.AffectedType.empty() && !pExt->SpeedBonus.AffectedType.Contains(pWhat))
				continue;

			auto nBonus = 0.000;
			switch (what)
			{
			case AircraftTypeClass::AbsID:
				nBonus = pExt->SpeedBonus.SpeedBonus_Aircraft;
				break;
			case BuildingTypeClass::AbsID:
				nBonus = pExt->SpeedBonus.SpeedBonus_Building;
				break;
			case UnitTypeClass::AbsID:
				nBonus = pExt->SpeedBonus.SpeedBonus_Unit;
				break;
			case InfantryTypeClass::AbsID:
				nBonus = pExt->SpeedBonus.SpeedBonus_Infantry;
				break;
			default:
				continue;
				break;
			}

			if (nBonus == 0.000)
				continue;

			fFactor *= std::pow(nBonus, nCount);
		}
	}

	return fFactor;
}

double BuildingTypeExtData::GetExternalFactorySpeedBonus(TechnoClass* pWhat)
{
	return BuildingTypeExtData::GetExternalFactorySpeedBonus(pWhat, pWhat->GetOwningHouse());
}

int BuildingTypeExtData::GetUpgradesAmount(BuildingTypeClass* pBuilding, HouseClass* pHouse) // not including producing upgrades
{
	int result = 0;
	bool isUpgrade = false;
	auto pPowersUp = pBuilding->PowersUpBuilding;

	if (!pHouse)
		return -1;

	auto checkUpgrade = [pHouse, pBuilding, &result, &isUpgrade](BuildingTypeClass* pTPowersUp)
		{
			if (!pTPowersUp)
				return;

			isUpgrade = true;
			for (auto const& pBld : pHouse->Buildings)
			{
				if (pBld->Type == pTPowersUp)
				{
					for (auto const& pUpgrade : pBld->Upgrades)
					{
						if (pUpgrade && pUpgrade == pBuilding)
							++result;
					}
				}
			}
		};

	if (pPowersUp[0])
	{
		if (auto const pTPowersUp = BuildingTypeClass::Find(pPowersUp))
			checkUpgrade(pTPowersUp);
	}

	for (auto pTPowersUp : BuildingTypeExtContainer::Instance.Find(pBuilding)->PowersUp_Buildings)
		checkUpgrade(pTPowersUp);

	return isUpgrade ? result : -1;
}

void BuildingTypeExtData::LoadFromINIFile(CCINIClass* pINI, bool parseFailAddr)
{
	auto pThis = this->AttachedToObject;
	const char* pSection = pThis->ID;
	const char* pArtSection = (!pThis->ImageFile || !pThis->ImageFile[0]) ? pSection : pThis->ImageFile;
	auto pArtINI = &CCINIClass::INI_Art();

	if (pINI->ReadString(pSection, "IsTrench", "", Phobos::readBuffer) > 0)
	{
		/*  Find the name in the list of kinds; if the list is empty, distance is 0, if the item isn't in
			the list, the index is the current list's size(); if the returned iterator is beyond the list,
			add the name to the list, which makes the previously calculated index (th distance) valid.
			(changed by AlexB 2014-01-16)

			I originally thought of using a map here, but I figured the probability that the kinds list
			grows so long that the search through all kinds takes up significant time is very low, and
			vectors are far simpler to use in this situation.
		*/
		const auto it = std::find_if(trenchKinds.begin(), trenchKinds.end(), [](auto const& pItem)
						{ return pItem == Phobos::readBuffer; });

		this->IsTrench = std::distance(trenchKinds.begin(), it);
		if (it == trenchKinds.end())
		{
			trenchKinds.emplace_back(Phobos::readBuffer);
		}
	}

	if (!parseFailAddr)
	{
		INI_EX exINI(pINI);

		this->PowersUp_Owner.Read(exINI, pSection, "PowersUp.Owner");
		this->PowersUp_Buildings.Read(exINI, pSection, "PowersUp.Buildings");
		this->PowerPlantEnhancer_Buildings.Read(exINI, pSection, "PowerPlantEnhancer.PowerPlants");
		this->PowerPlantEnhancer_Amount.Read(exINI, pSection, "PowerPlantEnhancer.Amount");
		this->PowerPlantEnhancer_Factor.Read(exINI, pSection, "PowerPlantEnhancer.Factor");

		if (pThis->PowersUpBuilding[0] == NULL && !this->PowersUp_Buildings.empty())
			PhobosCRT::strCopy(pThis->PowersUpBuilding, this->PowersUp_Buildings[0]->ID);

		//this->AllowAirstrike.Read(exINI, pSection, "AllowAirstrike");

		this->Grinding_AllowAllies.Read(exINI, pSection, "Grinding.AllowAllies");
		this->Grinding_AllowOwner.Read(exINI, pSection, "Grinding.AllowOwner");
		this->Grinding_AllowTypes.Read(exINI, pSection, "Grinding.AllowTypes");
		this->Grinding_DisallowTypes.Read(exINI, pSection, "Grinding.DisallowTypes");
		this->Grinding_Sound.Read(exINI, pSection, "Grinding.Sound");
		this->Grinding_Weapon.Read(exINI, pSection, "Grinding.Weapon", true);
		this->Grinding_PlayDieSound.Read(exINI, pSection, "Grinding.PlayDieSound");
		this->Grinding_Weapon_RequiredCredits.Read(exINI, pSection, "Grinding.Weapon.RequiredCredits");

		this->DisplayIncome.Read(exINI, pSection, "DisplayIncome");
		this->DisplayIncome_Houses.Read(exINI, pSection, "DisplayIncome.Houses");
		this->DisplayIncome_Offset.Read(exINI, pSection, "DisplayIncome.Offset");

		this->FreeUnit_Count.Read(exINI, pSection, "FreeUnit.Count");

		// Ares SuperWeapons tag
		auto const& pArray = SuperWeaponTypeClass::Array;
		std::string str_Supers = GameStrings::SuperWeapons();
		if (pArray->IsAllocated && pArray->Count > 0)
		{
			this->SuperWeapons.Read(exINI, pSection, str_Supers.c_str());

			for (size_t i = 0;; ++i)
			{
				NullableIdxVector<SuperWeaponTypeClass*> _readsupers {};
				_readsupers.Read(exINI, pSection, (str_Supers + std::to_string(i)).c_str());

				if (!_readsupers.HasValue() || _readsupers.empty())
					break;

				this->SuperWeapons.insert(this->SuperWeapons.end(), _readsupers.begin(), _readsupers.end());
			}
		}

		this->Refinery_UseStorage.Read(exINI, pSection, "Refinery.UseStorage");
		//const auto IscompatibilityMode = Phobos::Otamaa::CompatibilityMode;

		this->PlacementPreview_Show.Read(exINI, pSection, "PlacementPreview.Show");

		if (!this->PlacementPreview_Show.isset())
			this->PlacementPreview_Show.Read(exINI, pSection, "PlacementPreview");

		if (pINI->GetString(pSection, "PlacementPreview.Shape", Phobos::readBuffer))
		{
			if (GeneralUtils::IsValidString(Phobos::readBuffer))
			{
				// we cannot load same SHP file twice it may produce artifact , prevent it !
				if (IMPL_STRCMPI(Phobos::readBuffer, pSection) || IMPL_STRCMPI(Phobos::readBuffer, pArtSection))
					this->PlacementPreview_Shape.Read(exINI, pSection, "PlacementPreview.Shape");
				else
					Debug::LogInfo("Cannot Load PlacementPreview.Shape for [{}] Art [{}] ! ", pSection, pArtSection);
			}
		}

		this->PlacementPreview_ShapeFrame.Read(exINI, pSection, "PlacementPreview.ShapeFrame");
		this->PlacementPreview_Offset.Read(exINI, pSection, "PlacementPreview.Offset");
		this->PlacementPreview_Remap.Read(exINI, pSection, "PlacementPreview.Remap");
		this->PlacementPreview_Palette.Read(exINI, pSection, "PlacementPreview.Palette");
		this->PlacementPreview_TranslucentLevel.Read(exINI, pSection, "PlacementPreview.Translucency");

		this->AutoBuilding.Read(exINI, pSection, "AutoBuilding");
		this->AutoBuilding_Gap.Read(exINI, pSection, "AutoBuilding.Gap");
		this->LimboBuild.Read(exINI, pSection, "LimboBuild");
		this->LimboBuildID.Read(exINI, pSection, "LimboBuildID");
		this->LaserFencePost_Fence.Read(exINI, pSection, "LaserFencePost.Fence");
		this->PlaceBuilding_OnLand.Read(exINI, pSection, "PlaceBuilding.OnLand");
		this->PlaceBuilding_OnWater.Read(exINI, pSection, "PlaceBuilding.OnWater");

		this->Cameo_ShouldCount.Read(exINI, pSection, "Cameo.ShouldCount");

#pragma region Otamaa
		//   this->Get()->StartFacing = 32 * ((std::clamp(pINI->ReadInteger(pSection, "StartFacing", 0), 0, 255)) << 5);

		auto GetGarrisonAnim = [&exINI, pSection](
			std::vector<AnimTypeClass*>& nVec, const char* pBaseFlag, bool bAllocate = true, bool bParseDebug = false)
			{
				nVec.resize(HouseTypeClass::Array()->Count);
				//char tempBuffer[0x55];
				for (int i = 0; i < HouseTypeClass::Array()->Count; ++i)
				{
					AnimTypeClass* nBuffer;
					if (!detail::read(nBuffer, exINI, pSection, (std::string(pBaseFlag) + std::to_string(i)).c_str(), bAllocate) || !nBuffer)
						continue;

					nVec[i] = nBuffer;
				}
			};

		GetGarrisonAnim(this->GarrisonAnim_idle, "GarrisonAnim.IdleForCountry");
		GetGarrisonAnim(this->GarrisonAnim_ActiveOne, "GarrisonAnim.ActiveOneForCountry");
		GetGarrisonAnim(this->GarrisonAnim_ActiveTwo, "GarrisonAnim.ActiveTwoForCountry");
		GetGarrisonAnim(this->GarrisonAnim_ActiveThree, "GarrisonAnim.ActiveThreeForCountry");
		GetGarrisonAnim(this->GarrisonAnim_ActiveFour, "GarrisonAnim.ActiveFourForCountry");

		this->AIBuildInsteadPerDiff.Read(exINI, pSection, "AIBuildInstead");

		this->PackupSound_PlayGlobal.Read(exINI, pSection, "PackupSoundPlayGlobal");

		this->DamageFireTypes.Read(exINI, pSection, GameStrings::DamageFireTypes());

		this->RepairRate.Read(exINI, pSection, GameStrings::RepairRate());
		this->RepairStep.Read(exINI, pSection, GameStrings::RepairStep());

		this->DisableDamageSound.Read(exINI, pSection, "DisableFallbackDamagedSound");
		this->PlayerReturnFire.Read(exINI, pSection, "PlayerReturnFire");

		this->BuildingOccupyDamageMult.Read(exINI, pSection, GameStrings::OccupyDamageMultiplier());
		this->BuildingOccupyROFMult.Read(exINI, pSection, GameStrings::OccupyROFMultiplier());

		this->BuildingBunkerDamageMult.Read(exINI, pSection, GameStrings::BunkerDamageMultiplier());
		this->BuildingBunkerROFMult.Read(exINI, pSection, GameStrings::BunkerROFMultMultiplier());

		this->BunkerWallsUpSound.Read(exINI, pSection, GameStrings::BunkerWallsUpSound());
		this->BunkerWallsDownSound.Read(exINI, pSection, GameStrings::BunkerWallsDownSound());

		this->PipShapes01Palette.Read(exINI, pSection, "PipShapes.Palette");
		this->PipShapes01Remap.Read(exINI, pSection, "PipShapes.Remap");

		this->IsJuggernaut.Read(exINI, pSection, "IsJuggernaut");

		this->TurretAnim_LowPower.Read(exINI, pSection, "TurretAnim.LowPower");
		this->TurretAnim_DamagedLowPower.Read(exINI, pSection, "TurretAnim.DamagedLowPower");

		this->Power_DegradeWithHealth.Read(exINI, pSection, "Power.DegradeWithHealth");
		this->AutoSellTime.Read(exINI, pSection, "AutoSell.Time");
		this->BuildingPlacementGrid_Shape.Read(exINI, pSection, "PlacementGrid.Shape");
		this->SpeedBonus.Read(exINI, pSection);
		this->RadialIndicator_Visibility.Read(exINI, pSection, "RadialIndicatorVisibility");

		this->SpyEffect_Custom.Read(exINI, pSection, "SpyEffect.Custom");
		this->SpyEffect_VictimSuperWeapon.Read(exINI, pSection, "SpyEffect.VictimSuperWeapon");
		if (this->SpyEffect_VictimSuperWeapon.isset())
			this->SpyEffect_VictimSW_RealLaunch.Read(exINI, pSection, "SpyEffect.VictimSuperWeapon.RealLaunch");

		this->SpyEffect_InfiltratorSuperWeapon.Read(exINI, pSection, "SpyEffect.InfiltratorSuperWeapon");
		if (this->SpyEffect_InfiltratorSuperWeapon.isset())
			this->SpyEffect_InfiltratorSW_JustGrant.Read(exINI, pSection, "SpyEffect.InfiltratorSuperWeapon.JustGrant");

		this->SpyEffect_RevealProduction.Read(exINI, pSection, "SpyEffect.RevealProduction");
		this->SpyEffect_ResetSW.Read(exINI, pSection, "SpyEffect.ResetSuperweapons");
		this->SpyEffect_ResetRadar.Read(exINI, pSection, "SpyEffect.ResetRadar");
		this->SpyEffect_RevealRadar.Read(exINI, pSection, "SpyEffect.RevealRadar");
		this->SpyEffect_RevealRadarPersist.Read(exINI, pSection, "SpyEffect.KeepRadar");
		this->SpyEffect_GainVeterancy.Read(exINI, pSection, "SpyEffect.UnitVeterancy");

		ValueableVector<int> SpyEffect_StolenTechIndex;
		SpyEffect_StolenTechIndex.Read(exINI, pSection, "SpyEffect.StolenTechIndex");

		auto pos = SpyEffect_StolenTechIndex.begin();
		const auto end = SpyEffect_StolenTechIndex.end();

		if (pos != end)
		{
			this->SpyEffect_StolenTechIndex_result.reset();
			do
			{
				if ((*pos) > -1 && (*pos) < MaxHouseCount)
				{
					this->SpyEffect_StolenTechIndex_result.set((*pos));
				}
				else if ((*pos) != -1)
				{
					Debug::LogInfo("BuildingType {} has a SpyEffect.StolenTechIndex of {}. The value has to be less than 32.", pSection, (*pos));
					Debug::RegisterParserError();
				}
			}
			while (++pos != end);
		}

		this->SpyEffect_PowerOutageDuration.Read(exINI, pSection, "SpyEffect.PowerOutageDuration");
		this->SpyEffect_StolenMoneyAmount.Read(exINI, pSection, "SpyEffect.StolenMoneyAmount");
		this->SpyEffect_StolenMoneyPercentage.Read(exINI, pSection, "SpyEffect.StolenMoneyPercentage");
		this->SpyEffect_SabotageDelay.Read(exINI, pSection, "SpyEffect.SabotageDelay");
		this->SpyEffect_SuperWeapon.Read(exINI, pSection, "SpyEffect.SuperWeapon");
		this->SpyEffect_SuperWeaponPermanent.Read(exINI, pSection, "SpyEffect.SuperWeaponPermanent");
		this->SpyEffect_UnReverseEngineer.Read(exINI, pSection, "SpyEffect.UndoReverseEngineer");

		this->SpyEffect_InfantryVeterancy.Read(exINI, pSection, "SpyEffect.InfantryVeterancy"); { }
		this->SpyEffect_VehicleVeterancy.Read(exINI, pSection, "SpyEffect.VehicleVeterancy");
		this->SpyEffect_NavalVeterancy.Read(exINI, pSection, "SpyEffect.NavalVeterancy");
		this->SpyEffect_AircraftVeterancy.Read(exINI, pSection, "SpyEffect.AircraftVeterancy");
		this->SpyEffect_BuildingVeterancy.Read(exINI, pSection, "SpyEffect.BuildingVeterancy");

		this->SpyEffect_SellDelay.Read(exINI, pSection, "SpyEffect.SellDelay");

		this->SpyEffect_Anim.Read(exINI, pSection, "SpyEffect.Anim");
		this->SpyEffect_Anim_Duration.Read(exINI, pSection, "SpyEffect.Anim.Duration");
		this->SpyEffect_Anim_DisplayHouses.Read(exINI, pSection, "SpyEffect.Anim.DisplayHouses");

		this->SpyEffect_SWTargetCenter.Read(exINI, pSection, "SpyEffect.SWTargetCenter");
		this->ShowPower.Read(exINI, pSection, "ShowPower");
		this->EMPulseCannon_UseWeaponSelection.Read(exINI, pSection, "EMPulseCannon.UseWeaponSelection");

		this->CanC4_AllowZeroDamage.Read(exINI, pSection, "CanC4.AllowZeroDamage");
		this->C4_Modifier.Read(exINI, pSection, "C4Modifier");

		this->Solid_Height.Read(exINI, pSection, "SolidHeight");
		this->Solid_Level.Read(exINI, pSection, "SolidLevel");
		this->AIBaseNormal.Read(exINI, pSection, "AIBaseNormal");
		this->AIInnerBase.Read(exINI, pSection, "AIInnerBase");
		this->EngineerRepairable.Read(exINI, pSection, "EngineerRepairable");

		this->RubbleDestroyed.Read(exINI, pSection, "Rubble.Destroyed");
		this->RubbleIntact.Read(exINI, pSection, "Rubble.Intact");
		this->RubbleDestroyedAnim.Read(exINI, pSection, "Rubble.Destroyed.Anim");
		this->RubbleIntactAnim.Read(exINI, pSection, "Rubble.Intact.Anim");
		this->RubbleDestroyedOwner.Read(exINI, pSection, "Rubble.Destroyed.Owner");
		this->RubbleIntactOwner.Read(exINI, pSection, "Rubble.Intact.Owner");
		this->RubbleDestroyedStrength.Read(exINI, pSection, "Rubble.Destroyed.Strength");
		this->RubbleIntactStrength.Read(exINI, pSection, "Rubble.Intact.Strength");
		this->RubbleDestroyedRemove.Read(exINI, pSection, "Rubble.Destroyed.Remove");
		this->RubbleIntactRemove.Read(exINI, pSection, "Rubble.Intact.Remove");
		this->RubbleIntactConsumeEngineer.Read(exINI, pSection, "Rubble.Intact.ConsumeEngineer");

		this->TunnelType.Read(exINI, pSection, "Tunnel");

		this->SellBuildupLength.Read(exINI, pSection, "SellBuildupLength");

		this->UCPassThrough.Read(exINI, pSection, "UC.PassThrough");
		this->UCFatalRate.Read(exINI, pSection, "UC.FatalRate");
		this->UCDamageMultiplier.Read(exINI, pSection, "UC.DamageMultiplier");

		this->Cursor_Spy.Read(exINI, pSection, "Cursor.Spy");
		this->ImmuneToSaboteurs.Read(exINI, pSection, "ImmuneToSaboteurs");
		this->ReverseEngineersVictims.Read(exINI, pSection, "ReverseEngineersVictims");
		this->ReverseEngineersVictims_Passengers.Read(exINI, pSection, "ReverseEngineersVictims.IncludePassengers");

		this->Cursor_Sabotage.Read(exINI, pSection, "Cursor.Sabotage");

		this->GateDownSound.Read(exINI, pSection, "GateDownSound");
		this->GateUpSound.Read(exINI, pSection, "GateUpSound");
		this->UnitSell.Read(exINI, pSection, "UnitSell");

		this->LightningRod_Modifier.Read(exINI, pSection, "LightningRod.Modifier");
		this->Returnable.Read(exINI, pSection, "Returnable");
		this->BuildupTime.Read(exINI, pSection, "BuildupTime");
		this->SellTime.Read(exINI, pSection, "SellTime");
		this->SlamSound.Read(exINI, pSection, "SlamSound");
		this->Destroyed_CreateSmudge.Read(exINI, pSection, "Destroyed.CreateSmudge");

		//TODO , the hook is disabled
		// need better implementation
		//this->LaserFenceType.Read(exINI, pSection, "LaserFence.Type");
		//this->LaserFenceWEType.Read(exINI, pSection, "LaserFence.WEType");
		//this->LaserFencePostLinks.Read(exINI, pSection, "LaserFence.PostLinks");
		//this->LaserFenceDirection.Read(exINI, pSection, "LaserFence.Direction");

		// #218 Specific Occupiers
		this->AllowedOccupiers.Read(exINI, pSection, "CanBeOccupiedBy");
		if (!this->AllowedOccupiers.empty())
		{
			// having a specific occupier list implies that this building is supposed to be occupiable
			pThis->CanBeOccupied = true;
		}

		this->DisallowedOccupiers.Read(exINI, pSection, "CannotBeOccupiedBy");
		this->BunkerRaidable.Read(exINI, pSection, "Bunker.Raidable");
		this->Firestorm_Wall.Read(exINI, pSection, "Firestorm.Wall");

		//if (!pThis->FirestormWall && this->Firestorm_Wall)
		//	pThis->FirestormWall = this->Firestorm_Wall;
		//else if (!this->Firestorm_Wall && pThis->FirestormWall)
		//	this->Firestorm_Wall = pThis->FirestormWall;

		this->AbandonedSound.Read(exINI, pSection, "AbandonedSound");
		this->CloningFacility.Read(exINI, pSection, "CloningFacility");
		this->Factory_ExplicitOnly.Read(exINI, pSection, "Factory.ExplicitOnly");

		this->LostEvaEvent.Read(exINI, pSection, "LostEvaEvent");
		this->MessageCapture.Read(exINI, pSection, "Message.Capture");
		this->MessageLost.Read(exINI, pSection, "Message.Lost");

		this->AIBuildCounts.Read(exINI, pSection, "AIBuildCounts");
		this->AIExtraCounts.Read(exINI, pSection, "AIExtraCounts");
		this->LandingDir.Read(exINI, pSection, "LandingDir");

		std::string _boons = "SecretLab.PossibleBoons";

		this->Secret_Boons.Read(exINI, pSection, _boons.c_str());
		for (size_t i = 0;; ++i)
		{
			NullableVector<TechnoTypeClass*> _read {};
			_read.Read(exINI, pSection, (_boons + std::to_string(i)).c_str());

			if (!_read.HasValue() || _read.empty())
				break;

			this->Secret_Boons.insert(this->Secret_Boons.end(), _read.begin(), _read.end());
		}

		this->Secret_RecalcOnCapture.Read(exINI, pSection, "SecretLab.GenerateOnCapture");

		this->Academy.clear();
		this->AcademyWhitelist.Read(exINI, pSection, "Academy.Types");
		this->AcademyBlacklist.Read(exINI, pSection, "Academy.Ignore");
		this->AcademyInfantry.Read(exINI, pSection, "Academy.InfantryVeterancy");
		this->AcademyAircraft.Read(exINI, pSection, "Academy.AircraftVeterancy");
		this->AcademyVehicle.Read(exINI, pSection, "Academy.VehicleVeterancy");
		this->AcademyBuilding.Read(exINI, pSection, "Academy.BuildingVeterancy");
		this->IsAcademy();

		this->DegradeAmount.Read(exINI, pSection, "Degrade.Amount");
		this->DegradePercentage.Read(exINI, pSection, "Degrade.Percentage");
		this->IsPassable.Read(exINI, pSection, "IsPassable");
		this->ProduceCashDisplay.Read(exINI, pSection, "ProduceCashProclaim");
		this->ProduceCashDisplay.Read(exINI, pSection, "ProduceCashDisplay");

		this->Storage_ActiveAnimations.Read(exINI, pSection, "Storage.ActiveAnimations");

		this->PurifierBonus.Read(exINI, pSection, "PurifierBonus");
		this->PurifierBonus_RequirePower.Read(exINI, pSection, "PurifierBonus.RequirePower");
		this->FactoryPlant_RequirePower.Read(exINI, pSection, "FactoryPlant.RequirePower");
		this->SpySat_RequirePower.Read(exINI, pSection, "SpySat.RequirePower");
		this->Cloning_RequirePower.Read(exINI, pSection, "Cloning.RequirePower");
		this->SpawnCrewOnlyOnce.Read(exINI, pSection, "SpawnCrewOnlyOnce");
		this->IsDestroyableObstacle.Read(exINI, pSection, "IsDestroyableObstacle");

		this->EVA_Online.Read(exINI, pSection, "EVA.Online");
		this->EVA_Offline.Read(exINI, pSection, "EVA.Offline");

		this->Explodes_DuringBuildup.Read(exINI, pSection, "Explodes.DuringBuildup");

		this->FactoryPlant_AllowTypes.Read(exINI, pSection, "FactoryPlant.AllowTypes");
		this->FactoryPlant_DisallowTypes.Read(exINI, pSection, "FactoryPlant.DisallowTypes");

		if (Phobos::Otamaa::CompatibilityMode)
		{
			if (pThis->NumberOfDocks > 0)
			{
				this->DockPoseDir.clear();
				this->DockPoseDir.resize(pThis->NumberOfDocks);
				std::string base_tag("AircraftDockingDir");

				Nullable<DirType> nLandingDir;
				nLandingDir.Read(exINI, pSection, "AircraftDockingDir");

				if (nLandingDir.isset())
					this->DockPoseDir[0] = (FacingType)nLandingDir.Get();

				for (int i = 0; i < pThis->NumberOfDocks; ++i)
				{
					std::string tag = base_tag + std::to_string(i);
					nLandingDir.Read(exINI, pSection, tag.c_str());

					if (nLandingDir.isset())
						this->DockPoseDir[i] = (FacingType)nLandingDir.Get();
				}
			}
		}

		this->PrismForwarding.LoadFromINIFile(pThis, pINI);

		this->ExcludeFromMultipleFactoryBonus.Read(exINI, pSection, "ExcludeFromMultipleFactoryBonus");

		this->NoBuildAreaOnBuildup.Read(exINI, pSection, "NoBuildAreaOnBuildup");
		this->Adjacent_Allowed.Read(exINI, pSection, "Adjacent.Allowed");
		this->Adjacent_Disallowed.Read(exINI, pSection, "Adjacent.Disallowed");

		this->BarracksExitCell.Read(exINI, pSection, "BarracksExitCell");

		this->Units_RepairRate.Read(exINI, pSection, "Units.RepairRate");
		this->Units_RepairStep.Read(exINI, pSection, "Units.RepairStep");
		this->Units_RepairPercent.Read(exINI, pSection, "Units.RepairPercent");
		this->Units_UseRepairCost.Read(exINI, pSection, "Units.UseRepairCost");

		this->PowerPlant_DamageFactor.Read(exINI, pSection, "PowerPlant.DamageFactor");

		// Next Building
		{
			this->NextBuilding_Next.Read(exINI, pSection, "NextBuilding.Next");
			this->NextBuilding_Prev.Read(exINI, pSection, "NextBuilding.Prev");
		}

		this->AllowAlliesRepair.Read(exINI, pSection, "AllowAlliesRepair");
		this->AllowRepairFlyMZone.Read(exINI, pSection, "AllowRepairFlyMZone");

		this->Overpower_KeepOnline.Read(exINI, pSection, "Overpower.KeepOnline");
		this->Overpower_ChargeWeapon.Read(exINI, pSection, "Overpower.ChargeWeapon");

		this->NewEvaVoice.Read(exINI, pSection, "NewEVAVoice");
		this->NewEvaVoice_Index.Read(exINI, pSection, "NewEVAVoice.Index");
		this->NewEvaVoice_Priority.Read(exINI, pSection, "NewEVAVoice.Priority");
		this->NewEvaVoice_RecheckOnDeath.Read(exINI, pSection, "NewEVAVoice.RecheckOnDeath");
		this->NewEvaVoice_InitialMessage.Read(exINI, pSection, "NewEVAVoice.InitialMessage");

		this->BattlePointsCollector.Read(exINI, pSection, "BattlePointsCollector");
		this->BattlePointsCollector_RequirePower.Read(exINI, pSection, "BattlePointsCollector.RequirePower");

		this->BuildingRepairedSound.Read(exINI, pSection, "BuildingRepairedSound");
	}
#pragma endregion
	if (pArtINI->GetSection(pArtSection))
	{
		INI_EX exArtINI(pArtINI);

		this->Refinery_UseNormalActiveAnim.Read(exArtINI, pArtSection, "Refinery.UseNormalActiveAnim");

		if (this->IsCustom)
		{
			//Reset
			pThis->Foundation = BuildingTypeExtData::CustomFoundation;
			pThis->FoundationData = this->CustomData.data();
			pThis->FoundationOutside = this->OutlineData.data();
		}

		if (pThis->MaxNumberOccupants > 10)
		{
			//char tempMuzzleBuffer[32];
			this->OccupierMuzzleFlashes.clear();
			this->OccupierMuzzleFlashes.resize(pThis->MaxNumberOccupants);

			for (int i = 0; i < pThis->MaxNumberOccupants; ++i)
			{
				detail::read(this->OccupierMuzzleFlashes[i], exArtINI, pArtSection, (std::string(GameStrings::MuzzleFlash()) + std::to_string(i)).c_str());
			}
		}

		this->ZShapePointMove_OnBuildup.Read(exArtINI, pSection, "ZShapePointMove.OnBuildup");
#pragma region Otamaa
		this->HealthOnfire.Read(exArtINI, pArtSection, "OnFire.Health");

		this->DamageFire_Offs.clear();
		this->DamageFire_Offs.reserve(8u);

		//char tempFire_OffsBuffer[0x25];
		for (size_t i = 0;; ++i)
		{
			Point2D nFire_offs {};
			if (!detail::read(nFire_offs, exArtINI, pArtSection, (std::string(GameStrings::DamageFireOffset()) + std::to_string(i)).c_str()))
				break;

			this->DamageFire_Offs.emplace_back(nFire_offs);
		}

		this->BuildUp_UseNormalLIght.Read(exArtINI, pArtSection, "Buildup.UseNormalLight");
		this->RubblePalette.Read(exArtINI, pArtSection, "Rubble.Palette");

		if (!Phobos::Otamaa::CompatibilityMode)
		{
			if (pThis->Helipad)
			{
				//char keyDock[0x40];
				this->DockPoseDir.clear();
				this->DockPoseDir.resize(pThis->NumberOfDocks);

				for (int i = 0; i < pThis->NumberOfDocks; ++i)
				{
					detail::read(this->DockPoseDir[i], exArtINI, pArtSection, (std::string("DockingPoseDir") + std::to_string(i)).c_str(), false);
				}
			}
		}

#pragma endregion

		this->DockUnload_Cell.Read(exArtINI, pArtSection, "DockUnloadCell");
		this->DockUnload_Facing.Read(exArtINI, pArtSection, "DockUnloadFacing");
		this->IsAnimDelayedBurst.Read(exArtINI, pSection, "IsAnimDelayedBurst");
	}

	if (pThis->UnitRepair && pThis->Factory == AbstractType::AircraftType)
	{
		Debug::FatalErrorAndExit(
			"BuildingType [%s] has both UnitRepair=yes and Factory=AircraftType."
			"This combination causes Internal Errors and other unwanted behaviour.", pSection);
	}
}

bool BuildingTypeExtData::ShouldExistGreyCameo(TechnoTypeClass* pType)
{
	const auto techLevel = pType->TechLevel;

	if (techLevel <= 0 || techLevel > Game::TechLevel)
		return false;

	const auto pHouse = HouseClass::CurrentPlayer();

	if (!pHouse->InOwners(pType))
		return false;

	if (!pHouse->InRequiredHouses(pType))
		return false;

	if (pHouse->InForbiddenHouses(pType))
		return false;

	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);

	const auto& pNegTypes = pTypeExt->Cameo_NegTechnos;

	for (const auto& pNegType : pNegTypes)
	{
		if (pNegType && pHouse->CountOwnedAndPresent(pNegType))
			return false;
		else if (pNegType->WhatAmI() == AbstractType::BuildingType && BuildingTypeExtData::GetUpgradesAmount(static_cast<BuildingTypeClass*>(pNegType), pHouse) > 0)
			return false;
	}

	const auto& pAuxTypes = pTypeExt->Cameo_AuxTechnos;

	if (pAuxTypes.begin() == pAuxTypes.end())
	{
		const auto sideIndex = pType->AIBasePlanningSide;

		return (sideIndex == -1 || sideIndex == pHouse->Type->SideIndex);
	}

	for (const auto& pAuxType : pAuxTypes)
	{
		const auto pAuxTypeExt = TechnoTypeExtContainer::Instance.Find(pAuxType);

		if (!pAuxTypeExt->CameoCheckMutex)
		{
			if (pHouse->CountOwnedAndPresent(pAuxType))
				return true;
			else if (pAuxType->WhatAmI() == AbstractType::BuildingType && BuildingTypeExtData::GetUpgradesAmount(static_cast<BuildingTypeClass*>(pAuxType), pHouse) > 0)
				return true;

			pAuxTypeExt->CameoCheckMutex = true;
			const auto exist = BuildingTypeExtData::ShouldExistGreyCameo(pAuxType);
			pAuxTypeExt->CameoCheckMutex = false;

			if (exist)
				return true;
		}
	}

	return false;
}

#include <Ext/Scenario/Body.h>

// Check the cameo change
CanBuildResult BuildingTypeExtData::CheckAlwaysExistCameo(TechnoTypeClass* pType, CanBuildResult canBuild)
{
	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);

	if (pTypeExt->Cameo_AlwaysExist.Get(RulesExtData::Instance()->Cameo_AlwaysExist))
	{
		auto& vec = ScenarioExtData::Instance()->OwnedExistCameoTechnoTypes;
		const bool Ownedexist = vec.contains(pType);

		if (canBuild == CanBuildResult::Unbuildable) // Unbuildable + Satisfy basic limitations = Change it to TemporarilyUnbuildable
		{
			pTypeExt->CameoCheckMutex = true;
			const auto exist = BuildingTypeExtData::ShouldExistGreyCameo(pType);
			pTypeExt->CameoCheckMutex = false;

			if (exist)
			{
				if (!Ownedexist) // … + Not in the list = Need to add it into list
				{
					vec.push_back(pType);
					SidebarClass::Instance->SidebarNeedsRepaint();
					const EventClass event
					(
						HouseClass::CurrentPlayer->ArrayIndex,
						EventType::ABANDON_ALL,
						pType->WhatAmI(),
						pType->GetArrayIndex(),
						pType->Naval
					);
					EventClass::AddEvent(&event);
				}

				canBuild = CanBuildResult::TemporarilyUnbuildable;
			}
		}
		else if (Ownedexist) // Not Unbuildable + In the list = remove it from the list and play EVA
		{
			vec.remove(pType);
			SidebarClass::Instance->SidebarNeedsRepaint();
			VoxClass::Play(GameStrings::EVA_NewConstructionOptions);
		}
	}

	return canBuild;
}

template <typename T>
void BuildingTypeExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->Initialized)
		.Process(this->Type)
		.Process(this->PrismForwarding)
		.Process(this->PowersUp_Owner)
		.Process(this->PowersUp_Buildings)
		.Process(this->PowerPlantEnhancer_Buildings)
		.Process(this->PowerPlantEnhancer_Amount)
		.Process(this->PowerPlantEnhancer_Factor)
		.Process(this->SuperWeapons)
		.Process(this->OccupierMuzzleFlashes)
		.Process(this->Refinery_UseStorage)
		//.Process(this->AllowAirstrike)

		.Process(this->Grinding_AllowAllies)
		.Process(this->Grinding_AllowOwner)
		.Process(this->Grinding_AllowTypes)
		.Process(this->Grinding_DisallowTypes)
		.Process(this->Grinding_Sound)
		.Process(this->Grinding_Weapon)
		.Process(this->Grinding_PlayDieSound)
		.Process(this->Grinding_Weapon_RequiredCredits)

		.Process(this->PlacementPreview_Remap)
		.Process(this->PlacementPreview_Palette)
		.Process(this->PlacementPreview_Offset)
		.Process(this->PlacementPreview_Show)
		.Process(this->PlacementPreview_Shape)
		.Process(this->PlacementPreview_ShapeFrame)
		.Process(this->PlacementPreview_TranslucentLevel)

		.Process(this->DamageFireTypes)
		.Process(this->OnFireTypes)
		.Process(this->OnFireIndex)
		.Process(this->HealthOnfire)

		.Process(this->RubbleIntact)
		.Process(this->RubbleDestroyed)
		.Process(this->RubbleDestroyedAnim)
		.Process(this->RubbleIntactAnim)
		.Process(this->RubbleDestroyedOwner)
		.Process(this->RubbleIntactOwner)
		.Process(this->RubbleDestroyedStrength)
		.Process(this->RubbleIntactStrength)
		.Process(this->RubbleDestroyedRemove)
		.Process(this->RubbleIntactRemove)
		.Process(this->RubbleIntactConsumeEngineer)
		.Process(this->DamageFire_Offs)

		.Process(this->RepairRate)
		.Process(this->RepairStep)

		.Process(this->PlayerReturnFire)

		.Process(this->PackupSound_PlayGlobal)
		.Process(this->DisableDamageSound)

		.Process(this->BuildingOccupyDamageMult)
		.Process(this->BuildingOccupyROFMult)

		.Process(this->BuildingBunkerDamageMult)
		.Process(this->BuildingBunkerROFMult)

		.Process(this->BunkerWallsUpSound)
		.Process(this->BunkerWallsDownSound)

		.Process(this->AIBuildInsteadPerDiff)

		.Process(this->GarrisonAnim_idle)
		.Process(this->GarrisonAnim_ActiveOne)
		.Process(this->GarrisonAnim_ActiveTwo)
		.Process(this->GarrisonAnim_ActiveThree)
		.Process(this->GarrisonAnim_ActiveFour)

		.Process(this->PipShapes01Remap)
		.Process(this->PipShapes01Palette)

		.Process(this->TurretAnim_LowPower)
		.Process(this->TurretAnim_DamagedLowPower)
		.Process(this->BuildUp_UseNormalLIght)
		.Process(this->Power_DegradeWithHealth)
		.Process(this->AutoSellTime)
		.Process(this->IsJuggernaut)
		.Process(this->BuildingPlacementGrid_Shape)
		.Process(this->SpeedBonus)
		.Process(this->RadialIndicator_Visibility)
		.Process(this->SpyEffect_Custom)
		.Process(this->SpyEffect_VictimSuperWeapon)
		.Process(this->SpyEffect_InfiltratorSuperWeapon)
		.Process(this->SpyEffect_InfiltratorSW_JustGrant)
		.Process(this->SpyEffect_VictimSW_RealLaunch)
		.Process(this->SpyEffect_RevealProduction)
		.Process(this->SpyEffect_ResetSW)
		.Process(this->SpyEffect_ResetRadar)
		.Process(this->SpyEffect_RevealRadar)
		.Process(this->SpyEffect_RevealRadarPersist)
		.Process(this->SpyEffect_GainVeterancy)
		.Process(this->SpyEffect_UnReverseEngineer)
		.Process(this->SpyEffect_StolenTechIndex_result)
		.Process(this->SpyEffect_StolenMoneyAmount)
		.Process(this->SpyEffect_StolenMoneyPercentage)
		.Process(this->SpyEffect_PowerOutageDuration)
		.Process(this->SpyEffect_SabotageDelay)
		.Process(this->SpyEffect_SuperWeapon)
		.Process(this->SpyEffect_SuperWeaponPermanent)
		.Process(this->SpyEffect_InfantryVeterancy)
		.Process(this->SpyEffect_VehicleVeterancy)
		.Process(this->SpyEffect_NavalVeterancy)
		.Process(this->SpyEffect_AircraftVeterancy)
		.Process(this->SpyEffect_BuildingVeterancy)
		.Process(this->ZShapePointMove_OnBuildup)
		.Process(this->SellBuildupLength)
		.Process(this->CanC4_AllowZeroDamage)
		.Process(this->C4_Modifier)
		.Process(this->DockUnload_Cell)
		.Process(this->DockUnload_Facing)
		.Process(this->Solid_Height)
		.Process(this->Solid_Level)
		.Process(this->AIBaseNormal)
		.Process(this->AIInnerBase)
		.Process(this->RubblePalette)
		.Process(this->DockPoseDir)
		.Process(this->EngineerRepairable)
		.Process(this->IsTrench)
		.Process(this->TunnelType)
		.Process(this->UCPassThrough)
		.Process(this->UCFatalRate)
		.Process(this->UCDamageMultiplier)
		.Process(this->Cursor_Spy)
		.Process(this->Cursor_Sabotage)
		.Process(this->ImmuneToSaboteurs)
		.Process(this->ReverseEngineersVictims)
		.Process(this->ReverseEngineersVictims_Passengers)

		.Process(this->GateDownSound)
		.Process(this->GateUpSound)
		.Process(this->UnitSell)
		.Process(this->LightningRod_Modifier)
		.Process(this->Returnable)
		.Process(this->BuildupTime)
		.Process(this->SellTime)
		.Process(this->SlamSound)
		.Process(this->Destroyed_CreateSmudge)

		//.Process(this->LaserFenceType)
		//.Process(this->LaserFenceWEType)
		//.Process(this->LaserFencePostLinks)
		//.Process(this->LaserFenceDirection)
		.Process(this->AllowedOccupiers)
		.Process(this->DisallowedOccupiers)
		.Process(this->BunkerRaidable)
		.Process(this->Firestorm_Wall)

		.Process(this->AbandonedSound)
		.Process(this->CloningFacility)
		.Process(this->Factory_ExplicitOnly)

		.Process(this->LostEvaEvent)
		.Process(this->MessageCapture)
		.Process(this->MessageLost)
		.Process(this->AIBuildCounts)
		.Process(this->AIExtraCounts)
		.Process(this->LandingDir)
		.Process(this->SellFrames)

		.Process(this->IsCustom)
		.Process(this->CustomWidth)
		.Process(this->CustomHeight)
		.Process(this->OutlineLength)
		.Process(this->CustomData)
		.Process(this->OutlineData)
		.Process(this->FoundationRadarShape)
		.Process(this->Secret_Boons)
		.Process(this->Academy)
		.Process(this->Secret_RecalcOnCapture)
		.Process(this->AcademyWhitelist)
		.Process(this->AcademyBlacklist)
		.Process(this->AcademyInfantry)
		.Process(this->AcademyAircraft)
		.Process(this->AcademyVehicle)
		.Process(this->AcademyBuilding)
		.Process(this->DegradeAmount)
		.Process(this->DegradePercentage)
		.Process(this->IsPassable)
		.Process(this->ProduceCashDisplay)
		.Process(this->Storage_ActiveAnimations)
		.Process(this->PurifierBonus)
		.Process(this->PurifierBonus_RequirePower)
		.Process(this->FactoryPlant_RequirePower)
		.Process(this->SpySat_RequirePower)
		.Process(this->Cloning_RequirePower)
		.Process(this->DisplayIncome)
		.Process(this->DisplayIncome_Houses)
		.Process(this->DisplayIncome_Offset)
		.Process(this->FreeUnit_Count)
		.Process(this->SpawnCrewOnlyOnce)
		.Process(this->IsDestroyableObstacle)
		.Process(this->EVA_Online)
		.Process(this->EVA_Offline)
		.Process(this->Explodes_DuringBuildup)

		.Process(this->SpyEffect_SellDelay)
		.Process(this->SpyEffect_Anim)
		.Process(this->SpyEffect_Anim_Duration)
		.Process(this->SpyEffect_Anim_DisplayHouses)

		.Process(this->SpyEffect_SWTargetCenter)
		.Process(this->ShowPower)
		.Process(this->EMPulseCannon_UseWeaponSelection)

		.Process(this->FactoryPlant_AllowTypes)
		.Process(this->FactoryPlant_DisallowTypes)

		.Process(this->ExcludeFromMultipleFactoryBonus)

		.Process(this->NoBuildAreaOnBuildup)
		.Process(this->Adjacent_Allowed)
		.Process(this->Adjacent_Disallowed)

		.Process(this->BarracksExitCell)

		.Process(this->Units_RepairRate)
		.Process(this->Units_RepairStep)
		.Process(this->Units_RepairPercent)
		.Process(this->Units_UseRepairCost)
		.Process(this->PowerPlant_DamageFactor)

		.Process(this->NextBuilding_Prev)
		.Process(this->NextBuilding_Next)
		.Process(this->NextBuilding_CurrentHeapId)

		.Process(this->AutoBuilding)
		.Process(this->AutoBuilding_Gap)
		.Process(this->LimboBuild)
		.Process(this->LimboBuildID)
		.Process(this->LaserFencePost_Fence)
		.Process(this->PlaceBuilding_OnLand)
		.Process(this->PlaceBuilding_OnWater)
		.Process(this->Cameo_ShouldCount)

		.Process(this->IsAnimDelayedBurst)

		.Process(this->AllowAlliesRepair)
		.Process(this->AllowRepairFlyMZone)

		.Process(this->Overpower_KeepOnline)
		.Process(this->Overpower_ChargeWeapon)

		.Process(this->NewEvaVoice)
		.Process(this->NewEvaVoice_Index)
		.Process(this->NewEvaVoice_Priority)
		.Process(this->NewEvaVoice_RecheckOnDeath)
		.Process(this->NewEvaVoice_InitialMessage)

		.Process(this->BattlePointsCollector)
		.Process(this->BattlePointsCollector_RequirePower)
		.Process(this->BuildingRepairedSound)

		.Process(this->Refinery_UseNormalActiveAnim)
		;
}

// =============================
// container
BuildingTypeExtContainer BuildingTypeExtContainer::Instance;

bool BuildingTypeExtContainer::Load(BuildingTypeClass* key, IStream* pStm)
{
	if (!key)
		return false;

	BuildingTypeExtData* pExt = BuildingTypeExtContainer::Instance.Map.get_or_default(key);

	if (!pExt)
	{
		pExt = BuildingTypeExtContainer::Instance.Map.insert_unchecked(key, this->AllocateUnchecked(key));
	}

	this->ClearExtAttribute(key);
	this->SetExtAttribute(key, pExt);

	PhobosByteStream loader { 0 };
	if (loader.ReadBlockFromStream(pStm))
	{
		PhobosStreamReader reader { loader };
		if (reader.Expect(BuildingTypeExtData::Canary)
			&& reader.RegisterChange(pExt))
		{
			pExt->LoadFromStream(reader);
			if (reader.ExpectEndOfBlock())
			{
				// reset the buildup time
				BuildingTypeExtData::UpdateBuildupFrames(key);
				return true;
			}
		}
	}

	return false;
}

// =============================
// container hooks

ASMJIT_PATCH(0x45E50C, BuildingTypeClass_CTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, EAX);

	BuildingTypeExtData* pExt = BuildingTypeExtContainer::Instance.Map.get_or_default(pItem);

	if (!pExt)
	{
		pExt = BuildingTypeExtContainer::Instance.Map.insert_unchecked(pItem,
			   BuildingTypeExtContainer::Instance.AllocateUnchecked(pItem));
	}

	BuildingTypeExtContainer::Instance.SetExtAttribute(pItem, pExt);

	return 0;
}

ASMJIT_PATCH(0x45E707, BuildingTypeClass_DTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, ESI);

	auto extData = BuildingTypeExtContainer::Instance.GetExtAttribute(pItem);
	BuildingTypeExtContainer::Instance.ClearExtAttribute(pItem);
	BuildingTypeExtContainer::Instance.Map.erase(pItem);
	if (extData)
		DLLCallDTOR(extData);

	return 0;
}

ASMJIT_PATCH(0x465300, BuildingTypeClass_Save, 0x5)
{
	GET_STACK(BuildingTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BuildingTypeExtContainer::Instance.PrepareStream(pItem, pStm);

	return 0;
}

ASMJIT_PATCH(0x465010, BuildingTypeClass_Load, 0x5)
{
	GET_STACK(BuildingTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BuildingTypeExtContainer::Instance.PrepareStream(pItem, pStm);

	return 0;
}

ASMJIT_PATCH(0x465151, BuildingTypeClass_Load_Suffix, 0x7)
{
	BuildingTypeExtContainer::Instance.LoadStatic();
	return 0;
}

ASMJIT_PATCH(0x46531C, BuildingTypeClass_Save_Suffix, 0x6)
{
	BuildingTypeExtContainer::Instance.SaveStatic();
	return 0;
}

ASMJIT_PATCH(0x464A49, BuildingTypeClass_LoadFromINI, 0xA)
{
	GET(BuildingTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x364);

	BuildingTypeExtContainer::Instance.LoadFromINI(pItem, pINI, R->Origin() == 0x464A56);
	return 0;
}ASMJIT_PATCH_AGAIN(0x464A56, BuildingTypeClass_LoadFromINI, 0xA)

DEFINE_FUNCTION_JUMP(VTABLE, 0x7E4610, FakeBuildingTypeClass::_CanUseWaypoint)