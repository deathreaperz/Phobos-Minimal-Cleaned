#pragma once

#include <Memory.h>

#include <algorithm>
#include <type_traits>
#include <Helpers/Concepts.h>
#include <YRPPCore.h>

struct __declspec(align(4)) DummyDynamicVectorClass
{
	void* vftble;
	void** Vector_Item;
	int VectorMax;
	char IsValid;
	char IsAllocated;
	char VectorClassPad[2];
	int ActiveCount;
	int GrowthStep;
};

template<typename T>
struct TDummyDynamicVectorClass
{
	void* vftble;
	T* Vector_Item;
	int VectorMax;
	char IsValid;
	char IsAllocated;
	char VectorClassPad[2];
	int ActiveCount;
	int GrowthStep;
};

static_assert(sizeof(DummyDynamicVectorClass) == 0x18, "Invalid Size !");

enum class ArrayType : int
{
	Vector , DynamicVector , TypeList , Counter
};
//========================================================================
//=== VectorClass ========================================================
//========================================================================

/**
 * @brief Fixed-size array container with bounds checking
 * 
 * @tparam T Element type
 * 
 * @warning Iterator Invalidation:
 * - SetCapacity() invalidates ALL iterators
 * - Clear() invalidates ALL iterators
 * 
 * @note Thread Safety: Not thread-safe. External synchronization required.
 */
template <typename T>
class VectorClass
{
public:
	// the hidden element count messes with alignment. only applies to align 8, 16, ...
	static_assert(!needs_vector_delete<T>::value || (__alignof(T) <= 4), "Alignment of T needs to be less than or equal to 4.");

	static const ArrayType Type = ArrayType::Vector;

	VectorClass(noinit_t const&) { };
	COMPILETIMEEVAL VectorClass<T>() noexcept = default;

	explicit VectorClass<T>(int capacity, T* pMem = nullptr) :
		Items(nullptr),
		Capacity(capacity),
		IsInitialized(true),
		IsAllocated(false)
	{
		if (capacity != 0)
		{
			if (pMem) {
				this->Items = pMem;
			}
			else{
				GameAllocator<T> alloc {};
				this->Items = Memory::CreateArray<T>(alloc, static_cast<size_t>(capacity));
				this->IsAllocated = true;
			}
		}
	}

	VectorClass<T>(const VectorClass<T>& other)
	{
		if (other.Capacity > 0)
		{
			GameAllocator<T> alloc {};
			this->Items = Memory::CreateArray<T>(alloc, static_cast<size_t>(other.Capacity));
			this->IsAllocated = true;
			this->Capacity = other.Capacity;
			for (auto i = 0; i < other.Capacity; ++i) {
				this->Items[i] = other.Items[i];
			}
		}
	}

	VectorClass<T> (VectorClass<T>&& other) noexcept :
		Items(other.Items),
		Capacity(other.Capacity),
		IsInitialized(other.IsInitialized),
		IsAllocated(std::exchange(other.IsAllocated, false))
	{
	}

	virtual ~VectorClass<T>() noexcept
	{
		if (this->IsAllocated) {
			GameAllocator<T> alloc {};
			Memory::DeleteArray(alloc, this->Items, static_cast<size_t>(this->Capacity));
		}

		this->Items = nullptr;
		this->IsAllocated = false;
		this->Capacity = 0;
	}

	VectorClass<T>& operator = (const VectorClass<T>& other)
	{
		VectorClass<T>(other).Swap(*this);
		return *this;
	}

	VectorClass<T>& operator = (VectorClass<T>&& other) noexcept
	{
		VectorClass<T>(std::move(other)).Swap(*this);
		return *this;
	}

	virtual bool operator == (const VectorClass<T>& other) const
	{
		if (this->Capacity != other.Capacity)
		{
			return false;
		}

		for (auto i = 0; i < this->Capacity; ++i)
		{
			if (this->Items[i] == other.Items[i])
			{
				continue; // kapow! don't rewrite this to != unless you know why you're doing it
			}
			return false;
		}

		return true;
	}

	bool operator != (const VectorClass<T>& other) const
	{
		return !(*this == other);
	}

