#pragma once
#include <TerrainClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Debug.h>

#include <Ext/TerrainType/Body.h>

#include <LightSourceClass.h>
#include <CellClass.h>

class TerrainExtData final
{
public:
	static constexpr size_t Canary = 0xE1E2E3E4;
	using base_type = TerrainClass;
	//static constexpr size_t ExtOffset = 0xD0;

	base_type* AttachedToObject {};
	InitState Initialized { InitState::Blank };
public:

	Handle<LightSourceClass*, UninitLightSource> LighSource { nullptr };
	Handle<AnimClass*, UninitAnim> AttachedAnim { nullptr };
	std::vector<CellStruct> Adjencentcells {};

	TerrainExtData()  noexcept = default;
	~TerrainExtData() noexcept
	{
		LighSource.SetDestroyCondition(!Phobos::Otamaa::ExeTerminated);
		AttachedAnim.SetDestroyCondition(!Phobos::Otamaa::ExeTerminated);
	}

	void InvalidatePointer(AbstractClass* ptr, bool bRemoved);
	static bool InvalidateIgnorable(AbstractClass* ptr)
	{
		switch (ptr->WhatAmI())
		{
		case AnimClass::AbsID:
		case LightSourceClass::AbsID:
			return false;
		}

		return true;
	}

	void LoadFromStream(PhobosStreamReader& Stm) { this->Serialize(Stm); }
	void SaveToStream(PhobosStreamWriter& Stm) { this->Serialize(Stm); }

	static bool CanMoveHere(TechnoClass* pThis, TerrainClass* pTerrain);

	void InitializeLightSource();
	void InitializeAnim();

	constexpr FORCEINLINE static size_t size_Of()
	{
		return sizeof(TerrainExtData) -
			(4u //AttachedToObject
			 );
	}
private:
	template <typename T>
	void Serialize(T& Stm);

public:

	static void Unlimbo(TerrainClass* pThis, CoordStruct* pCoord);
};

class TerrainExtContainer final : public Container<TerrainExtData>
{
public:
	static TerrainExtContainer Instance;

	CONSTEXPR_NOCOPY_CLASSB(TerrainExtContainer, TerrainExtData, "TerrainClass");
};