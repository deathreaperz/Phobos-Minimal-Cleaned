#include "Body.h"

#include <Ext/House/Body.h>
#include <InfantryClass.h>
#include <BulletClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <AnimTypeClass.h>
#include <AnimClass.h>
#include <BitFont.h>
#include <TagTypeClass.h>

#include <Utilities/Helpers.h>
#include <Ext/Anim/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/EnumFunctions.h>
#include <New/Entity/FlyingStrings.h>

#include <Ext/SWType/NewSuperWeaponType/Reveal.h>

#include <New/Entity/VerticalLaserClass.h>
#include <Ext/Event/Body.h>

#include <Misc/DamageArea.h>

// Wrapper for DamageArea::Apply() that sets a pointer in WarheadTypeExt::ExtData that is used to figure 'intended' target of the Warhead detonation, if set and there's no CellSpread.
DamageAreaResult WarheadTypeExtData::DamageAreaWithTarget(CoordStruct coords, int damage, TechnoClass* pSource, WarheadTypeClass* pWH, bool affectsTiberium, HouseClass* pSourceHouse, TechnoClass* pTarget)
{
	this->IntendedTarget = pTarget;
	auto result = DamageArea::Apply(&coords, damage, pSource, pWH, true, pSourceHouse);
	this->IntendedTarget = nullptr;
	return result;
}

void WarheadTypeExtData::ApplyLocomotorInfliction(TechnoClass* pTarget) const
{
	auto pTargetFoot = flag_cast_to<FootClass*>(pTarget);
	if (!pTargetFoot)
		return;

	// same locomotor? no point to change
	CLSID targetCLSID{ };
	CLSID inflictCLSID = this->AttachedToObject->Locomotor;
	IPersistPtr pLocoPersist = pTargetFoot->Locomotor;
	if (SUCCEEDED(pLocoPersist->GetClassID(&targetCLSID)) && targetCLSID == inflictCLSID)
		return;

	// prevent endless piggyback
	IPiggybackPtr pTargetPiggy = pTargetFoot->Locomotor;
	if (pTargetPiggy != nullptr && pTargetPiggy->Is_Piggybacking())
		return;

	LocomotionClass::ChangeLocomotorTo(pTargetFoot, inflictCLSID);
}

void WarheadTypeExtData::ApplyLocomotorInflictionReset(TechnoClass* pTarget) const
{
	auto pTargetFoot = flag_cast_to<FootClass*>(pTarget);

	if (!pTargetFoot)
		return;

	// remove only specific inflicted locomotor if specified
	CLSID removeCLSID = this->AttachedToObject->Locomotor;
	if (removeCLSID != CLSID())
	{
		CLSID targetCLSID{ };
		IPersistPtr pLocoPersist = pTargetFoot->Locomotor;
		if (SUCCEEDED(pLocoPersist->GetClassID(&targetCLSID)) && targetCLSID != removeCLSID)
			return;
	}

	// // we don't want to remove non-ok-to-end locos
	// IPiggybackPtr pTargetPiggy = pTargetFoot->Locomotor;
	// if (pTargetPiggy != nullptr && (!pTargetPiggy->Is_Ok_To_End()))
	// 	return;

	LocomotionClass::End_Piggyback(pTargetFoot->Locomotor);
}

void WarheadTypeExtData::ApplyDirectional(BulletClass* pBullet, TechnoClass* pTarget) const
{
	//if (!pBullet || pBullet->IsInAir() != pTarget->IsInAir() || pBullet->GetCell() != pTarget->GetCell() || pTarget->IsIronCurtained())
	//	return;

	//if (pTarget->WhatAmI() != AbstractType::Unit || pBullet->Type->Vertical)
	//	return;

	//const auto pTarExt = TechnoExtContainer::Instance.Find(pTarget);
	//if (!pTarExt || (pTarExt->Shield && pTarExt->Shield->IsActive()))
	//	return;

	//const auto pTarType = pTarget->GetTechnoType();
	//const auto pTarTypeExt = TechnoTypeExtContainer::Instance.Find(pTarType);

	//const int tarFacing = pTarget->PrimaryFacing.Current().GetValue<16>();
	//int bulletFacing = BulletExtContainer::Instance.Find(pBullet)->InitialBulletDir.get().GetValue<16>();

	//const int angle = Math::abs(bulletFacing - tarFacing);
	//auto frontField = 64 * this->DirectionalArmor_FrontField;
	//auto backField = 64 * this->DirectionalArmor_BackField;

	//if (angle >= 128 - frontField && angle <= 128 + frontField)//�����ܻ�
	//	pTarExt->ReceiveDamageMultiplier = this->DirectionalArmor_FrontMultiplier.Get();
	//else if ((angle < backField && angle >= 0) || (angle > 192 + backField && angle <= 256))//�����ܻ�
	//	pTarExt->ReceiveDamageMultiplier = this->DirectionalArmor_BackMultiplier.Get();
	//else//�����ܻ�
	//	pTarExt->ReceiveDamageMultiplier = this->DirectionalArmor_SideMultiplier.Get();
}

