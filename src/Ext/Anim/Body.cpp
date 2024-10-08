#include "Body.h"

#include <Ext/House/Body.h>
#include <Ext/AnimType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Bullet/Body.h>

#include <Utilities/Macro.h>
#include <Utilities/Helpers.h>
#include <Utilities/AnimHelpers.h>

#include <ParticleSystemClass.h>
#include <ColorScheme.h>
#include <SmudgeTypeClass.h>

//std::vector<CellClass*> AnimExtData::AnimCellUpdater::Marked;
void AnimExtData::OnInit(AnimClass* pThis, CoordStruct* pCoord)
{
	if (!pThis->Type)
		return;

	const auto pTypeExt = AnimTypeExtContainer::Instance.Find(pThis->Type);

	if (pTypeExt->ConcurrentChance.Get() >= 1.0 && !pTypeExt->ConcurrentAnim.empty())
	{
		if (ScenarioClass::Instance->Random.RandomDouble() <= pTypeExt->ConcurrentChance.Get())
		{
			auto const nIdx = pTypeExt->ConcurrentAnim.size() == 1 ?
				0 : ScenarioClass::Instance->Random.RandomFromMax(pTypeExt->ConcurrentAnim.size() - 1);

			if (auto pType = pTypeExt->ConcurrentAnim[nIdx])
			{
				if (pType == pThis->Type)
					return;

				GameCreate<AnimClass>(pType, pCoord, 0, 1, AnimFlag::AnimFlag_400 | AnimFlag::AnimFlag_200, 0, false)->Owner = pThis->GetOwningHouse();
			}
		}
	}

	//if (auto const& pSpawns = pExt->SpawnData) {
	//	pSpawns->OnInit(pCoord);
	//}
}

bool AnimExtData::OnMiddle_SpawnSmudge(AnimClass* pThis, CellClass* pCell, Point2D nOffs)
{
	const auto pType = pThis->Type;
	if (!pType)
		return false;

	const auto pTypeExt = AnimTypeExtContainer::Instance.Find(pType);

	if (pTypeExt->SpawnCrater.Get(pThis->GetHeight() < 30))
	{
		auto nCoord = pThis->GetCoords();
		if (!pType->Scorch || (pType->Crater && ScenarioClass::Instance->Random.RandomDouble() >= pTypeExt->CraterChance.Get()))
		{
			if (pType->Crater)
			{
				if (pTypeExt->CraterDecreaseTiberiumAmount.Get() > 0)
					pCell->ReduceTiberium(pTypeExt->CraterDecreaseTiberiumAmount.Get());

				if (pType->ForceBigCraters)
					SmudgeTypeClass::CreateRandomSmudgeFromTypeList(nCoord, 300, 300, true);
				else
					SmudgeTypeClass::CreateRandomSmudgeFromTypeList(nCoord, nOffs.X, nOffs.Y, false);
			}
		}
		else
		{
			const bool bSpawn = (pTypeExt->ScorchChance.isset()) ? (
				ScenarioClass::Instance->Random.RandomDouble() >= pTypeExt->ScorchChance.Get()) : true;

			if (bSpawn)
				SmudgeTypeClass::CreateRandomSmudge(nCoord, nOffs.X, nOffs.Y, false);
		}
	}

	return true;
}

