#pragma once
#include <InfantryTypeClass.h>

#include <Ext/TechnoType/Body.h>

struct Phobos_DoControls
{
	static void ReadSequence(DoInfoStruct* Desig, InfantryTypeClass* pInf, CCINIClass* pINI);
};

class InfantryTypeExtData final
{
public:
	static COMPILETIMEEVAL size_t Canary = 0xAAAAACCA;
	using base_type = InfantryTypeClass;
	static COMPILETIMEEVAL size_t ExtOffset = 0xECC;

	base_type* AttachedToObject {};
	InitState Initialized { InitState::Blank };
public:
	TechnoTypeExtData* Type { nullptr };
	Valueable<bool> Is_Deso { false };
	Valueable<bool> Is_Cow { false };
	Nullable<double> C4Delay {};
	Nullable<int> C4ROF {};
	Nullable<int> C4Damage {};
	Nullable<WarheadTypeClass*> C4Warhead {};

	Valueable<bool> HideWhenDeployAnimPresent { false };
	Valueable<bool> DeathBodies_UseDieSequenceAsIndex { false };
	WeaponStruct CrawlingWeaponDatas[4] {};
	//std::vector<DoInfoStruct> Sequences {};

	ValueableIdxVector<VocClass> VoiceGarrison {};

	Valueable<bool> OnlyUseLandSequences { false };
	std::vector<int> SquenceRates {};

	// When infiltrates: detonates a weapon or damage
	Promotable<WarheadTypeClass*> WhenInfiltrate_Warhead {};
	Promotable<WeaponTypeClass*> WhenInfiltrate_Weapon {};
	Promotable<int> WhenInfiltrate_Damage {};
	Valueable<bool> WhenInfiltrate_Warhead_Full { true };

	Valueable<bool> AllSequnceEqualRates { false };
	Valueable<bool> AllowReceiveSpeedBoost { false };

	Nullable<double> ProneSpeed {};

	void LoadFromINIFile(CCINIClass* pINI, bool parseFailAddr);
	void LoadFromStream(PhobosStreamReader& Stm) { this->Serialize(Stm); }
	void SaveToStream(PhobosStreamWriter& Stm) { this->Serialize(Stm); }
	void Initialize();

	COMPILETIMEEVAL FORCEDINLINE static size_t size_Of()
	{
		return sizeof(InfantryTypeExtData) -
			(4u //AttachedToObject
			 );
	}
private:
	template <typename T>
	void Serialize(T& Stm);
};

class InfantryTypeExtContainer final : public Container<InfantryTypeExtData>
{
public:
	static InfantryTypeExtContainer Instance;

	//CONSTEXPR_NOCOPY_CLASSB(InfantryTypeExtContainer, InfantryTypeExtData, "InfantryTypeClass");
};

class NOVTABLE FakeInfantryTypeClass : public InfantryTypeClass
{
public:

	InfantryTypeExtData* _GetExtData()
	{
		return *reinterpret_cast<InfantryTypeExtData**>(((DWORD)this) + InfantryTypeExtData::ExtOffset);
	}
};
static_assert(sizeof(FakeInfantryTypeClass) == sizeof(InfantryTypeClass), "Invalid Size !");