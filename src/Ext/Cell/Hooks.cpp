#include "Body.h"

#include <TiberiumClass.h>
#include <OverlayTypeClass.h>
#include <OverlayClass.h>
#include <FileSystem.h>
#include <IsometricTileTypeClass.h>

#include <Ext/Tiberium/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/IsometricTileType/Body.h>

ASMJIT_PATCH(0x47FDF9, CellClass_GetOverlayShadowRect, 0xA)
{
	GET(CellClass*, pThis, EDI);
	GET(OverlayTypeClass*, pOverlay, ESI);

	if (pOverlay->Tiberium)
		pOverlay = OverlayTypeClass::Array->Items[CellExtData::GetOverlayIndex(pThis)];

	R->EBX(pOverlay->GetImage());

	return 0x47FE05;
}

ASMJIT_PATCH(0x47F641, CellClass_DrawShadow_Tiberium, 0x6)
{
	enum { SkipDrawing = 0x47F637, ContinueDrawing = 0x0 };
	GET(CellClass*, pThis, ESI);

	return (TiberiumClass::FindIndex(pThis->OverlayTypeIndex) >= 0) ? SkipDrawing : ContinueDrawing;
}

ASMJIT_PATCH(0x47F852, CellClass_DrawOverlay_Tiberium_, 0x6) // B
{
	GET(FakeCellClass*, pThis, ESI);
	GET(OverlayTypeClass*, pOverlay, EBX);

	if (!pOverlay->Tiberium)
	{
		return 0x47F96A;
	}

	const auto pTiberium = CellExtData::GetTiberium(pThis);

	if (!pTiberium)
	{
		return 0x47FB86;
	}

	const auto pTibExt = TiberiumExtContainer::Instance.Find(pTiberium);

	if (!pTibExt->EnableLighningFix.Get())
	{
		R->EBX(pTiberium);
		return 0x47F882;
	}

	GET_STACK(Point2D, nPos, 0x14);
	GET(RectangleStruct*, pBound, EBP);

	auto nIndex = CellExtData::GetOverlayIndex(pThis, pTiberium);
	const auto pShape = OverlayTypeClass::Array->Items[nIndex]->GetImage();

	if (!pShape)
	{
		return 0x47FB86;
	}

	const auto nZAdjust = -2 - 15 * (pThis->Level + 4 * (((int)pThis->Flags >> 7) & 1));
	auto nTint = pTibExt->Ore_TintLevel.Get(pTibExt->UseNormalLight.Get() ? 1000 : pThis->Intensity_Terrain);
	const int nOreTint = std::clamp(nTint, 0, 1000);
	auto nShadowFrame = (nIndex + pShape->Frames / 2);
	ConvertClass* pDecided = FileSystem::x_PAL();
	if (const auto pCustom = pTibExt->Palette.GetConvert())
	{
		pDecided = pCustom;
	}

	SHPStruct* pZShape = nullptr;
	if (auto nSlope = (int)pThis->SlopeIndex)
		pZShape = IsometricTileTypeClass::SlopeZshape[nSlope];	//this is just pointers to files in ram, no vector #tomsons26

	DSurface::Temp->DrawSHP(pDecided, pShape, pThis->OverlayData, &nPos, pBound, BlitterFlags(0x4E00), 0, nZAdjust, ZGradient::Ground, nOreTint, 0, pZShape, 0, 0, 0);
	DSurface::Temp->DrawSHP(pDecided, pShape, nShadowFrame, &nPos, pBound, BlitterFlags(0x4E01), 0, nZAdjust, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);

	return 0x47FB86;
}

