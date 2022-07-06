#pragma once
#include "GenType.h"

#include <new>

#define GENARRAY_FUNC template<typename ItemType>
template<typename ItemType>
class GenArray {
	public:
		GenArray() : items(nullptr), numItems(0), allocationSize(0) {};
		inline ~GenArray();

	private:
		inline void Reallocate(gens32 newNumItems);

	public:
		// Appends a default-constructed item and returns a reference
		inline ItemType& Append();

		// Destroys all items and cleans up the array
		inline void Clear();

		// Returns the number of items in the array
		inline gens32 GetNum() const;

	public:
		// Item getters
		inline ItemType& operator[](gens32 index);

		// C++wuteveryearitwas iterators
		inline ItemType* begin() {return items;}
		inline ItemType* end() {return &items[numItems];}
		inline const ItemType* begin() const {return items;}
		inline const ItemType* end() const {return &items[numItems];}

	private:
		ItemType* items;
		gens32 numItems;
		genu32 allocationSize;

		// Base interval of memory allocation, in bytes. When the allocation interval is exceeded, it's doubled.
		static const gens32 allocationInterval = 256;
};

GENARRAY_FUNC GenArray<ItemType>::~GenArray() {
	Clear();
}

GENARRAY_FUNC ItemType& GenArray<ItemType>::Append() {
	// Resize the list
	Reallocate(numItems + 1);
	
	// Construct and return new item
	new (&items[numItems]) ItemType();
	return items[numItems++];
}

GENARRAY_FUNC void GenArray<ItemType>::Reallocate(gens32 newNumItems) {
	genu32 numBytesRequired = newNumItems * sizeof (ItemType);
	
	if (numBytesRequired > allocationSize) {
		// Expand the allocation size until it fits the neccessary size
		if (allocationSize == 0) {
			allocationSize = allocationInterval;
		}

		while (allocationSize < numBytesRequired) {
			allocationSize <<= 1;
		}

		// Reallocate the items, in bytes
		ItemType* newItems = (ItemType*)new genu8[allocationSize];

		// Move the old items to the new list
		for (gens32 i = 0; i < numItems; ++i) {
			new (&newItems[i]) ItemType(std::move(items[i]));
		}

		// Delete the old list!
		for (gens32 i = 0; i < numItems; ++i) {
			items[i].~ItemType();
		}

		delete[] items;

		// Reassign the new list!
		items = newItems;
	}
}

GENARRAY_FUNC ItemType& GenArray<ItemType>::operator[](gens32 index) {
	return items[index];
}

GENARRAY_FUNC void GenArray<ItemType>::Clear() {
	// Destruct all items
	for (gens32 i = 0; i < numItems; ++i) {
		items[i].~ItemType();
	}
	
	delete[] items;

	// Reset variables
	items = nullptr;
	numItems = 0;
	allocationSize = 0;
}

GENARRAY_FUNC int GenArray<ItemType>::GetNum() const {
	return numItems;
}