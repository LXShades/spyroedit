typedef struct
{
	int type;
	int offset;

	bool hex;
	bool focused;

	HWND edit_hwnd;
	HWND hex_toggle;
} VarDef;

void CreateObjectPage();

void ObjectEditorLoop();

void UpdateVars();
void SetVars();
void UpdateVarHexToggle(int id);

int GetObjectID();
int NumObjects();

extern VarDef vardefs[50];
