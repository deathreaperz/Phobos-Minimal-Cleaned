#pragma once
#include <SpawnManagerClass.h>

#include <Helpers/Macro.h>
#include <Ext/Abstract/Body.h>
#include <Utilities/TemplateDef.h>

class SpawnManagerExt
{
public:
	static constexpr size_t Canary = 0x99954321;
	using base_type = SpawnManagerClass;

	class ExtData final : public Extension<SpawnManagerClass>
	{
	public:

		ExtData(SpawnManagerClass* OwnerObject) : Extension<SpawnManagerClass>(OwnerObject)
		{ }

		virtual ~ExtData() override = default;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }
		virtual bool InvalidateIgnorable(void* const ptr) const override { return true; }
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;
		virtual void InitializeConstants() override { }

	private:
		template <typename T>
		void Serialize(T& Stm);
	};


	class ExtContainer final : public Container<SpawnManagerExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);
};