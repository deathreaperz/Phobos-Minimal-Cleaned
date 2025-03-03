#pragma once

#include <GenericList.h>
#include <ArrayClasses.h>
#include <Helpers/CompileTime.h>
#include <PKey.h>

// Named constants for magic numbers
constexpr DWORD MIX_CACHE_INSERT = 0x5B3FF0;
constexpr DWORD MIX_CACHE_FIND = 0x5B4020;
constexpr DWORD MIX_CACHE_INVALIDATE = 0x5B4050;
constexpr DWORD MIX_CACHE_DELETE = 0x5B4080;
constexpr DWORD MIX_CACHE_FILE = 0x5B4270;
constexpr DWORD MIX_CACHE_DESTROY = 0x5B4310;
constexpr DWORD MIX_FILE_BOOTSTRAP = 0x5301A0;
constexpr DWORD MIX_FILE_FREE = 0x5B4400;
constexpr DWORD MIX_FILE_FREE_FILENAME = 0x5B3E90;
constexpr DWORD MIX_FILE_CACHE = 0x5B43F0;
constexpr DWORD MIX_FILE_CACHE_FILENAME = 0x5B43E0;
constexpr DWORD MIX_FILE_OFFSET = 0x5B4430;
constexpr DWORD MIX_FILE_RETRIEVE = 0x5B40B0;
constexpr DWORD MIX_FILE_CONSTRUCTOR = 0x5B3C20;

struct MixHeaderData
{
	DWORD ID;
	DWORD Offset;
	DWORD Size;
};

class MixCache
{
public:
	static void __fastcall Insert(MixCache* pNew, MixCache* pOld)
	{
		JMP_STD(MIX_CACHE_INSERT);
	}

	static MixCache* __fastcall Find(unsigned int CRC, MixCache* pNode)
	{
		JMP_STD(MIX_CACHE_FIND);
	}

	static MixCache* __fastcall Invalidate(unsigned int crc, MixCache* pNode)
	{
		JMP_STD(MIX_CACHE_INVALIDATE);
	}

	static void __fastcall Delete(MixCache* pNode)
	{
		JMP_STD(MIX_CACHE_DELETE);
	}

	static void* __fastcall CacheFile(char* filename)
	{
		JMP_STD(MIX_CACHE_FILE);
	}

	static void __fastcall Destroy()
	{
		JMP_STD(MIX_CACHE_DESTROY);
	}

public:
	MixCache* Prev;
	MixCache* Next;
	unsigned int CRC;
	void* FilePtr;
};

static_assert(sizeof(MixCache) == 0x10);

class MemoryBuffer;
class MixFileClass : public Node<MixFileClass>
{
	struct GenericMixFiles
	{
		MixFileClass* RA2MD;
		MixFileClass* RA2;
		MixFileClass* LANGUAGE;
		MixFileClass* LANGMD;
		MixFileClass* THEATER_TEMPERAT;
		MixFileClass* THEATER_TEMPERATMD;
		MixFileClass* THEATER_TEM;
		MixFileClass* GENERIC;
		MixFileClass* GENERMD;
		MixFileClass* THEATER_ISOTEMP;
		MixFileClass* THEATER_ISOTEM;
		MixFileClass* ISOGEN;
		MixFileClass* ISOGENMD;
		MixFileClass* MOVIES02D;
		MixFileClass* UNKNOWN_1;
		MixFileClass* MAIN;
		MixFileClass* CONQMD;
		MixFileClass* CONQUER;
		MixFileClass* CAMEOMD;
		MixFileClass* CAMEO;
		MixFileClass* CACHEMD;
		MixFileClass* CACHE;
		MixFileClass* LOCALMD;
		MixFileClass* LOCAL;
		MixFileClass* NTRLMD;
		MixFileClass* NEUTRAL;
		MixFileClass* MAPSMD02D;
		MixFileClass* MAPS02D;
		MixFileClass* UNKNOWN_2;
		MixFileClass* UNKNOWN_3;
		MixFileClass* SIDEC02DMD;
		MixFileClass* SIDEC02D;
	};

public:
	static COMPILETIMEEVAL reference<List<MixFileClass*>, 0xABEFD8u> const MIXes {};

	static COMPILETIMEEVAL reference<DynamicVectorClass<MixFileClass*>, 0x884D90u> const Array {};
	static COMPILETIMEEVAL reference<DynamicVectorClass<MixFileClass*>, 0x884DC0u> const Array_Alt {};
	static COMPILETIMEEVAL reference<DynamicVectorClass<MixFileClass*>, 0x884DA8u> const Maps {};
	static COMPILETIMEEVAL reference<DynamicVectorClass<MixFileClass*>, 0x884DE0u> const Movies {};
	static COMPILETIMEEVAL reference<PKey*, 0x886980u> const Key {};
	static COMPILETIMEEVAL reference<MixFileClass, 0x884DD8u> const MULTIMD {};
	static COMPILETIMEEVAL reference<MixFileClass, 0x884DDCu> const MULTI {};

	static COMPILETIMEEVAL reference<GenericMixFiles, 0x884DF8u> const Generics {};

	static bool Bootstrap()
	{ JMP_THIS(MIX_FILE_BOOTSTRAP); }

	virtual ~MixFileClass() RX;

	void Free()
	{ JMP_THIS(MIX_FILE_FREE); }

	static bool __fastcall Free(const char* pFilename)
	{ JMP_STD(MIX_FILE_FREE_FILENAME); }

	bool Cache(const MemoryBuffer* buffer = nullptr)
	{ JMP_THIS(MIX_FILE_CACHE); }

	static bool __fastcall Cache(const char* pFilename, MemoryBuffer const* buffer = nullptr)
	{ JMP_STD(MIX_FILE_CACHE_FILENAME); }

	static bool __fastcall Offset(const char* pFilename, void** realptr = nullptr, MixFileClass** mixfile = nullptr, long* offset = nullptr, long* size = nullptr)
	{ JMP_STD(MIX_FILE_OFFSET); }

	static void* __fastcall Retrieve(const char* pFilename, bool bLoadAsSHP = false)
	{ JMP_STD(MIX_FILE_RETRIEVE); }

	static bool __fastcall Offset(const char* filename, void*& data,
		MixFileClass*& mixfile, int& offset, int& length)
	{
		JMP_STD(MIX_FILE_OFFSET);
	}

	MixFileClass(const char* pFileName)
		: Node<MixFileClass>()
	{
		PUSH_IMM(0x886980);
		PUSH_VAR32(pFileName);
		THISCALL(MIX_FILE_CONSTRUCTOR);
	}

public:

	const char* FileName;
	bool Blowfish;
	bool Encryption;
	int CountFiles;
	int FileSize;
	int FileStartOffset;
	MixHeaderData* Headers;
	int field_24;
};

static_assert(sizeof(MixFileClass) == 0x28);
