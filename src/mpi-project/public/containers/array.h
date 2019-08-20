#pragma once

#include "core_types.h"
#include "templates/max.h"
#include "templates/const_ref.h"
#include "templates/is_trivially_copyable.h"

/**
 * @class Array containers/array.h
 * 
 * Replacement for STL vector, with aligned
 * allocator
 */
template<typename T>
class GCC_ALIGN(32) Array
{
	template<typename>	friend class Array;

public:
	/// Iterators type definitions
	using Iterator		= T*;
	using ConstIterator	= const T*;

protected:
	/// Element buffer
	T * buffer;

	/// Current size of buffer
	uint64 size;

	/// Actual number of elements
	uint64 count;

public:
	/// Default constructor, adds uninitialized
	explicit FORCE_INLINE Array(uint64 _size = 0) :
		buffer(nullptr),
		size(_size),
		count(_size)
	{
		// Allocate initial buffer
		if (size) posix_memalign((void**)&buffer, Max<alignof(T), sizeof(void*)>::value, size * sizeof(T));
	}

	/// Initialization constructor
	FORCE_INLINE Array(uint64 _size, typename ConstRef<T>::Type s) : Array(_size)
	{
		// Initialize
		for (uint64 i = 0; i < size; ++i)
			new (buffer + i) T(s);
	}

	/// Copy constructor
	FORCE_INLINE Array(const Array & other) : Array(other.size)
	{
		// Copy content
		count = other.count;
		moveOrCopy(buffer, other.buffer, count);
	}

	/// Move constructor
	FORCE_INLINE Array(Array && other) :
		buffer(other.buffer),
		size(other.size),
		count(other.count)
	{
		other.buffer = nullptr;
	}

	/// Copy assignment
	FORCE_INLINE Array & operator=(const Array & other)
	{
		// Free existing buffer
		if (buffer) free(buffer);

		size = other.size;
		posix_memalign((void**)&buffer, Max<alignof(T), sizeof(void*)>::value, size * sizeof(T));

		// Copy content
		count = other.count;
		moveOrCopy(buffer, other.buffer, count);

		return *this;
	}

	/// Move assignment
	FORCE_INLINE Array & operator=(Array && other)
	{
		// Free existing buffer
		if (buffer) free(buffer);

		buffer	= other.buffer;
		size	= other.size;
		count	= other.count;

		other.buffer = nullptr; 

		return *this;
	}

	/// Destructor
	FORCE_INLINE ~Array()
	{
		if (buffer) free(buffer);
	}

	/// Returns raw content
	/// @{
	FORCE_INLINE T *		operator*()			{ return buffer; }
	FORCE_INLINE const T *	operator*() const	{ return buffer; }

	FORCE_INLINE T *		data()			{ return buffer; }
	FORCE_INLINE const T *	data() const	{ return buffer; }
	/// @}

	/// STL compliant iterators
	/// @{
	FORCE_INLINE Iterator		begin()			{ return buffer; }
	FORCE_INLINE ConstIterator	begin() const	{ return buffer; }

	FORCE_INLINE Iterator		end()		{ return buffer + count; }
	FORCE_INLINE ConstIterator	end() const	{ return buffer + count; }
	/// @}

	/// Random access operator
	/// @{
	FORCE_INLINE const T &	operator[](uint64 i) const	{ return buffer[i]; }
	FORCE_INLINE T &		operator[](uint64 i)		{ return buffer[i]; }
	/// @}

	/// Returns item count
	FORCE_INLINE uint64 getCount() const { return count; }

	/// Returns array size
	FORCE_INLINE uint64 getSize() const { return size; }

	/// Returns actual buffer load in Bytes
	FORCE_INLINE uint64 getBytes() const { return count * sizeof(T); }

	/// Returns true if array is empty
	FORCE_INLINE bool isEmpty() const { return count == 0; }