void WarheadTypeExtData::applyIronCurtain(const CoordStruct& coords, HouseClass* Owner, int damage) const
{
	const int icDuration = this->IC_Duration.Get();
	if (icDuration == 0)
		return;

	const float cellSpread = this->AttachedToObject->CellSpread;
	const int icCap = this->IC_Cap.Get();
	
	// Get affected objects once and cache the result
	auto affectedTechnos = Helpers::Alex::getCellSpreadItems(coords, cellSpread, true);
	
	// Process each affected object
	for (auto* curTechno : affectedTechnos)
	{
		// Quick house relationship check
		if (!this->CanAffectHouse(curTechno->Owner, Owner))
			continue;

		// Cache armor calculation
		const auto& verses = this->GetVerses(TechnoExtData::GetTechnoArmor(curTechno, this->AttachedToObject));
		if (std::abs(verses.Verses) < 0.001)
			continue;

		// Calculate duration once with modifier
		int duration = icDuration;
		if (duration > 0)
		{
			auto* pType = curTechno->GetTechnoType();
			duration = static_cast<int>(duration * TechnoTypeExtContainer::Instance.Find(pType)->IronCurtain_Modifier.Get());
		}

		// Handle ForceShield protection type
		if (curTechno->ProtectType == ProtectTypes::ForceShield)
		{
			// Apply damage first if needed
			if (damage > 0) {
				int tempDamage = damage;
				curTechno->ReceiveDamage(&tempDamage, 0, this->AttachedToObject, nullptr, true, false, Owner);
			}

			// Apply iron curtain if unit survived
			if (curTechno->IsAlive) {
				curTechno->IronCurtain(duration, Owner, false);
				curTechno->IronCurtainTimer.Start(duration);
			}
			continue;
		}

		// Handle regular iron curtain logic
		const int timeLeft = curTechno->IronCurtainTimer.GetTimeLeft();
		const int oldValue = timeLeft <= 0 ? 0 : timeLeft;
		const int newValue = Helpers::Alex::getCappedDuration(oldValue, duration, icCap);

		if (oldValue <= 0)
		{
			// Start new iron curtain effect
			if (newValue > 0)
			{
				// Apply damage first if needed
				if (damage > 0)
				{
					int tempDamage = damage;
					curTechno->ReceiveDamage(&tempDamage, 0, this->AttachedToObject, nullptr, true, false, Owner);
				}

				// Apply iron curtain if unit survived
				if (curTechno->IsAlive)
				{
					curTechno->IronCurtain(newValue, Owner, false);
					curTechno->IronCurtainTimer.Start(newValue);
				}
			}
		}
		else
		{
			// Extend existing iron curtain effect
			if (newValue > 0)
			{
				curTechno->IronCurtainTimer.Start(newValue);
				curTechno->IronTintStage = 4;
			}
			else
			{
				// Remove iron curtain
				curTechno->IronCurtainTimer.Stop();
				curTechno->IronTintStage = 0;
			}
		}
	}
}

void WarheadTypeExtData::applyIronCurtain(TechnoClass* curTechno, HouseClass* Owner, int damage) const
{
	if (this->IC_Duration != 0)
	{
		// affect each object
		{
			// duration modifier
			int duration = this->IC_Duration;

			auto pType = curTechno->GetTechnoType();

			// modify good durations only
			if (duration > 0)
			{
				duration = static_cast<int>(duration * TechnoTypeExtContainer::Instance.Find(pType)->IronCurtain_Modifier);
			}

			// get the values
			int oldValue = (curTechno->IronCurtainTimer.Expired() ? 0 : curTechno->IronCurtainTimer.GetTimeLeft());
			int newValue = Helpers::Alex::getCappedDuration(oldValue, duration, this->IC_Cap);

			// update iron curtain
			if (oldValue <= 0)
			{
				// start iron curtain effect?
				if (newValue > 0)
				{
					// damage the victim before ICing it
					if (damage)
					{
						curTechno->ReceiveDamage(&damage, 0, this->AttachedToObject, nullptr, true, false, Owner);
					}

					// unit may be destroyed already.
					if (curTechno->IsAlive)
					{
						// start and prevent the multiplier from being applied twice
						curTechno->IronCurtain(newValue, Owner, false);
						curTechno->IronCurtainTimer.Start(newValue);
					}
				}
			}
			else
			{
				// iron curtain effect is already on.
				if (newValue > 0)
				{
					// set new length and reset tint stage
					curTechno->IronCurtainTimer.Start(newValue);
					curTechno->IronTintStage = 4;
				}
				else
				{
					// turn iron curtain off
					curTechno->IronCurtainTimer.Stop();
				}
			}
		}
	}
}

void WarheadTypeExtData::ApplyAttachTag(TechnoClass* pTarget) const
{
	if (!this->AttachTag)
		return;

	const auto pType = pTarget->GetTechnoType();
	bool AllowType = true;
	bool IgnoreType = false;

	if (!this->AttachTag_Types.empty())
	{
		AllowType = this->AttachTag_Types.Contains(pType);
	}

	if (!this->AttachTag_Types.empty())
	{
		IgnoreType = this->AttachTag_Types.Contains(pType);
	}

	if (!AllowType || IgnoreType)
		return;

	auto TagID = this->AttachTag.data();
	auto Imposed = this->AttachTag_Imposed;

	if ((!pTarget->AttachedTag || Imposed))
	{
		auto pTagType = TagTypeClass::FindOrAllocate(TagID);
		pTarget->AttachTrigger(TagClass::GetInstance(pTagType));
	}
}