	virtual bool SetCapacity(int capacity, T* pMem = nullptr)
	{
		if (capacity > 0)
		{
			this->IsInitialized = false;

			bool bMustAllocate = (pMem == nullptr);
			if (!pMem)
			{
				GameAllocator<T> alloc {};
				pMem = Memory::CreateArray<T>(alloc, (size_t)capacity);
			}

			this->IsInitialized = true;

			if (!pMem)
			{
				return false;
			}

			if (this->Items)
			{
				auto n = (capacity < this->Capacity) ? capacity : this->Capacity;
				for (auto i = 0; i < n; ++i)
				{
					pMem[i] = std::move_if_noexcept(this->Items[i]);
				}

				if (this->IsAllocated)
				{
					GameAllocator<T> alloc {};
					Memory::DeleteArray(alloc, this->Items,(size_t)this->Capacity);
					this->Items = nullptr;
				}
			}

			this->IsAllocated = bMustAllocate;
			this->Items = pMem;
			this->Capacity = capacity;
		}
		else
		{
			Clear();
		}
		return true;
	}

	virtual void Clear()
	{
		if (this->IsAllocated) {
			GameAllocator<T> alloc {};
			Memory::DeleteArray(alloc, this->Items, static_cast<size_t>(this->Capacity));
		}

		this->IsAllocated = false;
		this->Items = nullptr;
		this->Capacity = 0;
	}

	virtual int FindItemIndex(const T& item) const
	{
		if (!this->IsInitialized) {
			return -1;
		}

		for (auto i = 0; i < this->Capacity; ++i) {
			if (this->Items[i] == item) {
				return i;
			}
		}

		return -1;
	}

	virtual int GetItemIndex(const T* pItem) const final
	{
		if (!this->IsInitialized || !pItem || !this->Items) {
			return -1;
		}

		// Check if pointer is within bounds
		if (pItem < this->Items || pItem >= this->Items + this->Capacity) {
			return -1;
		}

		return static_cast<int>(pItem - this->Items);
	}

	virtual T GetItem(int i) const final
	{
		if (i < 0 || i >= this->Capacity) {
			return T{}; // Return default-constructed value for out of bounds
		}
		return this->Items[i];
	}

	T& operator [] (int i)
	{
		// VectorClass uses Capacity for bounds checking (no Count member)
		if (i < 0 || i >= this->Capacity) {
			static thread_local T dummy{};  // Thread-safe dummy
			return dummy;
		}
		return this->Items[i];
	}

	const T& operator [] (int i) const
	{
		// VectorClass uses Capacity for bounds checking (no Count member)
		if (i < 0 || i >= this->Capacity) {
			static thread_local const T dummy{};  // Thread-safe dummy
			return dummy;
		}
		return this->Items[i];
	}

	bool Reserve(int capacity)
	{
		if (!this->IsInitialized)
		{
			return false;
		}

		if (this->Capacity >= capacity)
		{
			return true;
		}

		return SetCapacity(capacity, nullptr);
	}

	void Swap(VectorClass<T>& other) noexcept
	{
		using std::swap;
		swap(this->Items, other.Items);
		swap(this->Capacity, other.Capacity);
		swap(this->IsInitialized, other.IsInitialized);
		swap(this->IsAllocated, other.IsAllocated);
	}

	int Length() const { return Capacity; }

	T* Items { nullptr };
	int Capacity { 0 };
	bool IsInitialized { true };
	bool IsAllocated { false };

protected:
	bool VectorClassPad[2];
};

//========================================================================
//=== DynamicVectorClass =================================================
//========================================================================

/**
 * @brief Dynamic array container with automatic growth and bounds checking
 * 
 * @tparam T Element type
 * 
 * @warning Iterator Invalidation:
 * - AddItem(), AddHead(), InsertAt() may invalidate ALL iterators if reallocation occurs
 * - RemoveAt(), Remove() invalidate iterators at and after the removal point
 * - SetCapacity(), Clear() invalidate ALL iterators
 * - EmplaceItem() may invalidate ALL iterators if reallocation occurs
 * 
 * @note Thread Safety: Not thread-safe. External synchronization required.
 * @note Exception Safety: Strong exception safety guarantee for most operations
 */
