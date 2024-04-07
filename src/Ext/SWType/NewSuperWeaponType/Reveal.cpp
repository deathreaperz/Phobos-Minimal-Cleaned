#include "Reveal.h"

#include <Misc/MapRevealer.h>
#include <Utilities/Helpers.h>
#include <Misc/Ares/Hooks/AresNetEvent.h>

std::vector<const char*> SW_Reveal::GetTypeString() const
{
	return { "Reveal" };
}

bool SW_Reveal::HandleThisType(SuperWeaponType type) const
{
	return (type == SuperWeaponType::PsychicReveal);
}

bool SW_Reveal::Activate(SuperClass* const pThis, const CellStruct& Coords, bool const IsPlayer)
{
	const auto pSW = pThis->Type;
	const auto pData = SWTypeExtContainer::Instance.Find(pSW);

	if (pThis->IsCharged)
	{
		const auto range = this->GetRange(pData);
		SW_Reveal::RevealMap(Coords, range.WidthOrRange, range.Height, pThis->Owner);
	}

	return true;
}

void SW_Reveal::Deactivate(SuperClass* pThis, CellStruct cell, bool isPlayer)
{ }

void SW_Reveal::Initialize(SWTypeExtData* pData)
{
	pData->AttachedToObject->Action = Action::PsychicReveal;
	pData->SW_RadarEvent = false;

	pData->EVA_Ready = VoxClass::FindIndexById(GameStrings::EVA_PsychicRevealReady);

	pData->SW_AITargetingMode = SuperWeaponAITargetingMode::ParaDrop;
	pData->CursorType = static_cast<int>(MouseCursorType::PsychicReveal);
}

void SW_Reveal::LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI)
{
	pData->AttachedToObject->Action = (this->GetRange(pData).WidthOrRange < 0.0) ? Action::None : static_cast<Action>(AresNewActionType::SuperWeaponAllowed);
}

int SW_Reveal::GetSound(const SWTypeExtData* pData) const
{
	return pData->SW_Sound.Get(RulesClass::Instance->PsychicRevealActivateSound);
}

bool SW_Reveal::IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const
{
	if (!this->IsLaunchsiteAlive(pBuilding))
		return false;

	if (!pData->SW_Lauchsites.empty() && pData->SW_Lauchsites.Contains(pBuilding->Type))
		return true;

	return this->IsSWTypeAttachedToThis(pData, pBuilding);
}

SWRange SW_Reveal::GetRange(const SWTypeExtData* pData) const
{
	if (pData->SW_Range->empty())
	{
		// Default range values
		const auto radius = std::min(RulesClass::Instance->PsychicRevealRadius, 10);
		return { radius };
	}
	return pData->SW_Range;
}

void SW_Reveal::RevealMap(const CellStruct& Coords, float range, int height, HouseClass* Owner)
{
	MapRevealer const revealer(Coords);

	if (revealer.AffectsHouse(Owner))
	{
		auto Apply = [&](bool add)
			{
				if (range < 0.0)
				{
					// Reveal all cells without excessive function calls
					MapClass::Instance->CellIteratorReset();
					while (auto const pCell = MapClass::Instance->CellIteratorNext())
					{
						const auto& cell = pCell->MapCoords;
						if (revealer.IsCellAvailable(cell) && revealer.IsCellAllowed(cell))
						{
							revealer.Process1(pCell, false, add);
						}
					}
				}
				else
				{
					// Default way to reveal, but one cell at a time
					const auto& base = revealer.Base();

					Helpers::Alex::for_each_in_rect_or_range<CellClass>(base, range, height,
						[&](CellClass* pCell) -> bool
					{
							const auto& cell = pCell->MapCoords;
							if (revealer.IsCellAvailable(cell) && revealer.IsCellAllowed(cell) &&
								(height > 0 || cell.DistanceFrom(base) < range))
							{
								revealer.Process1(pCell, false, add);
							}
							return true;
					});
				}
			};
		Apply(false);
		Apply(true);

		MapClass::Instance->MarkNeedsRedraw(1);
	}
}