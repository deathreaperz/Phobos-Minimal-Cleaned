#pragma once
#include <LineTrail.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

struct LineTrailData
{
public:

	CoordStruct LineTrailFLH;
	ColorStruct LineTrailColor;
	int LineTrailColorDecrement;

	bool operator==(const LineTrailData& that) const { return LineTrailFLH == that.LineTrailFLH || LineTrailColor == that.LineTrailColor || LineTrailColorDecrement == that.LineTrailColorDecrement; }
	bool operator!=(const LineTrailData& that) const { return !(*this == that); }
	operator bool() const { return LineTrailColor != ColorStruct{ 0,0,0 };}

	static void LoadFromINI(std::vector<LineTrailData>& nVec, INI_EX &parser, const char* pSection)
	{
		if (!pSection)
			return;

		char tempBuffer[32];

		for (size_t i = 0; ; ++i)
		{
			Nullable<ColorStruct> tempColor;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "LineTrail%dColor", i);
			tempColor.Read(parser, pSection, tempBuffer);

			if (!tempColor.isset() || tempColor.Get() == ColorStruct{ 0,0,0 })
				break;

			Valueable<CoordStruct> tempflh;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "LineTrail%dFLH", i);
			tempflh.Read(parser, pSection, tempBuffer);

			Valueable<int> tempDecrement;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "LineTrail%dColorDecrement", i);
			tempDecrement.Read(parser, pSection, tempBuffer);

			nVec.push_back({ tempflh, tempColor,tempDecrement });
		}
	}

};

class LineTrailExt
{
public:
	~LineTrailExt() = default;
	LineTrailExt() noexcept {}
	LineTrailExt(LineTrail* pTrail, CoordStruct nCoord) noexcept
	{ Insert(pTrail, nCoord); }

	static const CoordStruct GetCoord(LineTrail* pTrail)
	{
		return LineTrailMap.get_or_default(pTrail, CoordStruct::Empty);
	}

	//Override Color 0x5F51A3
	static const ColorStruct OverrideColor(ColorStruct nColorInput)
	{
		auto nColorOverrider = RulesClass::Instance->LineTrailColorOverride;

		ColorStruct nColorBuffer{ 0,0,0 };

		nColorBuffer.R = nColorOverrider.R ? nColorOverrider.R : nColorInput.R;
		nColorBuffer.G = nColorOverrider.G ? nColorOverrider.G : nColorInput.G;
		nColorBuffer.B = nColorOverrider.B ? nColorOverrider.B : nColorInput.B;

		return nColorBuffer;
	}

	//Construct
	static const bool Construct(DynamicVectorClass<LineTrail*>& nVec, ObjectClass* pOwner, ColorStruct nColor, int nDecrement, CoordStruct nFLH = CoordStruct::Empty)
	{
		bool created = false;

		if (auto pLineTrail = GameCreate<LineTrail>())
		{
			created = pLineTrail;
			LineTrailExt(pLineTrail, nFLH);
			pLineTrail->Color = OverrideColor(nColor);
			pLineTrail->SetDecrement(nDecrement);
			pLineTrail->Owner = pOwner;
			nVec.AddUnique(pLineTrail);
		}

		return created;
	}

	static bool DeallocateLineTrail(TechnoClass* pTech);
	static bool DeallocateLineTrail(BulletClass* pBullet);
	static bool DeallocateLineTrail(ObjectClass* pObject);
	static void ConstructLineTrails(TechnoClass* pTech);
	static void ConstructLineTrails(BulletClass* pBullet);

	static const bool RemoveLineTrail(LineTrail* pTrail)
	{
		if (LineTrailMap.contains(pTrail))
			return LineTrailMap.erase(pTrail);

		return false;
	}

	static void DetachLineTrails(ObjectClass* pThis);

private:

	static void Insert(LineTrail* pTrail, CoordStruct nCoord)
	{
		LineTrailMap[pTrail] = nCoord;
	}

	static PhobosMap<LineTrail*, CoordStruct> LineTrailMap;
};