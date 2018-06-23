#pragma once
#include "sdTypes.h"

#define MAXCTRLTAGLENGTH 32

#include <Windows.h> // HFONT + HWND
#include <CommCtrl.h> // etc

#define ANCHORPACK(a_left, a_right, a_top, a_bottom) (((a_left) & 3) | (((a_right) & 3) << 2) | (((a_top) & 3) << 4) | (((a_bottom) & 3) << 6))
#define GETANCHORL(pack) ((pack) & 3)
#define GETANCHORR(pack) (((pack) & 12) >> 2)
#define GETANCHORT(pack) (((pack) & 48) >> 4)
#define GETANCHORB(pack) (((pack) & 192) >> 6)

#define SDEDITABLECONTROLS

class Window; struct CtrlEventParams; class Ctrl;
class CtrlEditor;

typedef bool(*ctrleventfunc)(CtrlEventParams* params);
typedef uint8 anchorpack;
typedef uint8 anchorpart;
const anchorpart ANCHORL = 0, ANCHORR = 1;
const anchorpart ANCHORT = 0, ANCHORB = 1;
const anchorpart ANCHORSCALER = 2;

enum CtrlType : uint8 {
	CTRL_WINDOW = 0, // Window - technically not a control--but let's do this
	CTRL_GUI, // GUI container
	CTRL_GROUP, // group box

	CTRL_STATIC, // static text
	CTRL_SYSLINK, // static text with links
	CTRL_STATUS, // status bar

	CTRL_BUTTON, // button
	CTRL_EDIT, // text-editable box
	CTRL_NUMBER, // text-editable or scalable number --special type-- (not Windows default)

	CTRL_LISTBOX, // list
	CTRL_TREEVIEW, // list with expandable items
	CTRL_COMBOBOX, // drop-down list (typically)

	CTRL_TOOLBAR, // toolbar

	CTRL_COLOURPICKER,
	CTRL_NUMTYPES,
};

enum CtrlNumberType : uint8 { // CtrlNumber number type
	CN_INT,
	CN_UINT,
	CN_FLOAT,
};

enum CtrlEventTag : uint8 {
	EVT_NONE = 0, // no value

	// CtrlWindow
	EVT_CLOSE, // no value

	// CtrlButton
	EVT_PRESS, // no value

	// CtrlCombobox + CtrlListbox + CtrlTreeview
	EVT_SELCHANGE, // value = (uint32) index of new selection; in comboboxes and listboxes this is the 0-based index, in tree views this is the ID of the item

	// CtrlToolbar
	EVT_TOOLPRESS, // value = (uint32) command ID of the button pressed

	// CtrlColourPicker
	EVT_CLRCHANGE, // value = (uint32) new colour

	// CtrlNumber
	EVT_NUMBERCHANGED
};

struct CtrlDef {
	CtrlType type;
	const char* tag;

	const wchar* text;

	int32 x, y;
	int32 width, height;

	const char* anchorParent;
	anchorpack anchors;

	uint32 flags;
};

struct CtrlEventParams {
	Ctrl* ctrl; // pointer to affected control
	CtrlEventTag tag; // tag of enum CtrlEventTag
	union { // value union: extra value dependent on tag, may be one of various types:
		void *pValue;
		int32 iValue;
		uint32 uValue;
	};

	// Original Win32 message info associated with the event, if applicable (usually is)
	HWND wndHwnd; // HWND
	uint32 wndMsg; // message
	void* wndWparam; // wParam
	void* wndLparam; // lParam
	void* wndReturn; // [out]: value to return in WindowProc
};

/*
GUI Rules: 
 - Any GUI window should be able to be a standalone window. i.e. any GUI object can be a base if it wants to be.
 - The HWNDs of child controls are child HWNDs of the parent control's HWND. DID THAT MAKE SENSE
*/

// Controls
// These do not have constructors or destructors. Create and Destroy are used instead. However, the usual route to creating controls is with Window::AddControl.
class Ctrl {
	public:
		// Constructor: initialises values not initialised by Create()
		Ctrl() : hwndCtrl(nullptr), tag(""), absoluteX(0), absoluteY(0), subclassed(false), classProc(nullptr), eventHandler(nullptr), userData(nullptr), userDataSize(0),
				 hidden(false), selfSizing(false), anchorParent(""), anchorsActive(true), editor(nullptr) {};

		void Create(const CtrlDef* def, Ctrl* parent);
		void Destroy();

		void Update();
		void Render();

