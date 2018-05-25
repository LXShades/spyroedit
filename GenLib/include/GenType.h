#pragma once
// GEN: Unified data format used by Genesis states, files, and LiveGenesis.
// This can be used by a non-SD application as a library for LiveGenesis and also for loading and saving GEN files

#define GENINLINE inline

#define GENTYPEVERSION 1
#define GENPROPTYPEVERSION 1

#define GENPROPNAMELENGTH 16

#define GENID_NULL ((genid)0x00000000) // 0 is a reserved ID for miscellaneous uses such as an error code. This ID should NOT be used by objects.

// Base platform-independent types (or that's the goal!)
typedef unsigned __int8 genu8;
typedef unsigned __int16 genu16;
typedef unsigned __int32 genu32;
typedef unsigned __int64 genu64;
typedef __int8 gens8;
typedef __int16 gens16;
typedef __int32 gens32;
typedef __int64 gens64;
typedef float genf32;
typedef double genf64;
typedef genu8 genbool8;

typedef char genchar;
typedef wchar_t genwchar;

typedef gens32 genindex;


#ifdef _M_64
typedef gdu64 gduintptr;
#else
typedef genu32 genuintptr;
#endif

// Global ID: object/instance/etc IDs. All types of object ID share the same type of data; the name is purely for the sake of more descriptive variable types
typedef genu32 genid;
typedef genu16 gensubid;
typedef genid genmodelid;
typedef genid genmeshid;
typedef genid geninstid;
typedef genid genmodid;
typedef genid genmatid;
typedef genid genanimid;

// Object type typedef: sized variable guaranteed to hold all possible object types including GENOBJ_UNKNOWN
typedef gens8 genobjtype;

// Revision numbers
typedef genu32 genrevnum;

// Timestamps, for animations
typedef genu32 genframetime;
typedef genu32 gentimestamp;

// Validflags for validation
typedef genu32 genvalidflags;

// Genesis data types
enum GenType : genu32 {
	// NOTE: On adding data types remember to update genElementSizeTable!
	// Data structs
	GENTYPE_RAW = 0, // raw bytes (variable size, numElements = size in bytes)
	GENTYPE_STATE, // embedded GenState (variable size, numElements = size in bytes)
	GENTYPE_CSTRING, // char string (nullptr-terminated)/gds8/gdu8 (variable size, numElements = number of chars incl terminator (1 byte each))
	GENTYPE_WSTRING, // wide-char string (nullptr-terminated)/gds16/gdu16 (variable size, numElements = number of wide chars incl terminator (2 bytes each))
	GENTYPE_VEC2, // GenVec2
	GENTYPE_VEC3, // GenVec3
	GENTYPE_TRANSFORM, // GenTransform
	GENTYPE_PROP, // GenProp (variable size, use GenElementsEx)

	// ID
	GENTYPE_ID, // genid

	// Floats
	GENTYPE_F32, // gdf32
	GENTYPE_F64, // gdf64

	// Unsigned integers
	GENTYPE_U8, // gdu8
	GENTYPE_U16, // gdu16
	GENTYPE_U32, // gdu32
	GENTYPE_U64, // gdu64

	// Signed integers
	GENTYPE_S8, // gds8
	GENTYPE_S16, // gds16
	GENTYPE_S32, // gds32
	GENTYPE_S64, // gds64

	// Booleans
	GENTYPE_BOOL, // gdbool8
	GENTYPE_BOOLBITS, // gdu8 (bit mask)

	// Trigger
	GENTYPE_TRIGGER, // gdu8

	GENTYPE_NUMTYPES,

	GENTYPE_UNKNOWN = 0xFFFFFFFF,
};

struct GenTypeDef {
	const char*const name; // type name
	genu32 size; // size per type or 0 if variable/undefined
};

// Genesis struct types -----
#pragma pack(push, 1)

// GenVec[n] stuff
struct GenVec2 {
	genf32 x, y;

	inline void Set(float x, float y) {
		this->x = x; this->y = y;
	}

	inline void Zero() {
		x = 0.0f; y = 0.0f;
	}

	inline void One() {
		x = 1.0f; y = 1.0f;
	}
};

struct GenVec3 {
	genf32 x, y, z;

