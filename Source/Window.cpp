#include "Window.h"
#include "ObjectEdit.h"
#include "SpyroData.h"
#include "Online.h"
#include "Main.h"
#include "SpyroTextures.h"
#include "SpyroScene.h"
#include "Powers.h"
#ifdef SPYRORENDER
#include "SpyroRenderer/SpyroRender.h"
#endif

#include <cstdio>
#include <cmath>
#include <commctrl.h>
#include <vector>

HMODULE mainModule;

HWND hwndEditor;
HWND hwndVram;
HWND hwndTexture;

// Texture window stuff
bool textureWindowOpen = false;
char textureDroppedFile[MAX_PATH] = "";
int textureHighlightedTex = -1;
int textureHighlightedPalette = -1;
int textureHighlightedTile = 0;

// Buttons and boxes
HWND tab;
SpyroEditPage pageObjects("Objects"), pageOnline("Online"), pagePowers("Powers"), pageScene("Mods"), pageGenesis("Genesis"), pageStatus("Status");
#ifdef SPYRORENDER
SpyroEditPage pageRender("Render");
#endif
SpyroEditPage* pages[] = {
	&pageScene, &pagePowers, &pageObjects, &pageGenesis, &pageOnline, &pageStatus, 
	#ifdef SPYRORENDER
		&pageRender
	#endif
};

enum SpyroEditPageTypes {
	PAGE_TEXTURES = 0, PAGE_POWERS, PAGE_OBJECTS, PAGE_GENESIS, PAGE_ONLINE, PAGE_STATUS, 
#ifdef SPYRORENDER
	PAGE_RENDER, 
#endif
	PAGE_NUMPAGES
};
const int numPages = PAGE_NUMPAGES;

// Object page
HWND object_id;
HWND button_goto;
HWND button_get, button_set;
HWND button_butterflies; // trivia: I called this button_butterflies to search specifically for secret butterfly jars. Now it just searches for anything near Spyro
HWND button_spawncopy;
HWND button_next, button_prev;
HWND checkbox_autoupdate;
HWND checkbox_drag;
HWND anim_count;

// Online page
HWND edit_ip;
CtrlButton* button_host, *button_join;
HWND staticTransferStatus;

// Powers page
HWND buttonPowers[32];
int g_numPowers = 0;
HWND	edit_headbash_range;
HWND	edit_headbash_depth;
HWND	edit_headbash_rimHeight;
HWND	edit_headbash_rimWidth;
HWND	edit_headbash_crushFactor;
HWND	edit_headbash_crushFactorOuter;

// Textures page
HWND button_editTextures;
HWND button_saveAll, button_loadAll;
HWND button_setColours, button_getColours, button_tweakSky;
HWND button_makeLight, button_makeFog, button_makeSky;
HWND checkbox_texSeparate, checkbox_texGenPalettes, checkbox_texShufflePalettes, checkbox_texAutoLq, checkbox_autoLoad;
CtrlTextbox* edit_fogR, *edit_fogG, *edit_fogB, *edit_lightR, *edit_lightG, *edit_lightB, *edit_skyR, *edit_skyG, *edit_skyB;

// Genesis page
HWND edit_genIp;
HWND checkbox_genDisableSceneOccl, checkbox_genDisableSkyOccl;
HWND static_genSceneStatus, static_genCollisionStatus;
HWND edit_coordsInX, edit_coordsInY, edit_coordsInZ, edit_coordsInFlag;
HWND edit_coordsOutHalfwordBlock, edit_coordsOutWordBlock, edit_coordsOutWord;
HWND static_coordsOutHalfwordBlock, static_coordsOutWordBlock, static_coordsOutWord;
HWND listbox_sceneSectorStatus;
HWND static_sectorSizes;

// Status page
struct StatusObject {
	HWND hwnd;
	const char* name;
	const uintptr pointer;
	bool8 absolute;
};

StatusObject statusObjects[] = {{NULL, "Spyro", (uintptr) &spyro}, {NULL, "Objects", (uintptr) &mobys}, {NULL, "Camera", (uintptr) &spyroCamera},
								{NULL, "Scene model", (uintptr) &scene.spyroScene.address}, {NULL, "Sky model", (uintptr) &spyroSky}, 
								{NULL, "Collision data", (uintptr) &spyroCollision.address}, 
								{NULL, "Object models", (uintptr) &mobyModels}, 
								{NULL, "Scene occlusion", (uintptr) &sceneOcclusion}, {NULL, "Sky occlusion", (uintptr) &skyOcclusion}, 
								{NULL, "Textures (Spyro 1)", (uintptr) &lqTextures}, {NULL, "Textures (Spyro 2/3)", (uintptr) &textures}, 
								{NULL, "Level ID", (uintptr) &level}, {NULL, "Level area ID (Spyro 3)", (uintptr) &levelArea}, {NULL, "Level names", (uintptr) &levelNames}, 
								{NULL, "Spyro model", (uintptr) &spyroModelPointer, true},
								{NULL, "Spyro draw func (multidragon)", (uintptr) &spyroDrawFuncAddress, true}, 
								{NULL, "Joker (controller)", (uintptr) &jokerPtr, true}};
HWND static_memVisualiser;
HWND static_memStatus;
HWND button_vramViewer;

void StartupNetwork();
void SendLiveGenScene(); void SendLiveGenCollision(); void SendLiveGenSpyro(); void SendLiveGenMobyModel(int model, int mobyId = -1); void SendLiveGenAllMobys();
bool ConnectLiveGen(); void RebuildCollisionTree(); void RebuildCollisionTriangles(); void ResetLiveGenScene(); void SaveMobys(); void LoadMobys();

void UpdateTextureWindow();

enum PageFlags {PGF_LEFT = 0, PGF_RIGHT = 1, PGF_CENTRE = 2, PGF_HCENTRE = 4, PGF_VCENTRE = 8};

uint32 GetControlHex(HWND control);
void SetControlHex(HWND control, uint32 value);
void SetControlInt(HWND control, int value);
int GetControlInt(HWND control);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// +0x1C (word): Unknown variable that must do something.
// +0x40 and +43 (bytes): Something to do with animation
// +0x41 (byte) IsAnimPlaying
// +0x47 (byte) Draw priority
// +0x48: State, 0x49 anim (maybe), 0x4a occlusion (related), 0x4b unknown
// +0x50, 0x51, 0x52: (bytes) Read when enemy is flamed.

bool isInit = false;