//TODO : unify the naming !
template <typename T>
class DynamicVectorClass : public VectorClass<T>
{
public:
	static const ArrayType Type = ArrayType::DynamicVector;

#pragma region constructorandoperators
	COMPILETIMEEVAL DynamicVectorClass<T>() noexcept = default;

	explicit DynamicVectorClass<T>(int capacity, T* pMem = nullptr)
		: VectorClass<T>(capacity, pMem) , Count { 0 }, CapacityIncrement { 10 }
	{ }

	DynamicVectorClass<T>(const DynamicVectorClass<T>& other)
		: VectorClass<T>(), Count(0), CapacityIncrement(other.CapacityIncrement)
	{
		if (other.Capacity > 0)
		{
			GameAllocator<T> alloc {};
			// Exception-safe allocation and construction
			T* newItems = nullptr;
			try {
				newItems = Memory::CreateArray<T>(alloc, static_cast<size_t>(other.Capacity));
				
				// Copy elements with proper exception handling
				for (auto i = 0; i < other.Count; ++i) {
					newItems[i] = other.Items[i];
				}
				
				// Only assign after successful construction
				this->Items = newItems;
				this->IsAllocated = true;
				this->Capacity = other.Capacity;
				this->Count = other.Count;
			}
			catch (...) {
				// Clean up on exception
				if (newItems) {
					Memory::DeleteArray<T>(alloc, newItems, other.Capacity);
				}
				throw;
			}
		}
	}

	DynamicVectorClass<T>(DynamicVectorClass<T>&& other) noexcept
		: VectorClass<T>(std::move(other)), Count(other.Count),
		CapacityIncrement(other.CapacityIncrement)
	{
		other.Count = 0;
		other.CapacityIncrement = 10;
	}

	DynamicVectorClass<T>& operator = (const DynamicVectorClass<T>& other)
	{
		if (this != &other) {
			DynamicVectorClass<T>(other).Swap(*this);
		}
		return *this;
	}

	DynamicVectorClass<T>& operator = (DynamicVectorClass<T>&& other) noexcept
	{
		if (this != &other) {
			DynamicVectorClass<T>(std::move(other)).Swap(*this);
		}
		return *this;
	}

#pragma endregion

#pragma region virtuals

	virtual ~DynamicVectorClass<T>() = default;

	virtual bool SetCapacity(int capacity, T* pMem = nullptr) override
	{
		bool result = VectorClass<T>::SetCapacity(capacity, pMem);
		if (result && this->Capacity < this->Count) {
			this->Count = this->Capacity;
		}
		return result;
	}

	virtual void Clear() override
	{
		VectorClass<T>::Clear();
		this->Count = 0;
	}

	virtual int FindItemIndex(const T& item) const override final
	{
		if (!this->IsInitialized) {
			return -1;
		}

		T* iter = this->Find(item);
		return (iter && iter != this->end()) ? std::distance(this->begin(), iter) : -1;
	}

#pragma endregion

#pragma region iteratorpointer
	int FindItemIndexFromIterator(T* iter) const {
		if (!this->IsInitialized || !iter) {
			return -1;
		}

		if (iter == this->end())
			return -1;

		return std::distance(this->begin(), iter);
	}

	COMPILETIMEEVAL T* begin() const
	{
		return (this->Items && this->Count > 0) ? &this->Items[0] : nullptr;
	}

	COMPILETIMEEVAL T* end() const
	{
		return (this->Items && this->Count > 0) ? &this->Items[this->Count] : nullptr;
	}

	COMPILETIMEEVAL T* front() const {
		return (this->Items && this->Count > 0) ? &this->Items[0] : nullptr;
	}

	COMPILETIMEEVAL T* back() const {
		return (this->Items && this->Count > 0) ? &this->Items[this->Count - 1] : nullptr;
	}

	COMPILETIMEEVAL T* begin()
	{
		return (this->Items && this->Count > 0) ? &this->Items[0] : nullptr;
	}

	COMPILETIMEEVAL T* end()
	{
		return (this->Items && this->Count > 0) ? &this->Items[this->Count] : nullptr;
	}

	COMPILETIMEEVAL size_t size() const {
		return static_cast<size_t>(Count);
	}

