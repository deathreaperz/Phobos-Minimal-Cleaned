#pragma once

#include <Utilities/Enumerable.h>
#include <Utilities/TemplateDefB.h>
#include <Utilities/Enum.h>

#include <WWMouseClass.h>
#include <GeneralDefinitions.h>

class CursorTypeClass final : public Enumerable<CursorTypeClass>
{
public:

	Valueable<MouseCursor> CursorData;

	CursorTypeClass(const char* pTitle) : Enumerable<CursorTypeClass>(pTitle)
		, CursorData { }
	{ }

	static void OPTIONALINLINE AddDefaults()
	{
		if (!Array.empty())
			return;

		Array.reserve(MouseCursorTypeToStrings.size() + NewMouseCursorTypeToStrings.size());

		for (size_t i = 0; i < MouseCursorTypeToStrings.size(); ++i)
		{
			AllocateWithDefault(MouseCursorTypeToStrings[i], MouseCursor::DefaultCursors[i]);
		}

		for (size_t a = 0; a < NewMouseCursorTypeToStrings.size(); ++a)
		{
			AllocateWithDefault(NewMouseCursorTypeToStrings[a], CursorTypeClass::NewMouseCursorTypeData[a]);
		}
	}

	void LoadFromINI(CCINIClass* pINI);
	void LoadFromStream(PhobosStreamReader& Stm);
	void SaveToStream(PhobosStreamWriter& Stm);

	static void LoadFromINIList_New(CCINIClass* pINI, bool bDebug = false);

	static COMPILETIMEEVAL std::array<const char* const, (size_t)MouseCursorType::count> MouseCursorTypeToStrings {
		{
			{ "Default" }, { "MoveN" },{ "MoveNE" }, { "MoveE" }, { "MoveSE" },
			{ "MoveS" }, { "MoveSW" }, { "MoveW" }, { "MoveNW" }, { "NoMoveN" },
			{ "NoMoveNE" }, { "NoMoveE" }, { "NoMoveSE" }, { "NoMoveS" },
			{ "NoMoveSW" }, { "NoMoveW" }, { "NoMoveNW" }, { "Select" },
			{ "Move" }, { "NoMove" }, { "Attack" }, { "AttackOutOfRange" },
			{ "CUR_16" }, { "DesolatorDeploy" }, { "CUR_18" }, { "Enter" },
			{ "NoEnter" }, { "Deploy" }, { "NoDeploy" }, { "CUR_1D" },
			{ "Sell" }, { "SellUnit" }, { "NoSell" }, { "Repair" },
			{ "EngineerRepair" }, { "NoRepair" }, { "CUR_24" }, { "Disguise" },
			{ "IvanBomb" }, { "MindControl" }, { "RemoveSquid" }, { "Crush" },
			{ "SpyTech" }, { "SpyPower" }, { "CUR_2C" }, { "GIDeploy" },
			{ "CUR_2E" }, { "Paradrop" }, { "CUR_30" }, { "CUR_31" },
			{ "LightningStorm" }, { "Detonate" }, { "Demolish" }, { "Nuke" },
			{ "CUR_36" }, { "Power" }, { "CUR_38" }, { "IronCurtain" }, { "Chronosphere" },
			{ "Disarm" }, { "CUR_3C" }, { "Scroll" }, { "ScrollESW" }, { "ScrollSW" },
			{ "ScrollNSW" }, { "ScrollNW" }, { "ScrollNEW" }, { "ScrollNE" },
			{ "ScrollNES" }, { "ScrollES" }, { "CUR_46" }, { "AttackMove" },
			{ "CUR_48" }, { "InfantryAbsorb" }, { "NoMindControl" }, { "CUR_4B" },
			{ "CUR_4C" }, { "CUR_4D" }, { "Beacon" }, { "ForceShield" }, { "NoForceShield" },
			{ "GeneticMutator" }, { "AirStrike" }, { "PsychicDominator" }, { "PsychicReveal" },
			{ "SpyPlane" }
		}
	};

	static COMPILETIMEEVAL std::array<const char*, (size_t)NewMouseCursorType::count> NewMouseCursorTypeToStrings {
		{
			//86       //87					//88
	{ "Tote" }, { "EngineerDamage" }, { "TogglePower" },
	//89					//90				//91
{ "NoTogglePower" }, { "InfantryHeal" }, { "UnitRepair" },
//92					//93				//94
{ "TakeVehicle" }, { "Sabotage" }, { "RepairTrench" },
}
	};

	static COMPILETIMEEVAL std::array<const MouseCursor, (size_t)NewMouseCursorType::count> NewMouseCursorTypeData {
		{
			{ 239,10,4,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 299,10,4,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 399,6,0,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 384,1,0,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 355,1,0,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 150,20,4,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 89,10,4,100,10,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 89,10,4,100,10,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
			{ 150,20,4,-1,-1,MouseHotSpotX::Center , MouseHotSpotY::Middle } ,
		}
	};

	static OPTIONALINLINE COMPILETIMEEVAL void AllocateWithDefault(const char* Title, const MouseCursor& cursor)
	{
		Array.emplace_back(std::move(std::make_unique<CursorTypeClass>(Title)));
		Array.back()->CursorData = cursor;
	}

private:
	template <typename T>
	void Serialize(T& Stm);
};

//template <>
//void NOINLINE ValueableIdx<CursorTypeClass*>::Read(INI_EX& parser, const char* pSection, const char* pKey)
//{
//	if (parser.ReadString(pSection, pKey))
//	{
//		const char* val = parser.value();
//		const bool IsBlank = GameStrings::IsBlank(val);
//		const int idx = CursorTypeClass::FindIndexById(val);
//
//		//invalid idx , is not blank , and the first value is number
//		if (idx == -1 && !IsBlank && std::isdigit(val[0]))
//		{
//			const auto nEwIdx = CursorTypeClass::FindIndexById(pSection);
//
//			if(nEwIdx == -1)
//			{
//				std::string copyed = parser.value();
//				MouseCursor value {};
//
//				char* context = nullptr;
//				if (char* const pFrame = strtok_s(parser.value(), Phobos::readDelims, &context))
//				{
//					Parser<int>::Parse(pFrame, &value.StartFrame);
//				}
//				if (char* const pCount = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					Parser<int>::Parse(pCount, &value.FrameCount);
//				}
//				if (char* const pInterval = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					Parser<int>::Parse(pInterval, &value.FrameRate);
//				}
//				if (char* const pFrame = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					Parser<int>::Parse(pFrame, &value.SmallFrame);
//				}
//				if (char* const pCount = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					Parser<int>::Parse(pCount, &value.SmallFrameCount);
//				}
//				if (char* const pHotX = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					MouseCursorHotSpotX::Parse(pHotX, &value.X);
//				}
//				if (char* const pHotY = strtok_s(nullptr, Phobos::readDelims, &context))
//				{
//					MouseCursorHotSpotY::Parse(pHotY, &value.Y);
//				}
//
//				CursorTypeClass::AllocateWithDefault(pSection , value);
//				this->Value = CursorTypeClass::Array.size() - 1;
//				Debug::LogInfo("[Phobos]Parsing CusorTypeClass from raw Cursor value of [%s]%s=%s", pSection, pKey, copyed.c_str());
//				return;
//			}
//			else
//			{
//				this->Value = nEwIdx;
//				return;
//			}
//		}
//		else if (idx != -1)
//		{
//			this->Value = idx;
//			return;
//		}
//
//		Debug::INIParseFailed(pSection, pKey, val ,"Expect Valid CursorType");
//	}
//}