#include "Trajectories/PhobosTrajectory.h"

//#ifdef ENABLE_TRAJ_HOOKS
DEFINE_HOOK(0x4666F7, BulletClass_AI_Trajectories, 0x6)
{
	enum { Detonate = 0x467E53 };

	GET(BulletClass*, pThis, EBP);

	if (!pThis->SpawnNextAnim)
	{
		if (auto const pExt = BulletExt::ExtMap.Find(pThis))
			if (auto pTraj = pExt->Trajectory)
				return pTraj->OnAI(pThis) ? Detonate : 0x0;
	}

	return 0;
}

DEFINE_HOOK(0x467E53, BulletClass_AI_PreDetonation_Trajectories, 0x6)
{
	GET(BulletClass*, pThis, EBP);

	if (auto const pExt = BulletExt::ExtMap.Find(pThis))
		if (auto pTraj = pExt->Trajectory)
			pTraj->OnAIPreDetonate(pThis);

	return 0;
}

DEFINE_HOOK(0x46745C, BulletClass_AI_Position_Trajectories, 0x7)
{
	GET(BulletClass*, pThis, EBP);
	LEA_STACK(VelocityClass*, pSpeed, STACK_OFFS(0x1AC, 0x11C));
	LEA_STACK(VelocityClass*, pPosition, STACK_OFFS(0x1AC, 0x144));

	if (auto const pExt = BulletExt::ExtMap.Find(pThis))
		if (auto pTraj = pExt->Trajectory)
			pTraj->OnAIVelocity(pThis, pSpeed, pPosition);

	return 0;
}

DEFINE_HOOK(0x4677D3, BulletClass_AI_TargetCoordCheck_Trajectories, 0x5)
{
	enum { SkipCheck = 0x4678F8, ContinueAfterCheck = 0x467879, Detonate = 0x467E53 };

	GET(BulletClass*, pThis, EBP);
	REF_STACK(CoordStruct, coords, STACK_OFFS(0x1A8, 0x184));

	if (auto const pExt = BulletExt::ExtMap.Find(pThis))
	{
		if (auto pTraj = pExt->Trajectory)
		{
			switch (pTraj->OnAITargetCoordCheck(pThis, coords))
			{
			case TrajectoryCheckReturnType::SkipGameCheck:
				return SkipCheck;
			case TrajectoryCheckReturnType::SatisfyGameCheck:
				return ContinueAfterCheck;
			case TrajectoryCheckReturnType::Detonate:
				return Detonate;
			default:
				break;
			}
		}
	}


	return 0;
}

DEFINE_HOOK(0x467927, BulletClass_AI_TechnoCheck_Trajectories, 0x5)
{
	enum { SkipCheck = 0x467A2B, ContinueAfterCheck = 0x4679EB, Detonate = 0x467E53 };

	GET(BulletClass*, pThis, EBP);
	GET(TechnoClass*, pTechno, ESI);

	if (auto const pExt = BulletExt::ExtMap.Find(pThis))
	{
		if (auto pTraj = pExt->Trajectory)
		{
			switch (pTraj->OnAITechnoCheck(pThis, pTechno))
			{
			case TrajectoryCheckReturnType::SkipGameCheck:
				return SkipCheck;
			case TrajectoryCheckReturnType::SatisfyGameCheck:
				return ContinueAfterCheck;
			case TrajectoryCheckReturnType::Detonate:
				return Detonate;
			default:
				break;
			}
		}
	}

	return 0;
}
DEFINE_HOOK(0x468B72, BulletClass_Unlimbo_Trajectories, 0x5)
{
	GET(BulletClass*, pThis, EBX);
	GET_STACK(CoordStruct*, pCoord, STACK_OFFS(0x54, -0x4));
	GET_STACK(VelocityClass*, pVelocity, STACK_OFFS(0x54, -0x8));

	auto const pData = BulletTypeExt::ExtMap.Find(pThis->Type);
	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (pData && pExt)
		if (auto pType = pData->TrajectoryType)
			pExt->Trajectory = PhobosTrajectory::CreateInstance(pType, pThis, pCoord, pVelocity);

	return 0;
}

//#endif