	// Override VectorClass operator[] to use Count instead of Capacity for bounds checking
	T& operator [] (int i)
	{
		// DynamicVectorClass should use Count for bounds checking
		if (i < 0 || i >= this->Count) {
			static thread_local T dummy{};  // Thread-safe dummy
			return dummy;
		}
		return this->Items[i];
	}

	const T& operator [] (int i) const
	{
		// DynamicVectorClass should use Count for bounds checking
		if (i < 0 || i >= this->Count) {
			static thread_local const T dummy{};  // Thread-safe dummy
			return dummy;
		}
		return this->Items[i];
	}

#pragma endregion

#pragma region Funcs

	// this one doesnt destroy the memory , just reset the count
	// the vector memory may still contains dangling pointer if it vector of pointer
	COMPILETIMEEVAL void FORCEDINLINE Reset(int resetCount = 0) {
		this->Count = (resetCount >= 0 && resetCount <= this->Capacity) ? resetCount : 0;
	}

	COMPILETIMEEVAL bool FORCEDINLINE ValidIndex(int index) const {
		return index >= 0 && index < this->Count;
	}

	COMPILETIMEEVAL bool FORCEDINLINE ValidIndex(size_t index) const {
		return index < static_cast<size_t>(this->Count);
	}

	T GetItemOrDefault(size_t i) const
	{
		return this->GetItemOrDefault(i, T());
	}

	COMPILETIMEEVAL T GetItemOrDefault(size_t i, T def) const
	{
		if (!this->ValidIndex(i))
			return def;

		return this->Items[i];
	}

	bool Contains(const T& item) const
	{
		if (this->Count <= 0) {
			return false;
		}

		return this->Find(item) != this->end();
	}

	bool AddItem(T&& item)
	{
		if (!this->IsValidArray())
			return false;

		this->Items[this->Count++] = std::move(item);
		return true;
	}

	template< class... Args >
	T* EmplaceItem(Args&&... args) {
		if (!this->IsValidArray())
			return nullptr;
		
		T* location = &this->Items[this->Count++];
		new (location) T(std::forward<Args>(args)...);
		return location;
	}

	bool AddItem(const T& item)
	{
		if (!this->IsValidArray())
			return false;

		this->Items[this->Count++] = item;
		return true;
	}

	bool AddHead(const T& object)
	{
		if (!this->IsValidArray())
			return false;

		if (this->Count > 0)
		{
			// Use proper iterator arithmetic and safer move for non-trivial types
			if constexpr (std::is_trivially_copyable_v<T>) {
				T* dest = &this->Items[1];
				T* src = &this->Items[0];
				std::memmove(dest, src, this->Count * sizeof(T));
			} else {
				// Move elements backwards to avoid overlap issues
				for (int i = this->Count; i > 0; --i) {
					this->Items[i] = std::move(this->Items[i - 1]);
				}
			}
		}

		this->Items[0] = object;
		this->Count++;
		return true;
	}

	bool InsertAt(int index, const T& object)
	{
		if (index < 0 || index > this->Count) // Allow insertion at end
			return false;

		if (!this->IsValidArray())
			return false;

		if (index < this->Count) {
			// Move elements to make space
			if constexpr (std::is_trivially_copyable_v<T>) {
				T* dest = &this->Items[index + 1];
				T* src = &this->Items[index];
				size_t moveCount = this->Count - index;
				std::memmove(dest, src, moveCount * sizeof(T));
			} else {
				// Move elements backwards to avoid overlap issues
				for (int i = this->Count; i > index; --i) {
					this->Items[i] = std::move(this->Items[i - 1]);
				}
			}
		}

		this->Items[index] = object;
		++this->Count;
		return true;
	}

	T* UninitializedAdd() {
		return this->IsValidArray() ?
			&(this->Items[this->Count++]) : nullptr;
	}

	bool AddUnique(const T& item)
	{
		if (this->Count <= 0) {
			return this->AddItem(item);
		}
		
		// Early exit if item already exists
		if (this->Find(item) != this->end()) {
			return false;
		}
		
		return this->AddItem(item);
	}

	bool COMPILETIMEEVAL FORCEDINLINE Empty() const { return this->Count <= 0;}

