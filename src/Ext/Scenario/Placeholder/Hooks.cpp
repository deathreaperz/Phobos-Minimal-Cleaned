#include "Body.h"

#include <Ext/TAction/Body.h>

#include <HouseClass.h>
#include <Helpers/Macro.h>
#include <ThemeClass.h>

// score options
// score music for single player missions
//DEFINE_HOOK(0x6C9231, ScoreDialog_Handle_ScoreThemeA, 0xA)
//{
//	GET(ScenarioClass*, pScen, EAX);
//	GET(int, ScoreA, EDI);
//	GET(int, ScoreB, ESI);
//
//	char bufferTitle[0x1F];
//	char bufferMessage[0x1F];
//	const auto& Title = ScenarioExt::Global()->ParTitle;
//	const auto& Message = ScenarioExt::Global()->ParMessage;
//
//	if (!Title.isset()) {
//		CRT::strcpy(bufferTitle, ScoreA > ScoreB ? pScen->OverParTitle : pScen->UnderParTitle);
//	} else {
//		CRT::strcpy(bufferTitle, Title->data());
//	}
//
//	if(!Message.isset()) {
//		CRT::strcpy(bufferMessage, ScoreA > ScoreB ? pScen->OverParMessage : pScen->UnderParMessage);
//	} else {
//		CRT::strcpy(bufferMessage, Message->data());
//	}
//	
//	R->ECX(bufferTitle);
//	R->ESI(bufferMessage);
//	return 0x6C924F;
//}
//
//DEFINE_HOOK(0x6C9357, ScoreDialog_Handle_ScoreThemeB, 0x5)
//{
//	const auto& Theme = ScenarioExt::Global()->ScoreCampaignTheme;
//	R->EAX(ThemeClass::Instance->FindIndex(Theme.isset() ? Theme->data() : GameStrings::SCORE()));
//
//	return 0x6C9366;
//}
//
//DEFINE_HOOK(0x5AE11A, SelectNextMissionScenario, 0x6)
//{
//	const auto& NextMission = ScenarioExt::Global()->NextMission;
//
//	if (NextMission.isset()){
//		R->EAX(NextMission->data());
//		return 0x5AE120;
//	}
//
//	return 0;
//}

//DEFINE_HOOK(0x6851AC, LoadGame_Initialize_IonStormClass, 0x5)
//{
//	auto swap_data = []()
//	{
//		std::swap(ScenarioExt::Global()->DefaultAmbientOriginal, ScenarioClass::Instance->AmbientOriginal);
//		std::swap(ScenarioExt::Global()->DefaultAmbientCurrent, ScenarioClass::Instance->AmbientCurrent);
//		std::swap(ScenarioExt::Global()->DefaultAmbientTarget, ScenarioClass::Instance->AmbientTarget);
//		std::swap(ScenarioExt::Global()->DefaultNormalLighting, ScenarioClass::Instance->NormalLighting);
//	};
//
//	swap_data();
//
//	MapClass::Instance->CellIteratorReset();
//	for (auto pCell = MapClass::Instance->CellIteratorNext(); pCell; pCell = MapClass::Instance->CellIteratorNext())
//	{
//		if (pCell->LightConvert)
//			CallDTOR<false>(pCell->LightConvert);
//
//		pCell->InitLightConvert();
//	}
//
//	swap_data();
//
//
//	for (auto& pLightConvert : *LightConvertClass::Array)
//		pLightConvert->UpdateColors(
//			ScenarioExt::Global()->CurrentTint_Tiles.Red * 10,
//			ScenarioExt::Global()->CurrentTint_Tiles.Green * 10,
//			ScenarioExt::Global()->CurrentTint_Tiles.Blue * 10,
//			false);
//
//	if (ScenarioExt::Global()->CurrentTint_Schemes != TintStruct { -1,-1,-1 })
//	{
//		for (auto& pScheme : *ColorScheme::Array)
//			pScheme->LightConvert->UpdateColors(
//				ScenarioExt::Global()->CurrentTint_Schemes.Red * 10,
//				ScenarioExt::Global()->CurrentTint_Schemes.Green * 10,
//				ScenarioExt::Global()->CurrentTint_Schemes.Blue * 10,
//				false);
//	}
//
//	if (ScenarioExt::Global()->CurrentTint_Hashes != TintStruct { -1,-1,-1 })
//	{
//		ScenarioClass::UpdateHashPalLighting(
//			ScenarioExt::Global()->CurrentTint_Hashes.Red * 10,
//			ScenarioExt::Global()->CurrentTint_Hashes.Green * 10,
//			ScenarioExt::Global()->CurrentTint_Hashes.Blue * 10,
//			false);
//	}
//
//	TActionExt::RecreateLightSources();
//	ScenarioClass::UpdateCellLighting();
//
//	HouseClass::CurrentPlayer->RecheckRadar = true;
//
//	return 0x6851B1;
//}

//DEFINE_HOOK(0x683A3A, ScenarioClass_Init_Bugfix, 0x6)
//{
//	GET(ScenarioClass*, pThis, EBP);
//	pThis->NumberStartingPoints = 0;
//	return 0;
//}