bool WarheadTypeExtData::applyPermaMC(HouseClass* const Owner, AbstractClass* const Target) const
{
	if (!Owner || !this->PermaMC)
		return false;

	const auto pTargetTechno = flag_cast_to<TechnoClass*>(Target);
	if (!pTargetTechno)
		return false;

	if (//!this->CanDealDamage(pTargetTechno) ||
		TechnoExtData::IsPsionicsImmune(pTargetTechno))
		return false;

	if (TechnoExtContainer::Instance.Find(pTargetTechno)->Is_DriverKilled)
		return false;

	const auto pType = pTargetTechno->GetTechnoType();

	if (auto const pController = pTargetTechno->MindControlledBy)
	{
		pController->CaptureManager->FreeUnit(pTargetTechno);
	}

	const auto pOriginalOwner = pTargetTechno->GetOwningHouse();
	pTargetTechno->SetOwningHouse(Owner);
	pTargetTechno->MindControlledByAUnit = true;
	pTargetTechno->QueueMission(Mission::Guard, false);
	pTargetTechno->OriginallyOwnedByHouse = pOriginalOwner;
	pTargetTechno->MindControlledByHouse = nullptr;

	if (auto& pAnim = pTargetTechno->MindControlRingAnim)
	{
		pAnim->TimeToDie = true;
		pAnim->UnInit();
		pAnim = nullptr;
	}

	if (auto const pAnimType = RulesClass::Instance->PermaControlledAnimationType)
	{
		auto const pBld = cast_to<BuildingClass*, false>(pTargetTechno);

		CoordStruct location = pTargetTechno->GetCoords();

		if (pBld)
		{
			location.Z += pBld->Type->Height * Unsorted::LevelHeight;
		}
		else
		{
			location.Z += pType->MindControlRingOffset;
		}

		{
			auto const pAnim = GameCreate<AnimClass>(pAnimType, location);
			pTargetTechno->MindControlRingAnim = pAnim;
			pAnim->SetOwnerObject(pTargetTechno);
			if (pBld)
			{
				pAnim->ZAdjust = -1024;
			}
		}
	}

	return true;
}

void WarheadTypeExtData::applyStealMoney(TechnoClass* const Owner, TechnoClass* const Target) const
{
	const int nStealAmout = StealMoney.Get();

	if (nStealAmout != 0)
	{
		if (Owner && Target)
		{
			auto pBulletOwnerHouse = Owner->GetOwningHouse();
			auto pBulletTargetHouse = Target->GetOwningHouse();

			if (pBulletOwnerHouse && pBulletTargetHouse)
			{
				if ((!pBulletOwnerHouse->IsNeutral() && !HouseExtData::IsObserverPlayer(pBulletOwnerHouse))
					&& (!pBulletTargetHouse->IsNeutral() && !HouseExtData::IsObserverPlayer(pBulletTargetHouse)))
				{
					if (pBulletOwnerHouse->CanTransactMoney(nStealAmout) && pBulletTargetHouse->CanTransactMoney(-nStealAmout))
					{
						pBulletOwnerHouse->TransactMoney(nStealAmout);
						FlyingStrings::AddMoneyString(Steal_Display.Get(), nStealAmout, Owner, Steal_Display_Houses.Get(), Owner->GetCoords(), Steal_Display_Offset.Get());
						pBulletTargetHouse->TransactMoney(-nStealAmout);
						FlyingStrings::AddMoneyString(Steal_Display.Get(), -nStealAmout, Target, Steal_Display_Houses.Get(), Target->GetCoords(), Steal_Display_Offset.Get());
					}
				}
			}
		}
	}
}

void WarheadTypeExtData::applyTransactMoney(TechnoClass* pOwner, HouseClass* pHouse, BulletClass* pBullet, CoordStruct const& coords) const
{
	int nTransactVal = 0;
	bool bForSelf = true;
	bool bSucceed = false;

	auto const TransactMoneyFunc = [&]()
		{
			if (pBullet && pBullet->Target)
			{
				this->applyStealMoney(pBullet->Owner, static_cast<TechnoClass*>(pBullet->Target));

				if (TransactMoney != 0)
				{
					auto const pBulletTargetHouse = pBullet->Target->GetOwningHouse();
					if (pBulletTargetHouse)
					{
						if ((!pHouse->IsNeutral() && !HouseExtData::IsObserverPlayer(pHouse)) && (!pBulletTargetHouse->IsNeutral() && !HouseExtData::IsObserverPlayer(pBulletTargetHouse)))
						{
							if (Transact_AffectsOwner.Get() && pBulletTargetHouse == pHouse)
							{
								nTransactVal = TransactMoney;
								if (nTransactVal != 0 && pHouse->CanTransactMoney(nTransactVal))
								{
									pHouse->TransactMoney(TransactMoney);
									bSucceed = true;
									return;
								}
							}

							if (pHouse->IsAlliedWith(pBulletTargetHouse))
							{
								if (Transact_AffectsAlly.Get() && pBulletTargetHouse != pHouse)
								{
									nTransactVal = TransactMoney_Ally.Get(TransactMoney);
									if (nTransactVal != 0 && pBulletTargetHouse->CanTransactMoney(nTransactVal))
									{
										pBulletTargetHouse->TransactMoney(nTransactVal);
										bSucceed = true;
										bForSelf = false;
										return;
									}
								}
							}
							else
							{
								if (Transact_AffectsEnemies.Get())
								{
									nTransactVal = TransactMoney_Enemy.Get(TransactMoney);
									if (nTransactVal != 0 && pBulletTargetHouse->CanTransactMoney(nTransactVal))
									{
										pBulletTargetHouse->TransactMoney(nTransactVal);
										bSucceed = true;
										bForSelf = false;
										return;
									}
								}
							}
						}
					}
				}
			}

			nTransactVal = TransactMoney;
			if (nTransactVal != 0 && pHouse->CanTransactMoney(nTransactVal))
			{
				pHouse->TransactMoney(TransactMoney);
				bSucceed = true;
			}
		};

	TransactMoneyFunc();

	if (nTransactVal != 0 && bSucceed && TransactMoney_Display.Get())
	{
		auto displayCoord = TransactMoney_Display_AtFirer ? (pOwner ? pOwner->Location : coords) : (!bForSelf ? pBullet && pBullet->Target ? pBullet->Target->GetCoords() : coords : coords);
		auto pDrawOwner = TransactMoney_Display_AtFirer ? (pOwner ? pOwner : nullptr) : (!bForSelf ? (pBullet && pBullet->Target ? flag_cast_to<TechnoClass*>(pBullet->Target) : nullptr) : nullptr);

		FlyingStrings::AddMoneyString(true, nTransactVal, pDrawOwner, TransactMoney_Display_Houses.Get(), displayCoord, TransactMoney_Display_Offset.Get());
	}
}