void Startup() {
	if (isInit) {
		return;
	}

	// Register main window classes
    WNDCLASSEX wc;

	InitCommonControls();

    wc.cbSize        = sizeof (WNDCLASSEX);
    wc.hInstance     = mainModule;
    wc.lpszClassName = "SpyroLiveEditor";
    wc.lpfnWndProc   = WindowProc;
    wc.style         = 0;
    wc.hIcon         = NULL;
    wc.hIconSm       = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName  = NULL;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE + 1;

    RegisterClassEx(&wc);

	if (!GetClassInfoEx(mainModule, "STATIC", &wc))
		MessageBox(NULL, "It didn't work anyway!", "Well duh..", MB_OK);

	wc.lpszClassName = "WindowProcStatic";
	wc.lpfnWndProc = WindowProc;
	wc.hbrBackground = (HBRUSH) COLOR_APPWORKSPACE + 1;

	RegisterClassEx(&wc);

	// Create main windows
	hwndEditor = CreateWindowEx(WS_EX_ACCEPTFILES, "SpyroLiveEditor", NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 340, 566, NULL, NULL, mainModule, NULL);
	hwndVram = CreateWindowEx(0, "SpyroLiveEditor", "VRAM window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 500, NULL, NULL, mainModule, NULL);
	hwndTexture = CreateWindowEx(WS_EX_ACCEPTFILES, "SpyroLiveEditor", "Textures", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 512, 512, NULL, NULL, mainModule, NULL);

	if (hwndEditor == NULL || hwndVram == NULL || hwndTexture == NULL)
		MessageBox(0, "Error creating main windows", "", MB_OK);

	// Create pages
	tab = CreateWindowEx(0, WC_TABCONTROL, "", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwndEditor, NULL, mainModule, NULL);

	TCITEM tci;
    tci.mask = TCIF_TEXT | TCIF_IMAGE;
    tci.iImage = -1;

	for (int i = 0; i < numPages; i++) {
		// Add page hwnd
		pages[i]->hwnd = CreateWindowEx(0, "WindowProcStatic", "", WS_CHILD, 0, 0, 0, 0, hwndEditor, NULL, mainModule, NULL);

		// Add tab
		tci.pszText = const_cast<char*>(pages[i]->name);
		SendMessage(tab, TCM_INSERTITEM, (WPARAM) i, (LPARAM) &tci);
	}

	SendMessage(tab, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);

	// Init pages
	CreateObjectPage();
	CreateOnlinePage();
	CreatePowersPage();
	CreateTexturesPage();
	CreateStatusPage();
	CreateGenesisPage();
#ifdef SPYRORENDER
	CreateRenderPage();
#endif

	SetControlString(staticTransferStatus, "100% 100% 100% 100%\n100% 100% 100% 100%");

	// Init online stuff
	StartupNetwork();

	isInit = false;
}

void Shutdown() {
	/*for (int i = 0; i < 50; i++)
		vardefs[i].type = 0;

	DestroyWindow(hwndEditor);
	DestroyWindow(hwndVram);
	DestroyWindow(hwndTexture);

	UnregisterClass("SpyroLiveEditor", mainModule);
	UnregisterClass("WindowProcStatic", mainModule);*/
}

void CreateOnlinePage() {
	edit_ip = pageOnline.AddControl("EDIT", "127.0.0.1", WS_BORDER, 83, 144);

	pageOnline.AddLine(PGF_CENTRE);
	button_host = pageOnline.AddButton("Host", 85, 54, &Host);
	button_join = pageOnline.AddButton("Join", 169, 54, &Join);

	pageOnline.AddLine(PGF_CENTRE);
	staticTransferStatus = pageOnline.AddControl("STATIC", "", SS_LEFT, 1, 300);
}

void	UpdateHeadbash() {
	char	new_headbash_setting[20];

	SendMessage(edit_headbash_range, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_range = atof(new_headbash_setting);
	SendMessage(edit_headbash_depth, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_depth = atof(new_headbash_setting);
	SendMessage(edit_headbash_rimHeight, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_rimHeight = atof(new_headbash_setting);
	SendMessage(edit_headbash_rimWidth, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_rimWidth = atof(new_headbash_setting);
	SendMessage(edit_headbash_crushFactor, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_crushFactor = atof(new_headbash_setting);
	SendMessage(edit_headbash_crushFactorOuter, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(new_headbash_setting));
	headbash_crushFactorOuter = atof(new_headbash_setting);
}
// HWND test_int;
// void	test() {
// 	char text[20];
// 	SendMessage(test_int, WM_GETTEXT, 20, reinterpret_cast<LPARAM>(text));
// 	MessageBox(NULL, text, "this the text:\n\r\f\r\t\v", MB_OK);
// }
// pagePowers.AddButton("button test", 10, 128, &test);
void CreatePowersPage() {
	const char* powerNames[] = {"Attraction", "Repulsion", "Gem Attraction", "Forcefield (protection)", "Super Headbash", "Ultra Headbash", "Headbashpocalypse", 
		"Butterfly Breath", "Death Stare", "Death Field", "Repulsion Stare", "Attraction Stare", "Exorcist (S3PAL)", "Tornado", "Telekinesis (S3PAL)", 
		"Followers", "Look at Stuff (S3PAL)", "Roll (glide+X+L/R) (S3)", "Sanic Jumps (S3)", "Irreparable Giraffe", "Irreparable Beats"};

	for (int i = 0; i < sizeof (powerNames) / sizeof (const char*); i++) {
		if (!(i&1) && i)
			pagePowers.AddLine(0, 17);

		buttonPowers[g_numPowers] = pagePowers.AddControl("BUTTON", powerNames[i], BS_AUTOCHECKBOX, 10 + (i & 1) * 150, 130);
		SendMessage(buttonPowers[g_numPowers], BM_SETCHECK, ((powers & (1 << i)) ? BST_CHECKED : BST_UNCHECKED), 0);

		g_numPowers++;
	}

	pagePowers.AddLine();
	pagePowers.AddGroup("Headbashpocalypse settings:");

	pagePowers.AddControl("STATIC", "Range:", 0, 10, 100);
	edit_headbash_range = pagePowers.AddControl("EDIT", headbash_range, WS_BORDER, 115, 40);
	pagePowers.AddControl("STATIC", "Depth:", 0, 160, 100);
	edit_headbash_depth = pagePowers.AddControl("EDIT", headbash_depth, WS_BORDER, 260, 40);
	pagePowers.AddLine();

	pagePowers.AddControl("STATIC", "Rim Height:", 0, 10, 100);
	edit_headbash_rimHeight = pagePowers.AddControl("EDIT", headbash_rimHeight, WS_BORDER, 115, 40);
	pagePowers.AddControl("STATIC", "Rim Width:", 0, 160, 100);
	edit_headbash_rimWidth = pagePowers.AddControl("EDIT", headbash_rimWidth, WS_BORDER, 260, 40);
	pagePowers.AddLine();

	pagePowers.AddControl("STATIC", "Crush Factor:", 0, 10, 100);
	edit_headbash_crushFactor = pagePowers.AddControl("EDIT", headbash_crushFactor, WS_BORDER, 115, 40);
	pagePowers.AddControl("STATIC", "Crush Factor Outer:", 0, 160, 100);
	edit_headbash_crushFactorOuter = pagePowers.AddControl("EDIT", headbash_crushFactorOuter, WS_BORDER, 260, 40);
	pagePowers.AddLine();

	pagePowers.AddButton("Update Heabshpocalypse settings", 30, 250, UpdateHeadbash);
}

void CreateTexturesPage() {
	pageScene.AddGroup("General");
	checkbox_autoLoad = pageScene.AddControl("BUTTON", "Automatically load mods upon level entry",  BS_AUTOCHECKBOX, 20, 260);

	pageScene.AddLine();
	pageScene.AddButton("Save All",  20, 128, &SaveAllMods);
	pageScene.AddButton("Load All", 164, 128, &LoadAllMods);

	pageScene.AddLine();
	pageScene.AddGroup("Textures");
	pageScene.AddButton("Save scene textures", 20, 128, &SaveTextures);
	pageScene.AddButton("Load scene textures", 164, 128, &LoadTextures);
	pageScene.AddLine();
	pageScene.AddButton("Save object textures", 20, 128, [](){SaveObjectTextures();});
	pageScene.AddButton("Load object textures", 164, 128, &LoadObjectTextures);
	pageScene.AddLine();
	checkbox_texGenPalettes = pageScene.AddControl("BUTTON", "Generate palettes", BS_AUTOCHECKBOX, 20, 112);
	checkbox_texShufflePalettes = pageScene.AddControl("BUTTON", "Shuffle palettes", BS_AUTOCHECKBOX, 126, 102);
	checkbox_texAutoLq = pageScene.AddControl("BUTTON", "Auto LQs", BS_AUTOCHECKBOX, 228, 72);
	pageScene.AddLine();
	checkbox_texSeparate = pageScene.AddControl("BUTTON", "Separate bitmaps", BS_AUTOCHECKBOX, 20, 102);
	pageScene.AddLine();
	pageScene.AddButton("Open viewer", 20, 164+128-20, [](){textureWindowOpen = true; ShowWindow(hwndTexture, true);});
	
	pageScene.AddLine();
	pageScene.AddGroup("Sky");
	pageScene.AddButton("Save sky", 20, 128, &SaveSky);
	pageScene.AddButton("Load sky", 164, 128, [](){LoadSky();});
	
	pageScene.AddLine();
	checkbox_genDisableSkyOccl = pageScene.AddControl("BUTTON", "Disable sky occlusion", BS_AUTOCHECKBOX, 20, 150);

	pageScene.AddLine();
	pageScene.AddGroup("Colours");
	pageScene.AddButton("Save colours", 20, 128, &SaveColours);
	pageScene.AddButton("Load colours", 164, 128, &LoadColours);

	pageScene.AddLine();
	pageScene.AddControl("STATIC", "Avg. fog:", 0, 10, 80);
	edit_fogR = pageScene.AddTextbox("127", 100, 30);
	edit_fogG = pageScene.AddTextbox("127", 140, 30);
	edit_fogB = pageScene.AddTextbox("127", 180, 30);
	button_makeFog = pageScene.AddControl("BUTTON", "Make...",  BS_PUSHBUTTON, 230, 55);

	pageScene.AddLine();
	pageScene.AddControl("STATIC", "Light tweak:", 0, 10, 80);
	edit_lightR = pageScene.AddTextbox("100", 100, 30);
	edit_lightG = pageScene.AddTextbox("100", 140, 30);
	edit_lightB = pageScene.AddTextbox("100", 180, 30);
	button_makeLight = pageScene.AddControl("BUTTON", "Make...",  BS_PUSHBUTTON, 230, 55);
	
	pageScene.AddLine();
	pageScene.AddControl("STATIC", "Sky tweak:", 0, 10, 80);
	edit_skyR = pageScene.AddTextbox("100",  100, 30);
	edit_skyG = pageScene.AddTextbox("100",  140, 30);
	edit_skyB = pageScene.AddTextbox("100",  180, 30);
	button_makeSky = pageScene.AddControl("BUTTON", "Make...",  BS_PUSHBUTTON, 230, 55);
	
	pageScene.AddLine();
	button_setColours = pageScene.AddControl("BUTTON", "Assign Level Colours",  BS_PUSHBUTTON, 20, 128);
	pageScene.AddButton("Assign Sky Colours",  164, 128, [](){TweakSky(edit_skyR->GetAsInt(), edit_skyG->GetAsInt(), edit_skyB->GetAsInt());});

	pageScene.AddLine();
	pageScene.AddControl("STATIC", "Note: Sky tweaks may result in colour loss!", 0, 10, 350);

	pageScene.AddLine();
	pageScene.AddGroup("Objects");
	pageScene.AddButton("Save object positions", 20, 128, &SaveMobys);
	pageScene.AddButton("Load object positions", 164, 128, &LoadMobys);

	pageScene.AddLine();
	pageScene.AddGroup("Goofy presets");

	pageScene.AddButton("Color-B-gone", 20, 128, &ColorlessMode);
	pageScene.AddButton("Gratuitous Pink", 164, 128, &PinkMode);
	pageScene.AddLine();
	pageScene.AddButton("Creepypasta Mode", 20, 128, &CreepypastaMode);
	pageScene.AddButton("Flat Palette Mode", 164, 128, &IndieMode);
	pageScene.AddLine();
	pageScene.AddControl("STATIC", "Note: Effects may result in colour loss!", 0, 10, 350);
	
	if (texEditFlags & TEF_SEPARATE)
		SendMessage(checkbox_texSeparate, BM_SETCHECK, BST_CHECKED, 0);
	if (texEditFlags & TEF_AUTOLQ)
		SendMessage(checkbox_texAutoLq, BM_SETCHECK, BST_CHECKED, 0);
	if (texEditFlags & TEF_GENERATEPALETTES)
		SendMessage(checkbox_texGenPalettes, BM_SETCHECK, BST_CHECKED, 0);
	if (texEditFlags & TEF_SHUFFLEPALETTES)
		SendMessage(checkbox_texShufflePalettes, BM_SETCHECK, BST_CHECKED, 0);
}

void CreateStatusPage() {
	for (int i = 0; i < sizeof (statusObjects) / sizeof (StatusObject); i++) {
		statusObjects[i].hwnd = pageStatus.AddControl("STATIC", statusObjects[i].name, 0, 10, 300);
		pageStatus.AddLine();
	}

	pageStatus.AddLine();
	static_memVisualiser = pageStatus.AddControl("STATIC", "", SS_OWNERDRAW, 10, 300);
	pageStatus.AddLine();
	static_memStatus = pageStatus.AddControl("STATIC", "<Memory not analysed>", SS_SIMPLE, 10, 300);

	pageStatus.AddLine();
	pageStatus.AddLine();
	button_vramViewer = pageStatus.AddControl("BUTTON", "Open VRAM viewer", BS_PUSHBUTTON, 10, 300);
}

void CreateGenesisPage() {
	pageGenesis.AddControl("STATIC", "IP address:", 0, 7, 53);

	edit_genIp = pageGenesis.AddControl("EDIT", "127.0.0.1",  WS_BORDER, 64, 120);

	pageGenesis.AddLine();
	pageGenesis.AddButton("Connect", 7, 177, [](){if (!ConnectLiveGen()) MessageBox(NULL, "LiveGen Connection failed", "Please don't hurt me but", MB_OK);});

	pageGenesis.AddLine();
	pageGenesis.AddButton("Send Scene", 7, 80, &SendLiveGenScene);
	pageGenesis.AddButton("Reset Scene",  95, 80, &ResetLiveGenScene);
	pageGenesis.AddLine();
	pageGenesis.AddButton("Send Spyro", 7, 80, &SendLiveGenSpyro);
	pageGenesis.AddButton("Send Collision", 95, 87, &SendLiveGenCollision);
	pageGenesis.AddLine();
	pageGenesis.AddButton("Send Object", 7, 80, [](){SendLiveGenMobyModel(GetObjectID() >= 0 ? mobys[GetObjectID()].type : 0, GetObjectID());});
	pageGenesis.AddButton("Send All Objects", 95, 80, &SendLiveGenAllMobys);
	pageGenesis.AddLine();
	pageGenesis.AddButton("Rebuild Colltree", 7, 80, &RebuildCollisionTriangles);
	pageGenesis.AddButton("Regen Colltris", 95, 80, &RebuildCollisionTree);

	pageGenesis.AddLine();
	checkbox_genDisableSceneOccl = pageGenesis.AddControl("BUTTON", "Disable scene occlusion", BS_AUTOCHECKBOX, 7, 160);

	pageGenesis.AddLine();
	static_genSceneStatus = pageGenesis.AddControl("STATIC", "Scene status", 0, 7, 182);
	pageGenesis.AddLine();
	static_genCollisionStatus = pageGenesis.AddControl("STATIC", "Coll status", 0, 7, 182);

	pageGenesis.AddLine();
	edit_coordsInX = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 87, 40);
	edit_coordsInY = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 131, 40);
	edit_coordsInZ = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 173, 40);
	edit_coordsInFlag = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 233, 20);

	pageGenesis.AddLine();
	static_coordsOutHalfwordBlock = pageGenesis.AddControl("STATIC", "Halfword Block:", 0, 7, 80);
	edit_coordsOutHalfwordBlock = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 87, 213-87);

	pageGenesis.AddLine();
	static_coordsOutWordBlock = pageGenesis.AddControl("STATIC", "Word Block:", 0, 7, 80);
	edit_coordsOutWordBlock = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 87, 213-87);

	pageGenesis.AddLine();
	static_coordsOutWord = pageGenesis.AddControl("STATIC", "Word:", 0, 7, 80);
	edit_coordsOutWord = pageGenesis.AddControl("EDIT", "0",  WS_BORDER, 87, 213-87);

	pageGenesis.AddLine();
	static_sectorSizes = pageGenesis.AddControl("STATIC", "", 0, 7, 300);
	pageGenesis.AddLine();
	listbox_sceneSectorStatus = pageGenesis.AddControl("LISTBOX", "Scene sector info", WS_BORDER | WS_VSCROLL, 0, 290, 10);
}