	/// Returns true if array is valid
	FORCE_INLINE bool isValid() const { return buffer && size; }

protected:
	/// Force resize array
	FORCE_INLINE bool resize_internal(uint64 _size)
	{
		if (_size > size)
		{
			// Realloc buffer
			T * orig = buffer;
			posix_memalign((void**)&buffer, Max<alignof(T), sizeof(void*)>::value, _size * sizeof(T));

			if (orig)
			{
				// Copy memory
				moveOrCopy(buffer, orig, size);
				free(orig);
			}

			size = _size;

			// Buffer was resized
			return true;
		}
		else
		{
			size	= _size;
			count	= count < _size ? count : _size;

			return false;
		}
	}

	/// Resize only if new count is bigger than current size
	FORCE_INLINE bool resizeIfNecessary(uint64 _count)
	{
		if (_count > size)
		{
			uint64 _size = size ? size * 2 : 2;
			while (_count > _size) _size *= 2;

			return resize_internal(_size);
		}

		return false;
	}

public:
	/// Set new count and possibly resize the array
	FORCE_INLINE void resize(uint64 _count)
	{
		resizeIfNecessary(_count);
		count = _count;
	}

	/// Reserve space without incrementing the count
	FORCE_INLINE void reserve(uint64 _size)
	{
		resizeIfNecessary(_size);
	}

	/// Like @ref operator[] but extends the array if out of bounds
	FORCE_INLINE T & operator()(uint64 i)
	{
		const uint64 _size = i + 1;
		count = resizeIfNecessary(_size) ? _size : count;

		return buffer[i];
	}

	/**
	 * Insert a new item in the array
	 * 
	 * @param [in] item T operand
	 * @param [in] i target position
	 */
	FORCE_INLINE void insert(typename ConstRef<T>::Type item, uint64 i)
	{
		resizeIfNecessary(count + 1);

		// Move content up
		if (LIKELY(i < count))
			memmove(buffer + i + 1, buffer + i, (count - i) * sizeof(T));
		
		// Construct object
		moveOrCopy(buffer[i], item);
		++count;
	}

	/**
	 * Insert multiple items in the array
	 * 
	 * @param [in] items buffer of items
	 * @param [in] n number of items
	 * @param [in] i target position
	 */
	FORCE_INLINE void insert(const T * items, uint64 n, uint64 i)
	{
		resizeIfNecessary(count + n);

		// Move content up
		if (i < count)
			memmove(buffer + i + n, buffer + i, (count - i) * sizeof(T));

		// Construct objects
		moveOrCopy(buffer + i, items, n);
		count += n;
	}

	/**
	 * Add item at the end of the array
	 * 
	 * @param [in] item T operand
	 * @{
	 */
	FORCE_INLINE void add(typename ConstRef<T>::Type item)
	{
		resizeIfNecessary(count + 1);

		// Construct object
		moveOrCopy(buffer[count], item);
		++count;
	}
	FORCE_INLINE void push(typename ConstRef<T>::Type item) { add(item); }
	/// @}

	/**
	 * Add multiple items at the end of the array
	 * 
	 * @param [in] items buffer of items
	 * @param [in] n number of items
	 * @{
	 */
	FORCE_INLINE void add(const T * items, uint64 n)
	{
		resizeIfNecessary(count + n);

		// Construct objects
		moveOrCopy(buffer + count, items, n);
		count += n;
	}
	FORCE_INLINE void push(const T * items, uint64 n) { add(items, n); }
	/// @}

	/**
	 * Remove items at position
	 * 
	 * @param [in] i item's position
	 * @param [in] n number of items to remove
	 */
	FORCE_INLINE void removeAt(uint64 i, uint64 n = 1)
	{
		// Just move back memory
		if (i < count)
			memmove(buffer + i, buffer + i + n, (count - i) * sizeof(T));
		
		count -= n;
	}

	/// Remove item at the end of the array
	FORCE_INLINE void pop(uint64 i, uint64 n)
	{		
		--count;
	}
};