bool AnimExtData::OnExpired(AnimClass* pThis, bool LandIsWater, bool EligibleHeight)
{
	if (!pThis->Type)
		return false;

	auto const pAnimTypeExt = AnimTypeExtContainer::Instance.Find(pThis->Type);

	{
		TechnoClass* const pTechOwner = AnimExtData::GetTechnoInvoker(pThis);
		auto const pOwner = !pThis->Owner && pTechOwner ? pTechOwner->Owner : pThis->Owner;

		if (!LandIsWater || EligibleHeight)
		{
			Helper::Otamaa::DetonateWarhead(int(pThis->Type->Damage), pThis->Type->Warhead, pAnimTypeExt->Warhead_Detonate, pThis->Bounce.GetCoords(), pTechOwner, pOwner, pAnimTypeExt->Damage_ConsiderOwnerVeterancy.Get());

			if (auto const pExpireAnim = pThis->Type->ExpireAnim)
			{
				AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(pExpireAnim, pThis->Bounce.GetCoords(), 0, 1, AnimFlag::AnimFlag_400 | AnimFlag::AnimFlag_200 | AnimFlag::AnimFlag_2000, -30, 0),
					pOwner,
					nullptr,
					pTechOwner,
					false
				);
			}
		}
		else
		{
			if (!pAnimTypeExt->ExplodeOnWater)
			{
				if (auto pSplashAnim = Helper::Otamaa::PickSplashAnim(pAnimTypeExt->SplashList, pAnimTypeExt->WakeAnim, pAnimTypeExt->SplashIndexRandom.Get(), pThis->Type->IsMeteor))
				{
					AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(pSplashAnim, pThis->GetCoords(), 0, 1, AnimFlag::AnimFlag_400 | AnimFlag::AnimFlag_200, false),
						pOwner,
						nullptr,
						pTechOwner,
						false
					);
				}
			}
			else
			{
				auto const& [bPlayWHAnim, nDamage] = Helper::Otamaa::DetonateWarhead(int(pThis->Type->Damage), pThis->Type->Warhead, pAnimTypeExt->Warhead_Detonate, pThis->GetCoords(), pTechOwner, pOwner, pAnimTypeExt->Damage_ConsiderOwnerVeterancy.Get());
				if (bPlayWHAnim)
				{
					if (auto pSplashAnim = MapClass::SelectDamageAnimation(nDamage, pThis->Type->Warhead, pThis->GetCell()->LandType, pThis->GetCoords()))
					{
						AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(pSplashAnim, pThis->GetCoords(), 0, 1, AnimFlag::AnimFlag_400 | AnimFlag::AnimFlag_200 | AnimFlag::AnimFlag_2000, -30),
							pOwner,
							nullptr,
							pTechOwner,
							false
						);
					}
				}
			}
		}
	}

	return true;
}

#include <Misc/PhobosGlobal.h>

