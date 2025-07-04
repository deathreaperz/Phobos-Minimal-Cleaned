#pragma once

#include <Theater.h>

#include <Utilities/Enumerable.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/PhobosFixedString.h>
#include <Utilities/CSFText.h>

class CCINIClass;
class TheaterTypeClass final : public Enumerable<TheaterTypeClass>
{
public:
	TheaterTypeClass(const char* const pTitle) :
		Enumerable<TheaterTypeClass>(pTitle),
		UIName(),
		ControlFileName("X"),
		ArtFileName("X"),
		PaletteFileName("X"),
		Extension("X"),
		MMExtension("X"),
		Letter("X"),
		IsArctic(false),
		IsAllowedInMapGenerator(false),
		IsLunar(false),
		LowRadarBrightness1(0.0f),
		HighRadarBrightness(1.0f),
		unknown_float_60(0.0f),
		unknown_float_64(0.0f),
		unknown_int_68(0),
		unknown_int_6C(0),
		PaletteUnit(),
		PaletteISO(),
		TerrainControl(),
		PaletteOverlay(),
		RootMix(),
		RootMixMD(),
		ExpansionMDMix(),
		SuffixMix(),
		DataMix(),
		TerrainTypeExtension(),
		SmudgeTypeExtension(),
		AnimTypeExtension(),
		OverlayTypeExtension(),
		IsometricTileTypeExtension(),
		BuildingTypeExtension(),
		FallbackTheaterExtension()
	{ }

	TheaterTypeClass(const char* const pTitle, const Theater* theater, bool IsArtic, bool AllowMapGen, bool islunar) : Enumerable<TheaterTypeClass>(pTitle),
		UIName(),
		ControlFileName(theater->ControlFileName),
		ArtFileName(theater->ArtFileName),
		PaletteFileName(theater->PaletteFileName),
		Extension(theater->Extension),
		MMExtension(theater->MMExtension),
		Letter(theater->Letter),
		IsArctic(IsArtic),
		IsAllowedInMapGenerator(AllowMapGen),
		IsLunar(islunar),
		LowRadarBrightness1(theater->RadarTerrainBrightness),
		HighRadarBrightness(theater->RadarTerrainBrightnessAtMaxLevel),
		unknown_float_60(theater->unknown_float_60),
		unknown_float_64(theater->unknown_float_64),
		unknown_int_68(theater->unknown_int_68),
		unknown_int_6C(theater->unknown_int_6C),
		PaletteUnit(),
		PaletteISO(),
		TerrainControl(),
		PaletteOverlay(),
		RootMix(),
		RootMixMD(),
		ExpansionMDMix(),
		SuffixMix(),
		DataMix(),
		TerrainTypeExtension(),
		SmudgeTypeExtension(),
		AnimTypeExtension(),
		OverlayTypeExtension(),
		IsometricTileTypeExtension(),
		BuildingTypeExtension(),
		FallbackTheaterExtension(Theater::Array[0].Extension)
	{
		UIName = theater->UIName;
	}

	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);
	void LoadFromINI(CCINIClass* pINI);

	static void AddDefaults();
	static void LoadAllTheatersToArray();
	static TheaterTypeClass* FindFromTheaterType(TheaterType nType)
	{
		return (nType != TheaterType::None && (size_t)nType < Array.size() ?
			 Array[(int)nType] : Array[0]).get();
	}

	static FORCEDINLINE TheaterTypeClass* FindFromTheaterType_NoCheck(TheaterType nType)
	{
		return Array[(int)nType].get();
	}

	static OPTIONALINLINE void AllocateWithDefault(const char* Title, const Theater& theater, bool IsArtic, bool AllowMapGen, bool islunar)
	{
		Array.emplace_back(std::move(std::make_unique<TheaterTypeClass>(Title, &theater, IsArtic, AllowMapGen, islunar)));
	}

	// no !
	static OPTIONALINLINE void Clear() { }

private:

	template<typename T>
	void Swizzle(T& Stm)
	{
		Stm
			.Process(UIName)
			.Process(ControlFileName)
			.Process(ArtFileName)
			.Process(PaletteFileName)
			.Process(Extension)
			.Process(MMExtension)
			.Process(Letter)
			.Process(IsArctic)
			.Process(IsAllowedInMapGenerator)
			.Process(IsLunar)
			.Process(LowRadarBrightness1)
			.Process(HighRadarBrightness)
			.Process(unknown_float_60)
			.Process(unknown_float_64)
			.Process(unknown_int_68)
			.Process(unknown_int_6C)
			.Process(PaletteUnit)
			.Process(PaletteISO)
			.Process(TerrainControl)
			.Process(PaletteOverlay)

			.Process(RootMix)
			.Process(RootMixMD)
			.Process(ExpansionMDMix)
			.Process(SuffixMix)
			.Process(DataMix)

			.Process(TerrainTypeExtension)
			.Process(SmudgeTypeExtension)
			.Process(AnimTypeExtension)
			.Process(OverlayTypeExtension)
			.Process(IsometricTileTypeExtension)
			.Process(BuildingTypeExtension)
			.Process(FallbackTheaterExtension)
			;
	}

	bool IsDefaultTheater();
	static CCINIClass* GetConfigINI();
public:
	//2, UIname
	Valueable<CSFText> UIName;
	//3, root
	PhobosFixedString<10>ControlFileName;
	//4, datamix
	PhobosFixedString<10>ArtFileName;
	//5, expansionmix
	PhobosFixedString<10>PaletteFileName;
	//6, suffix
	PhobosFixedString<4>Extension;
	//7, mmxsuffix
	PhobosFixedString<4>MMExtension;
	//8, Letter
	PhobosFixedString<2>Letter;
	Valueable<bool> IsArctic;
	Valueable<bool> IsAllowedInMapGenerator;
	Valueable<bool> IsLunar;
	Valueable<float> LowRadarBrightness1;
	Valueable<float> HighRadarBrightness;
	Valueable<float> unknown_float_60;
	Valueable<float> unknown_float_64;
	Valueable<int> unknown_int_68;
	Valueable<int> unknown_int_6C;

	PhobosFixedString<0x80> PaletteUnit;
	PhobosFixedString<0x80> PaletteISO;
	PhobosFixedString<0x80> TerrainControl;
	PhobosFixedString<0x80> PaletteOverlay;

	PhobosFixedString<16> RootMix;
	PhobosFixedString<16> RootMixMD;
	PhobosFixedString<16> ExpansionMDMix;
	PhobosFixedString<16> SuffixMix;
	PhobosFixedString<16> DataMix;

	PhobosFixedString<4> TerrainTypeExtension;
	PhobosFixedString<4> SmudgeTypeExtension;
	PhobosFixedString<4> AnimTypeExtension;
	PhobosFixedString<4> OverlayTypeExtension;
	PhobosFixedString<4> IsometricTileTypeExtension;
	PhobosFixedString<4> BuildingTypeExtension;

	PhobosFixedString<4> FallbackTheaterExtension;
};