	template<bool avoidmemcpy = false>
	bool RemoveAt(int index)
	{
		if (!this->ValidIndex(index)) {
			return false;
		}

		if constexpr (!avoidmemcpy) {
			if constexpr (std::is_trivially_copyable_v<T>) {
				// Use memmove for trivially copyable types
				if (index < this->Count - 1) {
					T* dest = &this->Items[index];
					T* src = &this->Items[index + 1];
					size_t moveCount = this->Count - index - 1;
					std::memmove(dest, src, moveCount * sizeof(T));
				}
			} else {
				// Use move assignment for non-trivial types
				for (int i = index; i < this->Count - 1; ++i) {
					this->Items[i] = std::move(this->Items[i + 1]);
				}
			}
		} else {
			// Always use move assignment
			for (int i = index; i < this->Count - 1; ++i) {
				this->Items[i] = std::move(this->Items[i + 1]);
			}
		}
		
		--this->Count;
		return true;
	}

	template<bool avoidmemcpy = false>
	bool Remove(const T& item)
	{
		int index = this->FindItemIndex(item);
		if (index == -1) {
			return false;
		}
		
		return this->RemoveAt<avoidmemcpy>(index);
	}

	template<typename Func>
	COMPILETIMEEVAL bool FORCEDINLINE remove_if(Func&& act)
	{
		if (!this->IsInitialized || !this->Items || this->Count <= 0) {
			return false;
		}
		
		T* beginPtr = this->begin();
		T* endPtr = this->end();
		
		if (!beginPtr || !endPtr) {
			return false;
		}
		
		T* newEnd = std::remove_if(beginPtr, endPtr, act);
		this->Count = static_cast<int>(std::distance(beginPtr, newEnd));
		return true;
	}

	bool COMPILETIMEEVAL FORCEDINLINE FindAndRemove(const T& item) {
		int index = this->FindItemIndex(item);
		if (index == -1) {
			return false;
		}
		return this->RemoveAt(index);
	}

	void Swap(DynamicVectorClass<T>& other) noexcept
	{
		VectorClass<T>::Swap(other);
		using std::swap;
		swap(this->Count, other.Count);
		swap(this->CapacityIncrement, other.CapacityIncrement);
	}

	COMPILETIMEEVAL FORCEDINLINE T* Find(const T& item) const {
		if (!this->Items || this->Count <= 0) {
			return this->end();
		}
		return std::find(this->begin(), this->end(), item);
	}
#pragma endregion

#pragma region WrappedSTD
	template <typename Func>
	COMPILETIMEEVAL auto FORCEDINLINE find_if(Func&& act) const {
	    for (auto i = this->begin(); i != this->end(); ++i) {
			if (act(*i)) {
				return i;
			}
   		}

		return this->end();
	}

	template <typename Func>
	COMPILETIMEEVAL auto FORCEDINLINE find_if(Func&& act) {
		for (auto i = this->begin(); i != this->end(); ++i) {
			if (act(*i)) {
				return i;
			}
   		}

		return this->end();
	}

	template <typename Func>
	COMPILETIMEEVAL void FORCEDINLINE for_each(Func&& act) const {
		for (auto i = this->begin(); i != this->end(); ++i) {
        	act(*i);
    	}
	}

	template <typename Func>
	COMPILETIMEEVAL void FORCEDINLINE for_each(Func&& act) {
		for (auto i = this->begin(); i != this->end(); ++i) {
        	act(*i);
    	}
	}

	template<typename func>
	COMPILETIMEEVAL bool FORCEDINLINE none_of(func&& fn) const {
		for (auto i = this->begin(); i != this->end(); ++i) {
       	 	if (fn(*i)) {
           	 	return false;
        	}
    	}

    	return true;
	}

	template<typename func>
	COMPILETIMEEVAL bool FORCEDINLINE none_of(func&& fn) {
		for (auto i = this->begin(); i != this->end(); ++i) {
       	 	if (fn(*i)) {
           	 	return false;
        	}
    	}

    	return true;
	}

	template<typename func>
	COMPILETIMEEVAL bool FORCEDINLINE any_of(func&& fn) const {
		for (auto i = this->begin(); i != this->end(); ++i) {
       		if (fn(*i)) {
            	return true;
			}
        }

		return false;
	}