DWORD AnimExtData::DealDamageDelay(AnimClass* pThis)
{
	enum { SkipDamage = 0x42465D, CheckIsActive = 0x42464C };

	if (!pThis->Type)
		return CheckIsActive;

	const auto pExt = AnimExtContainer::Instance.Find(pThis);
	const auto pTypeExt = AnimTypeExtContainer::Instance.Find(pThis->Type);
	const int delay = pTypeExt->Damage_Delay.Get();
	TechnoClass* const pInvoker = AnimExtData::GetTechnoInvoker(pThis);
	const double damageMultiplier = (pThis->OwnerObject && pThis->OwnerObject->WhatAmI() == TerrainClass::AbsID) ? 5.0 : 1.0;

	double damage = 0;
	int appliedDamage = 0;

	if (pTypeExt->Damage_ApplyOnce.Get()) // If damage is to be applied only once per animation loop
	{
		if (pThis->Animation.Value == MaxImpl(delay - 1, 1))
			appliedDamage = static_cast<int>(std::round(pThis->Type->Damage) * damageMultiplier);
		else
			return SkipDamage;
	}
	else if (delay <= 0 || pThis->Type->Damage < 1.0) // If Damage.Delay is less than 1 or Damage is a fraction.
	{
		damage = damageMultiplier * pThis->Type->Damage + pThis->Accum;

		// Deal damage if it is at least 1, otherwise accumulate it for later.
		if (damage >= 1.0)
		{
			appliedDamage = static_cast<int>(std::round(damage));
			pThis->Accum = damage - appliedDamage;
		}
		else
		{
			pThis->Accum = damage;
			return SkipDamage;
		}
	}
	else
	{
		// Accum here is used as a counter for Damage.Delay, which cannot deal fractional damage.
		damage = pThis->Accum + 1.0;
		pThis->Accum = damage;

		if (damage < delay)
			return SkipDamage;

		// Use Type->Damage as the actually dealt damage.
		appliedDamage = static_cast<int>((pThis->Type->Damage) * damageMultiplier);
		pThis->Accum = 0.0;
	}

	if (appliedDamage <= 0 || pThis->IsPlaying)
		return  SkipDamage;

	const auto nCoord = pExt && pExt->BackupCoords.has_value() ? pExt->BackupCoords.get() : pThis->GetCoords();
	const auto pOwner = pThis->Owner ? pThis->Owner : pInvoker ? pInvoker->Owner : nullptr;

	if (auto const pWeapon = pTypeExt->Weapon)
	{
		AbstractClass* pTarget = AnimExtData::GetTarget(pThis);
		// use target loc instead of anim loc , it doesnt work well with bridges
		//auto pBullet = pWeapon->Projectile->CreateBullet(pTarget, pInvoker, nDamageResult, pWeapon->Warhead, pWeapon->Speed, pWeapon->Bright);
		//pBullet->SetWeaponType(pWeapon);
		//pBullet->Limbo();
		//pBullet->SetLocation(nCoord);
		//pBullet->Explode(true);
		//pBullet->UnInit();

		WeaponTypeExtData::DetonateAt(pWeapon, nCoord, pTarget, pInvoker, appliedDamage, pTypeExt->Damage_ConsiderOwnerVeterancy.Get(), pOwner);
	}
	else
	{
		auto const pWarhead = pThis->Type->Warhead ? pThis->Type->Warhead :
			!pTypeExt->IsInviso ? RulesClass::Instance->FlameDamage2 : RulesClass::Instance->C4Warhead;

		/*if (IS_SAME_STR_("ZTARGET_B", pThis->Type->ID) && pInvoker &&  IS_SAME_STR_("MDUMMY7", pInvoker->get_ID())){
			auto invoker_loc = pInvoker->GetCoords();
			Debug::Log(__FUNCTION__" Executed Invoker [%d %d %d] Anim[%d %d %d]\n",
				invoker_loc.X,
				invoker_loc.Y,
				invoker_loc.Z,
				nCoord.X,
				nCoord.Y,
				nCoord.Z
			);

			auto checks = Helpers::Alex::getCellSpreadItems(pThis->GetCoords(), pWarhead->CellSpread, false, false);
			for (auto pTech : checks) {
				if (pTech->Owner && !pTech->Owner->IsAlliedWith(pOwner)
					&& abs(GeneralUtils::GetWarheadVersusArmor(pWarhead, pTech->GetTechnoType()->Armor) > 0.001)
					)
				{
					GameDebugLog::Log("Affecting Techno[%s] with owner[%s] from [%s]\n", pTech->get_ID(), pTech->Owner->get_ID(), pOwner->get_ID());
				}
			}

			PhobosGlobal::Instance()->AnimAttachedto = pInvoker;
		}*/
		const auto nDamageResult = static_cast<int>(appliedDamage * TechnoExtData::GetDamageMult(pInvoker, !pTypeExt->Damage_ConsiderOwnerVeterancy.Get()));

		if (pTypeExt->Warhead_Detonate.Get())
		{
			AbstractClass* pTarget = AnimExtData::GetTarget(pThis);
			// use target loc instead of anim loc , it doesnt work well with bridges
			WarheadTypeExtData::DetonateAt(pWarhead, pTarget, pTarget ? pTarget->GetCoords() : nCoord, pInvoker, nDamageResult, pOwner);
		}
		else
		{
			// Ares keep the `Source` nullptr so it can affect everything
			// if the `Source` contains `OwnerObject` it will cause problem because the techno need `DamageSelf`
			// in order to deal damage to itself ,..
			MapClass::DamageArea(nCoord, nDamageResult, pInvoker, pWarhead, pWarhead->Tiberium, pOwner);
			//PhobosGlobal::Instance()->AnimAttachedto = nullptr;
			MapClass::FlashbangWarheadAt(nDamageResult, pWarhead, nCoord);
		}
	}

	return CheckIsActive;
}

