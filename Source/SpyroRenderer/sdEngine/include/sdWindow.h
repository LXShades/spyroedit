#pragma once
#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "sdMenu.h"
#include "sdGui.h"
#include "sdTypes.h"

class Window; class Menu;

/* Window - special type of Ctrl. Other than that it's basically a window.
You can use either Window::Create or Ctrl::Create to create a window, ditto with Destroy, so long as you don't use both. */
class Window : public Ctrl {
	public:
		Window() : Ctrl(), init(false), menu(nullptr), mouseScrollDelta(0.0f), winloopMouseScrollDelta(0.0f), altIgnore(false) {};

		void Create(const wchar* title, int width, int height, bool createHidden = false);
		void Destroy();

		void UpdateEx();

		bool OnWinMessageEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* retOut);

		bool IsInitialised();

		float GetMouseScroll() const;

		void SetName(const tchar* name);

		// Menus
		Menu* GetMenu();
		void SetMenu(Menu* menu, bool update = true);
		void UpdateMenu();

		void SetHidden(bool hidden);
		bool GetHidden() const;

		void SetAltIgnore(bool ignoreAlt); // causes the alt key to be ignored, so window focus isn't stolen

		// etc
		void Redraw();

	private:
		bool8 init;
		
		Menu* menu; // Attached menu; nullptr on init

		float32 mouseScrollDelta; // misc: mouse scroll delta
		float32 winloopMouseScrollDelta; // Mouse scroll delta collected from the windows loop. Sets mouseScrollDelta on Update() and is then reset.

		bool8 altIgnore; // misc: ignore alt key
};

