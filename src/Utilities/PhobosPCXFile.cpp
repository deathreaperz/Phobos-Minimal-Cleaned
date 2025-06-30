#include "PhobosPCXFile.h"

#include "Savegame.h"
#include "Debug.h"

#include <CCINIClass.h>
#include <SwizzleManagerClass.h>
#include <Surface.h>
#include <algorithm>
#include <thread>

// PCX Memory Pool Implementation
std::unique_ptr<MemoryPool> PCXMemoryPool::Pool;
std::mutex PCXMemoryPool::PoolMutex;
bool PCXMemoryPool::Initialized = false;

void PCXMemoryPool::Initialize()
{
	std::lock_guard<std::mutex> lock(PoolMutex);
	if (!Initialized && TheMemoryPoolFactory)
	{
		Pool = std::unique_ptr<MemoryPool>(
			TheMemoryPoolFactory->createMemoryPool("PCXSurfaces", sizeof(BSurface), 32, 16)
		);
		Initialized = true;
	}
}

void PCXMemoryPool::Shutdown()
{
	std::lock_guard<std::mutex> lock(PoolMutex);
	if (Initialized && TheMemoryPoolFactory && Pool)
	{
		TheMemoryPoolFactory->destroyMemoryPool(Pool.get());
		Pool.reset();
		Initialized = false;
	}
}

BSurface* PCXMemoryPool::AllocateSurface(int width, int height)
{
	if (!Initialized) Initialize();
	
	// For now, use standard allocation since BSurface creation is complex
	// In a real implementation, you'd want to create a custom surface allocator
	return GameCreate<BSurface>(width, height, 2, nullptr);
}

void PCXMemoryPool::DeallocateSurface(BSurface* surface)
{
	// Standard deallocation for now
	GameDelete(surface);
}

// Batch Loader Implementation
void PCXBatchLoader::LoadBatch(const std::vector<std::string>& filenames, 
	std::vector<std::future<BSurface*>>& futures)
{
	futures.clear();
	futures.reserve(filenames.size());

	for (const auto& filename : filenames)
	{
		futures.emplace_back(std::async(std::launch::async, [filename]() -> BSurface* {
			BSurface* pSource = PCX::Instance->GetSurface(filename.c_str());
			if (!pSource && PCX::Instance->LoadFile(filename.c_str()))
				pSource = PCX::Instance->GetSurface(filename.c_str());
			return pSource;
		}));
	}
}

void PCXBatchLoader::LoadBatchSync(const std::vector<std::string>& filenames,
	std::vector<BSurface*>& surfaces)
{
	surfaces.clear();
	surfaces.reserve(filenames.size());

	// Process in chunks to avoid overwhelming the system
	const size_t chunkSize = 8;
	for (size_t i = 0; i < filenames.size(); i += chunkSize)
	{
		size_t end = std::min(i + chunkSize, filenames.size());
		std::vector<std::future<BSurface*>> chunk_futures;
		
		// Load chunk asynchronously
		for (size_t j = i; j < end; ++j)
		{
			const auto& filename = filenames[j];
			chunk_futures.emplace_back(std::async(std::launch::async, [filename]() -> BSurface* {
				BSurface* pSource = PCX::Instance->GetSurface(filename.c_str());
				if (!pSource && PCX::Instance->LoadFile(filename.c_str()))
					pSource = PCX::Instance->GetSurface(filename.c_str());
				return pSource;
			}));
		}

		// Collect results
		for (auto& future : chunk_futures)
		{
			surfaces.push_back(future.get());
		}
	}
}

// PhobosPCXFile Implementation
bool PhobosPCXFile::Read(INIClass* pINI, const char* pSection, const char* pKey, const char* pDefault)
{
	char buffer[Capacity];
	if (pINI->ReadString(pSection, pKey, pDefault, buffer) > 0)
	{
		std::string cachedWithExt = _strlwr(buffer);

		if (cachedWithExt.find(".pcx") == std::string::npos)
			cachedWithExt += ".pcx";

		*this = cachedWithExt;

		if (this->filename && !this->Surface)
		{
			// Check preloaded cache first
			BSurface* preloaded = PCXPreloader::GetPreloaded(this->filename);
			if (preloaded)
			{
				this->Surface = preloaded;
			}
			else
			{
				Debug::INIParseFailed(pSection, pKey, this->filename, "PCX file not found.");
			}
		}
	}

	return buffer[0] != 0;
}

