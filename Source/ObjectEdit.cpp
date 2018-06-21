#include "Window.h"
#include "ObjectEdit.h"
#include "SpyroData.h"
#include "Main.h"

#include <cstdio>

#define TYPE_UINT    1
#define TYPE_INT     2
#define TYPE_USHORT  4
#define TYPE_SHORT   8
#define TYPE_UCHAR  16
#define TYPE_CHAR   32
#define TYPE_READONLY 64 // For the object address.

#define LINE(number) (4 + (number * 18))

#define CREATESTATIC(string, x, y) SendMessage( \
CreateWindowEx(0, "STATIC", string, WS_VISIBLE | WS_CHILD, x, y, 90, 16, pageObjects.hwnd, NULL, mainModule, NULL), \
WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);

#define CREATEANIMCOUNT(x, y) \
{ \
	CREATESTATIC("Anims:", x, y); \
	anim_count = \
		CreateWindowEx(0, "STATIC", "[unknown]", WS_VISIBLE | WS_CHILD, x + 90, y, 200, 16, pageObjects.hwnd, NULL, mainModule, NULL); \
	SendMessage(anim_count, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1); \
}

char* recbuffer;
static bool recording, playing;
int recpos;
int recbuffersize;
uint32 StringToHex(const char* string);

void Spyro1BetaTestFunction() {
	if (!memory)
		return;

	uint32* uintmem = (uint32*) memory;
	uint16* shortmem = (uint16*) memory;

	if (HIWORD(GetKeyState('R'))) {
		recording = 1;

		if (Reset_Recompiler) Reset_Recompiler();

		MessageBox(NULL, "Recording!", "", MB_OK);
	}

	if (HIWORD(GetKeyState('S'))) {
		if (recording) {
			FILE* f = fopen(".\\RECORD.rec", "wb");

			fwrite(recbuffer, 1, recpos * 42, f);

			fclose(f);
		}

		recording = 0;
		recpos = 0;
		playing = 0;

		free(recbuffer);
		recbuffer = NULL;
		recbuffersize = 0;

		uintmem[0x0006b1e8 / 4] = 0xa0620000;
		if (Reset_Recompiler) Reset_Recompiler();

		MessageBox(NULL, "Stopped.", "", MB_OK);
	}

	if (HIWORD(GetKeyState('P')) && !recording) {
		FILE* f = fopen(".\\RECORD.rec", "rb");
		int fsize;

		fseek(f, 0, SEEK_END); fsize = ftell(f); fseek(f, 0, SEEK_SET);

		recbuffer = (char*) malloc(fsize);
		fread(recbuffer, 1, fsize, f);

		fclose(f);

		uintmem[0x0006b1e8 / 4] = 0x00000000;
		if (Reset_Recompiler) Reset_Recompiler();

		playing = 1;
		recpos = 1;
		recbuffersize = fsize;

		MessageBox(NULL, "Playing!", "", MB_OK);
	}

	if (recording) {
		if (recbuffersize < (recpos + 1) * 42) {
			recbuffersize += 65535;
			recbuffer = (char*) realloc(recbuffer, recbuffersize);
		}

		memcpy(&recbuffer[recpos * 42], &shortmem[0x00077512 / 2], 2);
		memcpy(&recbuffer[recpos * 42 + 2], &shortmem[0x000778C8 / 2], 36);
		memcpy(&recbuffer[recpos * 42 + 38], &shortmem[0x000779E4 / 2], 2);

		recpos ++;
	}

	if (playing) {
		if (recpos * 42 < recbuffersize) {
			memcpy(&shortmem[0x00077512 / 2], &recbuffer[(recpos + 1) * 42], 2);
			memcpy(&shortmem[0x000778C8 / 2], &recbuffer[recpos * 42 + 2], 36);
			memcpy(&shortmem[0x000779E4 / 2], &recbuffer[recpos * 42 + 38], 2);
		} else {
			// End of recording has been reached
			free(recbuffer);
			recbuffer = NULL;
			recbuffersize = 0;

			uintmem[0x0006b1e8 / 4] = 0xa0620000;
			if (Reset_Recompiler) Reset_Recompiler();

			playing = 0;
		}

		recpos ++;
	}
}

