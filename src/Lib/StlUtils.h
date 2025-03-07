/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


 // Description : Various convenience utility functions for STL and alike
 //               Used in Animation subsystem, and in some tools
#pragma once
#include <mutex>

#include <unordered_map>
#include <unordered_set>

#if (defined(LINUX) || defined(APPLE))
#include "platform.h"
#endif

#define STATIC_ASSERT(condition, errMessage) static_assert(condition, errMessage)

#include <map>
#include <set>
#include <algorithm>
#include <deque>
#include <vector>

#undef std__hash_map
#define std__hash_map std::unordered_map
#define std__unordered_set std::unordered_set
#define std__hash_multimap std::unordered_multimap
#define std__hash std::hash
#define std__unordered_map std::unordered_map

template <class T, class Destructor>
class StaticInstance;

// auto-cleaner: upon destruction, calls the clear() method
template <class T>
class CAutoClear
{
public:
	CAutoClear(T* p)
		: m_p(p) { }
	~CAutoClear() { m_p->clear(); }
protected:
	T* m_p;
};


template <class Container>
unsigned sizeofArray(const Container& arr)
{
	return (unsigned)(sizeof(typename Container::value_type) * arr.size());
}

template <class Container>
unsigned sizeofVector(const Container& arr)
{
	return (unsigned)(sizeof(typename Container::value_type) * arr.capacity());
}

template <class Container>
unsigned sizeofArray(const Container& arr, unsigned nSize)
{
	return arr.empty() ? 0u : (unsigned)(sizeof(typename Container::value_type) * nSize);
}

template <class Container>
unsigned capacityofArray(const Container& arr)
{
	return (unsigned)(arr.capacity() * sizeof(arr[0]));
}

template <class T>
unsigned countElements(const std::vector<T>& arrT, const T& x)
{
	unsigned nSum = 0;
	for (typename std::vector<T>::const_iterator iter = arrT.begin(); iter != arrT.end(); ++iter)
	{
		if (x == *iter)
		{
			++nSum;
		}
	}
	return nSum;
}

