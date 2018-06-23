#pragma once
#include <new>
#include <stdarg.h>
#include "sdError.h"

#ifdef UNICODE
typedef wchar_t tchar;

#define STR(a) L##a
#else
typedef char tchar;

#define STR(a) a
#endif

typedef wchar_t wchar;

typedef __int64 int64; typedef unsigned __int64 uint64;
typedef __int32 int32; typedef unsigned __int32 uint32;
typedef __int16 int16; typedef unsigned __int16 uint16;
typedef __int8 int8; typedef unsigned __int8 uint8;

typedef uint32 color32;

typedef float float32;
typedef double float64;

typedef unsigned char bool8;
typedef uint32 bool32; // usually used for HLSL

#ifdef _M_64
typedef uint64 uintptr;
#else
typedef uint32 uintptr;
#endif

namespace sdHelper {
	// Calls move constructor on class to move it
	template<typename ClassType>
	inline void MoveConstruct(ClassType* destination, ClassType* source) {
		destination->~ClassType();
		new (destination) ClassType(std::move(*source));
	}
}

//--------------- String class ---------------
// This is a hybrid constructor class. Create and Destroy are only necessary for allocated uninitialised instances. See info.
// String uses dynamically allocated memory, which can make it quite slow. For optimum performance, re-use a string when you can
class String {
	public:
		String() : data(nullptr), cData(nullptr), wData(nullptr), length(0), staticLength(0) {};
		String(const char* str, ...);
		String(const wchar* str, ...);
		String(const String& str) : String(str.GetW()) {};
		String(char* staticA, wchar* staticW, int32 staticLength_) : cData(staticA), wData(staticW), data(cData), length(0), staticLength(staticLength_) {};
		~String();

		// Hybrid-Create functions: Creates and destroys a string if it isn't immediately initialised on the stack
		void Create();
		void Create(const char* str, ...);
		void Create(const wchar* str, ...);
		void Destroy();

		// GetW and GetC: returns a valid wide and char string (even if empty), respectively
		const wchar* GetW() const;
		const char* GetC() const;

		wchar* GetW();
		char* GetC();
		
		// Operator overloads
		inline void operator=(const char* str) {Set(str);}
		inline void operator=(const wchar* str) {Set(str);}
		inline void operator=(const String& str) {Set(str.GetW());}
		inline void operator+=(const wchar* str) {Append(str);}
		inline void operator+=(const String& str) {Append(str.GetW());}

		bool operator==(const char* str) const;
		bool operator==(const wchar* str) const;
		bool operator==(const String& str) const;
		bool operator!=(const char* str) const {return !(*this == str);}
		bool operator!=(const wchar* str) const {return !(*this == str);}
		bool operator!=(const String& str) const {return !(*this == str);}

		operator const wchar*() {return GetW();}
		operator const char*() {return GetC();}

		void Set(const wchar* str);
		void Set(const char* str);
		void Sprintf(const wchar* str, ...);
		void Sprintf(const char* str, ...);
		void Append(const wchar* str);

		// SetBufferLength: Sets the physical length of the string buffer and pads it with null terminators. This is not the same as Length()
		// The actual length allocated will usually be larger than minLength and allocated in large intervals. Do not rely on GetBufferLength() to return the same as your minLength value!
		void SetBufferLength(int minLength);
		// GetBufferLength: Returns the physical length of the string
		int GetBufferLength() const;

		// Length: Returns the length of the string in characters (discludes null terminator)
		int Length() const;
		void Trim(int length);

		// HasUnicode: Returns whether or not Unicode characters are present in the string
		bool HasUnicode() const;

		// MatchesFormat: Returns whether the entire string matches a wildcard-based (*) format string (not C-style formats!)
		// Useful for e.g. checking file extensions
		bool MatchesFormat(const wchar* wildcardFormat, int fromPosition = 0) const;
	protected:
		void VaSetW(const wchar* str, void* args);
		void VaSetC(const char* str, void* args);

