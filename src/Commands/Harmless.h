#pragma once

#include "Commands.h"

class HarmlessCommandClass : public PhobosCommandClass
{
public:

	static bool InHarmlessMode;

	// CommandClass
	virtual const char* GetName() const override;
	virtual const wchar_t* GetUIName() const override;
	virtual const wchar_t* GetUICategory() const override;
	virtual const wchar_t* GetUIDescription() const override;
	virtual void Execute(WWKey eInput) const override;

	static void AI();
};