// [Timur]
/** Contain extensions for STL library.
*/
namespace stl
{
	//////////////////////////////////////////////////////////////////////////
	//! Searches the given entry in the map by key, and if there is none, returns the default value
	//////////////////////////////////////////////////////////////////////////
	template <typename Map>
	OPTIONALINLINE typename Map::mapped_type find_in_map(const Map& mapKeyToValue, const typename Map::key_type& key, typename Map::mapped_type valueDefault)
	{
		typename Map::const_iterator it = mapKeyToValue.find(key);
		if (it == mapKeyToValue.end())
		{
			return valueDefault;
		}
		else
		{
			return it->second;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//! Fills vector with contents of map.
	//////////////////////////////////////////////////////////////////////////
	template <class Map, class Vector>
	OPTIONALINLINE void map_to_vector(const Map& theMap, Vector& aarray)
	{
		aarray.resize(0);
		aarray.reserve(theMap.size());
		for (typename Map::const_iterator it = theMap.begin(); it != theMap.end(); ++it)
		{
			aarray.push_back(it->second);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find and erase element from container.
	// @return true if item was find and erased, false if item not found.
	//////////////////////////////////////////////////////////////////////////
	template <class Container, class Value>
	OPTIONALINLINE bool find_and_erase(Container& container, const Value& value)
	{
		typename Container::iterator it = std::find(container.begin(), container.end(), value);
		if (it != container.end())
		{
			container.erase(it);
			return true;
		}
		return false;
	}

	template <typename K, typename P, typename A>
	OPTIONALINLINE bool find_and_erase(std::set<K, P, A>& container, const K& value)
	{
		return container.erase(value) > 0;
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find and erase element from container.
	// @return true if item was find and erased, false if item not found.
	//////////////////////////////////////////////////////////////////////////
	template <class CONTAINER, class PREDICATE>
	OPTIONALINLINE bool find_and_erase_if(CONTAINER& container, const PREDICATE& predicate)
	{
		typename CONTAINER::iterator end = container.end(), i = std::find_if(container.begin(), end, predicate);

		if (i != end)
		{
			container.erase(i);

			return true;
		}

		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find and erase all elements matching value from container.
	// Assume that this will invalidate any exiting iterators.
	// Commonly used for removing NULL pointers from collections.
	//////////////////////////////////////////////////////////////////////////
	template <class Container>
	OPTIONALINLINE void find_and_erase_all(Container& container, const typename Container::value_type& value)
	{
		// Shuffles all elements != value to the front and returns the start of the removed elements.
		typename Container::iterator endIter(container.end());
		typename Container::iterator newEndIter(std::remove(container.begin(), endIter, value));

		// Delete the removed range at the back of the container (low-cost for vector).
		container.erase(newEndIter, endIter);
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find and erase element from map.
	// @return true if item was find and erased, false if item not found.
	//////////////////////////////////////////////////////////////////////////
	template <class Container, class Key>
	OPTIONALINLINE bool member_find_and_erase(Container& container, const Key& key)
	{
		typename Container::iterator it = container.find(key);
		if (it != container.end())
		{
			container.erase(it);
			return true;
		}
		return false;
	}


	//////////////////////////////////////////////////////////////////////////
	//! Push back to container unique element.
	// @return true if item added, false overwise.
	template <class Container, class Value>
	OPTIONALINLINE bool push_back_unique(Container& container, const Value& value)
	{
		if (std::find(container.begin(), container.end(), value) == container.end())
		{
			container.push_back(value);
			return true;
		}
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find element in container.
	// @return true if item found.
	template <class Container, class Value>
	OPTIONALINLINE bool find(Container& container, const Value& value)
	{
		return std::find(container.begin(), container.end(), value) != container.end();
	}

	//////////////////////////////////////////////////////////////////////////
	//! Find element in a sorted container using binary search with logarithmic efficiency.
	//
	template <class Iterator, class T>
	OPTIONALINLINE Iterator binary_find(Iterator first, Iterator last, const T& value)
	{
		Iterator it = std::lower_bound(first, last, value);
		return (it == last || value != *it) ? last : it;
	}

	struct container_object_deleter
	{
		template<typename T>
		void operator()(const T* ptr) const
		{
			delete ptr;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	//! Convert arbitary class to const char*
	//////////////////////////////////////////////////////////////////////////
	template <class Type>
	OPTIONALINLINE const char* constchar_cast(const Type& type)
	{
		return type;
	}

	//! Specialization of string to const char cast.
	template <>
	OPTIONALINLINE const char* constchar_cast(const std::string& type)
	{
		return type.c_str();
	}

	//////////////////////////////////////////////////////////////////////////
	//! Case insensetive less key for any type convertable to const char*.
	template <class Type>
	struct less_stricmp
	{
		bool operator()(const Type& left, const Type& right) const
		{
			return _stricmp(constchar_cast(left), constchar_cast(right)) < 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Hash map usage:
	// typedef AZStd::unordered_map<string,int, stl::hash_string_insensitve<string>, stl::equality_string_insensitive<string> > StringToIntHash;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	//! Case sensitive string hash
	//////////////////////////////////////////////////////////////////////////
	template <class Key>
	class hash_string
	{
	public:
		enum    // parameters for hash table
		{
			bucket_size = 4,    // 0 < bucket_size
			min_buckets = 8
		};// min_buckets = 2 ^^ N, 0 < N

		size_t operator()(const Key& key) const
		{
			unsigned int h = 0;
			const char* s = constchar_cast(key);
			for (; *s; ++s)
			{
				h = 5 * h + *(unsigned char*)s;
			}
			return size_t(h);
		};
	};

	//////////////////////////////////////////////////////////////////////////
	//! Case sensitive string equality
	//////////////////////////////////////////////////////////////////////////
	template <class Key>
	class equality_string
	{
	public:
		bool operator()(const Key& key1, const Key& key2) const
		{
			return strcmp(constchar_cast(key1), constchar_cast(key2)) == 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////
	//! Case insensitive string hasher
	//////////////////////////////////////////////////////////////////////////
	template <class Key>
	class hash_string_caseless
	{
	public:
		enum    // parameters for hash table
		{
			bucket_size = 4,    // 0 < bucket_size
			min_buckets = 8
		};// min_buckets = 2 ^^ N, 0 < N

		size_t operator()(const Key& key) const
		{
			unsigned int h = 0;
			const char* s = constchar_cast(key);
			for (; *s; ++s)
			{
				h = 5 * h + tolower(*(unsigned char*)s);
			}
			return size_t(h);
		};
	};

	//////////////////////////////////////////////////////////////////////////
	//! Case insensitive string comparer
	//////////////////////////////////////////////////////////////////////////
	template <class Key>
	class equality_string_caseless
	{
	public:
		bool operator()(const Key& key1, const Key& key2) const
		{
			return _stricmp(constchar_cast(key1), constchar_cast(key2)) == 0;
		}
	};

	template <class T>
	OPTIONALINLINE void reconstruct(T& t)
	{
		t.~T();
		new(&t)T;
	}

	template <class T, class D>
	OPTIONALINLINE void reconstruct(StaticInstance<T, D>& instance)
	{
		reconstruct(*instance);
	}

	template <typename T, typename A1>
	OPTIONALINLINE void reconstruct(T& t, const A1& a1)
	{
		t.~T();
		new (&t)T(a1);
	}

	template <typename T, typename A1, typename A2>
	OPTIONALINLINE void reconstruct(T& t, const A1& a1, const A2& a2)
	{
		t.~T();
		new (&t)T(a1, a2);
	}

	template <typename T, typename A1, typename A2, typename A3>
	OPTIONALINLINE void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3)
	{
		t.~T();
		new (&t)T(a1, a2, a3);
	}

	template <typename T, typename A1, typename A2, typename A3, typename A4>
	OPTIONALINLINE void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3, const A4& a4)
	{
		t.~T();
		new (&t)T(a1, a2, a3, a4);
	}

	template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
	OPTIONALINLINE void reconstruct(T& t, const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5)
	{
		t.~T();
		new (&t)T(a1, a2, a3, a4, a5);
	}

	template <class T>
	OPTIONALINLINE void free_container(T& t)
	{
		reconstruct(t);
	}

	template <class T, class A>
	OPTIONALINLINE void free_container(std::deque<T, A>& t)
	{
		reconstruct(t);
	}

	template <class K, class D, class H, class A>
	OPTIONALINLINE void free_container(std__hash_map<K, D, H, A>& t)
	{
		reconstruct(t);
	}

	template <class T, class D>
	OPTIONALINLINE void free_container(StaticInstance<T, D>& instance)
	{
		reconstruct(*instance);
	}

	struct container_freer
	{
		template <typename T>
		void operator () (T& container) const
		{
			stl::free_container(container);
		}
	};
}