		int SetDataLength(int length, bool discardOld = false); // length discludes nullptr terminator

		void* data; // All allocated data is stored in this pool
		wchar* wData; // wData is the primary string data
		char* cData; // cData is used only by GetC()
		int32 length; // Length of the string
		int32 staticLength; // Length of supplied static string buffer (if applicable -- StaticStrings only)
};

// --- Static strings ---
// Templated String without dynamic allocation. Faster to use. Use this if you can guarantee a string's maximum length
template <int StaticLength = 0> // StaticLength = maximum length of the string, not including null terminator
class StaticString : public String {
	public:
		StaticString() : String(cData, wData, StaticLength) {};
		StaticString(const char* str, ...) : String(cData, wData, StaticLength) {
			va_list args;
			va_start(args, str);
			VaSetC(str, args);
			va_end(args);
		}
		StaticString(const wchar* str, ...) : String(cData, wData, StaticLength) {
			va_list args;
			va_start(args, str);
			VaSetW(str, args);
			va_end(args);
		}

	private:
		wchar wData[StaticLength + 1];
		char cData[StaticLength + 1];
};

//--------------- Simple geometry ---------------
// Rect::Inside - Checks if point arguments are inside the rectangle
struct PointF { // never second best
	PointF() = default;
	PointF(float32 x_, float32 y_) : x(x_), y(y_) {};

	float32 x, y;
};

struct Point32 {
	Point32() = default;
	Point32(int32 x_, int32 y_) : x(x_), y(y_) {};

	inline void Set(int32 x, int32 y) {
		this->x = x;
		this->y = y;
	}

	int32 x, y;
};

struct Line32 { // marked
	union {
		struct {int32 x1, y1;};
		Point32 p1;
	};
	union {
		struct {int32 x2, y2;};
		Point32 p2;
	};
};

struct Rect32 { // never in doubt
	Rect32() = default;
	Rect32(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height) {};

	int32 x, y, width, height;

	inline void Set(int x, int y, int width, int height) {
		this->x = x; this->y = y;
		this->width = width; this->height = height;
	}

	inline bool Inside(int x, int y) const {
		return (x >= this->x && y >= this->y && x < this->x + this->width && y < this->y + this->height);
	}
};

struct RectU32 { // LET THE DRAGON CONSUME YOU
	uint32 x, y, width, height;
	
	inline bool Inside(uint32 x, uint32 y) const {
		return (x >= this->x && y >= this->y && x < this->x + this->width && y < this->y + this->height);
	}
};

struct RectF { // see that which is unseen
	float32 x, y, width, height;

	inline bool Inside(float x, float y) const {
		return (x >= this->x && y >= this->y && x < this->x + this->width && y < this->y + this->height);
	}
};

//--------------- List classes---------------
class SdListBase {
	public:
		SdListBase() : items(nullptr), numItems(0) {};

		void** items;
		int32 numItems;
};

// A list of objects, where each object is stored as a pointer in the main list, and thus won't be reallocated.
// Differences to Arrays: Objects are stored as pointers in memory; Objects don't move. For smaller classes where reallocation is insignificant, use Arrays instead
template <class ItemType>
class List : public SdListBase {
public:
	List() : numAlloced(0) {};

public:
	inline ItemType* operator[](int id) const {return (ItemType*) items[id];}

public:
	int Find(ItemType* item, int count = 0) const;

public:
	// Add and remove items
	// Add default-constructed item
	inline ItemType* Add();

	// Add default-constructed item of a custom size
	inline ItemType* Add(uint32 itemSize);

	// Add copy-constructed item
	inline ItemType* Add(const ItemType& src);

	// Add copy-constructed item of a custom size
	inline ItemType* Add(const ItemType& src, uint32 itemSize);

	// Add move-constructed item (so you can e.g. call Add(ItemType(parameters)) constructors more easily)
	inline ItemType* Add(const ItemType&& src);

