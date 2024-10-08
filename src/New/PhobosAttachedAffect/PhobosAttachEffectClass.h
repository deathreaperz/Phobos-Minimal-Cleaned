#pragma once

#include <Utilities/SavegameDef.h>
#include <Utilities/Constructs.h>

#include <New/PhobosAttachedAffect/PhobosAttachEffectTypeClass.h>

class AbstractClass;
class TechnoClass;
class HouseClass;
class AnimClass;
class PhobosAttachEffectClass
{
public:

	PhobosAttachEffectClass() noexcept = default;

	////PhobosAttachEffectClass(PhobosAttachEffectTypeClass* pType, TechnoClass* pTechno, HouseClass* pInvokerHouse, TechnoClass* pInvoker,
	////	AbstractClass* pSource, int durationOverride, int delay, int initialDelay, int recreationDelay);

	PhobosAttachEffectClass::PhobosAttachEffectClass(const PhobosAttachEffectClass& that) :
		Duration { that.Duration },
		DurationOverride { that.DurationOverride },
		Delay { that.Delay },
		CurrentDelay { that.CurrentDelay },
		InitialDelay { that.InitialDelay },
		RecreationDelay { that.RecreationDelay },
		Type { that.Type },
		Techno { that.Techno },
		InvokerHouse { that.InvokerHouse },
		Invoker { that.Invoker },
		Source { that.Source },
		Animation { nullptr },
		IsAnimHidden { that.IsAnimHidden },
		IsInTunnel { that.IsInTunnel },
		IsUnderTemporal { that.IsUnderTemporal },
		IsOnline { that.IsOnline },
		IsCloaked { that.IsCloaked },
		HasInitialized { that.HasInitialized },
		NeedsDurationRefresh { that.NeedsDurationRefresh },
		IsFirstCumulativeInstance { that.IsFirstCumulativeInstance }
	{
		//oogly
		auto c_remove = const_cast<PhobosAttachEffectClass*>(&that);
		auto ptr = c_remove->Animation.release();
		this->Animation.reset(ptr);
	}

	PhobosAttachEffectClass& operator=(const PhobosAttachEffectClass& that)
	{
		this->Duration = that.Duration;
		this->DurationOverride = that.DurationOverride;
		this->Delay = that.Delay;
		this->CurrentDelay = that.CurrentDelay;
		this->InitialDelay = that.InitialDelay;
		this->RecreationDelay = that.RecreationDelay;
		this->Type = that.Type;
		this->Techno = that.Techno;
		this->InvokerHouse = that.InvokerHouse;
		this->Invoker = that.Invoker;
		this->Source = that.Source;
		this->IsAnimHidden = that.IsAnimHidden;
		this->IsInTunnel = that.IsInTunnel;
		this->IsUnderTemporal = that.IsUnderTemporal;
		this->IsOnline = that.IsOnline;
		this->IsCloaked = that.IsCloaked;
		this->HasInitialized = that.HasInitialized;
		this->NeedsDurationRefresh = that.NeedsDurationRefresh;
		this->IsFirstCumulativeInstance = that.IsFirstCumulativeInstance;

		//oogly
		auto c_remove = const_cast<PhobosAttachEffectClass*>(&that);
		auto ptr = c_remove->Animation.release();
		this->Animation.reset(ptr);
		return *this;
	}

	void Initialize(PhobosAttachEffectTypeClass* pType, TechnoClass* pTechno, HouseClass* pInvokerHouse,
	TechnoClass* pInvoker, AbstractClass* pSource, int durationOverride, int delay, int initialDelay, int recreationDelay);
	void AI();
	void AI_Temporal();
	void KillAnim();
	void SetAnimationTunnelState(bool visible);
	constexpr FORCEINLINE PhobosAttachEffectTypeClass* GetType() const
	{
		return this->Type;
	}

	void RefreshDuration(int durationOverride = 0);
	bool ResetIfRecreatable();
	constexpr FORCEINLINE bool IsSelfOwned() const
	{
		return this->Source == this->Techno;
	}
	constexpr FORCEINLINE bool HasExpired() const
	{
		return this->IsSelfOwned() && this->Delay >= 0 ? false : !this->Duration;
	}
	bool AllowedToBeActive() const;
	constexpr FORCEINLINE bool IsActive() const
	{
		if (this->IsSelfOwned())
			return this->InitialDelay <= 0 && this->CurrentDelay == 0 && this->HasInitialized && this->IsOnline && !this->NeedsDurationRefresh;
		else
			return this->Duration && this->IsOnline;
	}