	template<typename func>
	COMPILETIMEEVAL bool FORCEDINLINE any_of(func&& fn) {
		for (auto i = this->begin(); i != this->end(); ++i) {
       		if (fn(*i)) {
            	return true;
			}
        }

		return false;
	}

	/**
	 * @brief Validates internal consistency of the container
	 * @return true if container is in a valid state, false if corrupted
	 */
	bool ValidateIntegrity() const {
		// Check basic invariants
		if (this->Count < 0 || this->Count > this->Capacity) {
			return false;
		}
		
		if (this->Capacity < 0) {
			return false;
		}
		
		if (this->Capacity > 0 && !this->Items) {
			return false;
		}
		
		if (this->Capacity == 0 && this->Items) {
			return false;
		}
		
		if (this->CapacityIncrement < 0) {
			return false;
		}
		
		return true;
	}
#pragma endregion

	bool COMPILETIMEEVAL FORCEDINLINE IsValidArray()
	{
		if (this->Count >= this->Capacity)
		{
			if ((this->IsAllocated || !this->Capacity) && this->CapacityIncrement > 0)
			{
				return this->SetCapacity(this->Capacity + this->CapacityIncrement, nullptr);
			}
			else
			{
				return false;
			}
		}

		return true;
	}

public:
	int Count { 0 };
	int CapacityIncrement { 10 };
};

//========================================================================
//=== TypeList ===========================================================
//========================================================================

template <typename T>
class TypeList : public DynamicVectorClass<T>
{
public:
	COMPILETIMEEVAL TypeList<T>() noexcept = default;
	static const ArrayType Type = ArrayType::TypeList;
	using VectType = DynamicVectorClass<T>;

	explicit TypeList<T>(int capacity, T* pMem = nullptr)
		: VectType(capacity, pMem), unknown_18(0)
	{ }

	TypeList<T>(const TypeList<T>& other)
		: VectType(other), unknown_18(other.unknown_18)
	{ }

	TypeList<T>(TypeList<T>&& other) noexcept
		: VectType(std::move(other)), unknown_18(other.unknown_18)
	{ 
		other.unknown_18 = 0;
	}

	virtual ~TypeList<T>() = default;

	TypeList<T>& operator = (const TypeList<T>& other)
	{
		if (this != &other) {
			TypeList<T>(other).Swap(*this);
		}
		return *this;
	}

	TypeList<T>& operator = (TypeList<T>&& other) noexcept
	{
		if (this != &other) {
			TypeList<T>(std::move(other)).Swap(*this);
		}
		return *this;
	}

	void Swap(TypeList<T>& other) noexcept
	{
		VectType::Swap(other);
		using std::swap;
		swap(this->unknown_18, other.unknown_18);
	}

	int unknown_18 { 0 };
};

//========================================================================
//=== CounterClass =======================================================
//========================================================================

/**
 * @brief Counter array with automatic capacity expansion
 * 
 * Provides operations for incrementing/decrementing counters at specific indices.
 * Automatically expands capacity when accessing out-of-bounds indices.
 * 
 * @warning Iterator Invalidation:
 * - EnsureItem() may invalidate ALL iterators if capacity expansion occurs
 * - SetCapacity(), Clear() invalidate ALL iterators
 * 
 * @note Thread Safety: Not thread-safe. External synchronization required.
 * @note Binary Compatibility: Maintains exact same memory layout as original
 */
class CounterClass : public VectorClass<int>
{
public:
	COMPILETIMEEVAL CounterClass() noexcept = default;
	static const ArrayType Type = ArrayType::Counter;
	using VectType = VectorClass<int>;

	CounterClass(const CounterClass& other)
		: VectType(other), Total(other.Total)
	{ }

	CounterClass(CounterClass&& other) noexcept
		: VectType(std::move(other)), Total(other.Total)
	{ 
		other.Total = 0;
	}

	CounterClass& operator = (const CounterClass& other)
	{
		if (this != &other) {
			CounterClass(other).Swap(*this);
		}
		return *this;
	}

	CounterClass& operator = (CounterClass&& other) noexcept
	{
		if (this != &other) {
			CounterClass(std::move(other)).Swap(*this);
		}
		return *this;
	}