	// Add multiple default-constructed items and returns the starting index
	inline int AddMultiple(int32 numAdditionalItems);

	// Add default-constructed item that's actually a subclass of the parent class
	template<typename Subclass>
	inline Subclass* AddSub();

	// Add default-constructed item that's actually a subclass of the parent class, of a custom size
	template<typename Subclass>
	inline Subclass* AddSub(uint32 size);

	// Removes an item by index
	inline void Remove(int index);

	// Removes an item by a pointer to it
	inline void RemoveByPointer(ItemType* pointer);

public:
	template<typename SubclassType = ItemType>
	inline SubclassType* Resize(int itemIndex, uint32 newSize);

	template<typename SubclassType = ItemType>
	inline SubclassType* ResizeByPointer(SubclassType* pointer, uint32 newSize);

public:
	inline void Clear();

public:
	// C++11 Iterators
	inline ItemType** begin() {return (ItemType**) items;}
	inline ItemType** end() {return (ItemType**)&items[numItems];}
	
	inline const ItemType*const* begin() const {return (ItemType*const*) items;}
	inline const ItemType*const* end() const {return (ItemType*const*)&items[numItems];}

private:
	ItemType* AllocateItem(uint32 itemSize = sizeof (ItemType), uint32 sizeofItem = sizeof (ItemType));
	void ReallocItems(int32 numToAllocate);

private:
	int32 numAlloced;

	static const int allocInterval = 256;
};

// Templateless base class for Array
class ArrayBase {
	public:
		ArrayBase() : items(0), numItems(0), itemSize(0), allocSize(0) {};
		inline ArrayBase(ArrayBase&& other);
		~ArrayBase();

		inline void* Get(int id) {return (void*) ((uintptr) items + id * itemSize);}

		void SetNum(int numItems, uint32 itemSize);
		inline int GetNum() const {return numItems;}
		inline int GetSize() const {return numItems * (int)itemSize;}
		
		void* Append(uint32 itemSize);
		void* Insert(int id, uint32 itemSize);
		void Remove(int id);
		void Clear();
	protected:
		uint8* items;
		int32 numItems; // number of items in the array
		uint32 itemSize; // size per item
		uint32 allocSize; // total size of array data allocated, may be bigger than actual array size for efficiency
};


// Dynamic array--basically a vector with a much more convenient name.
// TODO: implement std::move
template <typename ItemType>
class Array : public ArrayBase {
	public:
		inline Array() {itemSize = sizeof (ItemType);}
		inline Array(uint32 numItems) {ArrayBase::SetNum(numItems, sizeof (ItemType));}
		inline Array(const Array<ItemType>& other);
		inline Array(Array<ItemType>&& other) : ArrayBase(std::move(other)) {}
		inline ~Array() {Clear();}

		// Conversion operators
		inline operator ItemType*() {return (ItemType*) items;};
		inline operator const ItemType*() const {return (ItemType*) items;};

		// Set length of the array
		inline void SetNum(uint32 numItems);

		// Append/insert/remove items from the array
		inline ItemType& Append();
		inline ItemType& Append(ItemType& copy);

		template<class ...Types> inline ItemType& Append(Types... constructorArgs);

		inline ItemType& Insert(int id);
		inline void Remove(int id);

		// Frees all items from array
		inline void Clear();

		// ZeroData: Destructs all objects and re-zero-initializes them
		inline void ZeroData();

		// C++11 Iterators
		inline ItemType* begin();

		inline ItemType* end();
		
		inline const ItemType* begin() const;
		inline const ItemType* end() const;
};

//--------------- Bitstreams ---------------
class BitStream {
	public:
		BitStream() : data(nullptr), dataSize(0), increment(16384 /* 8KB default */), position(0) {};

		// Seek: Sets bit position to a given value
		inline void Seek(uint32 pos);

