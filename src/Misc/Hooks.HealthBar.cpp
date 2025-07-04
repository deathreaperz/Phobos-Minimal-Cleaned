#include <Utilities/Macro.h>

#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Utilities/Helpers.h>
#include <Utilities/Cast.h>

#include <InfantryClass.h>
#include <TacticalClass.h>

ASMJIT_PATCH(0x6D9076, TacticalClass_RenderLayers_DrawBefore, 0x5)// FootClass
{
	GET(TechnoClass*, pTechno, ESI);
	GET(Point2D*, pLocation, EAX);

	if (pTechno->IsSelected && Phobos::Config::EnableSelectBox)
	{
		const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pTechno->GetTechnoType());

		if (!pTypeExt->HealthBar_Hide && !pTypeExt->HideSelectBox)
			TechnoExtData::DrawSelectBox(pTechno, pLocation, &DSurface::ViewBounds, true);
	}

	return 0;
}ASMJIT_PATCH_AGAIN(0x6D9134, TacticalClass_RenderLayers_DrawBefore, 0x5)// BuildingClass

ASMJIT_PATCH(0x709ACF, TechnoClass_DrawPip_PipShape1_A, 0x6)
{
	GET(TechnoClass* const, pThis, EBP);
	GET(SHPStruct*, pPipShape01, ECX);

	R->ECX(TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType())
		->PipShapes01.Get(pPipShape01));

	return 0;
}

ASMJIT_PATCH(0x709AE3, TechnoClass_DrawPip_PipShape1_B, 0x6)
{
	GET(TechnoClass* const, pThis, EBP);
	GET(SHPStruct*, pPipShape01, EAX);

	R->EAX(TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType())
		->PipShapes01.Get(pPipShape01));

	return 0;
}

ASMJIT_PATCH(0x709AF8, TechnoClass_DrawPip_PipShape2, 0x6)
{
	GET(TechnoClass* const, pThis, EBP);
	GET(SHPStruct*, pPipShape02, EBX);

	R->EBX(TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType())
		->PipShapes02.Get(pPipShape02));

	return 0;
}

bool HideBar(TechnoClass* pTechno, TechnoTypeClass* pType, bool isAllied)
{
	const auto what = pTechno->WhatAmI();

	if (auto pFoot = flag_cast_to<FootClass*, false>(pTechno))
	{
		auto pExt = TechnoExtContainer::Instance.Find(pTechno);

		if (pExt->Is_DriverKilled)
			return true;
	}

	if (what == UnitClass::AbsID)
	{
		const auto pUnit = (UnitClass*)pTechno;

		if (pUnit->DeathFrameCounter > 0)
			return true;
	}

	if (what == BuildingClass::AbsID)
	{
		const auto pBld = (BuildingClass*)pTechno;

		if (BuildingTypeExtContainer::Instance.Find(pBld->Type)->Firestorm_Wall)
			return true;
	}

	if ((TechnoTypeExtContainer::Instance.Find(pType)->HealthBar_Hide.Get())
		|| pTechno->TemporalTargetingMe
		|| pTechno->IsSinking
	)
		return true;

	if (!RulesClass::Instance->EnemyHealth && !HouseClass::IsCurrentPlayerObserver() && !isAllied)
		return true;

	return false;
}

#ifndef _OLD

ASMJIT_PATCH(0x6F66B3, TechnoClass_DrawHealth_Building_PipFile_A, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);
	GET(SHPReference*, pDefaultPip, EAX);

	const auto pBuildingTypeExt = BuildingTypeExtContainer::Instance.Find(pThis->Type);
	ConvertClass* nPal = FileSystem::THEATER_PAL();

	if (pBuildingTypeExt->PipShapes01Remap)
	{
		nPal = pThis->GetRemapColour();
	}
	else if (const auto pConvertData = pBuildingTypeExt->PipShapes01Palette.GetConvert())
	{
		nPal = pConvertData;
	}

	//PipShapes01Palette
	R->EDX(nPal);//
	R->EAX(pBuildingTypeExt->Type->PipShapes01.Get(pDefaultPip));

	return 0x6F66B9;
}