#ifdef SPYRORENDER
void CreateRenderPage() {
	pageRender.AddLine();
	pageRender.AddButton("Open Spyro Renderer", 90, 120, [](){SpyroRender::Open();});

	pageRender.AddLine();
	pageRender.AddButton("Reset Level Textures", 90, 120, [](){SpyroRender::UpdateTextures(true);});
}
#endif

void Open_Windows() {
	ShowWindow(hwndEditor, true);
	ShowWindow(hwndTexture, textureWindowOpen);
	UpdateWindow(hwndEditor);
}

void Close_Windows() {
	ShowWindow(hwndEditor, 0);
	ShowWindow(hwndTexture, 0);
	ShowWindow(hwndVram, 0);

#ifdef SPYRORENDER
	SpyroRender::Close();
#endif
}

void WinLoop() {
	MSG message;
	
	// Show the cursor
	ShowCursor(1);

	// Update window tab
	int cursel = SendMessage(tab, TCM_GETCURSEL, 0, 0);

	for (int i = 0; i < numPages; i ++)
		ShowWindow(pages[i]->hwnd, cursel == i ? true : false);

	// Main message loop
	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) > 0) {
		if (IsDialogMessage(NULL, &message))
			continue;

		TranslateMessage (&message);
		DispatchMessage  (&message);
	}

	// Main update
	MainLoop();

	// Page updates
	switch (SendMessage(tab, TCM_GETCURSEL, 0, 0)) {
		case PAGE_STATUS:
			UpdateStatusPage(); break;
		case PAGE_GENESIS:
			UpdateGenesisPage(); break;
	}

	// Update window title
	char titleMsg[256];

	if (game != UNKNOWN_GAME) {
		char* nameStrings[] = {"[lawl you shouldn't be seeing this]", "Spyro 1", "Spyro 2", "Spyro 3"};
		char* regionStrings[] = {"[unknown]", "NTSC", "PAL", "NTSC-JP"};
		char* versionStrings[] = {"[unknown]", "", "(Greatest Hits)", "(Beta)", "(Demo)"};
		sprintf(titleMsg, NAME " (%s %s %s)", nameStrings[game], regionStrings[gameRegion], versionStrings[gameVersion]);
	} else
		sprintf(titleMsg, NAME " (no game detected)", (uint32) memory);

	SendMessage(hwndEditor, WM_SETTEXT, 0, (LPARAM) titleMsg);

	// Draw onto texture window
	if (textureWindowOpen)
		UpdateTextureWindow();

	if (IsWindowVisible(hwndVram)) {
		int viewWidth = 2048, viewHeight = 512+512;
		HDC drawDc = CreateCompatibleDC(NULL);

		// Create and select a bitmap into that DC
		HBITMAP drawBitmap;

		uint32* autoBits;
		BITMAPINFO bi = {0};
		bi.bmiHeader.biSize = sizeof (bi.bmiHeader);
		bi.bmiHeader.biCompression = BI_RGB;
		bi.bmiHeader.biWidth = viewWidth;
		bi.bmiHeader.biHeight = -viewHeight;
		bi.bmiHeader.biPlanes = 1;
		bi.bmiHeader.biBitCount = 32;
		drawBitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**) &autoBits, NULL, 0);

		// Draw onto that bitmap
		RECT r = {0, 0, viewWidth, viewHeight};
	
		FillRect(drawDc, &r, (HBRUSH) BLACK_BRUSH);
		SelectObject(drawDc, (HGDIOBJ) drawBitmap);

		uint16* srcBits = vram.GetVram16();
		int drawPalette = 1;
		int paletteByteStart = 0;

		// Draw a texture
		uint32* uintmem = (uint32*) memory;
		Tex* curTex = NULL;
		int numTexs = 0;
		RECT wndRect;
		static int paletteX, paletteY;
		POINT mouse;
		
		GetCursorPos(&mouse);
		ScreenToClient(hwndVram, &mouse);

		if (keyDown[VK_SPACE]) {
			paletteX = mouse.x / 16 * 16;
			paletteY = mouse.y / 8 * 8;
		}
		if (keyPressed[VK_RIGHT]) paletteX ++;
		if (keyPressed[VK_LEFT]) paletteX --;
		if (keyPressed[VK_DOWN]) paletteY ++;
		if (keyPressed[VK_UP]) paletteY --;

		if (textures) {
			curTex = &textures[1];

			//x1 /= 2; x2 /= 2; y1 /= 2; y2 /= 2;

			//paletteByteStart = curTex->hqData[0].palette * 32;
	//		paletteX = curTex->lqData[0].palettex * 16;
	//		paletteY = curTex->lqData[0].palettey * 4;
			numTexs = *numTextures;
		}
		paletteByteStart = paletteY * 2048 + paletteX*2;