void WarheadTypeExtData::InterceptBullets(TechnoClass* pOwner, WeaponTypeClass* pWeapon, CoordStruct coords) const
{
	if (!pOwner || !pWeapon)
		return;

	const float cellSpread = this->AttachedToObject->CellSpread;

	if (cellSpread == 0.0)
	{
		if (auto const pBullet = cast_to<BulletClass*>(pOwner->Target))
		{
			// 1/8th of a cell as a margin of error.
			if (BulletTypeExtContainer::Instance.Find(pBullet->Type)->Interceptable && (pWeapon->Projectile->Inviso || pBullet->Location.DistanceFrom(coords) <= Unsorted::LeptonsPerCell / 8.0))
				BulletExtData::InterceptBullet(pBullet, pOwner, pWeapon);
		}
	}
	else
	{
		BulletClass::Array->for_each([&](BulletClass* pTargetBullet)
			{
				if (pTargetBullet)
				{
					const auto pBulletExt = BulletExtContainer::Instance.Find(pTargetBullet);
					if (pBulletExt->CurrentStrength > 0 && !pTargetBullet->InLimbo)
					{
						auto const pBulletTypeExt = BulletTypeExtContainer::Instance.Find(pTargetBullet->Type);
						// Cells don't know about bullets that may or may not be located on them so it has to be this way.
						if (pBulletTypeExt->Interceptable &&
							pTargetBullet->Location.DistanceFrom(coords) <= (cellSpread * Unsorted::LeptonsPerCell))
							BulletExtData::InterceptBullet(pTargetBullet, pOwner, pWeapon);
					}
				}
			});
	}
}

static void SpawnCrate(std::vector<int>& types, std::vector<int>& weights, CoordStruct& place)
{
	if (!types.empty()) {
		const int index = GeneralUtils::ChooseOneWeighted(ScenarioClass::Instance->Random.RandomDouble(), weights);

		if ((size_t)index < types.size()) {
			MapClass::Instance->Place_Crate(CellClass::Coord2Cell(place), (PowerupEffects)types[index]);
		}
	}
}

static bool NOINLINE IsCellSpreadWH(WarheadTypeExtData* pData)
{
	// Cache frequently accessed values
	const float cellSpread = pData->AttachedToObject->CellSpread;
	
	// Check actual cell spread first (most common case)
	if (std::abs(cellSpread) >= 0.1f)
		return true;
	
	// Group shield-related checks
	if (pData->Shield_Break.Get() ||
		pData->Shield_Respawn_Duration.Get() > 0 ||
		pData->Shield_SelfHealing_Duration.Get() > 0 ||
		!pData->Shield_AttachTypes.empty() ||
		!pData->Shield_RemoveTypes.empty() ||
		pData->Shield_RemoveAll.Get())
		return true;
	
	// Group effect-related checks
	if (!pData->PhobosAttachEffects.AttachTypes.empty() ||
		!pData->PhobosAttachEffects.RemoveTypes.empty() ||
		!pData->PhobosAttachEffects.RemoveGroups.empty())
		return true;
	
	// Group combat-related checks
	if (!pData->ConvertsPair.empty() ||
		pData->GattlingStage.Get() > 0 ||
		pData->GattlingRateUp.Get() != 0 ||
		pData->ReloadAmmo.Get() != 0 ||
		(pData->RevengeWeapon.Get() && pData->RevengeWeapon_GrantDuration.Get() > 0))
		return true;
	
	// Group special effect checks
	if (pData->Transact.Get() ||
		pData->PermaMC.Get() ||
		pData->AttachTag ||
		pData->IC_Duration.Get() != 0 ||
		!pData->LimboKill_IDs.empty() ||
		(pData->PaintBallData.Color != ColorStruct::Empty))
		return true;
	
	// Group locomotor and building checks
	if (pData->InflictLocomotor.Get() ||
		pData->RemoveInflictedLocomotor.Get() ||
		pData->BuildingSell.Get() ||
		pData->BuildingUndeploy.Get())
		return true;
	
	// Check critical hit chances
	if (!pData->Crit_CurrentChance.empty())
		return true;
	
	return false;
}