ASMJIT_PATCH(0x47F661, CellClass_DrawOverlay_Rubble_Shadow, 0x8)
{
	GET(CellClass*, pCell, ESI);
	GET_STACK(SHPStruct*, pImage, STACK_OFFSET(0x28, 0x8));
	GET_STACK(int, nFrame, STACK_OFFSET(0x28, 0x4));
	LEA_STACK(Point2D*, pPoint, STACK_OFFS(0x28, 0x10));
	GET_STACK(int, nOffset, STACK_OFFS(0x28, 0x18));
	GET(RectangleStruct*, pRect, EBX);

	if (R->AL())
	{
		auto const pBTypeExt = BuildingTypeExtContainer::Instance.Find(pCell->Rubble);

		ConvertClass* pDecided = pCell->LightConvert;
		if (const auto pCustom = pBTypeExt->RubblePalette.GetConvert())
		{
			pDecided = pCustom;
		}

		auto const zAdjust = -2 - nOffset;

		DSurface::Temp()->DrawSHP(pDecided, pImage, nFrame, pPoint, pRect, BlitterFlags(0x4601),
		0, zAdjust, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}
	return 0x47F637;
}

ASMJIT_PATCH(0x47FADB, CellClass_DrawOverlay_Rubble, 0x5)
{
	GET(OverlayTypeClass*, pOvl, ECX);
	GET(CellClass*, pCell, ESI);
	LEA_STACK(SHPStruct**, pImage, STACK_OFFS(0x24, 0x14));
	LEA_STACK(int*, pFrame, STACK_OFFSET(0x24, 0x8));
	LEA_STACK(Point2D*, pPoint, STACK_OFFS(0x24, 0x10));
	GET_STACK(int, nOffset, STACK_OFFSET(0x24, 0x4));
	GET(RectangleStruct*, pRect, EBP);
	GET(int, nVal, EDI);

	if (auto const pRubble = pCell->Rubble)
	{
		if (pRubble->CanLeaveRubble(pImage, pFrame))
		{
			auto const pBTypeExt = BuildingTypeExtContainer::Instance.Find(pRubble);
			ConvertClass* pDecided = pCell->LightConvert;
			if (const auto pCustom = pBTypeExt->RubblePalette.GetConvert())
			{
				pDecided = pCustom;
			}

			const auto zAdjust = nVal - nOffset - 2;

			DSurface::Temp()->DrawSHP(pDecided, *pImage, *pFrame, pPoint, pRect, BlitterFlags(0x4E00),
			0, zAdjust, pOvl->DrawFlat != 0 ? ZGradient::Ground : ZGradient::Deg90, pCell->Intensity_Terrain, 0, nullptr, 0, 0, 0);
		}
	}

	return 0x47FB86;
}

#include <TerrainTypeClass.h>
#include <TerrainClass.h>

bool FakeCellClass::_CanTiberiumGerminate(TiberiumClass* tiberium)
{
	if (!MapClass::Instance->IsWithinUsableArea(this->MapCoords, true)) return false;

	if (this->ContainsBridgeEx()) return false;

	/*
	**  Don't allow Tiberium to grow on a cell with a building unless that building is
	**  invisible. In such a case, the Tiberium must grow or else the location of the
	**  building will be revealed.
	*/
	BuildingClass const* building = this->GetBuilding();
	if (building && building->Health > 0 && !building->Type->Invisible && !building->Type->InvisibleInGame) return false;

	TerrainClass* terrain = this->GetTerrain(false);
	if (terrain && terrain->Type->SpawnsTiberium) return false;

	if (!GroundType::Get(this->LandType)->Build) return false;

	if (this->OverlayTypeIndex != -1 || this->SlopeIndex > 0) return false;

	if (this->IsoTileTypeIndex >= 0 && this->IsoTileTypeIndex < IsometricTileTypeClass::Array->Count)
	{
		IsometricTileTypeClass* ittype = IsometricTileTypeClass::Array->Items[this->IsoTileTypeIndex];
		if (!ittype->AllowTiberium) return false;

		const auto ittype_ext = IsometricTileTypeExtContainer::Instance.Find(ittype);

		if (!ittype_ext->AllowedTiberiums.empty() && !ittype_ext->AllowedTiberiums.Contains(tiberium)) return false;
	}

	return true;
}

bool FakeCellClass::_CanPlaceVeins()
{
	if (this->SlopeIndex <= 4)
	{
		if (this->LandType != LandType::Water
			&& this->LandType != LandType::Rock
			&& this->LandType != LandType::Ice
			&& this->LandType != LandType::Beach)
		{
			if (this->OverlayTypeIndex == -1 || OverlayTypeClass::Array->Items[this->OverlayTypeIndex]->IsVeins)
			{
				int ittype = this->IsoTileTypeIndex;
				if (ittype < 0 || ittype >= IsometricTileTypeClass::Array->Count)
				{
					ittype = 0;
				}

				const auto isotype_ext = IsometricTileTypeExtContainer::Instance.Find(IsometricTileTypeClass::Array->Items[ittype]);

				if (isotype_ext->AllowVeins)
				{
					for (int dir = 0; dir < 8; dir += 2)
					{
						CellStruct adjacent;
						MapClass::GetAdjacentCell(&adjacent, &this->MapCoords, static_cast<FacingType>(dir));
						auto adjacent_cell = MapClass::Instance->TryGetCellAt(adjacent);

						if (adjacent_cell->SlopeIndex > 4 && this->SlopeIndex == 0)
						{
							if (adjacent_cell->OverlayTypeIndex == -1 || !OverlayTypeClass::Array->Items[adjacent_cell->OverlayTypeIndex]->IsVeins)
							{
								return false;
							}
						}

						if (adjacent_cell->LandType != LandType::Water
							&& adjacent_cell->LandType != LandType::Rock
							&& adjacent_cell->LandType != LandType::Ice
							&& adjacent_cell->LandType != LandType::Beach)
						{
							return false;
						}

						if (adjacent_cell->OverlayTypeIndex != -1 || !OverlayTypeClass::Array->Items[adjacent_cell->OverlayTypeIndex]->IsVeins)
						{
							return false;
						}
					}
					return true;
				}
			}
		}
	}
	return false;
}

//DEFINE_FUNCTION_JUMP(LJMP, 0x4838E0, FakeCellClass::_CanTiberiumGerminate);

//seems causing large FPS drop
//ASMJIT_PATCH(0x6D7A46, TacticalClass_DrawPixelFX_Tiberium, 0x7)
//{
//	GET(CellClass*, pCell, ESI);
//
//	bool bDraw = false;
//
//	if (const auto pTiberium = CellExtData::GetTiberium(pCell)) {
//		if (TiberiumExtContainer::Instance.Find(pTiberium)->EnablePixelFXAnim)
//			bDraw = pTiberium->Value;
//	}
//
//	R->EAX(bDraw);
//	return 0x6D7A4D;
//}

/*
*    v3 = this->TileType;
	if ( v3 >= 0 && v3 < (int)*(&IsometricTileTypes + 4) && !(*(&IsometricTileTypes + 1))[v3]->AllowBurrowing
*/
//ASMJIT_PATCH(0x487022, CellClass_CanEnterCell_Add, 0x6)
//{
//	enum
//	{
//		allowed = 0x487093,
//		notallowed = 0x4870A1,
//		continue_check = 0x48702C,
//	};
//
//	GET(IsometricTileTypeClass*, pTile, EDX);
//
//	if (!pTile->AllowBurrowing)
//		return notallowed;
//	else
//		return continue_check;
//}

//ASMJIT_PATCH(0x4D9C41, FootClass_CanEnterCell_Restricted, 0x6)
//{
//	GET(FootClass*, pFoot, ESI);
//
//	if (auto pCell = pFoot->GetCell())
//	{
//		if (auto pILoco = pFoot->Locomotor.get())
//		{
//			auto const pLocoClass = static_cast<LocomotionClass*>(pILoco);
//			CLSID nID { };
//			if (SUCCEEDED(pLocoClass->GetClassID(&nID)))
//			{
//				if (nID == LocomotionClass::CLSIDs::Jumpjet)
//				{
//					const auto nTile = pCell->IsoTileTypeIndex;
//					if (nTile >= 0 && nTile < IsometricTileTypeClass::Array->Count
//					 )
//					{
//						if (auto const pIsoTileExt = IsometricTileTypeExt::ExtMap.Find(IsometricTileTypeClass::Array->Items[(nTile))))
//						{
//							if (pIsoTileExt->BlockJumpjet.Get())
//							{
//								R->EAX(Move::No);
//								return 0x4D9C4E;
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//
//	return 0x0;
//}

// Aircraft pathfinding is shit
//ASMJIT_PATCH(0x4196B0, AircraftClass_CanEnterCell_Restricted, 0x5)
//{
//	GET(AircraftClass*, pThis, ECX);
//
//	if (auto pCell = pThis->GetCell()) {
//		const auto nTile = pCell->IsoTileTypeIndex;
//		if (nTile >= 0 && nTile < IsometricTileTypeClass::Array->Count && nTile == 1499) {
//			R->EAX(Move::No);
//			return 0x4197A7;
//		}
//	}
//
//	return 0x0;
//}

//UniqueGamePtrB<LightConvertClass> SpawnTiberiumTreeConvert {};
//
//ASMJIT_PATCH(0x52C046, InitGame_CreateTiberiumDrawer, 0x5)
//{
//	LEA_STACK(BytePalette*, pUnitSnoPal, 0x2F40 - 0x2BC8);
//
//	SpawnTiberiumTreeConvert.reset(GameCreate<LightConvertClass>(
//		pUnitSnoPal, &FileSystem::TEMPERAT_PAL,
//		DSurface::Primary,1000, 1000, 1000,false , nullptr , 53));
//
//	return 0;
//}
//
//ASMJIT_PATCH(0x53AD00, ScenarioClass_RecalcLighting_TintTiberiumDrawer, 5)
//{
//	GET(int, red, ECX);
//	GET(int, green, EDX);
//	GET_STACK(int, blue, STACK_OFFSET(0x0, 0x4));
//	GET_STACK(bool, tint, STACK_OFFSET(0x0, 0x8));
//	SpawnTiberiumTreeConvert->UpdateColors(red, green, blue, tint);
//	return 0;
//}
//
//ASMJIT_PATCH(0x71C294, TerrainClass_DrawIt_TiberiumSpawn_Palette, 0x6)
//{
//	R->EDX(SpawnTiberiumTreeConvert.get());
//	return 0x71C29A;
//}