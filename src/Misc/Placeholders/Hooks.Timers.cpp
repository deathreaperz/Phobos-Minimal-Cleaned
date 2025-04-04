#include <Utilities/Macro.h>
#include <GameOptionsClass.h>
#include <FPSCounter.h>
#include <SessionClass.h>
#include <Phobos.h>

namespace TimerValueTemp
{
	static int oldValue;
};

ASMJIT_PATCH(0x6D4B50, PrintTimerOnTactical_Start, 0x6)
{
	if (!Phobos::Config::RealTimeTimers)
		return 0;

	REF_STACK(int, value, STACK_OFFSET(0, 0x4));
	TimerValueTemp::oldValue = value;

	if (Phobos::Config::RealTimeTimers_Adaptive
		|| GameOptionsClass::Instance->GameSpeed == 0
		|| (Phobos::Misc::CustomGS && !SessionClass::IsMultiplayer())) // TODO change when custom game speed gets merged
	{
		const auto nCur = (double)FPSCounter::CurrentFrameRate;
		value = (int)((double)value / (MaxImpl(nCur, 1.0) / 15.0));
		return 0;
	}

	switch (GameOptionsClass::Instance->GameSpeed)
	{
	case 1:	// 60 FPS
		value = value / 4;
		break;
	case 2:	// 30 FPS
		value = value / 2;
		break;
	case 3:	// 20 FPS
		value = (value * 3) / 4;
		break;
	case 4:	// 15 FPS
		break;
	case 5:	// 12 FPS
		value = (value * 5) / 4;
		break;
	case 6:	// 10 FPS
		value = (value * 3) / 2;
		break;
	default:
		break;
	}

	return 0;
}

ASMJIT_PATCH(0x6D4C68, PrintTimerOnTactical_End, 0x8)
{
	if (!Phobos::Config::RealTimeTimers)
		return 0;

	REF_STACK(int, value, STACK_OFFSET(0x654, 0x4));
	value = TimerValueTemp::oldValue;
	return 0;
}