	GENINLINE GenVec3() = default;
	GENINLINE GenVec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};

	GENINLINE void Set(float x, float y, float z) {
		this->x = x; this->y = y; this->z = z;
	}

	GENINLINE void Zero() {
		x = 0.0f; y = 0.0f; z = 0.0f;
	}

	GENINLINE void One() {
		x = 1.0f; y = 1.0f; z = 1.0f;
	}

	GENINLINE const GenVec3& operator+=(const GenVec3& vec) {
		x += vec.x; y += vec.y; z += vec.z;
		return *this;
	}

	GENINLINE const GenVec3& operator-=(const GenVec3& vec) {
		x -= vec.x; y -= vec.y; z -= vec.z;
		return *this;
	}

	GENINLINE const GenVec3& operator*=(const float factor) {
		x *= factor; y *= factor; z *= factor;
		return *this;
	}
};

// GenTransform stuff
struct GenTransform {
	GenVec3 pivotTranslation;
	GenVec3 pivotRotation;
	GenVec3 pivotScale;
	GenVec3 localTranslation;
	GenVec3 localRotation;
	GenVec3 localScale;

	inline void Reset() {
		pivotTranslation.Zero();
		pivotRotation.Zero();
		pivotScale.One();
		localTranslation.Zero();
		localRotation.Zero();
		localScale.One();
	}
};

// GenValue: single Gen Value that can be used as a helper object for setting values
struct GenValue {
	GenType type;

	union {
		genu32 uValue;
		gens32 sValue;
		genf32 fValue;
		genu64 uValue64;
		gens64 sValue64;
		genid idValue;
		GenVec3 vec3Value;
	};

	// Constructors
	GenValue() = default;
	GenValue(genu32 value) : uValue(value), type(GENTYPE_U32) {};
	GenValue(gens32 value) : sValue(value), type(GENTYPE_S32) {};
	GenValue(genf32 value) : fValue(value), type(GENTYPE_F32) {};
	GenValue(genu64 value) : uValue64(value), type(GENTYPE_U64) {};
	GenValue(gens64 value) : sValue64(value), type(GENTYPE_S64) {};
	GenValue(const GenVec3& value) : vec3Value(value), type(GENTYPE_VEC3) {};
};

// GenElements: array of multiple values of any GenType
#define GENELEMENTHEADERSIZE ((uintptr) &((GenElements*)nullptr)->raw)
struct GenProp;
struct GenElements {
	// note: keeping header 4-byte-aligned for now, so that the element data is accessed from an aligned base address, probably faster
	GenType type;
	genu32 numElements;
	// NOTE: If more vars are added, update GENELEMENTDATAHEADERSIZE!

	// Actual values below
	union {
		genu8 raw[1];
		gens8 s8[1]; genu8 u8[1];
		gens16 s16[1]; genu16 u16[1];
		gens32 s32[1]; genu32 u32[1];
		genf32 f32[1];
		genf64 f64[1];
		genbool8 bool8[1]; // tip: friendly reminder that these are all arrays. Don't make the mistake of checking if (elements->bool8)!
		GenVec3 vec3[1];
		GenVec2 vec2[1];
		GenTransform transform[1];
		genid id[1];
		genu8 trigger;
	};
	
	// Initialisers
	GenElements() {};
	GenElements(gens32 _iValue) : s32{_iValue} {};
	GenElements(genf32 _fValue) : f32{_fValue} {};

	// GetSize: Returns total (required) size of this struct + its element data
	GENINLINE genu32 GetSize() const;
	// GetSize/Static: Returns total required size of a struct + element data of the given properties
	static GENINLINE genu32 GetSize(genu32 dataType, genu32 numElements);
	// GetBaseSize: Returns the minimum size of a GenElements struct with no values
	static GENINLINE constexpr genu32 GetBaseSize() {return (genuintptr) &((GenElements*)nullptr)->raw;}

	GENINLINE bool Set(genu32 index, const GenValue& value);
	GENINLINE bool Get(genu32 index, GenValue* value) const;
	GENINLINE void Insert(genu32 index, const GenValue& value);
	GENINLINE void Remove(genu32 index);
	GENINLINE bool Copy(const GenElements& other);
	GENINLINE bool Compare(const GenElements& other) const;

	// GetString: Returns string of specified ID
	GENINLINE char* GetString(int id);
	GENINLINE int GetNumStrings();
};

// GenExElements: Self-managed version of GenElements with support for variable-size elements
struct GenExElement {
	genu32 size;
};

class GenExElements {
	public:
		GENINLINE GenExElements() {Create(GENTYPE_RAW);};
		GENINLINE GenExElements(GenType type) {Create(type);};
		GENINLINE ~GenExElements() {Destroy();};

		void Create(GenType type); // provided in case the class is dynamically allocated 
		void Destroy();
		
		void Clear();

		GENINLINE GenType GetType() const {return (GenType) type;};
		GENINLINE void SetType(GenType type) {this->type = type;};

