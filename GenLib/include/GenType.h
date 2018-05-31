#pragma once
// GEN: Unified data format used by Genesis states, files, and LiveGenesis.
// This can be used by a non-SD application as a library for LiveGenesis and also for loading and saving GEN files
#include <cstring> // temp?

#define GENTYPEVERSION 1
#define GENPROPTYPEVERSION 1

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
	// NOTE: On adding data types remember to update GenTypeDefs!
	// Data structs
	GENTYPE_RAW = 0, // raw bytes (numElements = size in bytes)
	GENTYPE_VALUESET, // embedded value set (numElements = size in bytes)
	GENTYPE_STATE, // embedded GenState (numElements = size in bytes)
	GENTYPE_CSTRING, // null-terminated char string gds8/gdu8 (numElements = number of chars incl terminator (1 byte each))
	GENTYPE_WSTRING, // null-terminated widechar string gds16/gdu16 (numElements = number of wide chars incl terminator (2 bytes each))
	GENTYPE_VEC2, // GenVec2
	GENTYPE_VEC3, // GenVec3
	GENTYPE_TRANSFORM, // GenTransform
	GENTYPE_PROP, // GenProp (numElements = size in bytes)

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

	inline GenVec3() = default;
	inline GenVec3(float magnitude) : x(magnitude), y(magnitude), z(magnitude) {};
	inline GenVec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {};

	inline void Set(float x, float y, float z) {
		this->x = x; this->y = y; this->z = z;
	}

	inline void Zero() {
		x = 0.0f; y = 0.0f; z = 0.0f;
	}

	inline void One() {
		x = 1.0f; y = 1.0f; z = 1.0f;
	}

	inline const GenVec3& operator+=(const GenVec3& vec) {
		x += vec.x; y += vec.y; z += vec.z;
		return *this;
	}

	inline const GenVec3& operator-=(const GenVec3& vec) {
		x -= vec.x; y -= vec.y; z -= vec.z;
		return *this;
	}

	inline const GenVec3& operator*=(const float factor) {
		x *= factor; y *= factor; z *= factor;
		return *this;
	}
};

// GenTransform stuff
struct GenTransform {
	GenTransform() = default;
	GenTransform(GenVec3 localTranslation_, GenVec3 localRotation_ = GenVec3(0.0f), GenVec3 localScale_ = GenVec3(1.0f), 
				 GenVec3 pivotTranslation_ = GenVec3(0.0f), GenVec3 pivotRotation_ = GenVec3(0.0f), GenVec3 pivotScale_ = GenVec3(1.0f)) : 
		localTranslation(localTranslation_), localRotation(localRotation_), localScale(localScale_), pivotTranslation(pivotTranslation_), pivotRotation(pivotRotation_),
		pivotScale(pivotScale_) {}

	// Transformation vectors
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
	"GENTYPE_VALUESET", 1,
	"GENTYPE_STATE", 1,
	"GENTYPE_CSTRING", 1, "GENTYPE_WSTRING", 2,
	"GENTYPE_VEC2", sizeof (GenVec2), "GENTYPE_VEC3", sizeof (GenVec3),
	"GENTYPE_TRANSFORM", sizeof (GenTransform),
	"GENTYPE_PROP", 1,
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

inline void GenListBase::Create() {
	items = 0;
	numItems = 0;
	numAlloced = 0;
}

inline genu32 GenListBase::GetNum() const {
	return numItems;
}

#undef inline