bool AnimExtData::OnMiddle(AnimClass* pThis)
{
	const auto pType = pThis->Type;
	if (!pType)
		return false;

	const auto pTypeExt = AnimTypeExtContainer::Instance.Find(pType);

	{
		auto pAnimTypeExt = pTypeExt;
		const auto pObject = AnimExtData::GetTechnoInvoker(pThis);
		const auto pHouse = !pThis->Owner && pObject ? pObject->Owner : pThis->Owner;
		const auto nCoord = pThis->Location;

		Helper::Otamaa::SpawnMultiple(
			pAnimTypeExt->SpawnsMultiple,
			pAnimTypeExt->SpawnsMultiple_amouts,
			nCoord, pObject, pHouse, pAnimTypeExt->SpawnsMultiple_Random.Get());

		if (pType->SpawnsParticle != -1)
		{
			if (const auto pParticleType = ParticleTypeClass::Array->Items[pType->SpawnsParticle])
			{
				CoordStruct InitialCoord = nCoord;
				InitialCoord.Z -= MapClass::Instance->GetCellFloorHeight(nCoord);

				if (!pAnimTypeExt->SpawnParticleModeUseAresCode)
				{
					for (int i = 0; i < pType->NumParticles; ++i)
					{
						CoordStruct nDestCoord = CoordStruct::Empty;
						if (!pAnimTypeExt->ParticleChance.isset() ||
							(ScenarioClass::Instance->Random.RandomFromMax(99) < abs(pAnimTypeExt->ParticleChance.Get())))
						{
							nDestCoord = Helper::Otamaa::GetRandomCoordsInsideLoops(pAnimTypeExt->ParticleRangeMin.Get(), pAnimTypeExt->ParticleRangeMax.Get(), InitialCoord, i);
							ParticleSystemClass::Instance->SpawnParticle(pParticleType, &nDestCoord);
						}
					}
				}
				else
				{
					int numParticle = pType->NumParticles;

					if (numParticle > 0)
					{
						const auto nMin = pAnimTypeExt->ParticleRangeMin.Get();
						const auto nMax = pAnimTypeExt->ParticleRangeMax.Get();

						if (nMin || nMax)
						{
							double rad = 6.283185307179586 / numParticle;
							double start_distance = 0.0;

							for (; numParticle > 0; --numParticle)
							{
								int rand = std::abs(ScenarioClass::Instance->Random.RandomRanged((int)nMin, (int)nMax));
								double randDouble = ScenarioClass::Instance->Random.RandomDouble() * rad + start_distance;
								CoordStruct dest {
									InitialCoord.X + int(rand * Math::cos(randDouble)),
									InitialCoord.Y - int(Math::sin(randDouble) * rand),
									nCoord.Z
								};

								dest.Z = InitialCoord.Z + MapClass::Instance->GetCellFloorHeight(dest);
								ParticleSystemClass::Instance->SpawnParticle(pParticleType, &dest);
								start_distance += rad;
							}
						}
						else
						{
							for (int i = 0; i < numParticle; ++i)
							{
								ParticleSystemClass::Instance->SpawnParticle(pParticleType, &pThis->Location);
							}
						}
					}
				}
			}
		}

		for (const auto& nLauch : pTypeExt->Launchs)
		{
			if (nLauch.LaunchWhat)
			{
				Helpers::Otamaa::LauchSW(nLauch, pHouse, nCoord, pObject);
			}
		}

		if (auto pWeapon = pTypeExt->WeaponToCarry)
		{
			AbstractClass* pTarget = AnimExtData::GetTarget(pThis);
			TechnoClass* const pInvoker = AnimExtData::GetTechnoInvoker(pThis);
			const auto nDamageResult = static_cast<int>(pWeapon->Damage * TechnoExtData::GetDamageMult(pInvoker, !pTypeExt->Damage_ConsiderOwnerVeterancy.Get()));
			const auto pOwner = pThis->Owner ? pThis->Owner : pInvoker ? pInvoker->Owner : nullptr;

			WeaponTypeExtData::DetonateAt(pWeapon, pTarget, pInvoker, pTypeExt->Damage_ConsiderOwnerVeterancy, pOwner);
		}
	}

	return true;
}