		// ReadRawDirect: Read directly from supplied raw GenExElements data. The pointer must be valid during use of this struct.
		bool ReadRawDirect(const void* raw, genu32 rawSize);
		// ReadRawLoad: Initialises data based on raw data. This differs from ReadRawDirect as this produces a sustained copy of the data and is generally slower.

		GENINLINE const void* GetRaw() const {UpdateRaw(); return rawData;}
		GENINLINE genu32 GetRawSize() const {UpdateRaw(); return rawSize;};

		void* AddElement(genu32 size);
		void* AddElementDirect(GenExElement* data);

		void* GetElement(genu32 index);
		genu32 GetElementSize(genu32 index);

		GENINLINE genu32 GetNumElements() const {return numElements;};
	private:
		void UpdateRaw() const;

		GenType type;
		genu32 numElements;
		genbool8 isDirect;
		
		GenExElement** elements;

		mutable genu8* rawData;
		mutable genu32 rawSize;
		mutable genbool8 rawValid;
};

struct GenProp {
	char name[GENPROPNAMELENGTH]; // name of the property; equivalent to the control's tag
	GenElements value; // value of the property
	
	GENINLINE void Copy(const GenProp& other);
	GENINLINE bool Compare(const GenProp& other);

	// GetSize: Returns total (required) size of this struct + its element data
	GENINLINE int GetSize() const;
	// GetSize alternative: Returns total required size of a struct + element data of the given properties
	static GENINLINE int GetSize(genu32 dataType, genu32 numElements);
};

struct GenStatePropTag {
	GenStatePropTag() = default;
	GenStatePropTag(const char* str) {
		int i;
		for (i = 0; i < 64 && str[i]; i++)
			tag[i] = str[i];

		if (i < 64)
			tag[i] = '\0';
	};

	inline GenStatePropTag GetSub() const {
		for (int i = 0; i < 64 && tag[i]; i++) {
			if (tag[i] == '.')
				return GenStatePropTag(&tag[i + 1]);
		}

		return GenStatePropTag("");
	};

	inline bool CompareWhole(const char* string) const {
		for (int i = 0; i < 64; i++) {
			if (tag[i] == '\0' && string[i] == '\0')
				return true;
			if (tag[i] != string[i])
				return false;
		}

		return false; // ??
	}

	inline bool ComparePart(const char* string) const {
		for (int i = 0; i < 64; i++) {
			if (tag[i] == '.' && string[i] == '\0' || tag[i] == '\0' && string[i] == '\0')
				return true;
			if (tag[i] != string[i])
				return false;
		}

		return false; // ??
	}

	char tag[64];
};

// Basic list type
class GenListBase {
	public:
		inline GenListBase() : items(0), numItems(0), numAlloced(0) {};

		inline void Create();
		void Destroy();

		void* AddSized(genu32 size);
		void Remove(genu32 index);

		inline genu32 GetNum() const;

	protected:
		void** items;
		genu32 numItems;
		genu32 numAlloced;
};

template <class Type>
class GenList : public GenListBase {
	public:
		inline Type* operator[](genu32 index) {
			return (Type*) items[index];
		}

		inline const Type* operator[](genu32 index) const {
			return (Type*) items[index];
		}

		inline Type* Add() {
			return (Type*) AddSized(sizeof (Type));
		}

		inline Type* AddSized(genu32 size) {
			return (Type*) GenListBase::AddSized(size);
		}
};
#pragma pack(pop)

void* LgAlloc(int dataSize);
void* LgRealloc(void* data, int dataSize);
void LgFree(void* data);
void LgZero(void* data, int size);

const GenTypeDef genTypeDefs[] = {
	"GENTYPE_RAW", 1,
	"GENTYPE_STATE", 1,
	"GENTYPE_CSTRING", 1, "GENTYPE_WSTRING", 2,
	"GENTYPE_VEC2", sizeof (GenVec2), "GENTYPE_VEC3", sizeof (GenVec3),
	"GENTYPE_TRANSFORM", sizeof (GenTransform),
	"GENTYPE_PROP", 0,
	"GENTYPE_ID", sizeof (genid), 
	"GENTYPE_F32", 4, "GENTYPE_F64", 8,
	"GENTYPE_U8", 1, "GENTYPE_U16", 2, "GENTYPE_U32", 4, "GENTYPE_U64", 8, "GENTYPE_S8", 1, "GENTYPE_S16", 2, "GENTYPE_S32", 4, "GENTYPE_S64", 8,
	"GENTYPE_BOOL", 1, "GENTYPE_BOOLBITS", 1, "GENTYPE_TRIGGER", 1,
};