#undef RGB
#define RGB(r,g,b) ((r) << 16) | ((g) << 8) | ((b) & 0xFF)
		for (int y = 0; y < 512; y++) {
			for (int x = 0; x < 1024; x++)
				autoBits[y*viewWidth+x] = RGB((srcBits[y*1024+x] & 31) * 256 / 32, ((srcBits[y*1024+x] >> 5) & 31) * 256 / 32, (srcBits[y*1024+x] >> 10) * 256 / 32) | 0xFF000000;
		}

		// Draw HQ palette colours
		uint16* palette = &srcBits[paletteByteStart / 2];
		for (int y = 0; y < 512; y++) {
			for (int x = 0; x < 1024; x++) {
				uint16 clr = palette[((uint8*) srcBits)[(x + 1024) + y * 2048]];
				autoBits[(x+1024) + y * viewWidth] = RGB((clr & 31) * 256 / 32, (clr >> 5 & 31) * 256 / 32, (clr >> 10 & 31) * 256 / 32) | 0xFF000000;
			}
		}

		// Draw LQ palette colours
		for (int y = 0; y < 512; y++) {
			for (int x = 0; x < 2048; x++) {
				uint16 clr = palette[((uint8*) srcBits)[(x/2) + 1024 + y * 2048] >> ((x & 1)*4) & 0x0F];
				autoBits[x + (y+512) * viewWidth] = RGB((clr & 31) * 256 / 32, (clr >> 5 & 31) * 256 / 32, (clr >> 10 & 31) * 256 / 32) | 0xFF000000;
			}
		}

		if (curTex) {
			const int increment = 256;
			int startx = (curTex->lqData[0].region * increment) % 2048, starty = ((curTex->lqData[0].region & 0x7F) / (16) * 256);
			int x1, y1, x2, y2;

			x1 = startx + curTex->lqData[0].xmin;
			x2 = startx + curTex->lqData[0].xmax;
			y1 = starty + curTex->lqData[0].ymin;
			y2 = starty + curTex->lqData[0].ymax;

			x1 -= 0; x2 -= 0;
			/* LQ palette
			x1 = curTex->lqData[0].palettex*16;
			y1 = curTex->lqData[0].palettey;
			x2 = x1 + 16;
			y2 = y1;*/

			int drawX[2] = {x1, x2}, drawY[2] = {y1, y2};

			for (int x = drawX[0]; x < drawX[1]; x ++) {
				autoBits[x + y1*viewWidth] = RGB(255, 0, 0);
				autoBits[x + y2*viewWidth] = RGB(255, 0, 0);
			}
		}

		// Draw palette position
		for (int y = -2; y <= 2; y ++) {
			int pos = (paletteY + y) * viewWidth + paletteX;
			if (pos >= 0 && pos < viewWidth * viewHeight)
				autoBits[pos] = RGB(255, 0, 255);
		}
			
		for (int x = -2; x <= 2; x ++) {
			int pos = paletteY * viewWidth + paletteX + x;
			if (pos >= 0 && pos < viewWidth * viewHeight)
				autoBits[pos] = RGB(255, 0, 255);
		}

		GdiFlush();

		// Copy to window
		HDC vramDc = GetDC(hwndVram);
		BitBlt(vramDc, 0, 0, viewWidth, viewHeight, drawDc, 0, 0, SRCCOPY);
		
		// Draw palette position
		char str[64];
		sprintf(str, "palette %i(%03X),%i(%03X)", paletteX, paletteX, paletteY, paletteY);
		RECT coords = {paletteX - 90, paletteY - 20, paletteX + 90, paletteY + 10};

		SelectObject(vramDc, GetStockObject(SYSTEM_FONT));
		SetTextColor(vramDc, RGB(255, 255, 255));
		SetBkMode(vramDc, TRANSPARENT);
		DrawTextA(vramDc, str, strlen(str), &coords, 0);

		// Draw mouse position
		RECT mouseInfoBox = {mouse.x - 90, mouse.y - 18, mouse.x + 90, mouse.y + 18};
		if (mouse.y >= 512) {
			sprintf(str, "(4-bit) %i(%03X),%i(%03X)", mouse.x + 2048, mouse.x + 2048, mouse.y - 512, mouse.y - 512);
		} else if (mouse.x < 1024) {
			sprintf(str, "(16-bit) %i(%03X),%i(%03X)", mouse.x + 1024, mouse.x + 1024, mouse.y, mouse.y);
		} else if (mouse.x > 0)
			sprintf(str, "(8-bit) %i(%03X),%i(%03X)", mouse.x, mouse.x, mouse.y, mouse.y);
		
		if (mouseInfoBox.left < 0) {
			mouseInfoBox.right = -mouseInfoBox.left + 180;
			mouseInfoBox.left = 0;
		}
		if (mouseInfoBox.right >= viewWidth) {
			mouseInfoBox.left = viewWidth - 180;
			mouseInfoBox.right = viewWidth - 1;
		}

		if (mouse.y >= 0 && mouse.x >= 0 && mouse.y < viewHeight && mouse.x < viewWidth)
			DrawTextA(vramDc, str, strlen(str), &mouseInfoBox, 0);
		
		ReleaseDC(hwndVram, vramDc);
		DeleteObject(drawBitmap);
		DeleteDC(drawDc);
	}
}

uint32 GetAverageLightColour();
#define bitsu(num, bitstart, numbits) ((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits)))
// get bits signed
#define bitss(num, bitstart, numbits) (int) ((((num) >> (bitstart) & ~(0xFFFFFFFF << (numbits))) | (0 - (((num) >> ((bitstart)+(numbits)-1) & 1)) << (numbits))))
#define outbits(num, bitstart, numbits) (((num) & ~(0xFFFFFFFF << (numbits))) << (bitstart))

struct ButCmd {
	HWND button;
	void (*function)();
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CLOSE: {
			if (hwnd == hwndTexture) {
				ShowWindow(hwnd, FALSE);
				textureWindowOpen = false;
				return 0;
			} else if (hwnd == hwndEditor) {
				return 0;
			} else if (hwnd == hwndVram) {
				ShowWindow(hwnd, false);
				return 0;
			}
			break;
		}
		case WM_SIZE:
			RECT r;

			GetClientRect(hwnd, &r);

			if (hwnd == hwndEditor) {
				MoveWindow(tab, 0, 0, LOWORD(lParam), 20, 1);

				for (int i = 0; i < sizeof (pages) / sizeof (pages[0]); i++)
					MoveWindow(pages[i]->hwnd, 0, 20, LOWORD(lParam), HIWORD(lParam) - 20, 1);
			}

