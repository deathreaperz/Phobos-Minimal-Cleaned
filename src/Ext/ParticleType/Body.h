#pragma once
#include <ParticleTypeClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDefB.h>

#include <New/Type/LaserTrailTypeClass.h>
#include <New/Type/PaletteManager.h>

#include <Misc/DynamicPatcher/Trails/TrailsManager.h>

class ParticleTypeExtData final
{
public:
	static COMPILETIMEEVAL size_t Canary = 0xEAEEEEEE;
	using base_type = ParticleTypeClass;

	base_type* AttachedToObject {};
	InitState Initialized { InitState::Blank };
public:

	ValueableIdxVector<LaserTrailTypeClass> LaserTrail_Types { };
	TrailsReader Trails { };
	Valueable<bool> ReadjustZ { true };
	CustomPalette Palette { CustomPalette::PaletteMode::Temperate }; //
	Valueable<double> DamageRange { 0.0 };
	Valueable<bool> DeleteWhenReachWater { false };

	std::array<Point2D, (size_t)FacingType::Count> WindMult {};

	Valueable<Point2D> Gas_DriftSpeedX { {2, -2} };
	Valueable<Point2D> Gas_DriftSpeedY { {2, -2} };

	Valueable<bool> Transmogrify { false };
	Valueable<int> TransmogrifyChance { -1 };
	Valueable<UnitTypeClass*> TransmogrifyType { nullptr };
	Valueable<OwnerHouseKind> TransmogrifyOwner { OwnerHouseKind::Neutral };

	Valueable<bool> Fire_DamagingAnim { false };

	void LoadFromINIFile(CCINIClass* pINI, bool parseFailAddr);
	void LoadFromStream(PhobosStreamReader& Stm) { this->Serialize(Stm); }
	void SaveToStream(PhobosStreamWriter& Stm) { this->Serialize(Stm); }
	void Initialize();

	COMPILETIMEEVAL FORCEDINLINE static size_t size_Of()
	{
		return sizeof(ParticleTypeExtData) -
			(4u //AttachedToObject
			 );
	}
private:
	template <typename T>
	void Serialize(T& Stm);
};

class ParticleTypeExtContainer final : public Container<ParticleTypeExtData>
{
public:
	static ParticleTypeExtContainer Instance;

	//CONSTEXPR_NOCOPY_CLASSB(ParticleTypeExtContainer, ParticleTypeExtData, "ParticleTypeClass");
};

class NOVTABLE FakeParticleTypeClass : public ParticleTypeClass
{
public:

	HRESULT __stdcall _Load(IStream* pStm);
	HRESULT __stdcall _Save(IStream* pStm, bool clearDirty);

	ParticleTypeExtData* _GetExtData()
	{
		return *reinterpret_cast<ParticleTypeExtData**>(((DWORD)this) + AbstractExtOffset);
	}
};
static_assert(sizeof(FakeParticleTypeClass) == sizeof(ParticleTypeClass), "Invalid Size !");