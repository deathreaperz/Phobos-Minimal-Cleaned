#include "Syringe.h"

#ifdef aaa
std::chrono::steady_clock::time_point DebugData::StartTime;

void DebugData::Start(DWORD origin, const char* funcName, int size)
{
	//GameDebugLog::Log("[Hook] 0x%X [%s - %d]\n",origin, funcName , size);
	DebugData::StartTime = std::chrono::high_resolution_clock::now();
}

void DebugData::End(DWORD origin, const char* funcName, int size)
{
	auto stop = std::chrono::high_resolution_clock::now();
	auto ret = std::chrono::duration_cast<std::chrono::microseconds>(StartTime - stop);
	const auto nRes = abs(ret.count());

	if(nRes > 0)
		GameDebugLog::Log("[Hook] 0x%X [%s - %d] end Taking %d ms\n", origin, funcName, size, nRes);
	//GameDebugLog::Log("[Hook] 0x%X [%s - %d] end\n", R->Origin(), #funcname, size);
}

void DebugData::StartO(DWORD origin, const char* funcName, int size)
{
	//GameDebugLog::Log("[Override Hook] 0x%X [%s - %d]\n",R->Origin(), #funcname , size);
	DebugData::StartTime = std::chrono::high_resolution_clock::now();
}

void DebugData::EndO(DWORD origin, const char* funcName, int size)
{
	auto stop = std::chrono::high_resolution_clock::now();
	auto ret = std::chrono::duration_cast<std::chrono::microseconds>(StartTime - stop);
	const auto nRes = abs(ret.count());

	if (nRes > 0)
		GameDebugLog::Log("[Override Hook] 0x%X [%s - %d] end %d ms\n", origin, funcName, size, nRes);
	//GameDebugLog::Log("[Override Hook] 0x%X [%s - %d] end\n", R->Origin(), #funcname, size);
}
#endif