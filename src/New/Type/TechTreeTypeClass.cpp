#include "TechTreeTypeClass.h"

#include <HouseClass.h>

#include <Utilities/TemplateDef.h>
#include "Utilities/Debug.h"

Enumerable<TechTreeTypeClass>::container_t Enumerable<TechTreeTypeClass>::Array;

size_t TechTreeTypeClass::CountSideOwnedBuildings(HouseClass* pHouse, BuildType buildType) const
{
	size_t count = 0;
	if (auto pBuild = this->GetBuildList(buildType))
	{
		for (const auto pBuilding : *pBuild)
		{
			count += pHouse->ActiveBuildingTypes.GetItemCount(pBuilding->ArrayIndex);
		}
	}

	return count;
}

bool TechTreeTypeClass::IsCompleted(HouseClass* pHouse, std::function<bool(BuildingTypeClass*)> const& filter) const
{
	for (BuildType i = BuildType::BuildPower; i < BuildType::BuildOther; i = BuildType((int)i + 1))
	{
		if (!GetBuildable(i, filter).empty() && CountSideOwnedBuildings(pHouse, i) < 1)
		{
			return false;
		}
	}

	for (const auto& [type, count] : BuildOtherCountMap)
	{
		if (filter(type) && CountSideOwnedBuildings(pHouse, BuildType::BuildOther) < count)
		{
			return false;
		}
	}

	return true;
}

std::vector<BuildingTypeClass*> TechTreeTypeClass::GetBuildable(BuildType buildType, std::function<bool(BuildingTypeClass*)> const& filter) const
{
	std::vector<BuildingTypeClass*> filtered;
	if (auto pBuild = this->GetBuildList(buildType))
	{
		std::ranges::copy_if(*pBuild, std::back_inserter(filtered), filter);
	}

	return filtered;
}

BuildingTypeClass* TechTreeTypeClass::GetRandomBuildable(BuildType buildType, std::function<bool(BuildingTypeClass*)> const& filter) const
{
	const std::vector<BuildingTypeClass*> buildable = GetBuildable(buildType, filter);
	if (!buildable.empty())
	{
		return buildable[ScenarioClass::Instance->Random.RandomRanged(0, buildable.size() - 1)];
	}

	return nullptr;
}

template<>
const char* Enumerable<TechTreeTypeClass>::GetMainSection()
{
	return "TechTreeTypes";
}

void TechTreeTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	INI_EX exINI(pINI);

	this->SideIndex.Read(exINI, section, "SideIndex");
	this->ConstructionYard.Read(exINI, section, "ConstructionYard");
	this->BuildPower.Read(exINI, section, "BuildPower");
	this->BuildRefinery.Read(exINI, section, "BuildRefinery");
	this->BuildBarracks.Read(exINI, section, "BuildBarracks");
	this->BuildWeapons.Read(exINI, section, "BuildWeapons");
	this->BuildRadar.Read(exINI, section, "BuildRadar");
	this->BuildHelipad.Read(exINI, section, "BuildHelipad");
	this->BuildNavalYard.Read(exINI, section, "BuildNavalYard");
	this->BuildTech.Read(exINI, section, "BuildTech");
	this->BuildAdvancedPower.Read(exINI, section, "BuildAdvancedPower");
	this->BuildDefense.Read(exINI, section, "BuildDefense");
	this->BuildOther.Read(exINI, section, "BuildOther");
	this->BuildOtherCounts.Read(exINI, section, "BuildOtherCounts");

	for (size_t i = 0; i < BuildOther.size(); i++)
	{
		if (i < BuildOtherCounts.size())
		{
			BuildOtherCountMap[BuildOther[i]] = BuildOtherCounts[i];
		}
		else
		{
			Debug::Log("TechTreeTypeClass::LoadFromINI: BuildOtherCounts is missing count for %s, setting to 0.", BuildOther[i]->Name);
			BuildOtherCountMap[BuildOther[i]] = 0;
		}
	}
}

template <typename T>
void TechTreeTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(SideIndex)
		.Process(ConstructionYard)
		.Process(BuildPower)
		.Process(BuildRefinery)
		.Process(BuildBarracks)
		.Process(BuildRadar)
		.Process(BuildHelipad)
		.Process(BuildNavalYard)
		.Process(BuildTech)
		.Process(BuildAdvancedPower)
		.Process(BuildDefense)
		.Process(BuildOther)
		.Process(BuildOtherCounts)
		.Process(BuildOtherCountMap)
		;
}

void TechTreeTypeClass::LoadFromStream(PhobosStreamReader& Stm)
{
	this->Serialize(Stm);
}

void TechTreeTypeClass::SaveToStream(PhobosStreamWriter& Stm)
{
	this->Serialize(Stm);
}