		// Tell: Returns the current bit position
		inline uint32 Tell() const;

		// Write: Writes 'value' to the current position in 'numBits' bits and advances the bit position by that much
		inline void Write(uint32 value, uint8 numBits);

		// Read: Reads a number of bits 'numBits' from the current position, returns the value and advances the bit position
		inline uint32 Read(uint8 numBits);

		// Set: Sets a 'value' at a specific bit 'position' with 'numBits', independent to the current bit position
		inline void Set(uint32 position, uint32 value, uint8 numBits);

		// Get: Returns the value at 'position' with 'numBits' bits, independent to the current bit position
		inline uint32 Get(uint32 position, uint8 numBits) const;

		// SetSizeIn[x]: Resizes the bit sequence
		void SetSizeInBits(uint32 numBits);
		void SetSizeInBytes(uint32 numBytes);

		// SetIncrement: Sets the size amount by which the BitStream will automatically increase when the boundary is exceeded
		inline void SetIncrement(uint32 value);

	private:
		uint32* data;
		uint32 dataSize;

		uint32 increment;

		uint32 position;
};

// ===== sdRawPtr: Raw pointer that can be implicitly converted =====
struct sdRawPtr {
	sdRawPtr(void* value_) : value(value_) {};

	template<class Type>
	operator Type() {
		return (Type)this->value;
	}

	void* value;
};

template<class ItemType> inline int List<ItemType>::Find(ItemType* item, int count) const {
	for (int32 i = 0; i < numItems; ++i) {
		if (items[i] == item) {
			--count;

			if (count < 0)
				return i;
		}
	}

	return -1;
}

template<class ItemType> inline ItemType* List<ItemType>::Add() {
	ItemType* item = AllocateItem();

	new (item) ItemType();
	return item;
}

template<class ItemType> inline ItemType* List<ItemType>::Add(uint32 itemSize) {
	ItemType* item = AllocateItem(itemSize);

	new (item) ItemType();
	return item;
}

template<class ItemType> inline ItemType* List<ItemType>::Add(const ItemType& src) {
	ItemType* item = AllocateItem();
		
	new (item) ItemType(src);
	return item;
}

template<class ItemType> inline ItemType* List<ItemType>::Add(const ItemType& src, uint32 itemSize) {
	ItemType* item = AllocateItem(itemSize);

	new (item) ItemType();
	return item;
}

template<class ItemType> inline ItemType* List<ItemType>::Add(const ItemType&& src) {
	ItemType* item = AllocateItem(itemSize);

	new (item) ItemType(src);
	return item;
}

template<class ItemType> inline int List<ItemType>::AddMultiple(int32 numAdditionalItems) {
	if (numAdditionalItems > 0) {
		ReallocItems(numItems + numAdditionalItems);

		int startingIndex = numItems;
		for (int32 i = 0; i < numAdditionalItems; ++i) {
			Add();
		}

		return startingIndex;
	}

	// This shouldn't really happen. Return numItems so an access exception occurs if the programmer added 0 items
	return numItems;
}

template<class ItemType> inline void List<ItemType>::Remove(int index) {
	SDASSERT(index < numItems);

	((ItemType*)items[index])->~ItemType(); // friendly reminder to use virtual destructors! Due to AddSub
	operator delete(items[index]);

	// Move the trailing item pointers into place
	--numItems;
	for (int32 i = index; i < numItems; ++i) {
		items[i] = items[i + 1];
	}
}

template<class ItemType> inline void List<ItemType>::RemoveByPointer(ItemType* pointer) {
	int index = Find(pointer);

	if (index >= 0) {
		Remove(index);
	}
}

template<class ItemType> inline void List<ItemType>::Clear() {
	for (int i = 0; i < numItems; i++) {
		((ItemType*)items[i])->~ItemType();
		operator delete(items[i]);
	}

	delete[] items;

	items = nullptr;
	numAlloced = 0;
	numItems = 0;
}