namespace DrawHeathData
{
	void DrawNumber(const TechnoClass* const pThis, Point2D* pLocation, RectangleStruct* pBounds)
	{
		auto const pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());

		if (!pTypeExt->HealthNumber_Show.Get() || !pThis->IsAlive)
			return;

		auto const pShpGreen = pTypeExt->HealthNumber_SHP.Get(nullptr);

		if (!pShpGreen)
			return;

		Point2D nCurrentDistance { 0,0 };
		auto const nLocation = *pLocation;
		auto const pType = pThis->GetTechnoType();
		int XOffset = 0;
		int	YOffset = 0;

		auto GetFrame = [pThis, pShpGreen](char nInput)
			{
				int nFrameResult = -1;
				int nInputToFrame = -1;

				switch (nInput)
				{
				case (*" "):
					nInputToFrame = 12;//blank frame
					break;
				case (*"%"):
					nInputToFrame = 10;
					break;
				case (*"/"):
					nInputToFrame = 11;
					break;
				default:
					nInputToFrame = nInput - 48;
					break;
				}

				int const nFrame_Total = std::clamp((int)pShpGreen->Frames, 12 + 1, 36 + 1);
				nFrameResult = nInputToFrame;

				// blank frames on the end (+1) !
				// 0  1  2  3  4  5  6  7  8  9 10 11

				// 12 13 14 15 16 17 18 19 20 21 22	23
				if (nFrame_Total == 25 && (pThis->IsYellowHP() || pThis->IsRedHP()))
				{
					nFrameResult = nInputToFrame + 12;
				}
				// 24 25 26 27 28 29 30 31 32 33 34 35
				else if (nFrame_Total == 37)
				{
					if (!pThis->IsGreenHP())
					{
						if (!(nInputToFrame == 12) && pThis->IsYellowHP())
							nFrameResult = nInputToFrame + 12;

						nFrameResult = nInputToFrame + 24;
					}
				}

				return nFrameResult;
			};

		//char nBuffer[0x100];
		std::string _buffer = !pTypeExt->HealthNumber_Percent.Get() ?
			fmt::format("{}/{}", pThis->Health, pThis->GetTechnoType()->Strength)
			:
			fmt::format("{}%", (int)(pThis->GetHealthPercentage() * 100.0));

		auto const bIsBuilding = pThis->WhatAmI() == BuildingClass::AbsID;

		{
			// coord calculation is not really right !
			if (bIsBuilding)
			{
				auto const pBuilding = static_cast<const BuildingClass*>(pThis);
				auto const pBldType = pBuilding->Type;
				CoordStruct nDimension { 0,0,0 };
				auto const nLocTemp = nLocation;
				pBldType->Dimension2(&nDimension);
				CoordStruct nDimension2 { -nDimension.X / 2,nDimension.Y / 2,nDimension.Z / 2 };
				Point2D nDest = TacticalClass::Instance->CoordsToScreen(nDimension2);

				XOffset = nDest.X + nLocTemp.X + pTypeExt->Healnumber_Offset.Get().X + 2;
				YOffset = nDest.Y + nLocTemp.Y + pTypeExt->Healnumber_Offset.Get().Y + pType->PixelSelectionBracketDelta;
			}
			else
			{
				XOffset = nLocation.X + pTypeExt->Healnumber_Offset.Get().X + 2 - 15;
				YOffset = nLocation.Y + pTypeExt->Healnumber_Offset.Get().Y + pType->PixelSelectionBracketDelta - 33;
			}
		}

		Point2D nDistanceFactor = pTypeExt->Healnumber_Decrement.Get(bIsBuilding ? Point2D { 10,-5 } : Point2D { 5 ,0 });