			break;
		case WM_COMMAND: {
			// Button or checkbox presses
			if (HIWORD(wParam) == BN_CLICKED) {
				HWND control = (HWND) lParam;

				if (control == checkbox_texSeparate) {
					if (SendMessage(checkbox_texSeparate, BM_GETCHECK, 0, 0) == BST_CHECKED)
						texEditFlags |= TEF_SEPARATE;
					else
						texEditFlags &= ~TEF_SEPARATE;
				} else if (control == checkbox_texGenPalettes) {
					if (SendMessage(checkbox_texGenPalettes, BM_GETCHECK, 0, 0) == BST_CHECKED)
						texEditFlags |= TEF_GENERATEPALETTES;
					else
						texEditFlags &= ~TEF_GENERATEPALETTES;
				} else if (control == checkbox_texShufflePalettes) {
					if (SendMessage(checkbox_texShufflePalettes, BM_GETCHECK, 0, 0) == BST_CHECKED)
						texEditFlags |= TEF_SHUFFLEPALETTES;
					else
						texEditFlags &= ~TEF_SHUFFLEPALETTES;
				} else if (control == checkbox_texAutoLq) {
					if (SendMessage(checkbox_texAutoLq, BM_GETCHECK, 0, 0) == BST_CHECKED)
						texEditFlags |= TEF_AUTOLQ;
					else
						texEditFlags &= ~TEF_AUTOLQ;
				} else if (control == checkbox_genDisableSceneOccl)
					disableSceneOccl = (SendMessage(checkbox_genDisableSceneOccl, BM_GETCHECK, 0, 0) == BST_CHECKED);
				else if (control == checkbox_genDisableSkyOccl)
					disableSkyOccl = (SendMessage(checkbox_genDisableSkyOccl, BM_GETCHECK, 0, 0) == BST_CHECKED);
				else if (control == button_setColours) {
					LevelColours clrs;
					char str[5] = {0,0,0,0,0};

					SendMessage(edit_fogR->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.fogR = atoi(str);
					SendMessage(edit_fogG->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.fogG = atoi(str);
					SendMessage(edit_fogB->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.fogB = atoi(str);

					SendMessage(edit_lightR->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.lightR = atoi(str);
					SendMessage(edit_lightG->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.lightG = atoi(str);
					SendMessage(edit_lightB->hwnd, WM_GETTEXT, 4, (LPARAM) str); clrs.lightB = atoi(str);

					SetLevelColours(&clrs);
				} else if (control == button_getColours) {
					LevelColours clrs;
					char str[5] = {0,0,0,0,0};

					GetLevelColours(&clrs);
				
					sprintf(str, "%hhu", clrs.fogR); SendMessage(edit_fogR->hwnd, WM_SETTEXT, 0, (LPARAM) str);
					sprintf(str, "%hhu", clrs.fogG); SendMessage(edit_fogG->hwnd, WM_SETTEXT, 0, (LPARAM) str);
					sprintf(str, "%hhu", clrs.fogB); SendMessage(edit_fogB->hwnd, WM_SETTEXT, 0, (LPARAM) str);

					strcpy(str, "127");
					SendMessage(edit_lightR->hwnd, WM_SETTEXT, 0, (LPARAM) str);
					SendMessage(edit_lightG->hwnd, WM_SETTEXT, 0, (LPARAM) str);
					SendMessage(edit_lightB->hwnd, WM_SETTEXT, 0, (LPARAM) str);
				} else if (control == button_next || control == button_prev) {
					unsigned short* list;
					int obj = GetObjectID();
			
					if (wadstart != NULL)
						list = (unsigned short*) ((int) wadstart + 0x0142);
					else if (game == SPYRO3 && gameRegion == PAL)
						list = (unsigned short*) MEMPTR(0x00075554 + 0x1A2);
			
					if (list == NULL) { // No object list found
						MessageBox(NULL, "Sorry; object list not found in this version", "Sorry", MB_OK);
						break;
					}
			
					if (obj < 0) obj = 0;
			
					for (int i = 0; i < 150; i++) {
						if (list[i] == 0) break;
				
						if (list[i] == mobys[obj].type) {
							mobys[obj].type = list[((HWND) lParam == button_next) ? i + 1 : i - 1];
					
							if (mobys[obj].type == 0) // If the new type is 0, set it back to something reasonable
								mobys[obj].type = list[i];

							UpdateVars();
							break;
						}
					}
				} else if (control == button_get) 
					UpdateVars();
				else if (control == button_set)
					SetVars();
				else if (LOWORD(wParam) == 14) { // hex toggle
					int id = -1;

					for (int i = 0; i < 50; i ++) {if (vardefs[i].hex_toggle == (HWND) lParam) id = i;}

					if (id != -1)
						UpdateVarHexToggle(id);
				} else if (control == button_goto && spyro && mobys) {
					int id = GetObjectID();

					if (id >= 0) {
						spyro->x = mobys[id].x;
						spyro->y = mobys[id].y;
						spyro->z = mobys[id].z + 1000;
					}
				} else if (control == button_butterflies && mobys && spyro) {
					unsigned int total_objects = NumObjects();

					char object_string[5000] = "";
					char display_string[5000];

					sprintf(object_string, "Player position: %i,%i,%i\nTotal objects: %i\n", spyro->x, spyro->y, spyro->z, total_objects);

					for (int i = 0; i < total_objects; i++) {
						char temp_string[50];
						char* isdead;
						int iType = mobys[i].type;
						int relx, rely, relz;
						float dist;

						if (Distance(mobys[i].x, mobys[i].y, mobys[i].z, spyro->x, spyro->y, spyro->z) > 2500)
							continue;

						if (mobys[i].state < 0)
							isdead = "(Dead)";
						else
							isdead = "(Alive)";

						sprintf(temp_string, "%i: Type %i/$%04X %s\n", i, iType, iType, isdead);

						strncat(object_string, temp_string, 50);
					}

					sprintf(display_string, "Found: \n\n%s", object_string);

					MessageBox(hwndEditor, display_string, "Object information", MB_OK);
				} else if (control == button_spawncopy) {
					char newObjId[8];
					int newId = 0;

					for (int i = 0; i < 500; i ++) {
						if (mobys[i].state == -1) {
							newId = i;
							break;
						}
					}

					mobys[newId] = mobys[GetObjectID()];
					mobys[newId + 1].state = -1;
					sprintf(newObjId, "%i", newId);
					SetControlString(object_id, newObjId);

					UpdateVars();
					break;
				} else if (control == button_vramViewer) {
					ShowWindow(hwndVram, !IsWindowVisible(hwndVram));
				} else {
					// Check page button commands
					for (SpyroEditPage* page : pages) {
						for (CtrlButton* button : page->buttons) {
							if (button->hwnd == control) {
								if (button->onClick) {
									button->onClick();
								}
								goto BreakAll;
							}
						}
					}
					BreakAll:;
				}
			}

			// Object auto-update
			if ((HIWORD(wParam) == EN_UPDATE && (HWND) lParam == object_id))
				UpdateVars();

			// Coord convert test
			static bool updatingCoords = false;
			if (HIWORD(wParam) == EN_UPDATE && !updatingCoords) {
				HWND ctrl = (HWND) lParam;
				updatingCoords = true;
				if (ctrl == edit_coordsOutHalfwordBlock) {
					uint32 val = GetControlHex(edit_coordsOutHalfwordBlock);
					int tZ = bitss(val, 2, 4), tY = bitss(val, 6, 5), tX = bitss(val, 11, 5);

					SetControlInt(edit_coordsInX, tX * 4);
					SetControlInt(edit_coordsInY, tY * 4);
					SetControlInt(edit_coordsInZ, tZ * 4);
					SetControlInt(edit_coordsInFlag, val & 3);
				} else if (ctrl == edit_coordsOutWord) {
					uint32 val = GetControlHex(edit_coordsOutWord);

					SetControlInt(edit_coordsInX, bitss(val, 21, 11));
					SetControlInt(edit_coordsInY, bitss(val, 10, 11));
					SetControlInt(edit_coordsInZ, bitss(val, 0, 10));
				} else if (ctrl == edit_coordsOutWordBlock) {
					uint32 val = GetControlHex(edit_coordsOutWordBlock);

					SetControlInt(edit_coordsInX, bitss(val, 22, 10) * 2);
					SetControlInt(edit_coordsInY, -bitss(val, 12, 10) * 2);
					SetControlInt(edit_coordsInZ, -bitss(val, 2, 10) * 2);
					SetControlInt(edit_coordsInFlag, val & 3);
				}

				int x = GetControlInt(edit_coordsInX), y = GetControlInt(edit_coordsInY), z = GetControlInt(edit_coordsInZ), flag = GetControlInt(edit_coordsInFlag);

				if (ctrl != edit_coordsOutHalfwordBlock)
					SetControlHex(edit_coordsOutHalfwordBlock, outbits(z / 4, 2, 4) | outbits(y / 4, 6, 5) | outbits(x / 4, 11, 5) | (flag & 3));
				if (ctrl != edit_coordsOutWord)
					SetControlHex(edit_coordsOutWord, outbits(x, 21, 11) | outbits(y, 10, 11) | outbits(z, 0, 10));
				if (ctrl != edit_coordsOutWordBlock)
					SetControlHex(edit_coordsOutWordBlock, outbits(x / 2, 22, 10) | outbits(-y / 2, 12, 10) | outbits(-z / 2, 2, 10) | (flag & 3));
				updatingCoords = false;
			}

			if (HIWORD(wParam) == BN_CLICKED && ((HWND) lParam == button_makeLight || (HWND) lParam == button_makeFog || (HWND) lParam == button_makeSky)) {
				CHOOSECOLOR cc;
				COLORREF derp[16];

				cc.lStructSize = sizeof (cc);
				cc.hInstance = hwndEditor;
				cc.hwndOwner = hwndEditor;
				cc.lpTemplateName = 0;
				cc.lpfnHook = NULL;
				cc.lCustData = 0;
				cc.lpCustColors = derp;
				cc.rgbResult = RGB(127, 127, 127);
				cc.Flags = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN;

				if ((HWND) lParam == button_makeLight || (HWND) lParam == button_makeSky) {
					uint32 avgClr = (HWND) lParam == button_makeLight ? GetAverageLightColour() : GetAverageSkyColour();
					int avgR = avgClr & 0xFF, avgG = avgClr >> 8 & 0xFF, avgB = avgClr >> 16 & 0xFF;
					
					cc.rgbResult = RGB(avgR, avgG, avgB);

					if (ChooseColor(&cc)) {
						char str[5] = {0,0,0,0,0};
						int diffR = 255, diffG = 255, diffB = 255;
						if (avgR) diffR = (GetRValue(cc.rgbResult))*127/avgR;
						if (avgG) diffG = (GetGValue(cc.rgbResult))*127/avgG;
						if (avgB) diffB = (GetBValue(cc.rgbResult))*127/avgB;
					
						if ((HWND) lParam == button_makeLight) {
							sprintf(str, "%hhu", diffR * 100 / 127); SendMessage(edit_lightR->hwnd, WM_SETTEXT, 0, (LPARAM) str);
							sprintf(str, "%hhu", diffG * 100 / 127); SendMessage(edit_lightG->hwnd, WM_SETTEXT, 0, (LPARAM) str);
							sprintf(str, "%hhu", diffB * 100 / 127); SendMessage(edit_lightB->hwnd, WM_SETTEXT, 0, (LPARAM) str);
						} else {
							sprintf(str, "%hhu", diffR * 100 / 127); SendMessage(edit_skyR->hwnd, WM_SETTEXT, 0, (LPARAM) str);
							sprintf(str, "%hhu", diffG * 100 / 127); SendMessage(edit_skyG->hwnd, WM_SETTEXT, 0, (LPARAM) str);
							sprintf(str, "%hhu", diffB * 100 / 127); SendMessage(edit_skyB->hwnd, WM_SETTEXT, 0, (LPARAM) str);
						}
					}
				} else if ((HWND) lParam == button_makeFog) {
					LevelColours clrs;
					char str[5] = {0,0,0,0,0};

					GetLevelColours(&clrs);

					cc.rgbResult = RGB(clrs.fogR, clrs.fogG, clrs.fogB);

					if (ChooseColor(&cc)) {
						char str[5] = {0,0,0,0,0};
						sprintf(str, "%hhu", GetRValue(cc.rgbResult)); SendMessage(edit_fogR->hwnd, WM_SETTEXT, 0, (LPARAM) str);
						sprintf(str, "%hhu", GetGValue(cc.rgbResult)); SendMessage(edit_fogG->hwnd, WM_SETTEXT, 0, (LPARAM) str);
						sprintf(str, "%hhu", GetBValue(cc.rgbResult)); SendMessage(edit_fogB->hwnd, WM_SETTEXT, 0, (LPARAM) str);
					}
				}
			}

			if (HIWORD(wParam) == EN_SETFOCUS) {
				for (int i = 0; i < 50; i++) {
					if (!vardefs[i].type)
						continue;

					if ((HWND) lParam == vardefs[i].edit_hwnd)
						vardefs[i].focused = 1;
					else
						vardefs[i].focused = 0;
				}
			}

			// Check if it's a power toggle
			int pwrId;
			for (pwrId = 0; pwrId < g_numPowers; pwrId++) {
				if ((HWND) lParam == buttonPowers[pwrId])
					break;
			}

			if (HIWORD(wParam) == BN_CLICKED && pwrId < g_numPowers)
				powers = (powers & ~(1 << pwrId)) | ((SendMessage((HWND) lParam, BM_GETCHECK, 0, 0) == BST_CHECKED) << pwrId);
		}
		break;
		case WM_DROPFILES: {
			HDROP hDrop = (HDROP) wParam;
			char droppedFile[MAX_PATH];

			DragQueryFile(hDrop, 0, droppedFile, MAX_PATH);
			DragFinish(hDrop);

			// Detect file extension
			int extensionDot = 0;
			for (int i = 0; i < MAX_PATH && droppedFile[i]; i++) {
				if (!droppedFile[i])
					break;
				if (droppedFile[i] == '.')
					extensionDot = i;
			}

			// Handle the file based on its extension
			if (!strcmp(&droppedFile[extensionDot], ".sky"))
				LoadSky(droppedFile);

			break;
		}
		case WM_DRAWITEM: {
			DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*) lParam;

			int numGoodStretches = 0;
			int curStretch = 0;
			const int barWidth = 300;
			for (int i = 0; i < barWidth; i++) {
				uint32 memStart = i * 0x00200000u / barWidth;
				uint32 memEnd = (i + 1) * 0x00200000u / barWidth;
				int maxStretch = 0;

				for (int j = memStart / 4, e = memEnd / 4; j < e; /*j++*/j+=1024) {
					if (/*umem32[j] || */(usedMem[j / (1024/4) / 32] & (1 << ((j / 1024) & 31)))) {
						if (curStretch > maxStretch)
							maxStretch = curStretch;
						if (curStretch >= 1024)
							numGoodStretches++;

						curStretch = 0;
					} else
						curStretch += 1024;//curStretch++;
				}
				
				if (curStretch > maxStretch)
					maxStretch = curStretch;
				if (maxStretch > 0x00200000u / barWidth / 4)
					maxStretch = 0x00200000u / barWidth / 4;

				int memValue = maxStretch * 255 / (0x00200000u / barWidth / 4);

				HPEN hoopin = CreatePen(PS_SOLID, 1, RGB(0, memValue, 255 - memValue));
				SelectObject(di->hDC, hoopin);
				MoveToEx(di->hDC, i, 0, NULL);
				LineTo(di->hDC, i, 10);
				DeleteObject(hoopin);
			}

			if (curStretch > 1024/4)
				numGoodStretches += curStretch / (1024/4);
			
			SelectObject(di->hDC, GetStockObject(NULL_PEN));

			// Update memory status message
			char statusMsg[100];

			sprintf(statusMsg, "%i stretches of 1KB+ memory found", numGoodStretches);
			SendMessage(static_memStatus, WM_SETTEXT, 0, (LPARAM) statusMsg);
			return TRUE;
		}
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

void UpdateStatusPage() {
	// Update status objects
	for (int i = 0; i < sizeof (statusObjects) / sizeof (StatusObject); i++) {
		char text[256];

		if (*(void**)(statusObjects[i].pointer) != NULL) {
			if (!statusObjects[i].absolute)
				sprintf(text, "%s: Detected! (%08X)", statusObjects[i].name, (*(uintptr*)statusObjects[i].pointer - (uintptr)memory) | 0x80000000);
			else
				sprintf(text, "%s: Detected! (%08X)", statusObjects[i].name, *(uint32*)statusObjects[i].pointer);
		} else
			sprintf(text, "%s: Undetected!", statusObjects[i].name);

		SendMessage(statusObjects[i].hwnd, WM_SETTEXT, 0, (LPARAM) text);
	}

	// Update memory monitor
	RedrawWindow(static_memVisualiser, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

bool isGenesisPageValid = false;

void UpdateGenesisPage() {
	if (!isGenesisPageValid && scene.spyroScene) {
		char itemString[200];
		SendMessage(listbox_sceneSectorStatus, LB_RESETCONTENT, 0, 0);
		
		for (int i = 0; i < scene.spyroScene->numSectors; ++i) {
			SceneSectorHeader* sector = scene.spyroScene->sectors[i];
			sprintf(itemString, "Addr: %08X Size: %i V/F/C: %i/%i/%i", scene.spyroScene->sectors[i].address, sector->GetSize(), sector->numHpVertices, sector->numHpFaces, sector->numHpColours);
			SendMessage(listbox_sceneSectorStatus, LB_ADDSTRING, 0, (LPARAM)itemString);
		}

		sprintf(itemString, "Original scene size: %i Current: %i", scene.GetOriginalSceneSize(), scene.spyroScene->GetSize());
		SendMessage(static_sectorSizes, WM_SETTEXT, 0, (LPARAM)itemString);

		isGenesisPageValid = true;
	}
}

void UpdateTextureWindow() {
#undef RGB
#define RGB(r,g,b) ((r) << 16) | ((g) << 8) | ((b) & 0xFF)
	static int panX, panY;
	static int lastMouseX, lastMouseY;

	// Setup window view
	RECT derp;
	GetClientRect(hwndTexture, &derp);

	int viewWidth = derp.right, viewHeight = derp.bottom;
	HDC drawDc = CreateCompatibleDC(NULL);

	if (viewWidth <= 0)
		viewWidth = 0;
	if (viewHeight <= 0)
		viewHeight = 0;

	// Create and select a bitmap into that DC
	HBITMAP drawBitmap;

	uint32* autoBits;
	BITMAPINFO bi = {0};
	bi.bmiHeader.biSize = sizeof (bi.bmiHeader);
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biWidth = viewWidth;
	bi.bmiHeader.biHeight = -viewHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	drawBitmap = CreateDIBSection(0, &bi, DIB_RGB_COLORS, (void**) &autoBits, NULL, 0);

	// Update view stuff
	POINT mouse;

	GetCursorPos(&mouse);
	ScreenToClient(hwndTexture, &mouse);

	if (keyDown[VK_LBUTTON] && GetFocus() == hwndTexture) {
		SetCursor(LoadCursor(mainModule, IDC_HAND));

		panX -= mouse.x - lastMouseX;
		panY -= mouse.y - lastMouseY;

		if (panY < 0)
			panY = 0;
	}

	if (strlen(textureDroppedFile)) {
		; // Derp
		// Show where the texture's gonna drop
	}

	lastMouseX = mouse.x;
	lastMouseY = mouse.y;

	// Prepare DC and bitmap for drawing
	RECT r = {0, 0, viewWidth, viewHeight};

	SelectObject(drawDc, GetStockObject(SYSTEM_FONT));
	SetTextColor(drawDc, RGB(0, 0, 0));
	SetBkMode(drawDc, TRANSPARENT);

	SelectObject(drawDc, (HGDIOBJ) drawBitmap);
	FillRect(drawDc, &r, (HBRUSH) LTGRAY_BRUSH);

	uint8* vram8 = vram.GetVram8();
	uint16* vram16 = vram.GetVram16();

	// Draw textures
	uint32* uintmem = (uint32*) memory;

	if (textures) {
		UpdatePaletteList();

		// Clip pan
		int maxPanY = 0;
		for (int i = 0; i < numPalettes; i++) {
			if (paletteTypes[i] == PT_HQ)
				maxPanY += 80;
		}

		if (panY > maxPanY - viewHeight)
			panY = maxPanY - viewHeight;

		// Reset highlights
		textureHighlightedPalette = -1;
		textureHighlightedTex = -1;

		// Find highlighted texture or palette
		int currentX = 0, currentY = -(panY % 80);
		int numTexs = *numTextures;
		int panSkip = 0;

		if (mouse.x >= 0 && mouse.x < viewWidth / 2 && mouse.y >= 0 && mouse.y < viewHeight) {
			for (int i = 0; i < numPalettes; i++) {
				if (paletteTypes[i] != PT_HQ)
					continue;

				if (panSkip + 80 < panY) {
					panSkip += 80;
					continue;
				}

				if (currentY <= mouse.y && currentY + 80 >= mouse.y)
					textureHighlightedPalette = i;

				currentY += 16;

				for (int j = 0; j < numTexs; j++) {
					TexHq* curHq = textures[j].hqData;

					// See if any palettes in this texture match
					int numMatchingPalettes = 0;
					for (int tile = 0; tile < 4; tile++) {
						if (curHq[tile].palette * 32 == palettes[i])
							numMatchingPalettes++;
					}

					if (!numMatchingPalettes)
						continue;

					for (int tile = 0; tile < 4; tile++) {
						// Check if the mouse is within this texture tile's border
						if (currentX + TILETOX(tile) <= mouse.x && currentY + TILETOY(tile) <= mouse.y && 
							currentX + TILETOX(tile) + 32 > mouse.x && currentY + TILETOY(tile) + 32 > mouse.y) {
							textureHighlightedTex = j;
							textureHighlightedTile = tile;
						}
					}

					currentX += 64;
				}

				currentX = 0;
				currentY += 64;
			}
		} else if (mouse.x >= viewWidth / 2 && mouse.x < viewWidth && mouse.y >= 0 && mouse.y < viewHeight) {
			for (int i = 0; i < numTexs && i < numTexCaches; i++) {
				int xStart = viewWidth / 2 + (i % ((viewWidth / 2) / 64)) * 64, yStart = i / ((viewWidth / 2) / 64) * 64;

				if (mouse.x >= xStart && mouse.x < xStart + 64 && mouse.y >= yStart && mouse.y < yStart + 64) {
					textureHighlightedTex = i;

					for (int tile = 0; tile < 4; tile++) {
						if (mouse.x >= xStart + TILETOX(tile) && mouse.x < xStart + TILETOX(tile) + 32 && 
							mouse.y >= yStart + TILETOY(tile) && mouse.y < yStart + TILETOY(tile) + 32) {
							textureHighlightedTile = tile;
							textureHighlightedPalette = textures ? textures[i].hqData[tile].palette * 32 : hqTextures[i].hqData[tile].palette * 32;
						}
					}
				}
			}
		}

		// Draw every texture by palette
		int curViewWidth = viewWidth / 2;
		currentX = 0; currentY = -(panY % 80);
		panSkip = 0;
		for (int i = 0; i < numPalettes; i++) {
			RECT paletteNameCoords = {currentX, currentY, currentX + 256, currentY + 16};
			char paletteName[100];

			if (paletteTypes[i] != PT_HQ)
				continue;

			if (panSkip + 80 < panY) {
				panSkip += 80;
				continue;
			}

			sprintf(paletteName, "Palette %08X", palettes[i]);
			DrawTextA(drawDc, paletteName, strlen(paletteName), &paletteNameCoords, 0);

			currentY += 16;

			for (int j = 0; j < numTexs && j < numTexCaches; j++) {
				TexHq* curHq = textures[j].hqData;

				// See if any palettes in this texture match
				int numMatchingPalettes = 0;
				for (int tile = 0; tile < 4; tile++) {
					if (curHq[tile].palette * 32 == palettes[i])
						numMatchingPalettes++;
				}

				if (!numMatchingPalettes)
					continue;

				// If so, draw the texture
				for (int tile = 0; tile < 4; tile++) {
					int paletteByteStart = curHq[tile].palette * 32;

					if (paletteByteStart == palettes[i]) {
						int destX = TILETOX(tile), destY = TILETOY(tile);
						for (int y = 0; y < 32; y++) {
							if (destY + y + currentY < 0 || destY + y + currentY >= viewHeight)
								continue;

							for (int x = 0; x < 32; x++) {
								if (destX + x + currentX < 0 || destX + x + currentX >= curViewWidth)
									continue;

								uint32 clr = texCaches[j].tiles[tile].bitmap[y * 32 + x];
								autoBits[(destY + currentY + y) * viewWidth + destX + currentX + x] = (clr & 0x00FF00) | (clr << 16 & 0xFF0000) | (clr >> 16 & 0x0000FF) | 0xFF000000;
							}
						}
					} else {
						// Different palette; fill this tile with grey
						for (int y = currentY + TILETOY(tile), y2 = y + 32; y < y2; y++) {
							if (y < 0 || y >= viewHeight)
								continue;

							for (int x = currentX + TILETOX(tile), x2 = x + 32; x < x2; x++) {
								if (x < 0 || x >= curViewWidth)
									continue;

								autoBits[y * viewWidth + x] = RGB(127, 127, 127);
							}
						}
					}
				}

				// If this texture is equivalent to the one highlighted, draw a border
				if (j == textureHighlightedTex) {
					int startY = currentY;

					if (startY < 0)
						startY = 0;

					for (int y = startY; y < currentY + 64 && y < viewHeight; y++) {
						if (currentX < viewWidth)
							autoBits[y * viewWidth + currentX] = RGB(255, 0, 0);
						if (currentX + 63 < viewWidth)
							autoBits[y * viewWidth + currentX + 63] = RGB(255, 0, 0);
					}
					
					for (int x = currentX; x < currentX + 64 && x < viewWidth; x++) {
						if (currentY < viewHeight && currentY >= 0)
							autoBits[currentY * viewWidth + x] = RGB(255, 0, 0);
						if (currentY + 63 < viewHeight && currentY + 63 >= 0)
							autoBits[(currentY + 63) * viewWidth + x] = RGB(255, 0, 0);
					}
				}

				currentX += 64;
			}

			currentX = 0;
			currentY += 64;

			if (currentY >= viewHeight)
				break;
		}

		// Draw every texture by texture
		if (viewWidth - curViewWidth >= 64) {
			TexHq* highlightedHq = textures ? textures[textureHighlightedTex].hqData : hqTextures[textureHighlightedTex].hqData;
			for (int i = 0; i < numTexs && i < numTexCaches; i++) {
				TexHq* curHq = textures ? textures[i].hqData : hqTextures[i].hqData;
				int xStart = curViewWidth + (i % ((viewWidth - curViewWidth) / 64)) * 64,
					yStart = i / ((viewWidth - curViewWidth) / 64) * 64;

				for (int tile = 0; tile < 4; tile++) {
					TexCacheTile* tileCache = &texCaches[i].tiles[tile];
					int curXStart = xStart + TILETOX(tile), curYStart = yStart + TILETOY(tile);

					// Find reused tiles
					int reuseTex = -1, reuseTile = -1;
					for (int j = 0; j <= i; j++) {
						TexHq* curHq2 = textures ? textures[j].hqData : hqTextures[j].hqData;

						for (int tile2 = 0, numTiles = (j == i ? tile : 4); tile2 < numTiles; tile2++) {
							if (curHq2[tile2].xmin == curHq[tile].xmin && curHq2[tile2].ymin == curHq[tile].ymin && curHq2[tile2].region == curHq[tile].region) {
								reuseTex = j;
								reuseTile = tile2;
								goto Break;
							}
						}

						continue;
						Break:
						break;
					}

					// Draw texture tile
					if (reuseTex == -1) {
						for (int y = 0; y < 32; y++) {
							for (int x = 0; x < 32; x++) {
								if (curXStart + x >= viewWidth)
									break;
								if (curYStart + y >= viewHeight)
									break;

								uint32 clr = tileCache->bitmap[y * 32 + x];
								autoBits[(curYStart + y) * viewWidth + (curXStart + x)] = 
									(clr & 0x00FF00) | (clr << 16 & 0xFF0000) | (clr >> 16 & 0x0000FF) | 0xFF000000;
							}
						}
					} else {
						// Blend with first texture
						TexCacheTile* tileBlend = &texCaches[reuseTex].tiles[reuseTile];
						for (int y = 0; y < 32; y++) {
							for (int x = 0; x < 32; x++) {
								if (curXStart + x >= viewWidth)
									break;
								if (curYStart + y >= viewHeight)
									break;

								uint32 clr = tileCache->bitmap[y * 32 + x];
								uint32 clr2 = tileBlend->bitmap[y * 32 + x];
								autoBits[(curYStart + y) * viewWidth + (curXStart + x)] = 
									MAKECOLOR32(((clr >> 16 & 0xFF) + (clr2 >> 16 & 0xFF)) >> 1, ((clr >> 8 & 0xFF) + (clr2 >> 8 & 0xFF)) >> 1,
												((clr & 0xFF) + (clr2 & 0xFF)) >> 1);
							}
						}
					}

					// Draw border
					uint32 borderColour = 0;
					if (textureHighlightedTex == i && textureHighlightedTile == tile)
						borderColour = RGB(255, 0, 255);
					else if (textureHighlightedTex != -1 && highlightedHq[textureHighlightedTile].region == curHq[tile].region && 
							highlightedHq[textureHighlightedTile].xmin == curHq[tile].xmin && highlightedHq[textureHighlightedTile].ymin == curHq[tile].ymin) {
						borderColour = RGB(0, 0, 255);
					} else if (curHq[i].palette * 32 == textureHighlightedPalette)
						borderColour = RGB(255, 0, 0);

					if (borderColour) {
						int borderYStart = yStart + TILETOY(tile), borderXStart = xStart + TILETOX(tile);
						for (int y = borderYStart; y < borderYStart + 32 && y < viewHeight; y++) {
							if (borderXStart < viewWidth)
								autoBits[y * viewWidth + borderXStart] = borderColour;
							if (borderXStart + 31 < viewWidth)
								autoBits[y * viewWidth + borderXStart + 31] = borderColour;
						}
					
						for (int x = borderXStart; x < borderXStart + 32 && x < viewWidth; x++) {
							if (borderYStart < viewHeight && borderYStart >= 0)
								autoBits[borderYStart * viewWidth + x] = borderColour;
							if (borderYStart + 31 < viewHeight && borderYStart + 31 >= 0)
								autoBits[(borderYStart + 31) * viewWidth + x] = borderColour;
						}
					}
				}
			}
		}
	}

	GdiFlush();

	// Copy to window
	HDC texDc = GetDC(hwndTexture);
	BitBlt(texDc, 0, 0, viewWidth, viewHeight, drawDc, 0, 0, SRCCOPY);

	ReleaseDC(hwndTexture, texDc);
	DeleteObject(drawBitmap);
	DeleteDC(drawDc);
}

HWND	SpyroEditPage::AddControl(const char* ctrlClass, const char* ctrlText, uint32 ctrlFlags, int x, int width, int heightInLines) {
	HWND ctrlHwnd = CreateWindowEx(0, ctrlClass, ctrlText, WS_CHILD | WS_VISIBLE | ctrlFlags,
								x, line, width, lineHeight * heightInLines, 
								hwnd, NULL, mainModule, NULL);
	
	SendMessage(ctrlHwnd, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	return ctrlHwnd;
}

HWND	SpyroEditPage::AddControl(const char* ctrlClass, float ctrlTextFloat, uint32 ctrlFlags, int x, int width, int heightInLines) {
	std::stringstream	sstream;

	sstream << ctrlTextFloat;
	const std::string	tmp = sstream.str();
	const char*	ctrlText = tmp.c_str();
	return AddControl(ctrlClass, ctrlText, ctrlFlags, x, width, heightInLines);
}

CtrlButton*	SpyroEditPage::AddButton(const char* buttonText, int x, int width, void (*onClick)()) {
	CtrlButton* button = new CtrlButton(AddControl("BUTTON", buttonText, BS_PUSHBUTTON, x, width, 1), onClick);
	buttons.push_back(button);
	return button;
}

CtrlTextbox*	SpyroEditPage::AddTextbox(const char* defaultText, int x, int width) {
	CtrlTextbox* textbox = new CtrlTextbox(AddControl("EDIT", defaultText, WS_BORDER, x, width, 1));
	textboxes.push_back(textbox);
	return textbox;
}

void	SpyroEditPage::AddLine(uint32 pgFlags, uint32 lineHeight) {
	this->lineHeight = lineHeight;

	line += lineHeight + 3;
	lineFlags = pgFlags;
	
	if (group) {
		MoveWindow(group, groupX, groupY, 300, line - groupY + lineHeight + 5, TRUE);
	}
}

void SpyroEditPage::AddGroup(const char* groupName) {
	if (group) {
		MoveWindow(group, groupX, groupY, 300, line - groupY + 5, TRUE);
		line += 8;
	}

	group = CreateWindowEx(0, "BUTTON", groupName, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 5, line, 300, 0, hwnd, NULL, mainModule, NULL);
	
	SendMessage(group, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);

	groupX = 5;
	groupY = line;
	line += 16;
}

void SetControlString(HWND control, const char* string) {
	SendMessage(control, WM_SETTEXT, 0, (LPARAM) string);
}

uint32 GetControlHex(HWND control) {
	char text[256];
	uint32 value = 0;

	SendMessage(control, WM_GETTEXT, 256, (LPARAM) text);
	text[255] = 0;

	sscanf(text, "%x", &value);
	return value;
}

int GetControlInt(HWND control) {
	char text[256];
	int value;

	SendMessage(control, WM_GETTEXT, 256, (LPARAM) text);
	text[255] = 0;

	sscanf(text, "%i", &value);
	return value;
}

void SetControlHex(HWND control, uint32 value) {
	char text[256];
	sprintf(text, "%08X", value);
	SendMessage(control, WM_SETTEXT, 0, (LPARAM) text);
}

void SetControlInt(HWND control, int value) {
	char text[256];
	sprintf(text, "%i", value);
	SendMessage(control, WM_SETTEXT, 0, (LPARAM) text);
}

const char* CtrlTextbox::GetAsText() {
	delete[] text;

	int textLen = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	text = new char[textLen + 1];
	SendMessage(hwnd, WM_GETTEXT, textLen, (LPARAM)text);
	return text;
}

void CtrlTextbox::SetText(const char* text) {
	SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)text);
}

int CtrlTextbox::GetAsInt() {
	char tempStr[25];

	SendMessage(hwnd, WM_GETTEXT, 24, (LPARAM) tempStr);
	tempStr[24] = '\0';

	return atoi(tempStr);
}
