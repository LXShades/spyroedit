#pragma once
#include "GenType.h"

#define GENSTATEVERSION 1

enum GenObjType : genu16;
struct GenStateTypeTranslator; struct GenStateFilter;
class GenState;

/* for GenState::PrintInfo/debugging */
typedef const genchar*(genstateidnamefunc)(genid id); // user-defined function to convert an ID to an ANSI debug name; used for debug printing

/* future compression notes 
Control byte: [compressionType][numLengthBytes] [lengthOfAffected]

0 = Raw: no compression in thie region
1 = RLE: same value x number of times
2 = relative along linear: offset off of a straight line gradient, e.g. average increase of +10 per byte give or take max 5 [gradient][origin][number]
3 = relative to last
4 = relative to centre
5 = pattern repeat (pattern of up to 8 bytes) which repeats itself more than twice
*/

// State subtypes (or 'state types' per object type)
// - GENCOMMON_MULTIPLE is a recurring subtype that is used to set or retrieve multiple states from an object.
//   On retrieval, all available states for the object are embedded into it. On set, any available embedded states will be set.
//   GENCOMMON_MULTIPLE states needn't contain all possible states for the object. However, on retrieval, the default behaviour is to retrieve everything available.
// - Event states generally shouldn't be saved or loaded, however they can be synced over LiveGen
// - As a general warning, event states shouldn't be added to GEN*_MULTIPLE states of the object's own self.
//   This is because an object could destroy itself, copy itself, etc while iteratively applying the states.
#define X(def) def,
#define X2(def, startOrEnd) def, startOrEnd = def,
#define X3(def, start, end) def, start = def, end = def,
enum GenStateType : gens16 {
#include "GenStateTypes.h"

	GENSTATE_UNKNOWN = -1,
};

enum GenStateTypeNums {
	GENCOMMON_NUMTYPES = GENCOMMON_END - GENCOMMON_START + 1,
	GENSCENE_NUMTYPES = GENSCENE_END - GENSCENE_START + 1,
	GENMESH_NUMTYPES = GENMESH_END - GENMESH_START + 1,
	GENMODEL_NUMTYPES = GENMODEL_END - GENMODEL_START + 1,
	GENMOD_NUMTYPES = GENMOD_END - GENMOD_START + 1,
	GENMAT_NUMTYPES = GENMAT_END - GENMAT_START + 1,
	GENANIM_NUMTYPES = GENANIM_END - GENANIM_START + 1,
	GENSTATE_NUMTYPES = GENSTATE_END,
};

#pragma pack(push,1)
struct GenStateInfo {
	genid id; // ID of the affected object, scene, etc.
	GenStateType type; // Type of state

	genu32 dataSize;
};

class GenStateData {
	public:
		GenStateInfo info;
		GenElements data;
	
		bool GetState(GenState* stateOut, int stateIndex) const; // Retrives an embedded state as a copy. stateOut must be initialised. Fails if no state is available
		const GenStateData* GetStateData(int stateIndex) const;
		GenStateData* GetStateData(int stateIndex);
		int GetNumStates() const; // Returns total number of embedded states, or 0 if this is not a valid embedded state type.

		inline GenType GetElementType() const {return (GenType) data.type;}
		GenObjType GetObjType() const;

		bool TranslateTypes(const GenStateTypeTranslator* translator);

		int CountFilteredStates(const GenStateFilter* filter) const; // Counts the number of states that would be filtered, returning the number
		int RemoveFilteredStates(const GenStateFilter* filter); // Removes filtered states and returns the state's total size difference

		static inline int GetSize(GenType elementType, genu32 numElements);
};
#pragma pack(pop)

