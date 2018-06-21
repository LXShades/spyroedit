#pragma once
#include <windows.h>
#include <vector>
#include "Types.h"

struct CtrlButton {
	CtrlButton(HWND hwnd_, void (*onClick_)()) : hwnd(hwnd_), onClick(onClick_) {};

	void (*onClick)();
	HWND hwnd;
};

struct CtrlTextbox {
	CtrlTextbox(HWND hwnd_) : hwnd(hwnd_), text(nullptr) {};

	int GetAsInt();
	const char* GetAsText();
	void SetText(const char* text);

	HWND hwnd;
	char* text;
};

struct SpyroEditPage {
	SpyroEditPage(const char* name_) : name(name_), line(0), lineHeight(22), lineFlags(0), group(0), groupX(0), groupY(0), isActive(false) {};
	
	void AddLine(uint32 pgFlags = 0, uint32 lineHeight = 19);
	void AddGroup(const char* groupName);
	HWND AddControl(const char* ctrlClass, const char* ctrlText, uint32 ctrlFlags, int x, int width, int heightInLines = 1);
	CtrlButton* AddButton(const char* buttonText, int x, int width, void (*onClick)() = nullptr);
	CtrlTextbox* AddTextbox(const char* defaultText, int x, int width);

	HWND hwnd;
	const char* name;

	uint32 line;
	uint32 lineHeight;
	uint32 lineFlags;
	HWND group;
	uint32 groupX, groupY;

	std::vector<CtrlButton*> buttons;
	std::vector<CtrlTextbox*> textboxes;

	bool isActive;
};

extern HMODULE mainModule;

extern HWND hwndEditor;
extern HWND hwndVram;

extern HWND tab;
extern SpyroEditPage pageObjects, pageOnline, pagePowers, pageScene, pageGenesis, pageStatus;
extern SpyroEditPage* pages[];

// Textures
extern HWND checkbox_autoLoad;

// Object page
extern HWND object_id;
extern HWND button_goto;
extern HWND button_get, button_set;
extern HWND button_butterflies;
extern HWND button_spawncopy;
extern HWND button_next, button_prev;
extern HWND checkbox_autoupdate;
extern HWND checkbox_drag;
extern HWND anim_count;

// Online page
extern HWND edit_ip;
extern CtrlButton* button_join, *button_host;
extern HWND staticTransferStatus;

// Genesis page
extern HWND static_genCollisionStatus;
extern HWND static_genSceneStatus;

extern bool isGenesisPageValid;

void Startup();

void SetControlString(HWND control, const char* string);

void CreateObjectPage();
void CreatePowersPage();
void CreateOnlinePage();
void CreateTexturesPage();
void CreateStatusPage();
void CreateGenesisPage();

void UpdateStatusPage();
void UpdateGenesisPage();