void WarheadTypeExtData::Detonate(TechnoClass* pOwner, HouseClass* pHouse, BulletClass* pBullet, CoordStruct coords, int damage)
{
	// Cache frequently accessed values to avoid repeated lookups
	const auto* pAttachedWarhead = this->AttachedToObject;
	const float cellSpread = pAttachedWarhead->CellSpread;
	const bool hasCellSpread = fabs(cellSpread) >= 0.1f;
	const bool hasInterceptor = pOwner && pBullet && 
		pOwner->IsAlive && pBullet->IsAlive &&
		TechnoExtContainer::Instance.Find(pOwner)->IsInterceptor() && 
		BulletExtContainer::Instance.Find(pBullet)->IsInterceptor;
	
	VocClass::SafeImmedietelyPlayAt(Sound, &coords);
	
	// Batch particle system creation
	if (!this->DetonateParticleSystem.empty())
	{
		for (auto const& pSys : this->DetonateParticleSystem)
		{
			GameCreate<ParticleSystemClass>(pSys, coords, nullptr, nullptr, CoordStruct::Empty, pHouse);
		}
	}

	SpawnCrate(this->SpawnsCrate_Types, this->SpawnsCrate_Weights, coords);

	if (!pBullet && !this->DetonatesWeapons.empty())
	{
		for (auto const& pWeapon : this->DetonatesWeapons)
		{
			WeaponTypeExtData::DetonateAt(pWeapon, coords, pOwner, true, pHouse);
		}
	}

	if (hasInterceptor)
	{
		this->InterceptBullets(pOwner, pBullet->WeaponType, coords);
	}

	// Optimize house iteration logic
	if (pHouse)
	{
		if (this->BigGap) {
			HouseClass::Array->for_each([&](HouseClass* pOtherHouse) {
				if (pOtherHouse->IsControlledByHuman() &&	  // Not AI
					!HouseExtData::IsObserverPlayer(pOtherHouse) &&		  // Not Observer
					!pOtherHouse->Defeated &&			  // Not Defeated
					pOtherHouse != pHouse &&			  // Not pThisHouse
					!pOtherHouse->SpySatActive && // No SpySat
					!pHouse->IsAlliedWith(pOtherHouse))   // Not Allied
				{
					pOtherHouse->ReshroudMap();
				}
				});
		}
		else {
			const int createGap = this->CreateGap.Get();
			if (createGap > 0)
			{
				const auto pCurrent = HouseClass::CurrentPlayer();

				if (pCurrent &&
					!pCurrent->IsObserver() &&		// Not Observer
					!pCurrent->Defeated &&						// Not Defeated
					pCurrent != pHouse &&						// Not pThisHouse
					!pCurrent->SpySatActive &&				// No SpySat
					!pCurrent->IsAlliedWith(pHouse))			// Not Allied
				{
					CellClass::CreateGap(pCurrent, createGap, coords);
				}
			}
			else if (createGap < 0)
			{
				HouseClass::Array->for_each([&](HouseClass* pOtherHouse)
					{
						if (pOtherHouse->IsControlledByHuman() &&	  // Not AI
							!HouseExtData::IsObserverPlayer(pOtherHouse) &&		  // Not Observer
							!pOtherHouse->Defeated &&			  // Not Defeated
							pOtherHouse != pHouse &&			  // Not pThisHouse
							!pOtherHouse->SpySatActive && // No SpySat
							!pHouse->IsAlliedWith(pOtherHouse))   // Not Allied
						{
							MapClass::Instance->Reshroud(pOtherHouse);
						}
					});
			}
		}

		const int revealValue = this->Reveal.Get();
		if (revealValue < 0)
		{
			MapClass::Instance->Reveal(pHouse);
		}
		else if (revealValue > 0)
		{
			SW_Reveal::RevealMap(CellClass::Coord2Cell(coords), (float)revealValue, 0, pHouse);
		}

		this->applyTransactMoney(pOwner, pHouse, pBullet, coords);
	}

	// Pre-calculate critical hit data once
	this->HasCrit = false;
	this->GetCritChance(pOwner, this->Crit_CurrentChance);
	const bool hasCritChance = !this->Crit_CurrentChance.empty() && 
		((this->Crit_CurrentChance.size() == 1 && this->Crit_CurrentChance[0] > 0.0) || 
		 this->Crit_CurrentChance.size() > 1);

	this->RandomBuffer = ScenarioClass::Instance->Random.RandomDouble();

	// Early exit if no cell spread and no crit chance
	if (!IsCellSpreadWH(this) && !hasCritChance)
		return;

	const bool bulletWasIntercepted = pBullet ? 
		BulletExtContainer::Instance.Find(pBullet)->InterceptedStatus == InterceptedStatus::Intercepted : false;

	// Optimize cell spread processing
	if (hasCellSpread)
	{
		// Get targets once and reuse
		std::vector<TechnoClass*> pTargetv = Helpers::Alex::getCellSpreadItems(coords, cellSpread, true);
		
		// Process all targets in a single loop
		for (TechnoClass* pTarget : pTargetv)
		{
			this->DetonateOnOneUnit(pHouse, pTarget, damage, pOwner, pBullet, bulletWasIntercepted);
		}

		if (this->Transact)
		{
			this->TransactOnAllUnits(pTargetv, pHouse, pOwner);
		}
	}
	else if (pBullet && pBullet->Target)
	{
		// Optimized single target processing
		const double distanceThreshold = Unsorted::LeptonsPerCell / 4.0;
		if (pBullet->DistanceFrom(pBullet->Target) < distanceThreshold) 
		{
			const auto targetType = pBullet->Target->WhatAmI();
			switch (targetType)
			{
			case BuildingClass::AbsID:
			case AircraftClass::AbsID:
			case UnitClass::AbsID:
			case InfantryClass::AbsID:
			{
				auto* pTechnoTarget = static_cast<TechnoClass*>(pBullet->Target);
				this->DetonateOnOneUnit(pHouse, pTechnoTarget, damage, pOwner, pBullet, bulletWasIntercepted);

				if (this->Transact)
				{
					// Inline eligibility check for performance
					if (CanDealDamage(pTechnoTarget) &&
						CanTargetHouse(pHouse, pTechnoTarget) &&
						pTechnoTarget->GetTechnoType()->Trainable)
					{
						this->TransactOnOneUnit(pTechnoTarget, pOwner, 1);
					}
				}
				break;
			}
			case CellClass::AbsID:
			{
				if (this->Transact)
					this->TransactOnOneUnit(nullptr, pOwner, 1);
				break;
			}
			default:
				break;
			}
		}
	}
	else if (auto pIntended = this->IntendedTarget)
	{
		const double distanceThreshold = Unsorted::LeptonsPerCell / 4.0;
		if (coords.DistanceFrom(pIntended->GetCoords()) < distanceThreshold) 
		{
			this->DetonateOnOneUnit(pHouse, pIntended, damage, pOwner, pBullet, bulletWasIntercepted);

			if (this->Transact) 
			{
				// Inline eligibility check for performance
				if (CanDealDamage(pIntended) &&
					CanTargetHouse(pHouse, pIntended) &&
					(pIntended->GetTechnoType()->Trainable || !this->Transact_Experience_IgnoreNotTrainable.Get()))
				{
					this->TransactOnOneUnit(pIntended, pOwner, 1);
				}
			}
		}
	}
}

