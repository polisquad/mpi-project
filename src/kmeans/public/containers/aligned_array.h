#pragma once

#include "core_types.h"
#include "templates/const_ref.h"

/**
 * A surrogate for std::vector
 */
template<typename T, uint32 memalign = alignof(T)>
class Array
{
public:
	/// Iterator types
	using Iterator		= T*;
	using ConstIterator	= const T*;

protected:
	/// Storage
	T * buffer;

	/// Size of the array
	uint64 size;

	/// Actual capacity of the array
	uint64 capacity;

public:
	/// Default constructor, with initial capacity
	FORCE_INLINE Array(uint32 _capacity = 2) :
		size(0),
		capacity(_capacity > 2 ? _capacity : 2)
	{
		// Initialize empty buffer
		int32 status = posix_memalign((void**)&buffer, memalign, capacity * sizeof(T));
		if (status) capacity = 0, size = 0;
	}

	/// Copy constructor
	FORCE_INLINE Array(const Array<T, memalign> & other) : Array(other.size)
	{
		// Copy content
		size = other.size;
		memcpy(buffer, other.buffer, size * sizeof(T));

		// It's funny how I use copy constructors
		// but then use memcpy. Anyway it doesn't
		// really matter in this project
	}

	/// Move constructor
	FORCE_INLINE Array(Array<T, memalign> && other) :
		size(other.size),
		capacity(other.capacity)
	{
		// Steal buffer, it's mine!!!
		buffer = other.buffer;
		other.buffer = nullptr;
		other.capacity = other.size = 0;
	}

	/// Copy assignment
	FORCE_INLINE Array<T, memalign> & operator=(const Array<T, memalign> & other)
	{
		if (other.size > capacity)
		{
			// Free old buffer
			if (buffer) free(buffer);

			// Create new buffer
			capacity = size = other.size;
			posix_memalign((void**)&buffer, memalign, capacity * sizeof(T));
		}
		else
			size = other.size;
		
		// Copy content
		memcpy(buffer, other.buffer, size * sizeof(T));
	}

	/// Move assignment
	FORCE_INLINE Array<T, memalign> & operator=(Array<T, memalign> && other)
	{
		capacity	= other.capacity,
		size		= other.size;

		// Free my buffer ...
		free(buffer);

		// and steal other's buffer, it's mine!!!
		buffer = other.buffer;
		other.buffer = nullptr;
		other.capacity = other.size = 0;
	}

	/// Destructor
	FORCE_INLINE ~Array()
	{
		if (buffer != nullptr)
			free(buffer);
	}
	
	/// Get array size
	FORCE_INLINE uint64 getSize() const { return size; }

	/// Get array current capacity
	FORCE_INLINE uint64 getCapacity() const { return capacity; }

	/// Get access to array buffer
	/// @{
	FORCE_INLINE T *		operator*()			{ return buffer; }
	FORCE_INLINE const T *	operator*() const	{ return buffer; }
	/// @}

	/// Random access operator
	/// @{
	FORCE_INLINE T &		operator[](uint64 i)		{ return buffer[i]; }
	FORCE_INLINE const T &	operator[](uint64 i) const	{ return buffer[i]; }
	/// @}

	/// Iterators
	/// @{
	FORCE_INLINE Iterator		begin()			{ return buffer; }
	FORCE_INLINE Iterator		end()			{ return buffer + size; }

	FORCE_INLINE ConstIterator	begin()	const	{ return buffer; }
	FORCE_INLINE ConstIterator	end()	const	{ return buffer + size; }
	/// @}

	/**
	 * Insert an element
	 * 
	 * Note that inserting an element at the
	 * beginning of the array is costly
	 * 
	 * @param [in] elem element to insert
	 * @param [in] i index
	 * @return ref to inserted element
	 */
	FORCE_INLINE T & insert(typename ConstRef<T>::Type elem, uint64 i)
	{
		// Check resize needed?
		resizeIfNecessary(size + 1);
		
		if (i < size)
			// Move memory
			memmove(buffer + i + 1, buffer + i, size - i);
		
		++size;

		// Copy construct element
		new (buffer + i) T(elem);
		return buffer[i];
	}

	/**
	 * Push element to the end of the array
	 * 
	 * @param [in] elem to insert
	 * @return ref to inserted element
	 * @{
	 */
	FORCE_INLINE T & push(typename ConstRef<T>::Type elem)
	{
		// Check resize needed
		resizeIfNecessary(size + 1);

		// Copy construct element
		new (buffer + size) T(elem);
		return buffer[size++];
	}
	FORCE_INLINE T & add(typename ConstRef<T>::Type elem) { return push(elem); }
	/// @}

	/// Like @ref push() but returns self
	FORCE_INLINE Array<T, memalign> & operator+(typename ConstRef<T>::Type elem)
	{
		push(elem);
		return *this;
	}

protected:
	/// Resize if necessary
	FORCE_INLINE bool resizeIfNecessary(uint64 _size)
	{
		if (UNLIKELY(_size > capacity))
		{
			uint64 _capacity = capacity;
			while(_capacity < _size) _capacity *= 2;

			resize(_capacity);
		}
	}

	/// Resize array to desired capacity
	FORCE_INLINE bool resize(uint64 _capacity)
	{
		if (LIKELY(_capacity > capacity))
		{
			// Create new storage
			T * newBuffer = nullptr;
			if (posix_memalign((void**)&newBuffer, memalign, _capacity * sizeof(T)) != 0) return false;

			// Copy content and free old buffer
			memcpy(newBuffer, buffer, capacity * sizeof(T));
			free(buffer);

			// Update array info
			buffer = newBuffer;
			capacity = _capacity;

			return true;
		}
		else if (_capacity < capacity)
		{
			// Just old fashioned realloc
			if (!realloc(buffer, _capacity)) return false;

			// Update array info
			capacity = _capacity;
			if (size < capacity) size = _capacity; // Capped size

			return true;
		}

		return false;
	}
};