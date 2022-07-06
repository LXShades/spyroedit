#pragma once
#include "sdTypes.h"

#include <windows.h>

#define WINMENUITEMOFFSET 256 // Menu item ID offset. I dunno, just in case Windows reserves some?...

struct MenuItem; struct MenuClickParams; struct MenuItemRef;
class Window; class Menu;

// Menu pool used: POOL_GUI
// Menu click functions are similar to Window event functions; they are assigned to a menu and activated when the menu item is clicked.
// Menu click functions can also be applied to children--either other menus or menu items. When this item or menu is clicked, its own click function overrides its parents'
// Like Windows menus, destroying a menu that contains other menus will DESTROY the contained menus as well.
typedef void (*mclickfunc)(MenuClickParams*);

struct MenuItem {
	String text; // item text/descriptor
	uint8 type; // item type (MenuItemType)
	uint8 state; // item state flags (MenuItemStateFlag)

	uint32 commandId; // unique command ID of this item

	mclickfunc onClick; // pointer to click event function or nullptr if unavailable

	Menu* menu; // if MIT_MENU, pointer to next menu; DO NOT MODIFY
	HMENU winMenu; // if MIT_MENU, pointer to the winMenu owning it; DO NOY MODIFY
	uint32 winIndex; // internal menu item ID; DO NOT MODIFY
};

class Menu {
	public:
		void Create(mclickfunc onClick = nullptr);
		void Destroy();

		void UpdateWinMenu();
		HMENU GetWinMenu(); // Retrieves windows HMENU. May call UpdateWinMenu.

		// Add menu(s)
		MenuItem* AddMenu(const wchar* name);

		// Add item(s)
		// Items with an empty or nullptr name will be created as separators
		MenuItem* AddItem(); // adds and returns an empty menu item
		MenuItem* AddItems(int numItems); // adds and returns an array of empty menu items

		// Retrieve items
		MenuItem* GetItemByIndex(int index);
		MenuItem* GetItemByWinIndex(int commandId);
		MenuItem* GetItemByCommandId(uint32 id);

		// Edit/update items
		bool SetItemCheck(int commandId, bool checked);

		// Base menu
		Menu* GetBaseMenu(); // Returns top-level menu, end of parent chain. Can return self.
		
		// Extra
		// OpenMenu: Opens the menu at a position, allowing the user to select an option. Closes when the user selects an option or clicks elsewhere.
		//			 clickParamsOut contains info on the item clicked, if any. If none are clicked, the command ID returned is -1.
		void OpenMenu(MenuClickParams* clickParamsOut, int displayX, int displayY, bool callClickEvent = false);

		// Events
		void CallClickEvents(MenuClickParams* params);

	private:
		int AddItemRef(Menu* menuPointer, int itemIndex); // Adds a new item ref and returns the command ID

		Menu* parent;

		Array<MenuItem> items;

		HMENU winMenu; // Windows menu. May be nullptr
		HMENU winPopup; // Windows... popup menu. May be nullptr
		List<MenuItemRef> winItemRefs;

		mclickfunc onClick; // Main OnClick event
};

enum MenuItemType {
	MIT_NORMAL = 0, // Normal clickable item. Calls event function
	MIT_MENU = 1, // Another menu MENUCEPTION
	MIT_SEPARATOR = 2, // Separator item
};

enum MenuItemStateFlag {
	MIS_CHECKED = 1, // checkbox is ticked
};

struct MenuItemRef {
	Menu* parentMenu;
	uint32 index;
};

struct MenuClickParams {
	Menu* callerMenu; // Menu affected, or nullptr if not applicable
	Window* callerWindow; // Window that owns the menu, or nullptr if not applicable
	MenuItem* item; // Pointer to the item, or nullptr if none/invalid
	uint16 itemWinIndex; // Command ID of the item, or FFFF if none/invalid
};