void WarheadTypeExtData::DetonateOnOneUnit(HouseClass* pHouse, TechnoClass* pTarget, int damage, TechnoClass* pOwner, BulletClass* pBullet, bool bulletWasIntercepted)
{
	// Early validation checks - fail fast
	if (!this->CanDealDamage(pTarget) || !this->CanTargetHouse(pHouse, pTarget))
		return;

	// Cache frequently accessed container lookups
	auto* pTargetExt = TechnoExtContainer::Instance.Find(pTarget);
	
	this->applyWebby(pTarget, pHouse, pOwner);

	if (!this->LimboKill_IDs.empty())
	{
		BuildingExtData::ApplyLimboKill(this->LimboKill_IDs, this->LimboKill_Affected, pTarget->Owner, pHouse);
	}

	this->ApplyShieldModifiers(pTarget);

	if (this->PermaMC)
		this->applyPermaMC(pHouse, pTarget);

	// Critical hit processing - check suppression condition once
	if (!this->Crit_CurrentChance.empty() && (!this->Crit_SuppressOnIntercept || !bulletWasIntercepted))
	{
		this->ApplyCrit(pHouse, pTarget, pOwner);

		if (!pTarget->IsAlive)
			return;
	}

	// PaintBall processing - optimize timer operations
	if (this->PaintBallDuration.isset() && this->PaintBallData.Color != ColorStruct::Empty) 
	{
		auto& paintball = pTargetExt->PaintBallStates[this->AttachedToObject];
		paintball.SetData(this->PaintBallData);
		paintball.Init();

		const int duration = this->PaintBallDuration.Get();
		if (duration < 0 || this->PaintBallData.Accumulate) 
		{
			int value = paintball.timer.GetTimeLeft() + duration;
			if (value <= 0) {
				paintball.timer.Stop();
			} else {
				paintball.timer.Add(value);
			}
		}
		else 
		{
			// Simplified timer logic - override condition is redundant since we're starting anyway
			paintball.timer.Start(duration);
		}
	}

	// Batch simple property modifications
	const int gattlingStage = this->GattlingStage.Get();
	const int gattlingRateUp = this->GattlingRateUp.Get();
	const int reloadAmmo = this->ReloadAmmo.Get();
	
	if (gattlingStage > 0)
		this->ApplyGattlingStage(pTarget, gattlingStage);

	if (gattlingRateUp != 0)
		this->ApplyGattlingRateUp(pTarget, gattlingRateUp);

	if (reloadAmmo != 0)
		this->ApplyReloadAmmo(pTarget, reloadAmmo);

	// Remaining effects - only call if needed
	TechnoTypeConvertData::ApplyConvert(this->ConvertsPair, pHouse, pTarget, this->Convert_SucceededAnim);

	if (this->AttachTag)
		this->ApplyAttachTag(pTarget);

	if (this->RevengeWeapon && this->RevengeWeapon_GrantDuration > 0)
		this->ApplyRevengeWeapon(pTarget);

	if (this->InflictLocomotor)
		this->ApplyLocomotorInfliction(pTarget);

	if (this->RemoveInflictedLocomotor)
		this->ApplyLocomotorInflictionReset(pTarget);

	// Phobos attach effects - single condition check
	if (!this->PhobosAttachEffects.AttachTypes.empty() ||
		!this->PhobosAttachEffects.RemoveTypes.empty() ||
		!this->PhobosAttachEffects.RemoveGroups.empty())
	{
		this->ApplyAttachEffects(pTarget, pHouse, pOwner);
	}
}

//void WarheadTypeExtData::DetonateOnAllUnits(HouseClass* pHouse, const CoordStruct coords, const float cellSpread, TechnoClass* pOwner)
//{
//	for (auto pTarget : Helpers::Alex::getCellSpreadItems(coords, cellSpread, true))
//	{
//		this->DetonateOnOneUnit(pHouse, pTarget, pOwner);
//	}
//}

