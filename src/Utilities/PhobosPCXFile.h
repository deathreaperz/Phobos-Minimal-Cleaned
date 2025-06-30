#pragma once

#include <PCX.h>
#include <Helpers/String.h>
#include <ExtraHeaders/MemoryPool.h>

#include <string>
#include <vector>
#include <future>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>

class BSurface;
class INIClass;
class PhobosStreamReader;
class PhobosStreamWriter;

// Simple PCX memory pool for better performance
class PCXMemoryPool
{
private:
	static std::unique_ptr<MemoryPool> Pool;
	static std::mutex PoolMutex;
	static bool Initialized;

public:
	static void Initialize();
	static void Shutdown();
	static BSurface* AllocateSurface(int width, int height);
	static void DeallocateSurface(BSurface* surface);
};

// Batch loader for multiple PCX files
class PCXBatchLoader
{
public:
	struct LoadRequest
	{
		std::string filename;
		std::promise<BSurface*> promise;
		bool completed = false;
	};

	static void LoadBatch(const std::vector<std::string>& filenames, 
		std::vector<std::future<BSurface*>>& futures);
	static void LoadBatchSync(const std::vector<std::string>& filenames,
		std::vector<BSurface*>& surfaces);

private:
	static void ProcessBatch(std::vector<std::shared_ptr<LoadRequest>>& requests);
};

// pcx filename storage with optional automatic loading
class PhobosPCXFile
{
	static COMPILETIMEEVAL const size_t Capacity = 0x20;
public:
	explicit PhobosPCXFile() : Surface(nullptr), filename(), AsyncFuture() { }

	PhobosPCXFile(const char* pFilename) : PhobosPCXFile()
	{
		*this = pFilename;
	}

	~PhobosPCXFile() = default;

	// Copy constructor - wait for async loading to complete before copying
	PhobosPCXFile(const PhobosPCXFile& other) : Surface(nullptr), filename(), AsyncFuture()
	{
		// Ensure the other object's async loading is complete
		const_cast<PhobosPCXFile&>(other).GetSurface();
		
		this->filename = other.filename;
		this->Surface = other.Surface;
		// AsyncFuture is intentionally not copied (it's not copyable anyway)
	}

	// Assignment operator - wait for async loading to complete before copying
	PhobosPCXFile& operator=(const PhobosPCXFile& other)
	{
		if (this != &other)
		{
			// Clean up our async future if it exists
			if (AsyncFuture.valid())
			{
				AsyncFuture.wait();
				AsyncFuture = std::future<BSurface*>{};
			}
			
			// Ensure the other object's async loading is complete
			const_cast<PhobosPCXFile&>(other).GetSurface();
			
			this->filename = other.filename;
			this->Surface = other.Surface;
			// AsyncFuture remains default-constructed (empty)
		}
		return *this;
	}

	PhobosPCXFile& operator=(const char* pFilename)
	{
		if (!pFilename || !*pFilename || !strlen(pFilename))
		{
			this->Clear();
			return *this;
		}

		this->filename = pFilename;
		auto& data = this->filename.data();
		_strlwr_s(data);

		// Add .pcx extension if missing
		std::string fullname = this->filename.data();
		if (fullname.find(".pcx") == std::string::npos)
			fullname += ".pcx";
		this->filename = fullname.c_str();

		// Try to get from cache first
		BSurface* pSource = PCX::Instance->GetSurface(this->filename);
		if (!pSource && PCX::Instance->LoadFile(this->filename))
			pSource = PCX::Instance->GetSurface(this->filename);

		this->Surface = pSource;
		return *this;
	}

	PhobosPCXFile& operator=(std::string& pFilename)
	{
		if (pFilename.empty() || !*pFilename.data())
		{
			this->Clear();
			return *this;
		}

		this->filename = pFilename.c_str();
		auto& data = this->filename.data();
		_strlwr_s(data);

		// Add .pcx extension if missing
		std::string fullname = this->filename.data();
		if (fullname.find(".pcx") == std::string::npos)
			fullname += ".pcx";
		this->filename = fullname.c_str();

		BSurface* pSource = PCX::Instance->GetSurface(this->filename);
		if (!pSource && PCX::Instance->ForceLoadFile(this->filename, 2, 0))
			pSource = PCX::Instance->GetSurface(this->filename);

		this->Surface = pSource;
		return *this;
	}

	const char* GetFilename() const
	{
		return this->filename.data();
	}

	BSurface* GetSurface() const
	{
		// If async loading is in progress, wait for it
		if (AsyncFuture.valid())
		{
			const_cast<PhobosPCXFile*>(this)->Surface = AsyncFuture.get();
			const_cast<PhobosPCXFile*>(this)->AsyncFuture = std::future<BSurface*>{};
		}
		return this->Surface;
	}

	bool Exists() const
	{
		return GetSurface() != nullptr;
	}

	// Async loading - starts loading in background
	void LoadAsync()
	{
		if (!filename.data() || Surface || AsyncFuture.valid())
			return;

		AsyncFuture = std::async(std::launch::async, [this]() -> BSurface* {
			BSurface* pSource = PCX::Instance->GetSurface(this->filename);
			if (!pSource && PCX::Instance->LoadFile(this->filename))
				pSource = PCX::Instance->GetSurface(this->filename);
			return pSource;
		});
	}

	// Check if async loading is complete without blocking
	bool IsAsyncComplete() const
	{
		return !AsyncFuture.valid() || 
			AsyncFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}

	// Validate PCX format (basic check)
	bool IsValidFormat() const
	{
		if (!Surface) return false;
		// Basic validation - check if surface has reasonable dimensions
		return Surface->Width > 0 && Surface->Height > 0 && 
			Surface->Width <= 2048 && Surface->Height <= 2048;
	}

	bool Read(INIClass* pINI, const char* pSection, const char* pKey, const char* pDefault = "");
	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;

	// Static batch operations
	static void LoadBatch(std::vector<PhobosPCXFile*>& files);
	static void LoadBatchAsync(std::vector<PhobosPCXFile*>& files);

private:

	void Clear()
	{
		this->Surface = nullptr;
		this->filename = nullptr;
		if (AsyncFuture.valid())
		{
			AsyncFuture.wait(); // Ensure cleanup
			AsyncFuture = std::future<BSurface*>{};
		}
	}

	BSurface* Surface { nullptr };
	FixedString<Capacity> filename;
	mutable std::future<BSurface*> AsyncFuture;
};

// Global PCX preloader for commonly used files
class PCXPreloader
{
private:
	static std::unordered_map<std::string, BSurface*> PreloadedSurfaces;
	static std::mutex PreloadMutex;

public:
	static void PreloadCommonFiles();
	static void PreloadFile(const char* filename);
	static BSurface* GetPreloaded(const char* filename);
	static void ClearPreloaded();
};