AbstractClass* AnimExtData::GetTarget(AnimClass* pThis)
{
	auto const pType = pThis->Type;
	auto const pTypeExt = AnimTypeExtContainer::Instance.Find(pType);

	if (!pTypeExt->Damage_TargetFlag.isset())
	{
		return pThis->GetCell();
	}

	switch (pTypeExt->Damage_TargetFlag.Get())
	{
	case DamageDelayTargetFlag::Cell:
		return  pThis->GetCell();
	case DamageDelayTargetFlag::AttachedObject:
	{
		if (pThis->AttachedBullet)
		{
			return pThis->AttachedBullet->Owner;
		}
		else
		{
			if (auto const pBullet = specific_cast<BulletClass*>(pThis->OwnerObject))
				return pBullet->Owner;
			else
				return pThis->OwnerObject;
		}
	}
	case DamageDelayTargetFlag::Invoker:
	{
		if (auto const pExt = AnimExtContainer::Instance.Find(pThis))
			return pExt->Invoker;
	}
	}

	return nullptr;
}

bool AnimExtData::InvalidateIgnorable(AbstractClass* ptr)
{
	switch (VTable::Get(ptr))
	{
	case BuildingClass::vtable:
	case InfantryClass::vtable:
	case UnitClass::vtable:
	case AircraftClass::vtable:
	case ParticleSystemClass::vtable:
		return false;
	}

	return true;
}

void AnimExtData::InvalidatePointer(AbstractClass* const ptr, bool bRemoved)
{
	AnnounceInvalidPointer(this->Invoker, ptr, bRemoved);
	AnnounceInvalidPointer(this->ParentBuilding, ptr, bRemoved);

	if (this->AttachedSystem.get() == ptr)
		this->AttachedSystem.release();
}

void AnimExtData::CreateAttachedSystem()
{
	const auto pThis = this->AttachedToObject;
	const auto pData = AnimTypeExtContainer::Instance.TryFind(pThis->Type);

	if (!pData || !pData->AttachedSystem || this->AttachedSystem)
		return;

	auto nLoc = pThis->Location;

	if (pData->AttachedSystem->BehavesLike == ParticleSystemTypeBehavesLike::Smoke)
		nLoc.Z += 100;

	this->AttachedSystem.reset(GameCreate<ParticleSystemClass>(
		pData->AttachedSystem.Get(),
		nLoc,
		pThis->GetCell(),
		pThis,
		CoordStruct::Empty,
		pThis->GetOwningHouse()
	));
}

//Modified from Ares
const std::pair<bool, OwnerHouseKind> AnimExtData::SetAnimOwnerHouseKind(AnimClass* pAnim, HouseClass* pInvoker, HouseClass* pVictim, bool defaultToVictimOwner)
{
	if (!pAnim || !pAnim->Type)
		return { false ,OwnerHouseKind::Default };

	auto const pTypeExt = AnimTypeExtContainer::Instance.Find(pAnim->Type);
	if (!pTypeExt->NoOwner)
	{
		const auto Owner = pTypeExt->GetAnimOwnerHouseKind();

		if (Owner == OwnerHouseKind::Invoker && !pInvoker || Owner == OwnerHouseKind::Victim && !pVictim)
			return { false , OwnerHouseKind::Default };

		const auto newOwner = HouseExtData::GetHouseKind(Owner, true, defaultToVictimOwner ? pVictim : nullptr, pInvoker, pVictim);

		if (!pAnim->Owner || pAnim->Owner != newOwner)
		{
			pAnim->SetHouse(newOwner);
			return { false , Owner };
		}
	}
	return { false , OwnerHouseKind::Default }; //yes return true
}