		for (auto const& nCurrentData : _buffer)
		{
			if (nCurrentData == *"\0")
				break;
			Point2D nOffset { nCurrentDistance.X + XOffset,YOffset + nCurrentDistance.Y };

			{
				auto const nFrameIndex = GetFrame(nCurrentData);

				if ((nFrameIndex >= 0) && Phobos::Otamaa::ShowHealthPercentEnabled)
				{
					DSurface::Temp->DrawSHP
					(FileSystem::PALETTE_PAL,
						pShpGreen,
						nFrameIndex,
						&nOffset,
						pBounds,
						BlitterFlags(0x600),
						0,
						0,
						ZGradient::Ground,
						1000,
						0,
						0,
						0,
						0,
						0
					);
				}
			}

			nCurrentDistance.Y += nDistanceFactor.Y;
			nCurrentDistance.X += nDistanceFactor.X;
		}
	}

	void DrawBar(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBound)
	{
		auto const& [pType, pOwner] = TechnoExtData::GetDisguiseType(pThis, false, true);
		LightConvertClass* pTechConvert = pThis->GetRemapColour();
		const bool bIsInfantry = pThis->WhatAmI() == InfantryClass::AbsID;
		bool IsDisguised = false;

		if (pThis->IsDisguised() && !pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer))
		{
			IsDisguised = true;
		}

		const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
		const auto pPipsShape = pTypeExt->HealthBarSHP.Get(FileSystem::PIPS_SHP());
		const auto pPipsShapeSelected = pTypeExt->HealthBarSHP_Selected.Get(FileSystem::PIPBRD_SHP());
		ConvertClass* pPalette = FileSystem::PALETTE_PAL();
		if (pTypeExt->HealthbarRemap.Get())
			pPalette = pTechConvert;
		else if (const auto pConvertData = pTypeExt->HealthBarSHP_Palette.GetConvert())
			pPalette = pConvertData;

		Point2D nLocation = *pLocation;
		nLocation += pTypeExt->HealthBarSHP_PointOffset.Get();
		const auto nBracketDelta = pType->PixelSelectionBracketDelta + pTypeExt->HealthBarSHPBracketOffset.Get();
		Point2D nPoint { 0,0 };
		const Point2D DrawOffset { 2,0 };

		if (pThis->IsSelected)
		{
			nPoint.X = nLocation.X + (bIsInfantry ? 11 : 1);
			nPoint.Y = nLocation.Y + nBracketDelta - (bIsInfantry ? 25 : 26);

			DSurface::Temp->DrawSHP(pPalette, pPipsShapeSelected, (bIsInfantry ? 1 : 0), &nPoint, pBound, BlitterFlags(0xE00), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			//TechnoExtData::DrawSelectBrd(pThis, pType, bIsInfantry ? 8 : 17, pLocation, pBound, bIsInfantry, IsDisguised);
			//TechnoExtData::DrawSelectBox(pThis, pLocation, pBound);
		}

		const int nOffsetX = (bIsInfantry ? -5 : -15);
		const int nLength = (bIsInfantry ? 8 : 17);
		const int nYDelta = nBracketDelta - (bIsInfantry ? 24 : 25);
		const int nDraw = pThis->IsAlive ? std::clamp((int)(std::round(pThis->GetHealthPercentage() * nLength)), 1, nLength) : 0;
		Point3D const nHealthFrame = pTypeExt->HealthBarSHP_HealthFrame.Get();
		int nHealthFrameResult = 0;

		if (pThis->IsYellowHP())
			nHealthFrameResult = nHealthFrame.Z; //Yellow
		else if (pThis->IsRedHP() || pThis->Health <= 0 || !pThis->IsAlive)
			nHealthFrameResult = nHealthFrame.X;//Red
		else
			nHealthFrameResult = nHealthFrame.Y; //Green

		for (int i = 0; i < nDraw; ++i)
		{
			nPoint.Y = nYDelta + nLocation.Y + DrawOffset.Y * i;
			nPoint.X = nOffsetX + nLocation.X + DrawOffset.X * i;
			DSurface::Temp->DrawSHP(pPalette, pPipsShape, nHealthFrameResult, &nPoint, pBound, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	int DrawBar_PipAmount(TechnoClass* pThis, int iLength)
	{
		const auto passed = Unsorted::CurrentFrame - pThis->IronCurtainTimer.StartTime;
		const auto left = static_cast<double>(pThis->IronCurtainTimer.TimeLeft - passed);
		const auto nTime = (left / pThis->IronCurtainTimer.TimeLeft);

		return std::clamp((int)std::round(nTime * iLength), 0, iLength);
	}

	void DrawBar_Building(TechnoClass* pThis, int iLength, Point2D* pLocation, RectangleStruct* pBound, int frame, int empty_frame, int bracket_delta)
	{
		CoordStruct vCoords = { 0, 0, 0 };
		pThis->GetTechnoType()->Dimension2(&vCoords);
		CoordStruct vCoords2 = { -vCoords.X / 2, vCoords.Y / 2,vCoords.Z };
		Point2D vPos2 = TacticalClass::Instance->CoordsToScreen(vCoords2);

		Point2D vLoc = *pLocation;
		vLoc.X -= 5;
		vLoc.Y -= 3 + bracket_delta;

		Point2D vPos = { 0, 0 };

		const int iTotal = DrawBar_PipAmount(pThis, iLength);

		if (iTotal > 0)
		{
			int frameIdx, deltaX, deltaY;
			for (frameIdx = iTotal, deltaX = 0, deltaY = 0;
				frameIdx;
				frameIdx--, deltaX += 4, deltaY -= 2)
			{
				vPos.X = vPos2.X + vLoc.X + 4 * iLength + 3 - deltaX;
				vPos.Y = vPos2.Y + vLoc.Y - 2 * iLength + 4 - deltaY;

				DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
					frame, &vPos, pBound, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}

		if (iTotal < iLength)
		{
			int frameIdx, deltaX, deltaY;
			for (frameIdx = iLength - iTotal, deltaX = 4 * iTotal, deltaY = -2 * iTotal;
				frameIdx;
				frameIdx--, deltaX += 4, deltaY -= 2)
			{
				vPos.X = vPos2.X + vLoc.X + 4 * iLength + 3 - deltaX;
				vPos.Y = vPos2.Y + vLoc.Y - 2 * iLength + 4 - deltaY;
				DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
					empty_frame, &vPos, pBound, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}
	}

	void DrawdBar_Other(TechnoClass* pThis, int iLength, Point2D* pLocation, RectangleStruct* pBound, int aframe, int bracket_delta)
	{
		const auto pPipsShape = FileSystem::PIPS_SHP();
		const auto pPipsShapeSelected = FileSystem::PIPBRD_SHP();

		Point2D vPos = { 0,0 };
		Point2D nLoc = *pLocation;
		Point2D vLoc = *pLocation;
		int frame, XOffset, YOffset;
		YOffset = pThis->GetTechnoType()->PixelSelectionBracketDelta + bracket_delta;
		vLoc.Y -= 5;

		if (iLength == 8)
		{
			vPos.X = vLoc.X + 11;
			vPos.Y = vLoc.Y - 25 + YOffset;
			frame = pPipsShapeSelected->Frames > 2 ? 3 : 1;
			XOffset = -5;
			YOffset -= 24;
		}
		else
		{
			vPos.X = vLoc.X + 1;
			vPos.Y = vLoc.Y - 26 + YOffset;
			frame = pPipsShapeSelected->Frames > 2 ? 2 : 0;
			XOffset = -15;
			YOffset -= 25;
		}

		if (pThis->IsSelected)
		{
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pPipsShapeSelected,
				frame, &vPos, pBound, BlitterFlags(0xE00), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		const auto iTotal = DrawBar_PipAmount(pThis, iLength);

		for (int i = 0; i < iTotal; ++i)
		{
			vPos.X = nLoc.X + XOffset + 2 * i;
			vPos.Y = nLoc.Y + YOffset;

			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pPipsShape,
				aframe, &vPos, pBound, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	void DrawIronCurtaindBar(TechnoClass* pThis, int iLength, Point2D* pLocation, RectangleStruct* pBound)
	{
		if (pThis->IsIronCurtained())
		{
			if (pThis->WhatAmI() == BuildingClass::AbsID)
				DrawBar_Building(pThis, iLength, pLocation, pBound, pThis->ProtectType != ProtectTypes::ForceShield ? 2 : 3, 0, 0);
			else
				DrawdBar_Other(pThis, iLength, pLocation, pBound, 18, 0);
		}
	}

	void DrawIronTemporalEraseDelayBarAndNumber(TechnoClass* pThis, int iLength, Point2D* pLocation, RectangleStruct* pBound)
	{
		//auto icur = pThis->TemporalTargetingMe->WarpRemaining;
		//auto imax = pThis->TemporalTargetingMe->GetWarpPerStep();
		//draw it here ? 0x71A88D
	}
}

ASMJIT_PATCH(0x6F65D1, TechnoClass_DrawdBar_Building, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(int, iLength, EBX);
	GET_STACK(Point2D*, pLocation, STACK_OFFS(0x4C, -0x4));
	GET_STACK(RectangleStruct*, pBound, STACK_OFFS(0x4C, -0x8));

	const auto pExt = TechnoExtContainer::Instance.Find(pThis);
	auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());

	if (pThis->IsSelected && Phobos::Config::EnableSelectBox && !pTypeExt->HideSelectBox)
		TechnoExtData::DrawSelectBox(pThis, pLocation, pBound);

	if (const auto pShieldData = pExt->Shield.get())
	{
		if (pShieldData->IsAvailable() && !pShieldData->IsBrokenAndNonRespawning())
			pShieldData->DrawShieldBar(iLength, pLocation, pBound);
	}

	//DrawHeathData::DrawNumber(pThis, pLocation, pBound);
	//DrawHeathData::DrawIronCurtaindBar(pThis, iLength, pLocation, pBound);
	TechnoExtData::ProcessDigitalDisplays(pThis);
	return 0;
}

ASMJIT_PATCH(0x6F683C, TechnoClass_DrawBar_Foot, 0x7)
{
	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(Point2D*, pLocation, STACK_OFFS(0x4C, -0x4));
	GET_STACK(RectangleStruct*, pBound, STACK_OFFS(0x4C, -0x8));

	if (TechnoExtContainer::Instance.Find(pThis)->Is_DriverKilled)
		return 0x6F6AB6u;

	auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType());

	if (pThis->IsSelected && Phobos::Config::EnableSelectBox && !pTypeExt->HideSelectBox)
		TechnoExtData::DrawSelectBox(pThis, pLocation, pBound);

	const int iLength = pThis->WhatAmI() == InfantryClass::AbsID ? 8 : 17;

	const auto pExt = TechnoExtContainer::Instance.Find(pThis);
	if (const auto pShieldData = pExt->Shield.get())
	{
		if (pShieldData->IsAvailable() && !pShieldData->IsBrokenAndNonRespawning())
		{
			pShieldData->DrawShieldBar(iLength, pLocation, pBound);
		}
	}

	DrawHeathData::DrawBar(pThis, pLocation, pBound);
	//DrawHeathData::DrawNumber(pThis, pLocation, pBound);
	//DrawHeathData::DrawIronCurtaindBar(pThis, iLength, pLocation, pBound);
	TechnoExtData::ProcessDigitalDisplays(pThis);

	if (HouseClass::IsCurrentPlayerObserver())
		return 0x6F6A8E;

	return 0x6F6A58u;
}

//TODO :Draw all the pip
//TODO : handle Healthdrawing

#ifdef _aaa
void DrawHealthbar(TechnoClass* pTechno, Point2D* pLocation, RectangleStruct* pBounds)
{
	const auto what = pTechno->WhatAmI();
	auto pType = pTechno->GetTechnoType();

	if (what == BuildingClass::AbsID)
	{
		//TODO : draw on different position instead of fixed place
		auto pBld = static_cast<BuildingClass*>(pTechno);
		int heihgt = pBld->GetHeight();
		CoordStruct leptonDimension;
		pType->Dimension2(&leptonDimension);
		CoordStruct halfDim = leptonDimension / 2;
		CoordStruct difference = halfDim - leptonDimension;
		Point2D screen = TacticalClass::Instance->CoordsToScreen(difference);
		difference.Y = -difference.Y;
		Point2D screen2 = TacticalClass::Instance->CoordsToScreen(difference);
		difference.Z = 0;
		difference.Y = -difference.Y;
		Point2D screen3 = TacticalClass::Instance->CoordsToScreen(difference);
		difference.Y = -difference.Y;

		int length = (screen.Y - screen2.Y) / 2;
		int ratio_length = int(pTechno->GetHealthPercentage_() * length);
		int drawLength = std::clamp(ratio_length, 1, length);
		int frame = 1;
		if (pTechno->IsYellowHP())
			frame = 2;
		else if (pTechno->IsRedHP())
			frame = 4;

		//DrawMain health
		CoordStruct coord { 0 , 0 , 2 - 2 * length };
		if (drawLength)
		{
			int v19 = 0;
			int a3 = 0;
			int drawLength_Copy = drawLength;
			bool drawLength_Copy_isOne = false;

			do
			{
				Point2D pint { screen.X + pLocation->X + 4 * length + 3 - v19 , screen.Y + pLocation->Y };

				coord.X = coord.Z + pint.Y + 2 - a3;
				CC_Draw_Shape(
					DSurface::Temp(),
					FileSystem::PALETTE_PAL(),
					FileSystem::PIPS_SHP(),
					frame,
					&pint,
					pBounds,
					0x600,
					0,
					0,
					0,
					1000,
					0,
					0,
					0,
					0,
					0);
				v19 += 4;
				drawLength_Copy_isOne = drawLength_Copy == 1;
				a3 = (a3 - 2);
				--drawLength_Copy;
			}
			while (!drawLength_Copy_isOne);
		}
	}
	else
	{
	}
}

// destroying a building (no health left) resulted in a single green pip shown
// in the health bar for a split second. this makes the last pip red.
ASMJIT_PATCH(0x6F661D, TechnoClass_DrawHealthBar_DestroyedBuilding_RedPip, 0x7)
{
	GET(BuildingClass*, pBld, ESI);
	return (pBld->Health <= 0 || pBld->IsRedHP()) ? 0x6F6628 : 0x6F6630;
}

ASMJIT_PATCH(0x6F64A0, TechnoClass_DrawHealthBar_Hide, 0x5)
{
	enum
	{
		Draw = 0x0,
		DoNotDraw = 0x6F6ABD
	};

	GET(TechnoClass*, pThis, ECX);

	const auto what = pThis->WhatAmI();

	if (what == UnitClass::AbsID)
	{
		const auto pUnit = (UnitClass*)pThis;

		if (pUnit->DeathFrameCounter > 0)
			return DoNotDraw;
	}

	if (what == BuildingClass::AbsID)
	{
		const auto pBld = (BuildingClass*)pThis;

		if (BuildingTypeExtContainer::Instance.Find(pBld->Type)->Firestorm_Wall)
			return DoNotDraw;
	}

	if ((TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType())->HealthBar_Hide.Get())
		|| pThis->TemporalTargetingMe
		|| pThis->IsSinking
	)
		return DoNotDraw;

	return Draw;
}
#endif

// destroying a building (no health left) resulted in a single green pip shown
// in the health bar for a split second. this makes the last pip red.
ASMJIT_PATCH(0x6F661D, TechnoClass_DrawHealthBar_DestroyedBuilding_RedPip, 0x7)
{
	GET(BuildingClass*, pBld, ESI);
	return (pBld->Health <= 0 || pBld->IsRedHP()) ? 0x6F6628 : 0x6F6630;
}

ASMJIT_PATCH(0x6F64A0, TechnoClass_DrawHealthBar_Hide, 0x5)
{
	enum
	{
		Draw = 0x0,
		DoNotDraw = 0x6F6ABD
	};

	GET(TechnoClass*, pThis, ECX);

	const auto what = pThis->WhatAmI();

	if (what == UnitClass::AbsID)
	{
		const auto pUnit = (UnitClass*)pThis;

		if (pUnit->DeathFrameCounter > 0)
			return DoNotDraw;
	}

	if (what == BuildingClass::AbsID)
	{
		const auto pBld = (BuildingClass*)pThis;

		if (BuildingTypeExtContainer::Instance.Find(pBld->Type)->Firestorm_Wall)
			return DoNotDraw;
	}

	if ((TechnoTypeExtContainer::Instance.Find(pThis->GetTechnoType())->HealthBar_Hide.Get())
		|| pThis->TemporalTargetingMe
		|| pThis->IsSinking
	)
		return DoNotDraw;

	return Draw;
}
#else

ASMJIT_PATCH(0x6F64A0, TechnoClass_DrawHealthBar, 0x5)
{
	enum { SkipDrawCode = 0x6F6ABD };

	GET(TechnoClass*, pThis, ECX);

	auto const& [pType, pOwner] = TechnoExtData::GetDisguiseType(pThis, false, true);
	const bool isAllied = pOwner->IsAlliedWith(HouseClass::CurrentPlayer);

	if (HideBar(pThis, pType, isAllied))
		return SkipDrawCode;

	GET_STACK(Point2D*, pLocation, 0x4);
	GET_STACK(RectangleStruct*, pBounds, 0x8);
	//GET_STACK(bool, drawFullyHealthBar, 0xC);

	const auto pExt = TechnoExtContainer::Instance.Find(pThis);
	const auto pTypeExt = TechnoTypeExtContainer::Instance.Find(pType);
	const auto whatAmI = pThis->WhatAmI();
	auto pBuilding = whatAmI == BuildingClass::AbsID ? static_cast<BuildingClass*>(pThis) : nullptr;

	Point2D position = *pLocation;
	Point2D pipsAdjust = Point2D::Empty;
	int pipsLength = 0;

	HealthBarTypeClass* pHealthBar = nullptr;

	if (pBuilding)
	{
		CoordStruct dimension {};
		pBuilding->Type->Dimension2(&dimension);
		dimension.X /= -2;
		dimension.Y /= 2;

		const auto drawAdjust = TacticalClass::CoordsToScreen(dimension);
		position += drawAdjust;

		dimension.Y = -dimension.Y;
		const auto drawStart = TacticalClass::CoordsToScreen(dimension);

		dimension.Z = 0;
		dimension.Y = -dimension.Y;
		pipsAdjust = TacticalClass::CoordsToScreen(dimension);

		pHealthBar = pTypeExt->HealthBar.Get(RulesExtData::Instance()->Buildings_DefaultHealthBar);
		pipsLength = (drawAdjust.Y - drawStart.Y) >> 1;
	}
	else
	{
		pipsAdjust = Point2D { -10, 10 };

		pHealthBar = pTypeExt->HealthBar.Get(RulesExtData::Instance()->DefaultHealthBar);

		constexpr int defaultInfantryPipsLength = 8;
		constexpr int defaultUnitPipsLength = 17;
		pipsLength = pHealthBar->PipsLength.Get(whatAmI == InfantryClass::AbsID ? defaultInfantryPipsLength : defaultUnitPipsLength);
	}

	const auto pShield = pExt->Shield.get();

	if (pShield && pShield->IsAvailable() && !pShield->IsBrokenAndNonRespawning())
	{
		pShield->DrawShieldBar(pipsLength, &position, pBounds);
	}

	if (pBuilding)
		TechnoExtData::DrawHealthBar(pBuilding, pHealthBar, pipsLength, &position, pBounds);
	else
		TechnoExtData::DrawHealthBar(pThis, pType, pHealthBar, pipsLength, &position, pBounds);

	TechnoExtData::ProcessDigitalDisplays(pThis, pType, &position);

	const bool canShowPips = isAllied || pThis->DisplayProductionTo.Contains(HouseClass::CurrentPlayer) || HouseClass::IsCurrentPlayerObserver();

	if (canShowPips || (pBuilding && pBuilding->Type->CanBeOccupied) || pType->PipsDrawForAll)
	{
		Point2D pipsLocation = *pLocation + pipsAdjust;
		pThis->DrawPipScalePips(&pipsLocation, pLocation, pBounds);
	}

	return SkipDrawCode;
}

#endif