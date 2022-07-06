#pragma once
#include "GenContainer.h"
#include "GenType.h"

// A variable-sized array of multiple values of any GenType
struct GenValueSet {
	// Header
	// Type of values
	GenType type;

	// Number of values. numElements must always be able to calculate the size of the array.
	// For variable-sized GenTypes (such as GenProp), numElements is the size of the data in bytes
	genu32 numValues;

	// Value array (depending on the type)
	union {
		// Arrays elements. Make sure you don't do e.g. if (element->bool8) as this will just check the pointer!
		genu8 byte[1];
		gens8 s8[1]; genu8 u8[1];
		gens16 s16[1]; genu16 u16[1];
		gens32 s32[1]; genu32 u32[1];
		genf32 f32[1];
		genf64 f64[1];
		genbool8 bool8[1];
		GenVec3 vec3[1];
		GenVec2 vec2[1];
		GenTransform transform[1];
		genid id[1];

		// Singular elements
		genu8 trigger;
	};

	// Size of the header
	static const int headerSize;

	// Constructors
	GenValueSet() {return;}  // Hack: Override default construction so that GenValueSet is not zero-initialised
	GenValueSet(const GenValueSet& other) {memcpy(this, &other, other.GetSize());}
	GenValueSet(gens32 _iValue) : type(GENTYPE_S32), s32{_iValue}, numValues(1) {};
	GenValueSet(genf32 _fValue) : type(GENTYPE_F32), f32{_fValue}, numValues(1) {};
	GenValueSet(const char* strValue) : type(GENTYPE_CSTRING) {
		int count;
		for (count = 0; strValue[count]; ++count) {
			u8[count] = strValue[count];
		}

		u8[count] = '\0';
		numValues = count + 1;
	}

	// Returns the total size of these elements including the header
	inline genu32 GetSize() const;

	// Returns the total size of the elements discluding theh eader
	inline genu32 GetDataSize() const;

	// Returns the total size of a hypothetical GenValueSet struct with the given parameters (including the header)
	static inline genu32 GetSize(genu32 dataType, genu32 numElements);

public:
	// Getters for embedded GenValueSets
	inline const GenValueSet* GetValueSet(int index, bool doCheckValid = true) const;
	inline int GetNumValueSets() const;

	// Getters for types unavailable in the value union
	// Prop retrieval functions (required because props contain a GenValueSet -- infinite recursion if we tried to include it in the union!)
	inline struct GenProp* GetProp();
	inline const struct GenProp* GetProp() const;

public:
	// GenValue getters and setters
	inline bool Set(genu32 index, const GenValue& value);
	inline bool Get(genu32 index, GenValue* value) const;

	inline void Insert(genu32 index, const GenValue& value);
	inline void Remove(genu32 index);

public:
	inline bool Copy(const GenValueSet& other);
	inline bool Compare(const GenValueSet& other) const;

	// Returns string of the given index
	inline char* GetString(int index);
	inline int GetNumStrings();
};

// A sized version of GenValueSet, so you can provide values of a specific size
template<const int setSize = 0>
struct GenValueSetContainer : public GenValueSet {
	// Inherit GenValueSet constructors (C++11 only)
	using GenValueSet::GenValueSet;

	genu8 extraBytes[setSize];
};

// A manager for storing value sets within value sets
class GenMixedValueSet {
	public:
		inline GenMixedValueSet() {Create();};
		inline ~GenMixedValueSet() {Destroy();};

		void Create();
		void Destroy();
		
		void Clear();

	public:
		// Reads directly from supplied elements. The pointer must remain valid for as long as this class is being used
		bool ReadPackagedElementsDirect(const GenValueSet& source);

	public:
		// Adds a GenValueSet based on the desired type and amount, and returns a pointer
		GenValueSet* AddValueSet(GenType type, genu32 numElements);

		// Adds a pointer to an existing GenValueSet. Faster than using AddElement, but elementsPointer must exist throughout the lifetime.
		// NOT COMPATIBLE WITH AddElement. Use only one or the other.
		GenValueSet* AddValueSetDirect(GenValueSet* elementsPointer);

		// Returns the element at the given index
		inline GenValueSet* GetElement(genu32 index);

		// Returns the number of elements
		inline genu32 GetNumValues() const {return elements.GetNum();};

	public:
		// Generates (if necessary) the GenValueSet and returns a pointer, if the elements exist
		inline const GenValueSet* GetPackagedElements() const {UpdatePackagedElements(); return cachedElements;}

		// Returns the size of the packaged elements, not including the GenValueSet header
		inline genu32 GetPackagedElementsSize() const {UpdatePackagedElements(); return cachedElementsSize;}

	private:
		void UpdatePackagedElements() const;
		
