#include "VerticalLaserClass.h"

#include <MapClass.h>
#include <WeaponTypeClass.h>
#include <TechnoClass.h>
#include <FootClass.h>

#include <LaserDrawClass.h>
#include <BulletClass.h>

#include <Ext/BulletType/Body.h>
#include <Base/SSE_Math.h>

std::vector<VerticalLaserClass*> VerticalLaserClass::Array;

VerticalLaserClass::VerticalLaserClass() : Expired { }
, count {} , angle {}, cur_frames {}, frames_max {}
, radius {}, Weapon { }, From {}, Height {}
, radius_decrement {}
{ VerticalLaserClass::Array.push_back(this); }

VerticalLaserClass::VerticalLaserClass(WeaponTypeClass* Weapon, CoordStruct From, int Height) : Expired { false }
,count { 5 } , angle { 0 }, cur_frames { 0 }, frames_max { 300 }
, radius { 1024.0 }, Weapon { Weapon }, From { From }, Height { Height }
, radius_decrement { 11 }
{ VerticalLaserClass::Array.push_back(this); }

void VerticalLaserClass::Reset()
{
	Expired = true;
}

void DrawSecond(const CoordStruct& from, const  CoordStruct& to, const ColorStruct& innerColor, const ColorStruct& outerColor, const ColorStruct& outerSpread, int Height)
{
	CoordStruct center = from + CoordStruct {0, 0, Height };
	CoordStruct focus = center + CoordStruct { 0, 0, 150 };

	int radius = 100;

	for (int angle = -180 - 45; angle < 180 - 45; angle += 90)
	{
		CoordStruct pos = CoordStruct { center.X + (int)(radius * roundoff(Math::cos(angle * Math::PI / 180), 5)), center.Y + (int)(radius * roundoff(Math::sin(angle * Math::PI / 180), 5)), center.Z };
		GameCreate<LaserDrawClass>(pos, focus, innerColor, outerColor, outerSpread, 15);
	}

	auto pLaser2 = GameCreate<LaserDrawClass>(focus, to, innerColor, outerColor, outerSpread, 15);
	pLaser2->IsHouseColor = true;
	pLaser2->Thickness = 2;
}

void DrawCircleLaser(const CoordStruct& center , const ColorStruct& color, int startAngle ,int delay, int radius = 512 )
{

	CoordStruct lastpos = CoordStruct(center.X + (int)(radius * roundoff(Math::cos(startAngle * Math::PI / 180), 5)), center.Y + (int)(radius * roundoff(Math::sin(startAngle * Math::PI / 180), 5)), center.Z);

	int lastAngle = startAngle + (360 / 80) * (80 - delay);

	for (int angle = startAngle + 5; angle < lastAngle; angle += 5)
	{
		auto currentPos = CoordStruct(center.X + (int)(radius * roundoff(Math::cos(angle * Math::PI / 180), 5)), center.Y + (int)(radius * roundoff(Math::sin(angle * Math::PI / 180), 5)), center.Z);
		auto pLaser = GameCreate<LaserDrawClass>(lastpos, currentPos, color, color, color, 5);
		pLaser->Thickness = 10;
		pLaser->IsHouseColor = true;
		lastpos = currentPos;
	}

	auto line = CoordStruct(center.X + (int)(radius * roundoff(Math::cos(lastAngle * Math::PI / 180), 5)), center.Y + (int)(radius * roundoff(Math::sin(lastAngle * Math::PI / 180), 5)), center.Z);
	auto pLine = GameCreate<LaserDrawClass>(lastpos, center, color, color, color, 5);
	pLine->Thickness = 10;
	pLine->IsHouseColor = true;

	//startAngle += 2;
}

void DrawCircleLaserB(const CoordStruct& center, const ColorStruct& trailOuterColor, int trailStartAngle, int trailRadius , int zAdjust)
{
	CoordStruct lastpos = CoordStruct(center.X + (int)(trailRadius * roundoff(Math::cos(trailStartAngle * Math::PI / 180), 5)), center.Y + (int)(trailRadius * roundoff(Math::sin(trailStartAngle * Math::PI / 180), 5)), center.Z + zAdjust);

	for (int angle = trailStartAngle + 5; angle < trailStartAngle + 360; angle += 5)
	{
		auto currentPos = CoordStruct(center.X + (int)(trailRadius * roundoff(Math::cos(angle * Math::PI / 180), 5)), center.Y + (int)(trailRadius * roundoff(Math::sin(angle * Math::PI / 180), 5)), center.Z + zAdjust);
		auto pLaser = GameCreate<LaserDrawClass>(lastpos, currentPos, trailOuterColor, trailOuterColor, trailOuterColor, 5);
		pLaser->Thickness = 10;
		pLaser->IsHouseColor = true;
		lastpos = currentPos;
	}

	trailStartAngle += 2;
}