const std::pair<bool, OwnerHouseKind> AnimExtData::SetAnimOwnerHouseKind(AnimClass* pAnim, HouseClass* pInvoker, HouseClass* pVictim, TechnoClass* pTechnoInvoker, bool defaultToVictimOwner, bool forceOwnership)
{
	if (!pAnim || !pAnim->Type)
		return { false ,OwnerHouseKind::Default };

	auto const pTypeExt = AnimTypeExtContainer::Instance.Find(pAnim->Type);

	if (forceOwnership || !pTypeExt->NoOwner)
	{
		if (auto const pAnimExt = AnimExtContainer::Instance.Find(pAnim))
			pAnimExt->Invoker = pTechnoInvoker;

		const auto Owner = pTypeExt->GetAnimOwnerHouseKind();

		if (Owner == OwnerHouseKind::Invoker && !pInvoker || Owner == OwnerHouseKind::Victim && !pVictim)
			return { false , OwnerHouseKind::Default };

		const auto newOwner = HouseExtData::GetHouseKind(Owner, true, defaultToVictimOwner ? pVictim : nullptr, pInvoker, pVictim);

		if (!pAnim->Owner || pAnim->Owner != newOwner)
		{
			pAnim->SetHouse(newOwner);
			return { false , Owner };
		}
	}

	return { false , OwnerHouseKind::Default };
}

TechnoClass* AnimExtData::GetTechnoInvoker(AnimClass* pThis)
{
	if (!AnimTypeExtContainer::Instance.Find(pThis->Type)->Damage_DealtByInvoker)
		return nullptr;

	if (pThis->OwnerObject)
	{
		switch (pThis->OwnerObject->WhatAmI())
		{
		case BuildingClass::AbsID:
		case UnitClass::AbsID:
		case InfantryClass::AbsID:
		case AircraftClass::AbsID:
			return static_cast<TechnoClass*>(pThis->OwnerObject);
		case BulletClass::AbsID:
			return static_cast<BulletClass*>(pThis->OwnerObject)->Owner;
		}
	}

	//additional behaviour 1
	auto const pExt = AnimExtContainer::Instance.Find(pThis);
	if (pExt && pExt->Invoker)
		return pExt->Invoker;

	//additional behaviour 2
	if (auto const pBullet = pThis->AttachedBullet)
		return pBullet->Owner;

	return nullptr;
}

Layer __fastcall AnimExtData::GetLayer_patch(AnimClass* pThis, void* _)
{
	if (!pThis->OwnerObject)
		return pThis->Type ? pThis->Type->Layer : Layer::Air;

	const auto pExt = AnimTypeExtContainer::Instance.Find(pThis->Type);

	if (!pExt || !pExt->Layer_UseObjectLayer.isset())
		return Layer::Ground;

	if (pExt->Layer_UseObjectLayer.Get())
	{
		if (auto const pFoot = generic_cast<FootClass*>(pThis->OwnerObject))
		{
			if (auto const pLocomotor = pFoot->Locomotor.GetInterfacePtr())
				return pLocomotor->In_Which_Layer();
		}
		else if (auto const pBullet = specific_cast<BulletClass*>(pThis->OwnerObject))
			return pBullet->InWhichLayer();

		return pThis->OwnerObject->ObjectClass::InWhichLayer();
	}

	return pThis->Type ? pThis->Type->Layer : Layer::Air;
}