bool PhobosPCXFile::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->Clear();
	void* oldPtr;
	const auto ret = Stm.Load(oldPtr) && Stm.Load(this->filename);

	if (!ret)
		return false;

	if (oldPtr && this->filename)
	{
		// Check preloaded first
		BSurface* pSource = PCXPreloader::GetPreloaded(this->filename);
		if (!pSource)
		{
			pSource = PCX::Instance->GetSurface(this->filename);
			if (!pSource && PCX::Instance->LoadFile(this->filename))
				pSource = PCX::Instance->GetSurface(this->filename, nullptr);
		}

		this->Surface = pSource;

		if (!this->Surface)
		{
			Debug::LogInfo("PCX file[{}] not found.", this->filename.data());
		}

		SwizzleManagerClass::Instance().Here_I_Am((long)oldPtr, this->Surface);
	}

	return true;
}

bool PhobosPCXFile::Save(PhobosStreamWriter& Stm) const
{
	// Wait for async loading to complete before saving
	GetSurface();
	Stm.Save(this->Surface);
	Stm.Save(this->filename);
	return true;
}

void PhobosPCXFile::LoadBatch(std::vector<PhobosPCXFile*>& files)
{
	if (files.empty()) return;

	// Collect filenames that need loading
	std::vector<std::string> filenames;
	std::vector<size_t> indices;
	
	for (size_t i = 0; i < files.size(); ++i)
	{
		auto* file = files[i];
		if (file && file->filename.data() && !file->Surface)
		{
			// Check if already preloaded
			BSurface* preloaded = PCXPreloader::GetPreloaded(file->filename);
			if (preloaded)
			{
				file->Surface = preloaded;
			}
			else
			{
				filenames.push_back(file->filename.data());
				indices.push_back(i);
			}
		}
	}

	if (filenames.empty()) return;

	// Load in batch
	std::vector<BSurface*> surfaces;
	PCXBatchLoader::LoadBatchSync(filenames, surfaces);

	// Assign results
	for (size_t i = 0; i < surfaces.size() && i < indices.size(); ++i)
	{
		files[indices[i]]->Surface = surfaces[i];
	}
}

void PhobosPCXFile::LoadBatchAsync(std::vector<PhobosPCXFile*>& files)
{
	if (files.empty()) return;

	// Start async loading for files that need it
	for (auto* file : files)
	{
		if (file && file->filename.data() && !file->Surface && !file->AsyncFuture.valid())
		{
			// Check preloaded first
			BSurface* preloaded = PCXPreloader::GetPreloaded(file->filename);
			if (preloaded)
			{
				file->Surface = preloaded;
			}
			else
			{
				file->LoadAsync();
			}
		}
	}
}

// PCX Preloader Implementation
std::unordered_map<std::string, BSurface*> PCXPreloader::PreloadedSurfaces;
std::mutex PCXPreloader::PreloadMutex;

void PCXPreloader::PreloadCommonFiles()
{
	// Common UI elements that are frequently used
	std::vector<std::string> commonFiles = {
		"sidebar.pcx",
		"tabs.pcx",
		"dialog.pcx",
		"cameos.pcx"
	};

	std::lock_guard<std::mutex> lock(PreloadMutex);
	
	for (const auto& filename : commonFiles)
	{
		if (PreloadedSurfaces.find(filename) == PreloadedSurfaces.end())
		{
			BSurface* surface = PCX::Instance->GetSurface(filename.c_str());
			if (!surface && PCX::Instance->LoadFile(filename.c_str()))
				surface = PCX::Instance->GetSurface(filename.c_str());
			
			if (surface)
			{
				PreloadedSurfaces[filename] = surface;
			}
		}
	}
}

void PCXPreloader::PreloadFile(const char* filename)
{
	if (!filename || !*filename) return;

	std::string key = filename;
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	
	if (key.find(".pcx") == std::string::npos)
		key += ".pcx";

	std::lock_guard<std::mutex> lock(PreloadMutex);
	
	if (PreloadedSurfaces.find(key) == PreloadedSurfaces.end())
	{
		BSurface* surface = PCX::Instance->GetSurface(key.c_str());
		if (!surface && PCX::Instance->LoadFile(key.c_str()))
			surface = PCX::Instance->GetSurface(key.c_str());
		
		if (surface)
		{
			PreloadedSurfaces[key] = surface;
		}
	}
}

BSurface* PCXPreloader::GetPreloaded(const char* filename)
{
	if (!filename || !*filename) return nullptr;

	std::string key = filename;
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);

	std::lock_guard<std::mutex> lock(PreloadMutex);
	auto it = PreloadedSurfaces.find(key);
	return (it != PreloadedSurfaces.end()) ? it->second : nullptr;
}

void PCXPreloader::ClearPreloaded()
{
	std::lock_guard<std::mutex> lock(PreloadMutex);
	PreloadedSurfaces.clear();
}