#pragma once
#include <DiskLaserClass.h>

#include <Ext/Abstract/Body.h>

class DiskLaserExt
{
public:
	static constexpr size_t Canary = 0x87659771;
	using base_type = DiskLaserClass;

	class ExtData final : public Extension<DiskLaserClass>
	{
	public:

		ExtData(DiskLaserClass* OwnerObject) : Extension<DiskLaserClass>(OwnerObject)
		{ }

		virtual ~ExtData() override = default;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override { }
		virtual bool InvalidateIgnorable(void* const ptr) const  override { return true; };
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;
		virtual void InitializeConstants() override { }

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<DiskLaserExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);
};