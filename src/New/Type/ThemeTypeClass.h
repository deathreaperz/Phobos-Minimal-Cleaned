#pragma once

#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>
#include <Utilities/TemplateDef.h>

class ThemeTypeClass final : public Enumerable<ThemeTypeClass>
{
public:

	int DefaultTo;
	PhobosFixedString<64U> NextText;
	PhobosFixedString<100U> HousesText;

	Valueable<CSFText> UIName;
	Valueable<bool> Normal;
	Valueable<bool> Repeat;
	Valueable<int> Side;

	ThemeTypeClass(const char* const pTitle) : Enumerable<ThemeTypeClass> { pTitle }
		, NextText {}
		, HousesText {}
		, UIName {}
		, Normal { true }
		, Repeat { false }
		, Side { -1 }
	{ }

	virtual ~ThemeTypeClass() override = default;

	virtual void LoadFromINI(CCINIClass* pINI) override;
	virtual void LoadFromStream(PhobosStreamReader& Stm) { }
	virtual void SaveToStream(PhobosStreamWriter& Stm) { }
};