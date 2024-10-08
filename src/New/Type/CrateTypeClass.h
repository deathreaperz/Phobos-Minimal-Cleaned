#pragma once
#include <SuperWeaponTypeClass.h>

#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>
#include <Utilities/TemplateDefB.h>
#include <Utilities/GeneralUtils.h>

class AnimTypeClass;
class VocClass;
class VoxClass;
class WeaponTypeClass;

class CrateTypeClass final : public Enumerable<CrateTypeClass>
{
public:

	Valueable<int> Weight { 0 };
	Valueable<AnimTypeClass*> Anim { nullptr };
	Valueable<double> Argument { 0.0 };
	Valueable<bool> Naval { false };
	ValueableIdx<VocClass> Sound { -1 };

	CrateTypeClass(const char* const pTitle) : Enumerable<CrateTypeClass>(pTitle)
		, Weight { }
		, Anim { nullptr }
		, Argument { 0.0 }
		, Naval { false }
		, Sound {}
	{ }

	static void ReadListFromINI(CCINIClass* pINI);
	static void AddDefaults();
	static void ReadFromINIList(CCINIClass* pINI);

	virtual ~CrateTypeClass() override = default;
	virtual void LoadFromINI(CCINIClass* pINI) override;
	virtual void LoadFromStream(PhobosStreamReader& Stm) override;
	virtual void SaveToStream(PhobosStreamWriter& Stm) override;

private:
	template <typename T>
	void Serialize(T& Stm);
};