void AnimExtData::SpawnFireAnims(AnimClass* pThis)
{
	auto const pType = pThis->Type;
	auto const pExt = AnimExtContainer::Instance.Find(pThis);
	auto const pTypeExt = AnimTypeExtContainer::Instance.Find(pType);
	auto const coords = pThis->GetCoords();

	auto SpawnAnim = [&coords, pThis, pExt](AnimTypeClass* pType, int distance, bool constrainToCellSpots, bool attach)
		{
			if (!pType)
				return;

			CoordStruct newCoords = coords;

			if (distance > 0)
			{
				newCoords = MapClass::GetRandomCoordsNear(coords, distance, false);

				if (constrainToCellSpots)
					newCoords = MapClass::PickInfantrySublocation(newCoords, true);
			}

			auto const loopCount = ScenarioClass::Instance->Random.RandomRanged(1, 2);
			auto const pAnim = GameCreate<AnimClass>(pType, newCoords, 0, loopCount, 0x600u, 0, false);
			pAnim->Owner = pThis->Owner;

			if (attach && pThis->OwnerObject)
				pAnim->SetOwnerObject(pThis->OwnerObject);

			auto const pExtNew = AnimExtContainer::Instance.Find(pAnim);
			pExtNew->Invoker = pExt->Invoker ? pExt->Invoker : pExtNew->Invoker;
		};

	auto LoopAnims = [&coords, SpawnAnim](std::vector<AnimTypeClass*>* const anims, std::vector<double>* const chances, std::vector<double>* const distances,
		int count, AnimTypeClass* defaultAnimType, double defaultChance0, double defaultChanceRest, int defaultDistance0, int defaultDistanceRest, bool constrainToCellSpots, bool attach)
		{
			double chance = 0.0;
			int distance = 0;
			AnimTypeClass* pAnimType = nullptr;

			for (size_t i = 0; i < static_cast<unsigned int>(count); i++)
			{
				if (chances->size() > 0 && chances->size() > i)
					chance = (*chances)[i];
				else if (chances->size() > 0)
					chance = (*chances)[chances->size() - 1];
				else
					chance = i == 0 ? defaultChance0 : defaultChanceRest;

				if (chance < ScenarioClass::Instance->Random.RandomDouble())
					continue;

				if (anims->size() > 1)
					pAnimType = (*anims)[ScenarioClass::Instance->Random.RandomRanged(0, anims->size() - 1)];
				else if (anims->size() > 0)
					pAnimType = (*anims)[0];
				else
					pAnimType = defaultAnimType;

				if (distances->size() > 0 && distances->size() < i)
					distance = static_cast<int>((*distances)[i] * Unsorted::LeptonsPerCell);
				else if (distances->size() > 0)
					distance = static_cast<int>((*distances)[distances->size() - 1] * Unsorted::LeptonsPerCell);
				else
					distance = i == 0 ? defaultDistance0 : defaultDistanceRest;

				SpawnAnim(pAnimType, distance, constrainToCellSpots, attach);
			}
		};

	auto const disallowedLandTypes = pTypeExt->FireAnimDisallowedLandTypes.Get(pType->Scorch ? LandTypeFlags::Default : LandTypeFlags::None);

	if (IsLandTypeInFlags(disallowedLandTypes, pThis->GetCell()->LandType))
		return;

	std::vector<AnimTypeClass*>* anims = &pTypeExt->SmallFireAnims;
	std::vector<double>* chances = &pTypeExt->SmallFireChances;
	std::vector<double>* distances = &pTypeExt->SmallFireDistances;
	bool constrainToCellSpots = pTypeExt->ConstrainFireAnimsToCellSpots;
	bool attach = pTypeExt->AttachFireAnimsToParent.Get(pType->Scorch);
	int smallCount = pTypeExt->SmallFireCount.Get(1 + pType->Flamer);

	if (pType->Flamer)
	{
		LoopAnims(anims, chances, distances, smallCount, RulesClass::Instance->SmallFire, 0.5, 1.0, 64, 160, constrainToCellSpots, attach);

		anims = &pTypeExt->LargeFireAnims;
		chances = &pTypeExt->LargeFireChances;
		distances = &pTypeExt->LargeFireDistances;

		LoopAnims(anims, chances, distances, pTypeExt->LargeFireCount, RulesClass::Instance->LargeFire, 0.5, 0.5, 112, 112, constrainToCellSpots, attach);
	}
	else if (pType->Scorch)
	{
		LoopAnims(anims, chances, distances, smallCount, RulesClass::Instance->SmallFire, 1.0, 1.0, 0, 0, constrainToCellSpots, attach);
	}
}