	constexpr FORCEINLINE bool IsFromSource(TechnoClass* pInvoker, AbstractClass* pSource) const
	{
		return pInvoker == this->Invoker && pSource == this->Source;
	}

	void InvalidatePointer(AbstractClass* ptr, bool removed);

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;

	static bool Attach(PhobosAttachEffectTypeClass* pType, TechnoClass* pTarget, HouseClass* pInvokerHouse, TechnoClass* pInvoker,
		AbstractClass* pSource, int durationOverride = 0, int delay = 0, int initialDelay = 0, int recreationDelay = -1);

	// Used for figuring out the correct values to use for a particular effect index when attaching them.
	static constexpr void SetValuesHelper(unsigned int index, std::vector<int>& durationOverrides, std::vector<int> const& delays, std::vector<int> const& initialDelays, std::vector<int> const& recreationDelays, int& durationOverride, int& delay, int& initialDelay, int& recreationDelay)
	{
		if (durationOverrides.size() > 0)
			durationOverride = durationOverrides[durationOverrides.size() > index ? index : durationOverrides.size() - 1];

		if (delays.size() > 0)
			delay = delays[delays.size() > index ? index : delays.size() - 1];

		if (initialDelays.size() > 0)
			initialDelay = initialDelays[initialDelays.size() > index ? index : initialDelays.size() - 1];

		if (recreationDelays.size() > 0)
			recreationDelay = recreationDelays[recreationDelays.size() > index ? index : recreationDelays.size() - 1];
	}

	static int Attach(std::vector<PhobosAttachEffectTypeClass*> const& types, TechnoClass* pTarget, HouseClass* pInvokerHouse, TechnoClass* pInvoker,
		AbstractClass* pSource, std::vector<int>& durationOverrides, std::vector<int> const* delays, std::vector<int> const* initialDelays, std::vector<int> const* recreationDelays);

	static int Detach(PhobosAttachEffectTypeClass* pType, TechnoClass* pTarget, int minCount = -1, int maxCount = -1);
	static int Detach(std::vector<PhobosAttachEffectTypeClass*> const& types, TechnoClass* pTarget, std::vector<int> const& minCounts, std::vector<int> const& maxCounts);
	static int Detach(std::vector<PhobosAttachEffectTypeClass*> const& types, TechnoClass* pTarget, int minCount = -1, int maxCount = -1, bool recalc = true);

	static int DetachByGroups(std::vector<std::string> const& groups, TechnoClass* pTarget, std::vector<int> const& minCounts, std::vector<int> const& maxCounts);
	static void TransferAttachedEffects(TechnoClass* pSource, TechnoClass* pTarget);

	~PhobosAttachEffectClass()
	{
		Animation.SetDestroyCondition(!Phobos::Otamaa::ExeTerminated);
	}

	void OnlineCheck();
	void CloakCheck();
	void AnimCheck();
	void CreateAnim();

	static PhobosAttachEffectClass* CreateAndAttach(PhobosAttachEffectTypeClass* pType, TechnoClass* pTarget, HelperedVector<PhobosAttachEffectClass>& targetAEs,
		HouseClass* pInvokerHouse, TechnoClass* pInvoker, AbstractClass* pSource, int durationOverride = 0, int delay = 0, int initialDelay = 0, int recreationDelay = -1);

	static int RemoveAllOfType(PhobosAttachEffectTypeClass* pType, TechnoClass* pTarget, int minCount, int maxCount);
	static int RemoveAllOfTypeAndSource(PhobosAttachEffectTypeClass* pType, TechnoClass* pTarget, AbstractClass* pSource, int minCount, int maxCount);

	template <typename T>
	bool Serialize(T& Stm);
public:

	int Duration { 0 };
	int DurationOverride { 0 };
	int Delay { 0 };
	int CurrentDelay { 0 };
	int InitialDelay { 0 };
	int RecreationDelay { -1 };
	PhobosAttachEffectTypeClass* Type { nullptr };
	TechnoClass* Techno { nullptr };
	HouseClass* InvokerHouse { nullptr };
	TechnoClass* Invoker { nullptr };
	AbstractClass* Source { nullptr };
	Handle<AnimClass*, UninitAnim> Animation { nullptr };
	bool IsAnimHidden { false };
	bool IsInTunnel { false };
	bool IsUnderTemporal { false };
	bool IsOnline { false };
	bool IsCloaked { false };
	bool HasInitialized { false };
	bool NeedsDurationRefresh { false };
	AnimTypeClass* SelectedAnim { nullptr };
	bool IsFirstCumulativeInstance { false };
};