// GenState: State data container for Genesis objects. This is used to save, load, transfer all kinds of object information and elements.
// Data is contained as either GenElements or embedded GenStates. GenStates can be obtained as writable GenState copies, or read directly from the GenStateData.
// When using data as GenElements, use the Lock function to both read the data and reallocate enough space for it at the same time.
// When using data as embedded states, use AddState, GetState, and GetNumStates. GetNumStates will return 0 if the data isn't state data.
// GenState has constructors and destructors but also provides equivalent Create and Destroy functions in case it's dynamically allocated.
// Destroy is safe to call multiple times after initialisation
// Note: As of 9/01/17, state data size will never be smaller than sizeof (GenStateInfo) + GenElements::GetSize(0, GENTYPE_RAW), in other words guaranteeing
//   that the type and number fields of the GenElements struct will be available
class GenState {
	public:
		// Auto creation/destruction
		GenState(); // init empty
		GenState(const GenState* srcCopy); // copy constructor
		GenState(const GenState& srcCopy); // reference-based copy constructor to avoid the dumb default C++ copy constructor
		GenState(genid id, GenStateType type);
		GenState(genid id, GenStateType type, const GenElements* initElementData); // init with a copy of elements
		~GenState();

		// Manual creation/destruction
		void Create(genid id, GenStateType type);
		void Create(genid id, GenStateType type, const GenElements* initElements);
		void Create(const GenState* srcCopy);
		void Create();
		void Destroy();

		// Element functions
		GenElements* Lock(); // Locks and returns (if available) element data without reallocating anything
		GenElements* Lock(GenType dataType, genu32 numElements); // Locks data with a size to contain elements of type and num.
		void Unlock(); // My OCD tells me this function needs a comment, my brain tells me it doesn't

		const GenElements* GetConst() const; // Returns a const pointer to elements without locking. For ease of use. Ensure the class isn't locked during this time.

		bool ClearData(); // Clears state elements (not state info). Must be unlocked.

		// Embedded-state functions. (Data must be UNLOCKED to use these)
		// Embedded States only work if this state is a) empty or b) contains only state elements
		bool AddState(const GenState* stateIn); // Copies and embeds 'stateIn' to this state
		int  AddStates(const GenState*const* sourceStates, int numStates); // Embeds states via a list of GenState pointers
		bool GetState(GenState* stateOut, int stateIndex) const; // Retrives an embedded state as a copy. stateOut must be initialised. Fails if no state is available
		const GenStateData* GetStateData(int embeddedStateIndex) const; // Retrieves an embedded state directly as a pointer. Returns nullptr if unavailable
		int GetNumStates() const; // Returns total number of embedded states, or 0 if this is not a valid embedded state type

		// State transferral
		// MoveFrom: Moves state data and info from another state object. The source state is zeroed out after this as two states cannot share the same data!
		bool MoveFrom(GenState* source);

		// Data conversion
		bool TranslateTypes(const GenStateTypeTranslator* translator); // Converts state type IDs

		int FilterStates(const GenStateFilter* filter); // Removes states flagged false in the GenStateFilter, returning the number of states removed. 0 means no change

		// Getters/setters
		inline genid GetId() const {return state->info.id;}
		inline genu32 GetSize() const {return state->info.dataSize;} // returns total size of embedded GenElements OR embedded states
		inline GenStateType GetType() const {return (GenStateType) state->info.type;}
		inline GenType GetElementType() const {return (GenType) state->data.type;}
		inline genu32 GetNumElements() const {return state->data.numElements;}

		GenObjType GetObjType() const; // Discovers this state's object type based on its state type. If GENCOMMON_MULTIPLE, this also searches embedded states
		inline void GetAllInfo(GenStateInfo* infoOut) const {*infoOut = state->info;}
		inline const GenStateData* GetStateData() const {return state;}

		inline void SetId(genid id) {state->info.id = id;}
		inline void SetType(GenStateType type) {state->info.type = type;}
		inline void SetInfo(const GenStateInfo* infoIn) {state->info.id = infoIn->id; state->info.type = infoIn->type;} // Warning: dataSize is ignored. Use Lock.
		inline void SetInfo(genid id, GenStateType type) {state->info.id = id; state->info.type = type;}
		
		// PrintInfo: detailed information about the state for debugging
		void PrintInfo(bool printChildren = true, int indentNum = 0, genstateidnamefunc idNameFunc = 0, bool firstPrint = true) const;

