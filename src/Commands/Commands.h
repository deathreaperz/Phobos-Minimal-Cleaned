#pragma once

#include <CommandClass.h>
#include <StringTable.h>
#include <MessageListClass.h>
#include <Phobos.h>

class PhobosCommandClass : public CommandClass
{
protected:
	bool CheckDebugDeactivated() const
	{
		auto const bAllow = Phobos::Config::DevelopmentCommands || Phobos::Otamaa::IsAdmin;

		if (!bAllow)
		{
			if (const wchar_t* text = StringTable::LoadString("TXT_COMMAND_DISABLED"))
			{
				wchar_t msg[0x100] = L"\0";
				wsprintfW(msg, text, this->GetUIName());
				MessageListClass::Instance->PrintMessage(msg);
			}
			return true;
		}
		return false;
	}
};

#define CATEGORY_TEAM StringTable::LoadString(GameStrings::TXT_TEAM())
#define CATEGORY_INTERFACE StringTable::LoadString(GameStrings::TXT_INTERFACE())
#define CATEGORY_TAUNT StringTable::LoadString(GameStrings::TXT_TAUNT())
#define CATEGORY_SELECTION StringTable::LoadString(GameStrings::TXT_SELECTION())
#define CATEGORY_CONTROL StringTable::LoadString(GameStrings::TXT_CONTROL())
#define CATEGORY_DEBUG GeneralUtils::LoadStringUnlessMissing("TXT_DEBUG", L"Debug")
#define CATEGORY_GUIDEBUG StringTable::LoadString("GUI:Debug")
#define CATEGORY_DEVELOPMENT GeneralUtils::LoadStringUnlessMissing("TXT_DEVELOPMENT", L"Development")