	virtual void Clear() override
	{
		if (this->Items) {
			for (int i = 0; i < this->Capacity; ++i)
			{
				this->Items[i] = 0;
			}
		}
		this->Total = 0;
	}

	virtual ~CounterClass() = default;

	int GetTotal() const
	{
		return this->Total;
	}

	bool EnsureItem(int index)
	{
		if (index < 0) {
			return false;
		}

		if (index < this->Capacity)
		{
			return true;
		}

		int count = this->Capacity;
		if (this->SetCapacity(index + 10, nullptr))
		{
			for (auto i = count; i < this->Capacity; ++i)
			{
				this->Items[i] = 0;
			}
			return true;
		}

		return false;
	}

	int operator[] (int index) const
	{
		return this->GetItemCount(index);
	}

	int GetItemCount(int index)
	{
		return this->EnsureItem(index) ? this->Items[index] : 0;
	}

	COMPILETIMEEVAL int GetItemCount(int index) const
	{
		return (index >= 0 && index < this->Capacity) ? this->Items[index] : 0;
	}

	int Increment(int index)
	{
		if (this->EnsureItem(index))
		{
			++this->Total;
			return ++this->Items[index];
		}
		return 0;
	}

	int Decrement(int index)
	{
		if (this->EnsureItem(index) && this->Items[index] > 0)
		{
			--this->Total;
			return --this->Items[index];
		}
		return 0;
	}

	void Swap(CounterClass& other) noexcept
	{
		VectorClass<int>::Swap(other);
		using std::swap;
		swap(this->Total, other.Total);
	}

	//HRESULT LoadFromStream(IStream* pStm) { JMP_THIS(0x49FBE0); }
	//HRESULT SaveFromStream(IStream* pStm) { JMP_THIS(0x49FB70); }
	
public:
	int Total { 0 };
};

template<typename T, const int size>
class ArrayHelper
{
public:
	// Constructor with proper initialization
	ArrayHelper() {
		initialize();
	}
	
	// Destructor to properly clean up non-trivial types
	~ArrayHelper() {
		if (_initialized && !std::is_trivially_destructible_v<T>) {
			T* ptr = reinterpret_cast<T*>(_dummy);
			for (int i = 0; i < size; ++i) {
				ptr[i].~T();
			}
		}
	}
	
	// Copy constructor
	ArrayHelper(const ArrayHelper& other) {
		initialize();
		if (other._initialized) {
			T* thisPtr = reinterpret_cast<T*>(_dummy);
			const T* otherPtr = reinterpret_cast<const T*>(other._dummy);
			for (int i = 0; i < size; ++i) {
				thisPtr[i] = otherPtr[i];
			}
		}
	}
	
	// Assignment operator
	ArrayHelper& operator=(const ArrayHelper& other) {
		if (this != &other) {
			if (!_initialized) initialize();
			if (other._initialized) {
				T* thisPtr = reinterpret_cast<T*>(_dummy);
				const T* otherPtr = reinterpret_cast<const T*>(other._dummy);
				for (int i = 0; i < size; ++i) {
					thisPtr[i] = otherPtr[i];
				}
			}
		}
		return *this;
	}

	// Use safer access methods instead of dangerous reinterpret_cast
	T* data() { 
		// Ensure proper initialization on first access
		if (!_initialized) {
			initialize();
		}
		return reinterpret_cast<T*>(_dummy); 
	}
	
	const T* data() const { 
		return reinterpret_cast<const T*>(_dummy); 
	}
	
	operator T* () { return data(); }
	operator const T* () const { return data(); }
	
	T& operator[](int index) { 
		// Add bounds checking with thread-safe dummy
		if (index < 0 || index >= size) {
			static thread_local T dummy{};
			return dummy;
		}
		return data()[index]; 
	}
	
	const T& operator[](int index) const { 
		// Add bounds checking with thread-safe dummy
		if (index < 0 || index >= size) {
			static thread_local const T dummy{};
			return dummy;
		}
		return data()[index]; 
	}

	T* begin() { return data(); }
	T* end() { return data() + size; }

	const T* begin() const { return data(); }
	const T* end() const { return data() + size; }