		// Static functions in case you don't have a GenState in hand
		static GenObjType GetObjType(GenStateType type); // Converts state type to its associated obj type. If you can, use the nonstatic version.
		static inline const char* GetTypeName(GenStateType type);

	private:
		GenStateData* state;
		GenStateData emptyState; // why yes, this is dumb, currently a hack to avoid having leftover global data before the program quits

		genbool8 locked;

		void SetStateDataSize(genu32 size);
};

struct GenStateTypeTranslator {
	// stateTypes and elementTypes are indexes that contain type IDs from an external source/format
	// e.g. if the local GENTYPE_U32 is 5 and the external one is 2, elementTypes[GENTYPE_U32] = 2
	gens16 stateTypes[GENSTATE_NUMTYPES];
	gens16 elementTypes[GENTYPE_NUMTYPES];

	inline void ResetToLocal() {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++)
			stateTypes[i] = i;
		for (int i = 0; i < GENTYPE_NUMTYPES; i++)
			elementTypes[i] = i;
	};

	inline void ResetToUnknown() {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++)
			stateTypes[i] = -1;
		for (int i = 0; i < GENTYPE_NUMTYPES; i++)
			elementTypes[i] = -1;
	};

	inline GenType ConvertType(GenType type) {
		for (int i = 0; i < GENTYPE_NUMTYPES; i++) {
			if (elementTypes[i] == type)
				return (GenType) i;
		}
		return GENTYPE_UNKNOWN;
	}

	inline GenStateType ConvertStateType(GenStateType type) {
		for (int i = 0; i < GENSTATE_NUMTYPES; i++) {
			if (stateTypes[i] == type)
				return (GenStateType) i;
		}
		return GENSTATE_UNKNOWN;
	}
};

struct GenStateChangeLog {
	genu32 flags[(GENSTATE_NUMTYPES + 31) / 32];

	inline bool GetChanged(GenStateType type) {
		return (flags[(int) type / 32] & (1 << (type & 31))) != 0;
	}

	inline void SetChanged(GenStateType type, bool changed) {
		flags[(int) type / 32] = changed ? (flags[(int) type / 32] | (1 << (type & 31))) : (flags[(int) type / 32] & ~(1 << (type & 31)));
	}
};

// GenStateFilter: Declares what is desired and what isn't desired for a GenState
// Used to decide which states to retrieve from objects, or which states to send/receive over LiveGen. Optimises bandwidth.
struct GenStateFilter {
	GenStateFilter() : enabledStates() {
		enabledStates[GENCOMMON_MULTIPLE / 32] |= GENCOMMON_MULTIPLE & 31; // GENCOMMON_MULTIPLE should always be allowed
	};

	genu32 enabledStates[(GENSTATE_NUMTYPES + 31) / 32];

	inline bool IsEnabled(GenStateType index) const {
		if (index < GENSTATE_NUMTYPES)
			return (enabledStates[index / 32] & (1 << (index & 31))) != 0;
		return false;
	}

	void EnableStates(int numStates, GenStateType firstState, ...);
	void DisableStates(int numStates, GenStateType firstState, ...);

	inline void EnableAll() {
		for (int i = 0; i < sizeof (enabledStates)/4; i++)
			enabledStates[i] = 0xFFFFFFFF;
	}

	inline void DisableAll() {
		for (int i = 0; i < sizeof (enabledStates)/4; i++)
			enabledStates[i] = 0;
		enabledStates[GENCOMMON_MULTIPLE / 32] |= GENCOMMON_MULTIPLE & 31;
	}
};

#undef X
#undef X2
#undef X3
#define X(def) #def,
#define X2(def, startOrEnd) #def,
#define X3(def, start, end) #def,
static const char* genStateTypeNames[] = {
#include "GenStateTypes.h"
};

inline const char* GenState::GetTypeName(GenStateType type) {
	if ((genu32) type < sizeof (genStateTypeNames) / sizeof (genStateTypeNames[0]))
		return genStateTypeNames[(int) type];
	else
		return "<genstate_unknown>";
}

inline int GenStateData::GetSize(GenType type, genu32 numElements) {
	return sizeof (GenStateInfo) + GenElements::GetSize(type, numElements);
}