#pragma once

#include <TActionClass.h>

class HouseClass;
class ObjectClass;
class TriggerClass;
class HouseClass;
class CellStruct;
enum class PhobosTriggerAction : unsigned int
{
	SaveGame = 500,
	EditVariable = 501,
	GenerateRandomNumber = 502,
	PrintVariableValue = 503,
	BinaryOperation = 504,
	RunSuperWeaponAtLocation = 505,
	RunSuperWeaponAtWaypoint = 506,

	//#1144
	DumpVariables = 507,

	//#1266 , modified number
	PrintMessageRemainingTechnos = 508,

	//#844
	ToggleMCVRedeploy = 510,

	//#1164
	UndeployToWaypoint = 511,

	SetDropCrate = 600, // Only change this number if the PR is merged into develop!

	//ES
	SetTriggerTechnoVeterancy = 700,
	TransactMoneyFor = 701,
	SetAIMode = 703,
	DrawAnimWithin = 704,
	SetAllOwnedFootDestinationTo = 705,
	FlashTechnoFor = 713,
	UnInitTechno = 716,
	GameDeleteTechno = 717,
	LightningStormStrikeAtObject = 720,

	//#1549
	ResetHateValue = 606,

	//
	EditAngerNode = 607,
	ClearAngerNode = 608,
	SetForceEnemy = 609,

	CreateBannerGlobal = 800, // any banner w/ global variable
	CreateBannerLocal = 801, // any banner w/ local variable
	DeleteBanner = 802,

	//#620
	MessageForSpecifiedHouse = 9931,

	//#658
	RandomTriggerPut = 12000,
	RandomTriggerRemove = 12001,
	RandomTriggerEnable = 12002,
	ScoreCampaignText = 19000,
	ScoreCampaignTheme = 19001,
	SetNextMission = 19002,

	//DrawLaserBetweenWeaypoints = 9940,
	//AdjustLighting = 505,

	count
};

class TActionExt
{
public:
	/*
		static std::map<int, std::vector<TriggerClass*>> RandomTriggerPool;

		class ExtData final : public Extension<TActionClass>
		{
		public:
			static COMPILETIMEEVAL size_t Canary = 0x87154321;
			using base_type = TActionClass;

		public:

			//std::string Value1 { };
			//std::string Value2 { };
			//std::string Parm3 { };
			//std::string Parm4 { };
			//std::string Parm5 { };
			//std::string Parm6 { };

			ExtData(TActionClass* const OwnerObject) : Extension<base_type>(OwnerObject)
			{ }

			virtual ~ExtData() override = default;

			void LoadFromStream(PhobosStreamReader& Stm) { this->Serialize(Stm); }
			void SaveToStream(PhobosStreamWriter& Stm) { this->Serialize(Stm); }

		private:
			template <typename T>
			void Serialize(T& Stm);
		};

		class ExtContainer final : public Container<TActionExt::ExtData>
		{
		public:
			CONSTEXPR_NOCOPY_CLASS(TActionExt::ExtData, "TActionClass");
		public:

			void Clear() {
				RandomTriggerPool.clear();
			}

			void InvalidatePointer(AbstractClass* ptr, bool bRemoved) {
				for (auto& nMap : RandomTriggerPool) {
					AnnounceInvalidPointer(nMap.second, ptr);
				}
			}
		};

		static ExtContainer ExtMap;
	*/
	static void RecreateLightSources();
	static bool Occured(TActionClass* pThis, ActionArgs const& args, bool& bHandled);
	static bool RunSuperWeaponAt(TActionClass* pThis, int X, int Y);

#define ACTION_FUNC(name) \
	static bool name(TActionClass* pThis, HouseClass* pHouse, \
		ObjectClass* pObject, TriggerClass* pTrigger, CellStruct* plocation)

	ACTION_FUNC(PlayAudioAtRandomWP);
	ACTION_FUNC(SaveGame);
	ACTION_FUNC(EditVariable);
	ACTION_FUNC(GenerateRandomNumber);
	ACTION_FUNC(PrintVariableValue);
	ACTION_FUNC(BinaryOperation);
	//ACTION_FUNC(AdjustLighting);

	ACTION_FUNC(RunSuperWeaponAtLocation);
	ACTION_FUNC(RunSuperWeaponAtWaypoint);

	ACTION_FUNC(DrawLaserBetweenWaypoints);

	//ACTION_FUNC(RandomTriggerPut);
	//ACTION_FUNC(RandomTriggerEnable);
	//ACTION_FUNC(RandomTriggerRemove);

	ACTION_FUNC(ScoreCampaignText);
	ACTION_FUNC(ScoreCampaignTheme);
	ACTION_FUNC(SetNextMission);
	ACTION_FUNC(DumpVariables);
	ACTION_FUNC(ToggleMCVRedeploy);

	ACTION_FUNC(MessageForSpecifiedHouse);

	ACTION_FUNC(SetTriggerTechnoVeterancy);
	ACTION_FUNC(TransactMoneyFor);
	ACTION_FUNC(SetAIMode);
	ACTION_FUNC(DrawAnimWithin);
	ACTION_FUNC(SetAllOwnedFootDestinationTo);
	ACTION_FUNC(FlashTechnoFor);
	ACTION_FUNC(UnInitTechno);
	ACTION_FUNC(GameDeleteTechno);
	ACTION_FUNC(LightningStormStrikeAtObject);

	ACTION_FUNC(UndeployToWaypoint);

	ACTION_FUNC(PrintMessageRemainingTechnos);

	ACTION_FUNC(SetDropCrate);

	ACTION_FUNC(ResetHateValue);

	ACTION_FUNC(EditAngerNode);
	ACTION_FUNC(ClearAngerNode);
	ACTION_FUNC(SetForceEnemy);

	ACTION_FUNC(CreateBannerGlobal);
	ACTION_FUNC(CreateBannerLocal);
	ACTION_FUNC(DeleteBanner);

#undef ACTION_FUNC
};

class NOVTABLE FakeTActionClass : public TActionClass
{
public:

	bool _OperatorBracket(HouseClass* pTargetHouse, ObjectClass* pSourceObject, TriggerClass* pTrigger, CellStruct* plocation);
}; static_assert(sizeof(FakeTActionClass) == 0x94, "Invalid Size !");