#ifdef _DEBUG
enum WarningNum {
	____ERRORNUM = 1 / (int)((sizeof(genTypeDefs) / sizeof(genTypeDefs[0]) == GENTYPE_NUMTYPES)),
};
#endif

GENINLINE genu32 GenElements::GetSize() const {
	if (type >= GENTYPE_NUMTYPES)
		return GetBaseSize();

	return GetBaseSize() + numElements * genTypeDefs[type].size;
}

GENINLINE genu32 GenElements::GetSize(genu32 dataType, genu32 numElements) {
	if (dataType >= GENTYPE_NUMTYPES)
		return GetBaseSize();

	return GetBaseSize() + numElements * genTypeDefs[dataType].size;
}

GENINLINE bool GenElements::Compare(const GenElements& other) const {
	if (numElements != other.numElements || type != other.type)
		return false;

	if (type >= GENTYPE_NUMTYPES)
		return false;

	for (int i = 0, e = (numElements * genTypeDefs[type].size) / 4; i < e; i++) {
		if (u32[i] != other.u32[i])
			return false;
	}

	for (int i = (numElements * genTypeDefs[type].size) / 4 * 4, e = (numElements * genTypeDefs[type].size); i < e; i++) {
		if (u8[i] != other.u8[i])
			return false;
	}

	return true;
}

GENINLINE bool GenElements::Set(genu32 index, const GenValue& value) {
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

GENINLINE bool GenElements::Get(genu32 index, GenValue* value) const {
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

GENINLINE void GenElements::Insert(genu32 index, const GenValue& value) {
	switch (type) {
		case GENTYPE_U8: case GENTYPE_S8: case GENTYPE_BOOL:
			for (genu32 i = ++numElements; i-- > index; )
				u8[i] = u8[i - 1];

			u8[index] = value.uValue;
			break;
		case GENTYPE_U16: case GENTYPE_S16:
			for (genu32 i = ++numElements; i-- > index; )
				u16[i] = u16[i - 1];

			u16[index] = value.uValue;
			break;
		case GENTYPE_U32: case GENTYPE_S32:
			for (genu32 i = ++numElements; i-- > index; )
				u32[i] = u32[i - 1];

			u32[index] = value.uValue;
			break;
		case GENTYPE_F32:
			for (genu32 i = ++numElements; i-- > index; )
				f32[i] = f32[i - 1];

			f32[index] = value.fValue;
			break;
		case GENTYPE_VEC3:
			for (genu32 i = ++numElements; i-- > index; )
				vec3[i] = vec3[i - 1];

			vec3[index] = value.vec3Value;
			break;
		default:
			return;
	}
}

GENINLINE void GenElements::Remove(genu32 index) {
	if (index >= numElements)
		return;

	numElements--;
	for (genu32 i = index * genTypeDefs[type].size; i < numElements * genTypeDefs[type].size; i++)
		u8[i] = u8[i + genTypeDefs[type].size];
}

GENINLINE bool GenElements::Copy(const GenElements& other) {
	if (other.type >= GENTYPE_NUMTYPES)
		return false;

	type = other.type;
	numElements = other.numElements;
	
	for (genu32 i = 0, e = (numElements * genTypeDefs[type].size) / 4; i < e; i++)
		u32[i] = other.u32[i];
	for (genu32 i = (numElements * genTypeDefs[type].size) / 4 * 4; i < numElements * genTypeDefs[type].size; i++)
		u8[i] = other.u8[i];
	return true;
}

GENINLINE bool GenProp::Compare(const GenProp& other) {
	for (int i = 0; i < GENPROPNAMELENGTH; i++) {
		if (name[i] != other.name[i])
			return false;
		if (!name[i])
			break;
	}

	return value.Compare(other.value);
}

GENINLINE void GenProp::Copy(const GenProp& other) {
	for (int i = 0; i < GENPROPNAMELENGTH; i++)
		name[i] = other.name[i];
	value.Copy(other.value);
}

GENINLINE int GenProp::GetSize() const {
	return GENPROPNAMELENGTH + value.GetSize();
}

GENINLINE int GenProp::GetSize(genu32 dataType, genu32 numElements) {
	return GENPROPNAMELENGTH + GenElements::GetSize(dataType, numElements);
}

inline void GenListBase::Create() {
	items = 0;
	numItems = 0;
	numAlloced = 0;
}

inline genu32 GenListBase::GetNum() const {
	return numItems;
}

#undef GENINLINE