/*
void LaunchCannon()
{
	var start = Owner.OwnerObject.Ref.Base.Base.GetCoords() + new CoordStruct(0, 0, -100);

	if (targetLocation != null && targetLocation != default)
	{
		var bigLaserColor = new ColorStruct(192, 192, 1920);
		Pointer<LaserDrawClass> pLaser = YRMemory.Create<LaserDrawClass>(start, targetLocation, bigLaserColor, bigLaserColor, bigLaserColor, 30);
		pLaser.Ref.Thickness = 30;
		pLaser.Ref.IsHouseColor = true;

		//开火动画
		var bulletFire = pBullet.Ref.CreateBullet(Owner.OwnerObject.Convert<AbstractClass>(), Owner.OwnerObject, 1, shootWarhead, 100, true);
		bulletFire.Ref.DetonateAndUnInit(Owner.OwnerObject.Ref.Base.Base.GetCoords());

		//伤害和剩余血量成正比
		var damage = (Owner.OwnerObject.Ref.Base.Health / (MAX_STRENGTH / 5)) * 150 + 200;

		//爆炸动画
		var bullet = pBullet.Ref.CreateBullet(Owner.OwnerObject.Convert<AbstractClass>(), Owner.OwnerObject, damage, expWarhead, 100, true);
		bullet.Ref.DetonateAndUnInit(targetLocation);

		//在周围造成伤害
		for (var angle = 0; angle < 360; angle += 60)
		{
			var height = Owner.OwnerObject.Ref.Base.GetHeight();
			var center = Owner.OwnerObject.Ref.Base.Base.GetCoords();

			var radius = 8 * 256;
			var targetPos = new CoordStruct(center.X + (int)(radius * Math.Round(Math.Cos(angle * Math.PI / 180), 5)), center.Y + (int)(radius * Math.Round(Math.Sin(angle * Math.PI / 180), 5)), center.Z - height);

			var cell = MapClass.Coord2Cell(targetPos);

			if (MapClass.Instance.TryGetCellAt(cell, out var pCell))
			{
				var exp2damge = (Owner.OwnerObject.Ref.Base.Health / (MAX_STRENGTH / 5)) * 100;
				var inviso = misissle.Ref.CreateBullet(Owner.OwnerObject.Convert<AbstractClass>(), Owner.OwnerObject, exp2damge, exp2Warhead, 50, false);
				inviso.Ref.MoveTo(targetPos + new CoordStruct(0, 0, 2000), new BulletVelocity(0, 0, 0));
				inviso.Ref.SetTarget(pCell.Convert<AbstractClass>());
			}
		}

	}
}*/

void VerticalLaserClass::AI(int start, int count)
{
	const int increasement = 360 / count;
	CoordStruct from = From;
	from.Z += 5000;

	for (int i = 0; i < count; i++)
	{
		const CoordStruct to = From + GetCoords(start, i , increasement);
		Draw(from, to);

		if (cur_frames > frames_max)
			DealDamage(to);
		else
			cur_frames++;
	}

}

CoordStruct VerticalLaserClass::GetCoords(int start, int i, int increase)
{
	const double x = radius * Math::cos((start + (i * increase)) * Math::C_Sharp_Pi / 180);
	const double y = radius * Math::sin((start + (i * increase)) * Math::C_Sharp_Pi / 180);
	return  { static_cast<int>(x), static_cast<int>(y), -Height };
}

void  VerticalLaserClass::DealDamage(const CoordStruct& to)
{
	auto const pWeaponExt = BulletTypeExt::ExtMap.Find(Weapon->Projectile);
	if (const auto pBullet = pWeaponExt->CreateBullet(Map[to], nullptr, Weapon))
	{

		pBullet->Limbo();
		pBullet->SetLocation(to);
		pBullet->Explode(true);
		pBullet->UnInit();
	}
}

void VerticalLaserClass::Draw(const CoordStruct& from, const CoordStruct& to)
{
	if (Weapon->IsLaser)
	{
		if (auto pLaser = GameCreate<LaserDrawClass>(from, to, Weapon->LaserInnerColor,
			Weapon->LaserOuterColor,
			Weapon->LaserOuterSpread,
			Weapon->LaserDuration)
		)
		{
			pLaser->Thickness = 10;
			pLaser->IsHouseColor = Weapon->IsHouseColor;
		}
	}
}

void VerticalLaserClass::AI()
{
	AI(angle, count);
	angle = (angle + 4) % 360;
	radius -= radius_decrement;

	if (radius <= 0)
	{
		Reset();
	}

}

void VerticalLaserClass::Clear()
{
	for (auto const& pData : Array)
	{
		GameDelete(pData);
	}

	Array.clear();
}

void VerticalLaserClass::OnUpdateAll()
{
	if (Array.empty())
		return;

	for (int i = Array.size() - 1; i >= 0; --i)
	{
		VerticalLaserClass* vLaser = Array[i];

		if (!vLaser)
			continue;

		vLaser->AI();

		if (vLaser->Expired)
		{
			Array.erase(Array.begin() + i);
			GameDelete(vLaser);
		}
	}
}

void VerticalLaserClass::PointerGotInvalid(void* ptr, bool bDetach) {}

bool VerticalLaserClass::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Process(Array)
		.Success();
}

bool VerticalLaserClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Process(Array)
		.Success();
}