void WarheadTypeExtData::ApplyShieldModifiers(TechnoClass* pTarget) const
{
	if (!pTarget)
		return;

	auto* pExt = TechnoExtContainer::Instance.Find(pTarget);
	auto* pCurrentShield = pExt->GetShield();
	
	int shieldIndex = -1;
	double oldRatio = 1.0;

	// Remove shield logic
	if (pCurrentShield)
	{
		const bool shouldRemoveAll = this->Shield_RemoveAll.Get();
		if (shouldRemoveAll)
		{
			shieldIndex = 0; // Mark for removal
		}
		else if (!this->Shield_RemoveTypes.empty())
		{
			shieldIndex = this->Shield_RemoveTypes.IndexOf(pCurrentShield->GetType());
		}

		if (shieldIndex >= 0 || shouldRemoveAll)
		{
			oldRatio = pCurrentShield->GetHealthRatio();
			pExt->CurrentShieldType = ShieldTypeClass::Array.begin()->get();
			pExt->Shield.reset(nullptr);
			pCurrentShield = nullptr; // Clear reference since shield was removed
		}
	}

	// Attach shield logic - only if we have types to attach
	if (!this->Shield_AttachTypes.empty())
	{
		ShieldTypeClass* shieldType = nullptr;

		if (this->Shield_ReplaceOnly.Get())
		{
			if (shieldIndex >= 0)
			{
				const int nMax = static_cast<int>(Shield_AttachTypes.size() - 1);
				shieldType = Shield_AttachTypes[std::min(shieldIndex, nMax)];
			}
			else if (this->Shield_RemoveAll.Get())
			{
				shieldType = Shield_AttachTypes[0];
			}
		}
		else
		{
			shieldType = Shield_AttachTypes[0];
		}

		if (shieldType && shieldType->Strength)
		{
			const bool shouldReplace = !pExt->Shield || 
				(this->Shield_ReplaceNonRespawning.Get() && 
				 pExt->Shield->IsBrokenAndNonRespawning() &&
				 pExt->Shield->GetFramesSinceLastBroken() >= this->Shield_MinimumReplaceDelay.Get());

			if (shouldReplace)
			{
				pExt->CurrentShieldType = shieldType;
				pExt->Shield = std::make_unique<ShieldClass>(pTarget, true);

				if (this->Shield_ReplaceOnly.Get() && this->Shield_InheritStateOnReplace.Get())
				{
					const int newHP = static_cast<int>(shieldType->Strength * oldRatio);
					pExt->Shield->SetHP(newHP);

					if (newHP == 0)
					{
						pExt->Shield->SetRespawn(shieldType->Respawn_Rate, shieldType->Respawn, 
							shieldType->Respawn_Rate, this->Shield_Respawn_RestartTimer.Get());
					}
				}
				
				pCurrentShield = pExt->Shield.get(); // Update reference
			}
		}
	}

	// Apply other modifiers - only if we have a shield and it passes type checks
	if (pCurrentShield)
	{
		const auto* pCurrentType = pCurrentShield->GetType();

		// Early exit if type filtering excludes this shield
		if (!this->Shield_AffectTypes.empty() && !this->Shield_AffectTypes.Contains(const_cast<ShieldTypeClass*>(pCurrentType)))
			return;

		// Break shield
		if (this->Shield_Break.Get() && pCurrentShield->IsActive() && 
			this->Shield_Break_Types.Eligible(this->Shield_AffectTypes, const_cast<ShieldTypeClass*>(pCurrentType)))
		{
			pCurrentShield->BreakShield(this->Shield_BreakAnim.Get(), this->Shield_BreakWeapon.Get());
		}

		// Respawn modification
		const int respawnDuration = this->Shield_Respawn_Duration.Get();
		if (respawnDuration > 0 && this->Shield_Respawn_Types.Eligible(this->Shield_AffectTypes, const_cast<ShieldTypeClass*>(pCurrentType)))
		{
			double amount = this->Shield_Respawn_Amount.Get(pCurrentType->Respawn);
			pCurrentShield->SetRespawn(respawnDuration, amount, this->Shield_Respawn_Rate.Get(), 
				this->Shield_Respawn_RestartTimer.Get());
		}

		// Self-healing modification
		const int selfHealingDuration = this->Shield_SelfHealing_Duration.Get();
		if (selfHealingDuration > 0 && this->Shield_SelfHealing_Types.Eligible(this->Shield_AffectTypes, const_cast<ShieldTypeClass*>(pCurrentType)))
		{
			double amount = this->Shield_SelfHealing_Amount.Get(pCurrentType->SelfHealing);
			pCurrentShield->SetSelfHealing(selfHealingDuration, amount, this->Shield_SelfHealing_Rate.Get(),
				this->Shield_SelfHealing_RestartInCombat.Get(pCurrentType->SelfHealing_RestartInCombat),
				this->Shield_SelfHealing_RestartInCombatDelay.Get(), this->Shield_SelfHealing_RestartTimer.Get());
		}
	}
}

void WarheadTypeExtData::ApplyRemoveMindControl(HouseClass* pHouse, TechnoClass* pTarget) const
{
	if (const auto pController = pTarget->MindControlledBy)
		pController->CaptureManager->FreeUnit(pTarget);
}

void WarheadTypeExtData::ApplyRemoveDisguise(HouseClass* pHouse, TechnoClass* pTarget) const
{
	//this is here , just in case i need special treatment for `TankDisguiseAsTank`
	if (auto const pFoot = flag_cast_to<FootClass*>(pTarget))
	{
		if (pFoot->IsDisguised())
		{
			pFoot->ClearDisguise();
		}
	}
}

// https://github.com/Phobos-developers/Phobos/pull/1263
 // TODO : update