	private:
		// Whether this class directly references its GenValueSets from other sources
		genbool8 isDirect;

		// Actual elements
		GenArray<GenValueSet*> elements;

	private:
		// Packaged elements cache
		mutable GenValueSet* cachedElements;
		mutable genu32 cachedElementsSize;

		mutable genbool8 isCacheValid;
};

struct GenProp {
	static const int maxNameLength = 16;
	
	char name[maxNameLength]; // name of the property; equivalent to the control's tag
	GenValueSet value; // value of the property

	// Constructors
	inline GenProp() {return;}; // Hack: Override default construction so that GenProp is not zero-initialised
	inline GenProp(const char* name, const struct GenValueSet& value);

	// Setters
	inline void SetName(const char* name);

	// Copies
	inline void Copy(const GenProp& other);

	// Comparisons
	inline bool Compare(const GenProp& other) const;
	inline bool CompareName(const char* name) const;

	// Returns total (required) size of this struct + its element data
	inline int GetSize() const;
	// Returns total required size of a struct + element data of the given properties
	static inline int GetSize(genu32 dataType, genu32 numElements);
};

inline genu32 GenValueSet::GetSize() const {
	if (type >= GENTYPE_NUMTYPES)
		return headerSize;

	return headerSize + numValues * genTypeDefs[type].size;
}

inline genu32 GenValueSet::GetDataSize() const {
	if (type < GENTYPE_NUMTYPES) {
		return numValues * genTypeDefs[type].size;
	} else {
		return 0;
	}
}

inline genu32 GenValueSet::GetSize(genu32 dataType, genu32 numElements) {
	if (dataType >= GENTYPE_NUMTYPES)
		return headerSize;

	return headerSize + numElements * genTypeDefs[dataType].size;
}

inline const GenValueSet* GenValueSet::GetValueSet(int index, bool doCheckValid) const {
	// Validity checks
	if (type != GENTYPE_VALUESET) {
		return nullptr;
	}

	if (doCheckValid) {
		if (GetNumValueSets() <= index) {
			return nullptr;
		}
	}
	
	// Start the search!
	genu32 byteIndex = 0;
	genu32 maxByteIndex = GetDataSize();
	GenValueSet* currentValueSet = (GenValueSet*)&byte[0];

	// Find the set at the given index
	for (int stateIndex = 0; stateIndex <= index; ++stateIndex) {
		// Check that the size of this alleged state fits
		if (byteIndex + currentValueSet->GetSize() > maxByteIndex) {
			return nullptr;
		}

		if (stateIndex == index) {
			// Found it!
			return currentValueSet;
		}

		// Advance to the next set
		byteIndex += currentValueSet->GetSize();
		currentValueSet = (GenValueSet*)(&byte[byteIndex]);
	}

	// Not found
	return nullptr;
}

int GenValueSet::GetNumValueSets() const {
	if (type != GENTYPE_VALUESET) {
		return 0;
	}

	// Start the search!
	genu32 byteIndex = 0;
	genu32 maxByteIndex = GetDataSize();
	GenValueSet* currentValueSet = (GenValueSet*)&byte[0];
	int numSetsCounted = 0;

	// Find the set at the given index
	while (byteIndex < maxByteIndex) {
		// Check that the size of this alleged state fits
		if (byteIndex + currentValueSet->GetSize() > maxByteIndex) {
			return 0;
		}

		// Advance to the next set
		byteIndex += currentValueSet->GetSize();
		currentValueSet = (GenValueSet*)(&byte[byteIndex]);
		++numSetsCounted;
	}

	if (byteIndex == maxByteIndex) {
		return numSetsCounted;
	} else {
		// Last state wasn't aligned with the end of the state data--possible corruption
		return 0;
	}
}

inline GenProp* GenValueSet::GetProp() {
	if (type == GENTYPE_PROP) {
		return (GenProp*)byte;
	} else {
		return nullptr;
	}
}

inline const GenProp* GenValueSet::GetProp() const {
	if (type == GENTYPE_PROP) {
		return (GenProp*)byte;
	} else {
		return nullptr;
	}
}

inline bool GenValueSet::Compare(const GenValueSet& other) const {
	if (numValues != other.numValues || type != other.type)
		return false;

	if (type >= GENTYPE_NUMTYPES)
		return false;

	for (int i = 0, e = (numValues * genTypeDefs[type].size) / 4; i < e; i++) {
		if (u32[i] != other.u32[i])
			return false;
	}

	for (int i = (numValues * genTypeDefs[type].size) / 4 * 4, e = (numValues * genTypeDefs[type].size); i < e; i++) {
		if (u8[i] != other.u8[i])
			return false;
	}

	return true;
}