		// nullptr/type-replaceable functions
		// These are functions that can be optionally overloaded by Ctrl derivatees(?!)
		void CreateEx() {return;};
		void DestroyEx() {return;};
		void UpdateEx() {return;};
		void RenderEx() {return;};
		bool OnWinMessageEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* retValueOut) {return true;} // return true = allow default action to take place

		// Getters/Setters
		HWND GetHwnd() const; // This isn't really const. But I can't return const HWNDs. THIS SUCKS

		CtrlType GetType() const;

		const char* GetTag() const;
		void SetTag(const char* tag);
		
		uint32 GetFlags() const; // gets the flags originally used to create the control

		// Parent control functions
		const Ctrl* GetParentCtrl() const;
		Ctrl* GetParentCtrl();

		// GetTopParentCtrl(): Returns the top-level parent of this control. If this control is at the top level, itself is returned.
		const Ctrl* GetTopParentCtrl() const;
		Ctrl* GetTopParentCtrl();

		// Child control functions
		Ctrl* AddControl(CtrlType type, const char* tag, const wchar* text, int x, int y, int width, int height, const char* anchorParent, anchorpack anchors, uint32 flags);
		bool AddControls(const CtrlDef* ctrlDefs, int numCtrlDefs);
		bool RemoveControl(const char* tag);
		bool RemoveControlByIndex(int index); // note: why is this by ID? use tags instead

		void ClearControls();

		// GetControl: Returns child control by tag. This will also search children of child controls, etc, if immediate child is not found
		template <typename CtrlType> inline CtrlType* GetControl(const char* tag) {
			return (CtrlType*) GetControl(tag); // currently no typechecking, may change in the future?
		}

		Ctrl* GetControl(const char* tag);
		Ctrl* GetControlByIndex(int index);
		
		int GetNumControls() const;

		// Text and font
		const wchar* GetText() const;
		void SetText(const wchar* text);

		HFONT GetFont();

		// Dimensions
		void GetDimensions(Rect32* out) const;
		void SetDimensions(const Rect32* in);

		void GetBaseDimensions(Rect32* dimensionsOut) const;
		void SetBaseDimensions(const Rect32* dimensionsIn);

		void GetParentBaseDimensions(Rect32* dimensionsOut) const;
		void SetParentBaseDimensions(const Rect32* dimensionsIn);

		void UpdateDimensions();

		// Dimensions (shortcuts)
		int GetX() const, GetY() const;
		int GetWidth() const, GetHeight() const;

		// Anchors
		anchorpack GetAnchors() const;
		void SetAnchors(anchorpack anchors);

		void EnableAnchors(bool enable);

		const char* GetAnchorParent() const;
		void SetAnchorParent(const char* anchorParentTag);

		bool IsSelfSizing() const;

		// Inputs
		int GetMouseX() const;
		int GetMouseY() const;

		bool HasFocus(bool checkChildren = false, bool checkParents = false) const;

		// Feedback
		void SetCursor(LPCWSTR str) const;

		// User events
		void SetEventHandler(ctrleventfunc eventFunc);
		ctrleventfunc GetEventHandler(bool includeParents) const;

		bool CallEvent(CtrlEventTag tag, void* pValue);

		// User extra
		void SetExtraData(const void* data, int dataSize);
		void* GetExtraData() const;
		int GetExtraDataSize() const;

		// Debug
#ifdef SDEDITABLECONTROLS
		void SetEditMode(bool enable);
#endif

		// System events+responses, dunno what to even call these
		void OnChildDestroy(Ctrl* child);

		bool OnWindowAttach(Window* parent);
		bool OnWindowDetach(Window* parent);

		LRESULT OnWinMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		// Win32 stuff that should maybe be internal?
		void CreateWin32Stuff();
		void DestroyWin32Stuff();

		// Misc/statics
		// ControlTypeToSize: Returns byte size of a control class based on 'type'
		static int ControlTypeToSize(CtrlType type);
		static const wchar* ControlTypeToString(CtrlType type);

		// GetTextDimensions: Gets pixel dimensions of a string of text written with the given font, or default font if none supplied
		static bool GetTextDimensions(Rect32* dimsOut, const wchar* textIn, HFONT fontIn = nullptr);
	protected:
		bool RelayDefaultEvents(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, CtrlEventParams& params);

		Ctrl* parent; // Parent control or nullptr if N/A

		HWND hwndCtrl; // Windows control or window

		CtrlType type; // Type of control (CTRL_)
		char tag[MAXCTRLTAGLENGTH]; // Control 'tag', a short text-based reference for the control

		mutable String text; // Text associated with the control--typically the title or input (mutable because GetString can revalidate it)

		int32 x, y, width, height; // Dimensions
		int32 absoluteX, absoluteY; // X and Y relative to the top window
		int32 originalX, originalY, originalWidth, originalHeight; // Original unadjusted dimensions
		int32 parentInitX, parentInitY, parentInitWidth, parentInitHeight; // Initial dimensions of the parent window used for anchors adjustment

		char anchorParent[MAXCTRLTAGLENGTH]; // Anchor parent, by tag. Anchor parent must be owned by the parent control. A blank or invalid anchor parent uses the base control instead.
		anchorpart anchorL, anchorR, anchorT, anchorB; // Anchor parts
		bool8 anchorsActive; // Whether the anchors will actively resize the controls and stuff. Sometimes disabled for debugging

		uint32 flags; // Type-specific flags

		bool8 subclassed; // Whether this control is subclassed
		WNDPROC classProc; // Original control wndproc

		ctrleventfunc eventHandler; // User-defined event handler; nullptr by default
		void* userData; // User-set extra/misc data unaffected by SD
		int32 userDataSize; // Size of the user data

		// States
		bool8 hidden;

		bool8 selfSizing; // True while the control is resizing itself by its anchors

		Array<Ctrl*> controls; // Child controls

		// Debug states