void WarheadTypeExtData::ApplyCrit(HouseClass* pHouse, TechnoClass* pTarget, TechnoClass* pOwner)
{
	if (TechnoExtData::IsCritImmune(pTarget))
		return;

	const auto& tresh = this->Crit_GuaranteeAfterHealthTreshold.Get(pTarget);
	size_t level = 0;

	if (!tresh->isset() || pTarget->GetHealthPercentage() > tresh->Get())
	{
		if (!this->Crit_AffectBelowPercent.empty()) {
			for (; level < this->Crit_AffectBelowPercent.size() - 1; level++) {
				if (pTarget->GetHealthPercentage() > this->Crit_AffectBelowPercent[level + 1])
					break;
			}
		}

		const double dice = this->Crit_ApplyChancePerTarget ?
			ScenarioClass::Instance->Random.RandomDouble() : this->RandomBuffer;

		const auto chance = this->Crit_CurrentChance.size() == 1 ? this->Crit_CurrentChance[0] :
			this->Crit_CurrentChance.size() < level ? this->Crit_CurrentChance[level] : 0.0;

		if (!this->Crit_ActiveChanceAnims.empty() && chance > 0.0) {
			int idx = ScenarioClass::Instance->Random.RandomRanged(0, this->Crit_ActiveChanceAnims.size() - 1);

			AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(this->Crit_ActiveChanceAnims[idx], pTarget->Location),
				pHouse,
				pTarget->GetOwningHouse(),
				pOwner,
				false, false);
		}

		if (chance < dice) {
			return;
		}
	}

	if (auto pExt = TechnoExtContainer::Instance.Find(pTarget))
	{
		const auto pSld = pExt->Shield.get();
		if (pSld && pSld->IsActive() && pSld->GetType()->ImmuneToCrit)
			return;
	}

	if (!EnumFunctions::CanTargetHouse(this->Crit_AffectsHouses, pHouse, pTarget->GetOwningHouse()))
		return;

	if (!EnumFunctions::IsCellEligible(pTarget->GetCell(), this->Crit_Affects))
		return;

	if (!EnumFunctions::IsTechnoEligible(pTarget, this->Crit_Affects))
		return;

	this->HasCrit = true;

	if (this->Crit_AnimOnAffectedTargets && !this->Crit_AnimList.empty())
	{
		if (!this->Crit_AnimList_CreateAll.Get(false))
		{
			const int idx = this->AttachedToObject->EMEffect || this->Crit_AnimList_PickRandom.Get(this->AnimList_PickRandom) ?
				ScenarioClass::Instance->Random.RandomFromMax(this->Crit_AnimList.size() - 1) : 0;

			AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(this->Crit_AnimList[idx], pTarget->Location),
				pHouse,
				pTarget->GetOwningHouse(),
				pOwner,
				false, false
			);
		}
		else {
			for (auto const& pType : this->Crit_AnimList) {
				AnimExtData::SetAnimOwnerHouseKind(GameCreate<AnimClass>(pType, pTarget->Location),
					pHouse,
					pTarget->GetOwningHouse(),
					pOwner,
					false, false
				);
			}
		}
	}

	int damage = 0;

	if (this->Crit_ExtraDamage.size() == 1)
		damage = Crit_ExtraDamage[0];
	else if (this->Crit_ExtraDamage.size() > level)
		damage = Crit_ExtraDamage[level];

	damage = static_cast<int>(TechnoExtData::GetDamageMult(pOwner, damage, !this->Crit_ExtraDamage_ApplyFirepowerMult));

	if (this->Crit_Warhead)
		WarheadTypeExtData::DetonateAt(this->Crit_Warhead.Get(), pTarget, pOwner, damage, pHouse);
	else
		pTarget->ReceiveDamage(&damage, 0, this->AttachedToObject, pOwner, false, false, pHouse);
}

void WarheadTypeExtData::ApplyGattlingStage(TechnoClass* pTarget, int Stage) const
{
	auto pData = pTarget->GetTechnoType();
	if (pData->IsGattling)
	{
		// if exceeds, pick the largest stage
		if (Stage > pData->WeaponStages)
		{
			Stage = pData->WeaponStages;
		}

		pTarget->CurrentGattlingStage = Stage - 1;
		if (Stage == 1)
		{
			pTarget->GattlingValue = 0;
			pTarget->GattlingAudioPlayed = false;
		}
		else
		{
			pTarget->GattlingValue = pTarget->Veterancy.IsElite() ? pData->EliteStage[Stage - 2] : pData->WeaponStage[Stage - 2];
			//pTarget->GattlingAudioPlayed = true;
			pTarget->Audio4.AudioEventHandleStop();
			pTarget->GattlingAudioPlayed = false;
		}
	}
}

void WarheadTypeExtData::ApplyGattlingRateUp(TechnoClass* pTarget, int RateUp) const
{
	auto pData = pTarget->GetTechnoType();
	if (pData->IsGattling)
	{
		auto curValue = pTarget->GattlingValue + RateUp;
		auto maxValue = pTarget->Veterancy.IsElite() ? pData->EliteStage[pData->WeaponStages - 1] : pData->WeaponStage[pData->WeaponStages - 1];

		//set current weapon stage manually
		if (curValue <= 0)
		{
			pTarget->GattlingValue = 0;
			pTarget->CurrentGattlingStage = 0;
			pTarget->GattlingAudioPlayed = false;
		}
		else if (curValue >= maxValue)
		{
			pTarget->GattlingValue = maxValue;
			pTarget->CurrentGattlingStage = pData->WeaponStages - 1;
			pTarget->GattlingAudioPlayed = true;
		}
		else
		{
			pTarget->GattlingValue = curValue;
			pTarget->GattlingAudioPlayed = true;
			for (int i = 0; i < pData->WeaponStages; i++)
			{
				if (pTarget->Veterancy.IsElite() && curValue < pData->EliteStage[i])
				{
					pTarget->CurrentGattlingStage = i;
					break;
				}
				else if (curValue < pData->WeaponStage[i])
				{
					pTarget->CurrentGattlingStage = i;
					break;
				}
			}
		}
	}
}

void WarheadTypeExtData::ApplyReloadAmmo(TechnoClass* pTarget, int ReloadAmount) const
{
	auto pData = pTarget->GetTechnoType();
	if (pData->Ammo > 0)
	{
		auto const ammo = pTarget->Ammo + ReloadAmount;
		pTarget->Ammo = std::clamp(ammo, 0, pData->Ammo);
	}
}

void WarheadTypeExtData::ApplyRevengeWeapon(TechnoClass* pTarget) const
{
	auto const maxCount = this->RevengeWeapon_MaxCount;
	if (!maxCount)
		return;

	int count = 0;

	if (auto const pExt = TechnoExtContainer::Instance.Find(pTarget))
	{
		if (!this->RevengeWeapon_Cumulative)
		{
			for (auto& weapon : pExt->RevengeWeapons)
			{
				// If it is same weapon just refresh timer.
				if (weapon.SourceWarhead && weapon.SourceWarhead == this->AttachedToObject)
				{
					auto const nDur = this->RevengeWeapon_GrantDuration.Get();
					auto const nTime = weapon.Timer.GetTimeLeft();

					weapon.Timer.Start(MaxImpl(nDur, nTime));
					return;
				}

				count++;
			}
		}

		if (maxCount < 0 || count < maxCount)
		{
			pExt->RevengeWeapons.emplace_back(this->RevengeWeapon.Get(), this->RevengeWeapon_GrantDuration, this->RevengeWeapon_AffectsHouses, this->AttachedToObject);
		}
	}
}