// =============================
// load / save

template <typename T>
void AnimExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->Initialized)
		.Process(this->BackupCoords)
		.Process(this->DeathUnitFacing)
		.Process(this->DeathUnitTurretFacing)
		.Process(this->Invoker)
		.Process(this->OwnerSet)
		.Process(this->AllowCreateUnit)
		.Process(this->WasOnBridge)
		.Process(this->AttachedSystem)
		.Process(this->ParentBuilding)
		.Process(this->CreateUnitLocation)
		.Process(this->SpawnsStatusData)
		;
}

// =============================
// container

AnimExtContainer AnimExtContainer::Instance;
std::vector<AnimExtData*> AnimExtContainer::Pool;

// =============================
// hooks

//Only Extend Anim that Has "Type" Pointer
DEFINE_HOOK_AGAIN(0x4228D2, AnimClass_CTOR, 0x5)
DEFINE_HOOK(0x422131, AnimClass_CTOR, 0x6)
{
	GET(AnimClass*, pItem, ESI);

	if (pItem)
	{
		if (pItem->Fetch_ID() == -2 && pItem->Type)
		{
			Debug::Log("Anim[%s - %x] with some weird ID\n", pItem->Type->ID, pItem);
		}

		if (!pItem->Type)
		{
			Debug::Log("Anim[%x] with no Type pointer\n", pItem);
			return 0x0;
		}

		if (auto pExt = AnimExtContainer::Instance.Allocate(pItem))
		{
			// Something about creating this in constructor messes with debris anims, so it has to be done for them later.
			if (!pItem->HasExtras)
				pExt->CreateAttachedSystem();
		}
	}

	return 0;
}

DEFINE_HOOK(0x422A52, AnimClass_DTOR, 0x6)
{
	GET(AnimClass* const, pItem, ESI);
	AnimExtContainer::Instance.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x425280, AnimClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x4253B0, AnimClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(AnimClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);
	AnimExtContainer::Instance.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK_AGAIN(0x425391, AnimClass_Load_Suffix, 0x7)
DEFINE_HOOK_AGAIN(0x4253A2, AnimClass_Load_Suffix, 0x7)
DEFINE_HOOK(0x425358, AnimClass_Load_Suffix, 0x7)
{
	AnimExtContainer::Instance.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x4253FF, AnimClass_Save_Suffix, 0x5)
{
	AnimExtContainer::Instance.SaveStatic();
	return 0;
}

//DEFINE_HOOK(0x425164, AnimClass_Detach, 0x6)
//{
//	GET(AnimClass* const, pThis, ESI);
//	GET(void*, target, EDI);
//	GET_STACK(bool, all, STACK_OFFS(0xC, -0x8));
//
//	AnimExtContainer::Instance.InvalidatePointerFor(pThis, target, all);
//
//	R->EBX(0);
//	return pThis->OwnerObject == target && target ? 0x425174 : 0x4251A3;
//}

void __fastcall AnimClass_Detach_Wrapper(AnimClass* pThis, DWORD, AbstractClass* target, bool all)
{
	AnimExtContainer::Instance.InvalidatePointerFor(pThis, target, all);
	pThis->AnimClass::PointerExpired(target, all);
}

DEFINE_JUMP(VTABLE, 0x7E337C, GET_OFFSET(AnimClass_Detach_Wrapper));
DEFINE_JUMP(VTABLE, 0x7E3390, GET_OFFSET(AnimExtData::GetOwningHouse_Wrapper));