#ifdef SDEDITABLECONTROLS
		CtrlEditor* editor;
#endif
};

class CtrlGui : public Ctrl {
};

class CtrlGroup : public Ctrl {
};

class CtrlStatic : public Ctrl {
};

class CtrlSysLink : public Ctrl {
};

class CtrlStatus : public Ctrl {
	public:
		void GetStatus(int section, wchar* statusOut);
		int  GetStatusLength(int section);

		void SetStatus(int section, const wchar* status);

		void SetParts(int numParts, const int* partSizes);
};

class CtrlButton : public Ctrl {
	public:
		CtrlButton() : pressedState(0), numPressed(0) {};

		bool GetPressed();
		bool PeekPressed();

		bool GetToggled() const; // for togglable buttons
		void SetToggled(bool toggle);
	private:
		uint8 pressedState;

		uint8 numPressed;
};

class CtrlEdit : public Ctrl {
};

class CtrlNumber : public Ctrl {
	public:
		CtrlNumber() : numberType(CN_INT), hwndSlider(0), sliderPressed(false), constrained(false), sliderOriginalWndProc(0) {};

		void SetNumberType(CtrlNumberType type);

		void UpdateEx();
		void DestroyEx();

		LRESULT OnSliderWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

		float GetValueF() const;
		int GetValueI() const;
		unsigned int GetValueU() const;
		void SetValueF(float value);
		void SetValueI(int value);
		void SetValueU(unsigned int value);

		void SetConstrained(bool constrained); // if unconstrained, min/max is unlimited
		void SetMinMaxF(float min, float max); // sets min/max range and sets constrained to true
		void SetMinMaxI(int min, int max); // ditto
	private:
		CtrlNumberType numberType;

		HWND hwndSlider;
		bool8 sliderPressed;

		int32 sliderStartMousePos;
		int32 sliderLastMousePos;
		int64 sliderStartValueI;
		float32 sliderStartValueF;
		
		bool8 constrained;
		int32 minValueI, maxValueI;
		float32 minValueF, maxValueF;

		LONG sliderOriginalWndProc;
};

class CtrlListbox : public Ctrl {
	public:
		int AddItem(const wchar* itemString);
		void ClearItems();
		int GetNumItems() const;

		void SetSel(int index);
		int GetSel() const;

		void SetItemData(int index, void* data);
		void* GetItemData(int index);
};

class CtrlTreeview : public Ctrl {
	public:
		int AddItem(int id, const wchar* itemString, uintptr miscData = 0); // Adds item to either the list (id=0) or to another item (id=id of the item to add to) and returns an ID
};

class CtrlCombobox : public Ctrl {
	public:
		int AddItem(const wchar* itemString);
		void ClearItems();

		void SetSel(int index);
		int GetSel() const;
};

class CtrlToolbar : public Ctrl {
	public:
		bool AddButtons(const TBBUTTON* buttons, int numButtons); // TB_ADDBUTTONS
		
		uint32 GetButtonState(int buttonCmdId) const;
		void SetButtonState(int buttonCmdId, int state);

		void PressButton(int buttonCmdId, bool press); // TB_PRESSBUTTON, used for check buttons and grouping etc

		void SetImageList(int id, const HIMAGELIST list); // TB_SETIMAGELIST
		void LoadDefaultImages(int database); // TB_LOADIMAGES
};

class CtrlColourPicker : public Ctrl {
	public:
		CtrlColourPicker() : colour(0), pressed(false) {};

		bool OnWinMessageEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* retValueOut);

		void SetColour(uint32 colour);
		uint32 GetColour() const;
	private:
		uint32 colour;

		bool8 pressed;
};

typedef void(Ctrl::*exfunc)();
typedef bool(Ctrl::*winmsgfunc)(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* retValueOut);

struct CtrlTypeDef {
	int32 classSize;
	const wchar* name;
	const wchar* winClass;
	uint32 winDefaultStyleFlags;

	exfunc createEx; // called after create
	exfunc destroyEx; // called before destroy
	exfunc updateEx; // called after update
	exfunc renderEx; // called after render
	winmsgfunc onWinMessageEx; // called after user-overridden event handler and before Ctrl default event handler

	Ctrl* (*instantiator)();
};

extern const CtrlTypeDef ctrlTypeDefs[CTRL_NUMTYPES];