template<class ItemType> inline ItemType* List<ItemType>::AllocateItem(uint32 itemSize, uint32 sizeofItem) {
		// Realloc
		ReallocItems(numItems + 1);

		// Now allocate the item itself
#ifndef _DEBUG
		items[numItems] = operator new(itemSize);
#else
		// Debug memset causes crashes when used on objects with variable sizes (unions etc)
		items[numItems] = operator new(itemSize >= sizeofItem ? itemSize : sizeofItem);
#endif

		// Add the item to the list and return
		return (ItemType*)items[numItems++];
	}

template<class ItemType> inline void List<ItemType>::ReallocItems(int32 numToAllocate) {
	// Extend the list if the allocated memory isn't enough
	if (numAlloced < allocInterval) {
		numAlloced = allocInterval;
	}

	// Double it until it's big enough
	while (numAlloced < numToAllocate) {
		numAlloced *= 2;
	}

	// Copy the item list to a new item list
	void** newItems = new void*[numAlloced];

	memcpy(newItems, items, numItems * sizeof (void*));
		
	// Delete the last list and reassign items
	delete[] items;
	items = newItems;
}

template<class ItemType> template<typename Subclass> inline Subclass* List<ItemType>::AddSub() {
	Subclass* item = (Subclass*)AllocateItem(sizeof (Subclass));

	new (item) Subclass();
	return item;

#ifdef _DEBUG
	// Report an error if Subclass isn't a subclass of ItemType
	(item >= (ItemType*)0);
#endif
}

template<class ItemType> template<typename Subclass> inline Subclass* List<ItemType>::AddSub(uint32 size) {
	Subclass* item = (Subclass*)AllocateItem(size);

	new (item) Subclass();
	return item;

#ifdef _DEBUG
	// Report an error if Subclass isn't a subclass of ItemType
	(item >= (ItemType*)0);
#endif
}

template<class ItemType> template<typename SubclassType> inline SubclassType* List<ItemType>::Resize(int itemIndex, uint32 newSize) {
	SDASSERT(itemIndex < numItems);

	// Allocate new memory for the item
	SubclassType* newItem = (SubclassType*)operator new(newSize);

	// Move-construct the new item
	new (newItem) SubclassType(std::move(*(SubclassType*)items[itemIndex]));

	// Delete the old item
	((SubclassType*)items[itemIndex])->~SubclassType();
	operator delete(items[itemIndex]);

	// Reassign the pointer
	items[itemIndex] = newItem;
		
	return newItem;
}

template<class ItemType> template<typename SubclassType> inline SubclassType* List<ItemType>::ResizeByPointer(SubclassType* pointer, uint32 newSize) {
	int index = Find(pointer);

	if (index >= 0) {
		return Resize(index, newSize);
	}

	return nullptr;
}

ArrayBase::ArrayBase(ArrayBase&& other) : items(other.items), numItems(other.numItems), itemSize(other.itemSize), allocSize(other.allocSize) {
	other.items = nullptr;
	other.numItems = 0;
	other.itemSize = 0;
	other.allocSize = 0;
}

template<typename ItemType> inline Array<ItemType>::Array(const Array<ItemType>& other) {
	ArrayBase::SetNum(other.numItems, other.itemSize);

	for (int i = 0; i < numItems; ++i) {
		new (&(*this)[i]) ItemType(other[i]);
	}
}

template<typename ItemType> inline void Array<ItemType>::SetNum(uint32 numItems) {
	ArrayBase::SetNum(numItems, sizeof (ItemType));
}

template<typename ItemType> inline ItemType& Array<ItemType>::Append() {
	ItemType* item = (ItemType*) ArrayBase::Append(sizeof (ItemType));

	new (item) ItemType();
	return *item;
}

template<typename ItemType> inline ItemType& Array<ItemType>::Append(ItemType& copy) {
	ItemType* item = (ItemType*) ArrayBase::Append(sizeof (ItemType));

	new (item) ItemType(copy);
	return *item;
}

