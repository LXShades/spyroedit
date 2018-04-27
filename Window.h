#include <windows.h>

extern HMODULE mainModule;

extern HWND hwndEditor;
extern HWND hwndVram;

extern HWND tab;
extern HWND object_page;
extern HWND online_page;

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
extern HWND button_host;
extern HWND button_join;
extern HWND staticTransferStatus;

// Genesis page
extern HWND static_genCollisionStatus;
extern HWND static_genSceneStatus;

void Startup();

void SetControlString(HWND control, const char* string);

void CreateObjectPage();
void CreatePowersPage();
void CreateOnlinePage();
void CreateTexturesPage();
void CreateStatusPage();
void CreateGenesisPage();

void UpdateStatusPage();
