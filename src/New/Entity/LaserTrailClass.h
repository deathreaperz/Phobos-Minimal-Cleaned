#pragma once

#include <GeneralStructures.h>
#include <LaserDrawClass.h>
#include <HouseClass.h>

#include <New/Type/LaserTrailTypeClass.h>

#include <vector>

class LaserTrailClass final
{
public:
	LaserTrailTypeClass* Type;
	bool Visible;
	CoordStruct FLH;
	bool IsOnTurret;
	ColorStruct CurrentColor;
	OptionalStruct<CoordStruct, true> LastLocation;
	bool CanDraw;
	bool Cloaked;
	int InitialDelay;
	CDTimerClass InitialDelayTimer;

	LaserTrailClass(LaserTrailTypeClass* pTrailType, ColorStruct nHouseColor,
		CoordStruct flh = { 0, 0, 0 }, bool isOnTurret = false) :
		Type { pTrailType }
		, Visible { true }
		, FLH { flh }
		, IsOnTurret { isOnTurret }
		, CurrentColor { (pTrailType->IsHouseColor.Get() && (nHouseColor != ColorStruct::Empty)) ? nHouseColor : pTrailType->Color }
		, LastLocation { }
		, CanDraw { false }
		, Cloaked { false }
		, InitialDelay { pTrailType->InitialDelay.Get() }
		, InitialDelayTimer { }
	{
	}

	LaserTrailClass() :
		Type { nullptr }
		, Visible { false }
		, FLH { }
		, IsOnTurret { false }
		, CurrentColor { 0, 0, 0 }
		, LastLocation { }
		, CanDraw { true }
		, Cloaked { false }
		, InitialDelay { 0 }
		, InitialDelayTimer { }
	{
	}

	virtual ~LaserTrailClass() = default;

	LaserTrailClass(const LaserTrailClass& other) = default;
	LaserTrailClass& operator=(const LaserTrailClass& other) = default;
	LaserTrailClass(LaserTrailClass&&) = default;

	bool Update(CoordStruct const& location);
	void FixZLoc(bool forWho);

	bool Load(PhobosStreamReader& stm, bool registerForChange);
	bool Save(PhobosStreamWriter& stm) const;

private:
	template <typename T>
	bool Serialize(T& stm);

	bool AllowDraw(CoordStruct const& location)
	{
		return Type && this->Visible && !this->Cloaked && (this->Type->IgnoreVertical ?
		  (abs(location.X - this->LastLocation.get().X) > 16 || abs(location.Y - this->LastLocation.get().Y) > 16) : true) && IsInitialDelayFinish();
	}

	bool IsInitialDelayFinish()
	{
		if (!Type)
			return false;

		if (!CanDraw)
		{
			if (InitialDelay > 0)
			{
				InitialDelayTimer.Start(InitialDelay);
				InitialDelay = 0;
			}

			CanDraw = InitialDelayTimer.Expired();
		}

		return CanDraw;
	}
};

template <>
struct Savegame::ObjectFactory<LaserTrailClass>
{
	std::unique_ptr<LaserTrailClass> operator() (PhobosStreamReader& Stm) const
	{
		return std::make_unique<LaserTrailClass>();
	}
};