inline bool GenValueSet::Set(genu32 index, const GenValue& value) {
	switch (type) {
		case GENTYPE_U8: case GENTYPE_S8: case GENTYPE_BOOL:
			u8[index] = value.uValue;
			return true;
		case GENTYPE_U16: case GENTYPE_S16:
			u16[index] = value.uValue;
			return true;
		case GENTYPE_U32: case GENTYPE_S32:
			u32[index] = value.uValue;
			return true;
		case GENTYPE_F32:
			f32[index] = value.fValue;
			return true;
		case GENTYPE_VEC3:
			vec3[index] = value.vec3Value;
			return true;
	}
	return false;
}

inline bool GenValueSet::Get(genu32 index, GenValue* value) const {
	switch (type) {
		case GENTYPE_U8: case GENTYPE_BOOL:
			value->uValue = u8[index];
			return true;
		case GENTYPE_S8: 
			value->sValue = s8[index];
			return true;
		case GENTYPE_U16:
			value->uValue = u16[index];
			return true;
		case GENTYPE_S16:
			value->sValue = s16[index];
			return true;
		case GENTYPE_U32:
			value->uValue = u32[index];
			return true;
		case GENTYPE_S32:
			value->sValue = s32[index];
			return true;
		case GENTYPE_F32:
			value->fValue = f32[index];
			return true;
		case GENTYPE_VEC3:
			value->vec3Value = vec3[index];
			return true;
		default:
			return false;
	}
}

inline void GenValueSet::Insert(genu32 index, const GenValue& value) {
	switch (type) {
		case GENTYPE_U8: case GENTYPE_S8: case GENTYPE_BOOL:
			for (genu32 i = ++numValues; i-- > index; )
				u8[i] = u8[i - 1];

			u8[index] = value.uValue;
			break;
		case GENTYPE_U16: case GENTYPE_S16:
			for (genu32 i = ++numValues; i-- > index; )
				u16[i] = u16[i - 1];

			u16[index] = value.uValue;
			break;
		case GENTYPE_U32: case GENTYPE_S32:
			for (genu32 i = ++numValues; i-- > index; )
				u32[i] = u32[i - 1];

			u32[index] = value.uValue;
			break;
		case GENTYPE_F32:
			for (genu32 i = ++numValues; i-- > index; )
				f32[i] = f32[i - 1];

			f32[index] = value.fValue;
			break;
		case GENTYPE_VEC3:
			for (genu32 i = ++numValues; i-- > index; )
				vec3[i] = vec3[i - 1];

			vec3[index] = value.vec3Value;
			break;
		default:
			return;
	}
}

inline void GenValueSet::Remove(genu32 index) {
	if (index >= numValues)
		return;

	numValues--;
	for (genu32 i = index * genTypeDefs[type].size; i < numValues * genTypeDefs[type].size; i++)
		u8[i] = u8[i + genTypeDefs[type].size];
}

inline bool GenValueSet::Copy(const GenValueSet& other) {
	if (other.type >= GENTYPE_NUMTYPES)
		return false;

	type = other.type;
	numValues = other.numValues;
	
	for (genu32 i = 0, e = (numValues * genTypeDefs[type].size) / 4; i < e; i++)
		u32[i] = other.u32[i];
	for (genu32 i = (numValues * genTypeDefs[type].size) / 4 * 4; i < numValues * genTypeDefs[type].size; i++)
		u8[i] = other.u8[i];
	return true;
}

inline GenValueSet* GenMixedValueSet::GetElement(genu32 index) {
	return elements[index];
}

inline bool GenProp::Compare(const GenProp& other) const {
	for (int i = 0; i < maxNameLength; i++) {
		if (name[i] != other.name[i])
			return false;
		if (!name[i])
			break;
	}

	return value.Compare(other.value);
}

inline bool GenProp::CompareName(const char* checkName) const {
	for (int j = 0; j < maxNameLength; j++) {
		if (checkName[j] != name[j]) {
			return false;
		}

		if (!name[j]) {
			return true;
		}
	}

	return false;
}

inline GenProp::GenProp(const char* name, const struct GenValueSet& value) {
	strcpy(this->name, name);
	this->value.Copy(value);
}

inline void GenProp::SetName(const char* newName) {
	int j;

	for (j = 0; j < maxNameLength - 1 && newName[j] != '\0'; j++)
		name[j] = newName[j];

	name[j] = '\0';
}

inline void GenProp::Copy(const GenProp& other) {
	for (int i = 0; i < maxNameLength; i++)
		name[i] = other.name[i];
	value.Copy(other.value);
}

inline int GenProp::GetSize() const {
	return maxNameLength + value.GetSize();
}

inline int GenProp::GetSize(genu32 dataType, genu32 numElements) {
	return maxNameLength + GenValueSet::GetSize(dataType, numElements);
}