#pragma once

#include "NewSWType.h"
#include "SWStateMachine.h"

class SW_EMPulse : public NewSWType
{
public:
	virtual std::vector<const char*> GetTypeString() const override;

	virtual bool Activate(SuperClass* pThis, const CellStruct& Coords, bool IsPlayer) override;

	virtual void Initialize(SWTypeExtData* pData) override;

	virtual void LoadFromINI(SWTypeExtData* pData, CCINIClass* pINI) override;

	virtual bool IsLaunchSite(const SWTypeExtData* pData, BuildingClass* pBuilding) const override;
	virtual std::pair<double, double> GetLaunchSiteRange(const SWTypeExtData* pData, BuildingClass* pBuilding) const override;
};