void CREATEVAR(char* string, int x, int y, int type, int offset);

VarDef vardefs[50];

void CreateObjectPage() {
	CREATESTATIC("Object ID:", 10, LINE(0));

	CREATEVAR("X:", 10, LINE(2), TYPE_INT, 0x0C);
	CREATEVAR("Y:", 10, LINE(3), TYPE_INT, 0x10);
	CREATEVAR("Z:", 10, LINE(4), TYPE_INT, 0x14);

	CREATEVAR("Type:", 10, LINE(6), TYPE_SHORT, 0x36);
	
	CREATEVAR("Anim1:", 10, LINE(8), TYPE_CHAR, 0x3C);
	CREATEVAR("Anim2:", 10, LINE(9), TYPE_CHAR, 0x3D);
	CREATEVAR("Frame1:", 10, LINE(10), TYPE_CHAR, 0x3E);
	CREATEVAR("Frame2:", 10, LINE(11), TYPE_CHAR, 0x3F);

	CREATEVAR("X Angle:", 10, LINE(13), TYPE_UCHAR, 0x44);
	CREATEVAR("Y Angle:", 10, LINE(14), TYPE_UCHAR, 0x45);
	CREATEVAR("Z Angle:", 10, LINE(15), TYPE_UCHAR, 0x46);

	CREATEVAR("Attack flags:", 10, LINE(17), TYPE_UINT, 0x18);
	CREATEVAR("State:", 10, LINE(18), TYPE_CHAR, 0x48);

	CREATEVAR("MaxDrawDist", 10, LINE(20), TYPE_CHAR, 0x4C);
	CREATEVAR("LowPolyDist", 10, LINE(21), TYPE_CHAR, 0x4E);

	CREATEVAR("ObjectAddr", 10, LINE(23), TYPE_READONLY, 0x00);

	CREATEANIMCOUNT(10, LINE(24));

	object_id = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 100, LINE(0), 50, 17, pageObjects.hwnd, NULL, mainModule, NULL);

	button_goto = CreateWindowEx(0, "BUTTON", "Goto", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 100, LINE(1), 50, 17, pageObjects.hwnd, NULL, mainModule, NULL);

	button_get = 
	CreateWindowEx(0, "BUTTON", "Get values", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, LINE(25), 80, 18, pageObjects.hwnd, NULL, mainModule, NULL);
	button_set = 
	CreateWindowEx(0, "BUTTON", "Set values", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 150, LINE(25), 80, 18, pageObjects.hwnd, NULL, mainModule, NULL);

	button_butterflies = 
	CreateWindowEx(0, "BUTTON", "Find Nearby Objects", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, LINE(26), 180, 19, pageObjects.hwnd, NULL, mainModule, NULL);

	button_spawncopy = 
	CreateWindowEx(0, "BUTTON", "Spawn Copy", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 235, LINE(25), 70, 16, pageObjects.hwnd, NULL, mainModule, NULL);

	button_next = 
	CreateWindowEx(0, "BUTTON", "Next Type", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, LINE(6) - 9, 70, 16, pageObjects.hwnd, NULL, mainModule, NULL);

	button_prev = 
	CreateWindowEx(0, "BUTTON", "Prev Type", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 240, LINE(6) + 9, 70, 16, pageObjects.hwnd, NULL, mainModule, NULL);

	checkbox_autoupdate = 
	CreateWindowEx(0, "BUTTON", "Auto-Update", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_LEFTTEXT, 200, LINE(0), 110, 16,
		pageObjects.hwnd, NULL, mainModule, NULL);

	checkbox_drag = 
	CreateWindowEx(0, "Button", "Drag with Spyro", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_LEFTTEXT, 200, LINE(1), 110, 16, 
		pageObjects.hwnd, NULL, mainModule, NULL);

	SendMessage(object_id, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_goto, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_get, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_set, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_next, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_prev, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_spawncopy, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(button_butterflies, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(checkbox_autoupdate, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(checkbox_drag, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
}

void ObjectEditorLoop() {
	static int lastobjid = 0;
	static int reldragx, reldragy, reldragz;
	static bool lastchecked = 0;

	if (!mobys)
		return;

	if (SendMessage(checkbox_autoupdate, BM_GETCHECK, 0, 0) == BST_CHECKED && GetObjectID() >= 0)
		UpdateVars();

	if (SendMessage(checkbox_drag, BM_GETCHECK, 0, 0) == BST_CHECKED && GetObjectID() >= 0) {
		int id = GetObjectID();

		if (id != lastobjid || !lastchecked) {
			reldragx = mobys[id].x - spyro->x;
			reldragy = mobys[id].y - spyro->y;
			reldragz = mobys[id].z - spyro->z;
		}

		mobys[id].SetPosition(spyro->x + reldragx, spyro->y + reldragy, spyro->z + reldragz);

		lastobjid = id;
		lastchecked = 1;
	} else {
		lastchecked = 0;
	}

	// Show animcount
	char temp[40];

	int id = GetObjectID();

	if (GetObjectID() < 0)
		return;

	if (mobyModels) {
		if ((mobyModels[mobys[id].type].address & 0x003FFFFF) < 0x00200000)
			sprintf(temp, "%02X", mobyModels[mobys[id].type]->numAnims);
		else
			sprintf(temp, "[no model found]");
	} else
		sprintf(temp, "[model list undetected]");

	SendMessage(anim_count, WM_SETTEXT, 0, (LPARAM) temp);

	// Fix anim issue.
	if (GetObjectID() >= 0) { // Redundant check due to the above return;, but keep just in case
		return; // Causes crashes sometimes?
		if (mobys[GetObjectID()].animProgress == 0x00)
			mobys[GetObjectID()].animProgress = 0x00;
	}
}

void UpdateVars() {
	int id = GetObjectID();

	if (id < 0)
		return;

	if (!mobys)
		return;

	for (int i = 0; i < 50; i ++) {
		if (!vardefs[i].type)
			continue;

		if (SendMessage(checkbox_autoupdate, BM_GETCHECK, 0, 0) == BST_CHECKED && 
			vardefs[i].focused) // If they're using auto-update and want to modify something..
			continue;

		char write_string[15];
		uintptr addr = ((uintptr) &mobys[id] + vardefs[i].offset);

		char* convert = "%i";
		char* hexconvert;

		switch (vardefs[i].type) {
			case TYPE_UINT:
			case TYPE_INT:
				hexconvert = "%08X"; break;
			case TYPE_USHORT:
			case TYPE_SHORT:
				hexconvert = "%04X"; break;
			case TYPE_UCHAR:
			case TYPE_CHAR:
				hexconvert = "%02X"; break;
		}

		if (vardefs[i].hex)
			convert = hexconvert;

		switch (vardefs[i].type) {
			case TYPE_UINT:     sprintf(write_string, convert[1] == 'i' ? "%u" : convert, *(unsigned int*) addr); break;
			case TYPE_INT:      sprintf(write_string, convert, *(int*) addr); break;
			case TYPE_USHORT:   sprintf(write_string, convert, *(unsigned short*) addr); break;
			case TYPE_SHORT:    sprintf(write_string, convert, *(short*) addr); break;
			case TYPE_UCHAR:    sprintf(write_string, convert, *(unsigned char*) addr); break;
			case TYPE_CHAR:     sprintf(write_string, convert, *(char*) addr); break;
			case TYPE_READONLY: sprintf(write_string, "%08X", ((uint32)&mobys[id] - (uint32)memory) | 0x80000000); break;
		}

		SendMessage(vardefs[i].edit_hwnd, WM_SETTEXT, 15, (LPARAM) write_string);
	}
}

void UpdateVarHexToggle(int id) {
	char in[100];
	char out[100];
	bool last_hex = vardefs[id].hex; // Just in case.

	vardefs[id].hex = (SendMessage(vardefs[id].hex_toggle, BM_GETCHECK, 0, 0) == BST_CHECKED);

	// Convert the value if this has been changed from non-hex to hex.
	if (vardefs[id].hex && !last_hex) {
		unsigned __int64 value = 0x00000000;

		SendMessage(vardefs[id].edit_hwnd, WM_GETTEXT, 100, (LPARAM) in);

		value = atoi(in);

		switch (vardefs[id].type) {
			case TYPE_UINT:
			case TYPE_INT:
				sprintf(out, "%08X", value & 0xFFFFFFFF); break;
			case TYPE_USHORT:
			case TYPE_SHORT:
				sprintf(out, "%04X", value & 0xFFFFFFFF); break;
			case TYPE_UCHAR:
			case TYPE_CHAR:
				sprintf(out, "%02X", value & 0xFFFFFFFF); break;
		}

		SendMessage(vardefs[id].edit_hwnd, WM_SETTEXT, 9, (LPARAM) out);
	}

	if (!vardefs[id].hex && last_hex) {
		unsigned __int64 value = 0x00000000;

		SendMessage(vardefs[id].edit_hwnd, WM_GETTEXT, 100, (LPARAM) in);

		// Hack - convert negative numbers too...in a hacky way!
		if (in[0] == '-')
			value = 0x100000000ll - StringToHex(&in[1]);
		else
			value = StringToHex(&in[0]);

		sprintf(out, "%u", value & 0xFFFFFFFF);

		SendMessage(vardefs[id].edit_hwnd, WM_SETTEXT, 9, (LPARAM) out);
	}
}

void SetVars() {
	int id = GetObjectID();

	if (!mobys || id < 0)
		return;

	char string[500];

	for (int i = 0; i < 50; i ++) {
		if (!vardefs[i].type)
			continue;

		SendMessage(vardefs[i].edit_hwnd, WM_GETTEXT, 500, (LPARAM) string);

		int number = atoi(string);

		if (vardefs[i].hex)
			number = StringToHex(string);

		uintptr addr = (uintptr) &mobys[id] + vardefs[i].offset;

		switch (vardefs[i].type) {
			case TYPE_UINT:
				*(unsigned int*) addr = number;
				break;
			case TYPE_INT:
				*(int*) addr = number;
				break;
			case TYPE_USHORT:
				*(unsigned short*) addr = number;
				break;
			case TYPE_SHORT:
				*(short*) addr = number;
				break;
			case TYPE_UCHAR:
				*(unsigned char*) addr = number;
				break;
			case TYPE_CHAR:
				*(char*) addr = number;
				break;
		}
	}
}

int GetObjectID() {
	int id;
	char text[500];

	SendMessage(object_id, WM_GETTEXT, 500, (LPARAM) text);

	id = atoi(text);

	if (id < 0)
		return -1;

	return id;
}

int NumObjects() {
	if (!mobys)
		return 0;

	for (int num = 0; num < 600; num ++) {
		if (mobys[num].state == -1) {
			return num;
		}
	}

	return 0;
}

uint32 StringToHex(const char* string) {
	int i;
	uint32 result = 0;

	for (i = 0; i < 8; i++) {
		if (!string[i])
			return result;

		char digit = string[i];

		if (digit >= '0' && digit <= '9')
			result |= (digit - '0') << (i << 2);
		else if (digit >= 'A' && digit <= 'F')
			result |= (digit - 'A') << (i << 2);
		else if (digit >= 'a' && digit <= 'f')
			result |= (digit - 'a') << (i << 2);
		else
			return 0;
	}

	return result;
}

void CREATEVAR(char* string, int x, int y, int type, int offset) {
	VarDef* new_vardef;

	for (int i = 0; i < 50; i++) {
		if (!vardefs[i].type)
			new_vardef = &vardefs[i];
	}

	new_vardef->type = type;
	new_vardef->offset = offset;

	new_vardef->edit_hwnd = CreateWindowEx(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, x + 90, y, 80, 17, pageObjects.hwnd, NULL, mainModule, NULL);

	new_vardef->hex_toggle = CreateWindowEx(0, "BUTTON", "Hex", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 
											x + 180, y, 50, 17, pageObjects.hwnd, (HMENU) 14, mainModule, NULL);

	SendMessage(CreateWindowEx(0, "STATIC", string, WS_CHILD | WS_VISIBLE, x, y, 90, 17, pageObjects.hwnd, NULL, mainModule, NULL), WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);

	SendMessage(new_vardef->edit_hwnd, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
	SendMessage(new_vardef->hex_toggle, WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 1);
}