	constexpr int Size() const {
		return size;
	}

private:
	void initialize() {
		if (_initialized) return;
		
		// Initialize memory to zero for safety
		std::memset(_dummy, 0, sizeof(_dummy));
		
		// For non-trivial types, call default constructor
		if constexpr (!std::is_trivially_constructible_v<T>) {
			T* ptr = reinterpret_cast<T*>(_dummy);
			for (int i = 0; i < size; ++i) {
				new (ptr + i) T{};
			}
		}
		_initialized = true;
	}

	alignas(T) char _dummy[size * sizeof(T)];
	bool _initialized = false;
};

template<typename T, const int rows, const int cols>
class ArrayHelper2D
{
public:
	// Constructor with proper initialization
	ArrayHelper2D() {
		initialize();
	}
	
	// Destructor to properly clean up non-trivial types
	~ArrayHelper2D() {
		if (_initialized && !std::is_trivially_destructible_v<T>) {
			T* ptr = reinterpret_cast<T*>(_dummy);
			for (int i = 0; i < rows * cols; ++i) {
				ptr[i].~T();
			}
		}
	}
	
	// Copy constructor
	ArrayHelper2D(const ArrayHelper2D& other) {
		initialize();
		if (other._initialized) {
			T* thisPtr = reinterpret_cast<T*>(_dummy);
			const T* otherPtr = reinterpret_cast<const T*>(other._dummy);
			for (int i = 0; i < rows * cols; ++i) {
				thisPtr[i] = otherPtr[i];
			}
		}
	}
	
	// Assignment operator
	ArrayHelper2D& operator=(const ArrayHelper2D& other) {
		if (this != &other) {
			if (!_initialized) initialize();
			if (other._initialized) {
				T* thisPtr = reinterpret_cast<T*>(_dummy);
				const T* otherPtr = reinterpret_cast<const T*>(other._dummy);
				for (int i = 0; i < rows * cols; ++i) {
					thisPtr[i] = otherPtr[i];
				}
			}
		}
		return *this;
	}

	// Use safer access methods instead of dangerous reinterpret_cast
	T* data() { 
		// Ensure proper initialization on first access
		if (!_initialized) {
			initialize();
		}
		return reinterpret_cast<T*>(_dummy); 
	}
	
	const T* data() const { 
		return reinterpret_cast<const T*>(_dummy); 
	}
	
	// Safe 2D array access with bounds checking
	T* operator[](int row) { 
		if (row < 0 || row >= rows) {
			static thread_local T dummy_row[cols] = {};
			return dummy_row;
		}
		return &data()[row * cols]; 
	}
	
	const T* operator[](int row) const { 
		if (row < 0 || row >= rows) {
			static thread_local const T dummy_row[cols] = {};
			return dummy_row;
		}
		return &data()[row * cols]; 
	}
	
	// Direct access with row/col bounds checking
	T& at(int row, int col) {
		if (row < 0 || row >= rows || col < 0 || col >= cols) {
			static thread_local T dummy{};
			return dummy;
		}
		return data()[row * cols + col];
	}
	
	const T& at(int row, int col) const {
		if (row < 0 || row >= rows || col < 0 || col >= cols) {
			static thread_local const T dummy{};
			return dummy;
		}
		return data()[row * cols + col];
	}

	// Iterator support
	T* begin() { return data(); }
	T* end() { return data() + (rows * cols); }
	
	const T* begin() const { return data(); }
	const T* end() const { return data() + (rows * cols); }
	
	// Size information
	constexpr int Rows() const { return rows; }
	constexpr int Cols() const { return cols; }
	constexpr int Size() const { return rows * cols; }

private:
	void initialize() {
		if (_initialized) return;
		
		// Zero-initialize memory for safety
		std::memset(_dummy, 0, sizeof(_dummy));
		
		// For non-trivial types, call constructors using placement new
		if constexpr (!std::is_trivially_constructible_v<T>) {
			T* ptr = reinterpret_cast<T*>(_dummy);
			for (int i = 0; i < rows * cols; ++i) {
				new (ptr + i) T{};
			}
		}
		_initialized = true;
	}

	alignas(T) char _dummy[rows * cols * sizeof(T)];
	bool _initialized = false;
};