template<typename ItemType> template<class ...Types> inline ItemType& Array<ItemType>::Append(Types... constructorArgs) {
	ItemType* item = (ItemType*) ArrayBase::Append(sizeof(ItemType));

	new (item) ItemType(constructorArgs...);
	return *item;
}

template<typename ItemType> inline ItemType& Array<ItemType>::Insert(int id) {
	ItemType* item = (ItemType*) ArrayBase::Insert(id, sizeof (ItemType));

	new (item) ItemType();
	return *item;
}

template<typename ItemType> inline void Array<ItemType>::Remove(int id) {
	if (id < numItems) {
		((ItemType*)Get(id))->~ItemType();

		ArrayBase::Remove(id);
	}
}

template<typename ItemType> inline void Array<ItemType>::Clear() {
	for (int32 i = 0; i < numItems; i++)
		((ItemType*)Get(i))->~ItemType();

	ArrayBase::Clear();
}

template<typename ItemType> inline void Array<ItemType>::ZeroData() {
	for (int32 i = 0; i < numItems; i++)
		((ItemType*)Get(i))->~ItemType();

	memset(items, 0, numItems * itemSize);

	for (int32 i = 0; i < numItems; i++)
		new (Get(i)) ItemType();
}

template<typename ItemType> inline ItemType* Array<ItemType>::begin() {
	return (ItemType*) items;
}

template<typename ItemType> inline ItemType* Array<ItemType>::end() {
	return &((ItemType*)items)[numItems];
}

template<typename ItemType> inline const ItemType* Array<ItemType>::begin() const {
	return (ItemType*) items;
}

template<typename ItemType> inline const ItemType* Array<ItemType>::end() const {
	return &((ItemType*)items)[numItems];
}

// Seek: Sets bit position to a given value
inline void BitStream::Seek(uint32 pos) {
	position = pos;
}

// Tell: Returns the current bit position
inline uint32 BitStream::Tell() const {
	return position;
}

// Write: Writes 'value' to the current position in 'numBits' bits and advances the bit position by that much
inline void BitStream::Write(uint32 value, uint8 numBits) {
	if ((position + numBits) / 8 > dataSize)
		SetSizeInBits(position + numBits);

	Set(position, value, numBits);
	position += numBits;
}

// Read: Reads a number of bits 'numBits' from the current position, returns the value and advances the bit position
inline uint32 BitStream::Read(uint8 numBits) {
	position += numBits;
	return Get(position - numBits, numBits);
}

// Set: Sets a 'value' at a specific bit 'position' with 'numBits', independent to the current bit position
inline void BitStream::Set(uint32 position, uint32 value, uint8 numBits) {
	uint32 offset = (position & 31);
	data[position / 32] = (data[position / 32] & ~(~(0xFFFFFFFF << numBits) << offset)) | 
							((value & ~(0xFFFFFFFF << numBits)) << offset);

	if (32 - offset < numBits)
		data[position / 32 + 1] = (data[position / 32 + 1] & (0xFFFFFFFF << (numBits + offset - 32))) | 
									((value & ~(0xFFFFFFFF << numBits)) >> (32 - offset));
}

// Get: Returns the value at 'position' with 'numBits' bits, independent to the current bit position
inline uint32 BitStream::Get(uint32 position, uint8 numBits) const {
	uint32 offset = (position & 31);
	if (offset + numBits > 32)
		return ((data[position / 32] >> offset) | (data[position / 32 + 1] << (32 - offset))) & ~(0xFFFFFFFF << numBits);
	else
		return ((data[position / 32] >> offset) & ~(0xFFFFFFFF << numBits));
}

// SetIncrement: Sets the size amount by which the BitStream will automatically increase when the boundary is exceeded
inline void BitStream::SetIncrement(uint32 value) {
	increment = value;
}