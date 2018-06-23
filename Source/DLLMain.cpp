#include "Main.h"
#include "Types.h"
#include "Vram.h"
#include <windows.h>

#ifdef SPYRORENDER
#include "SpyroRenderer/SpyroRender.h"
#endif

#define DLLFUNCTION __declspec(dllexport)

#define LOADROUTE(function, type) route_##function = type GetProcAddress(route_GPU_module, #function)

#define STR(a) #a

extern HMODULE mainModule;

void Startup();
void Shutdown();
void Open_Windows();
void Close_Windows();
void WinLoop();

extern "C"
{
HMODULE route_GPU_module;

char plugin_name[] = "SpyroEdit";

void (__stdcall * Reset_Recompiler)(void);
long (__stdcall * route_GPUinit)(void);
long (__stdcall * route_GPUshutdown)(void);
long (__stdcall * route_GPUopen)(HWND);
long (__stdcall * route_GPUclose)(void);
void (__stdcall * route_GPUwriteStatus)(unsigned long);
void (__stdcall * route_GPUwriteData)(unsigned long);
void (__stdcall * route_GPUwriteDataMem)(unsigned long *, int);
unsigned long (__stdcall * route_GPUreadStatus)(void);
unsigned long (__stdcall* route_GPUreadData)(void);
void (__stdcall* route_GPUreadDataMem)(unsigned long *, int);
long (__stdcall* route_GPUdmaChain)(unsigned long *,unsigned long);
void (__stdcall* route_GPUupdateLace)(void);
long (__stdcall* route_GPUconfigure)(void);
long (__stdcall* route_GPUtest)(void);
void (__stdcall* route_GPUabout)(void);
void (__stdcall* route_GPUmakeSnapshot)(void);
void (__stdcall* route_GPUkeypressed)(int);
void (__stdcall* route_GPUdisplayText)(char *);
typedef struct {
	unsigned long ulFreezeVersion;
	unsigned long ulStatus;
	unsigned long ulControl[256];
	unsigned char psxVRam[1024*512*2];
} GPUFreeze_t;
long (__stdcall* route_GPUfreeze)(unsigned long, GPUFreeze_t *);
long (__stdcall* route_GPUgetScreenPic)(unsigned char *);
long (__stdcall* route_GPUshowScreenPic)(unsigned char *);
void (__stdcall* route_GPUclearDynarec)(void (__stdcall *callback)(void));
void (__stdcall* route_GPUsetMode)(unsigned long);
long (__stdcall* route_GPUgetMode)(void);

void LoadRouteGPU();

unsigned __int32 ulStatus;
unsigned __int32 ulStatusControl[256];

DLLFUNCTION void __stdcall GPUwriteDataMem(unsigned long* mem, int size);

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_THREAD_ATTACH || fdwReason == DLL_THREAD_DETACH)
		return TRUE;

	if (mainModule == NULL)
		mainModule = (HMODULE) hinstDLL;

	if (fdwReason == DLL_PROCESS_DETACH /* || fdwReason == DLL_THREAD_DETACH)*/) {
		if (route_GPU_module == NULL)
			return FALSE;

		FreeLibrary(route_GPU_module);
		route_GPU_module = NULL;

		return FALSE;
	}

	return TRUE;
}

DLLFUNCTION long __stdcall GPUinit(void) {
	LoadRouteGPU();

	Startup();

	memset(ulStatusControl, 0, sizeof (ulStatusControl));

	if (route_GPUinit)
		return route_GPUinit();
	else
		return 0;
}

DLLFUNCTION long __stdcall GPUshutdown(void) {
	Shutdown();
	
	if (route_GPUshutdown)
		return route_GPUshutdown();
	else
		return 0;
}

DLLFUNCTION long __stdcall GPUopen(HWND hwndGPU) {
	LoadRouteGPU();

	Open_Windows();

	//Reset_Recompiler();

	return route_GPUopen(hwndGPU);
}

DLLFUNCTION long __stdcall GPUclose(void) {
	Close_Windows();

	return route_GPUclose();
}

DLLFUNCTION long __stdcall GPUdmaChain(unsigned long* baseAddrL, unsigned long addr) {
	memory = (void*) baseAddrL; // Set the PlayStation memory pointer
	umem8 = (uint8*) baseAddrL;
	umem16 = (uint16*) baseAddrL;
	umem32 = (uint32*) baseAddrL;

	return route_GPUdmaChain(baseAddrL, addr);
}


DLLFUNCTION void __stdcall GPUwriteStatus(unsigned long gdata);
DLLFUNCTION void __stdcall GPUupdateLace(void) {
	static char c = 0;

	c ++; if (c > 1) c = 0;

	if (!c)
		WinLoop();

#ifdef SPYRORENDER
	SpyroRender::OnUpdateLace();
#endif

	route_GPUupdateLace();
}

DLLFUNCTION void __stdcall GPUclearDynarec(void (__stdcall* callback_function)()) {
	Reset_Recompiler = callback_function;

	if (route_GPUclearDynarec != NULL)
		route_GPUclearDynarec(callback_function);
}

DLLFUNCTION void __stdcall GPUsetMode(unsigned long gdata) {
	if (route_GPUsetMode != NULL)
		route_GPUsetMode(gdata);
}

DLLFUNCTION long __stdcall GPUgetMode() {
	if (route_GPUgetMode != NULL)
		return route_GPUgetMode();

	return 0;
}

DLLFUNCTION unsigned long __stdcall GPUreadData(void) {
	return route_GPUreadData();
}

DLLFUNCTION void __stdcall GPUreadDataMem(unsigned long* route_argone, int route_argtwo) {
	route_GPUreadDataMem(route_argone, route_argtwo);
}

DLLFUNCTION unsigned long __stdcall GPUreadStatus(void) {
	return route_GPUreadStatus();
}

DLLFUNCTION void __stdcall GPUwriteData(unsigned long route_argument) {
	route_GPUwriteData(route_argument);
}

DLLFUNCTION void __stdcall GPUwriteDataMem(unsigned long* mem, int size) {
	route_GPUwriteDataMem(mem, size);
}

DLLFUNCTION void __stdcall GPUwriteStatus(unsigned long gdata) {
	unsigned long lCommand=(gdata>>24)&0xff;

	// LX: Store for savestates
	ulStatusControl[lCommand]=gdata;

	route_GPUwriteStatus(gdata);
}

// Extra functions
void GetSnapshot(GPUSnapshot* out) {
	GPUFreeze_t* tempFreeze = (GPUFreeze_t*) malloc(sizeof (GPUFreeze_t));
	tempFreeze->ulFreezeVersion = 1;

	route_GPUfreeze(1, tempFreeze);
	
	memcpy(out->vram, tempFreeze->psxVRam, 1024 * 1024);
	memcpy(out->ulControl, tempFreeze->ulControl, sizeof (tempFreeze->ulControl));
	out->ulStatus = tempFreeze->ulStatus;

	free(tempFreeze);
}

void SetSnapshot(const GPUSnapshot* in) {
	GPUFreeze_t* tempFreeze = (GPUFreeze_t*) malloc(sizeof (GPUFreeze_t));
	tempFreeze->ulFreezeVersion = 1;

	memcpy(tempFreeze->psxVRam, in->vram, 1024 * 1024);
	memcpy(tempFreeze->ulControl, in->ulControl, sizeof (tempFreeze->ulControl));
	tempFreeze->ulStatus = in->ulStatus;

	route_GPUfreeze(0, tempFreeze);

	free(tempFreeze);
}

// Miscellaneous function rerouting.
DLLFUNCTION void __stdcall GPUdisplayText(char* route_argument) {route_GPUdisplayText(route_argument);}
DLLFUNCTION void __stdcall GPUmakeSnapshot(void) {route_GPUmakeSnapshot();}
DLLFUNCTION long __stdcall GPUfreeze(unsigned long ulGetFreezeData, GPUFreeze_t* pF) {return route_GPUfreeze(ulGetFreezeData, pF);}
DLLFUNCTION long __stdcall GPUgetScreenPic(unsigned char* route_argument) {return route_GPUgetScreenPic(route_argument);}
DLLFUNCTION long __stdcall GPUshowScreenPic(unsigned char* route_argument) {return route_GPUshowScreenPic(route_argument);}

bool configuring = false;

LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CLOSE:
			configuring = false;
			break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

DLLFUNCTION long __stdcall GPUconfigure(void) {
/*	WNDCLASSEX derp;
	MSG message;
	HINSTANCE inst = GetModuleHandle(NULL);

	// Create config window
	ZeroMemory(&derp, sizeof (derp));

	derp.cbSize = sizeof (derp);
	derp.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE) + 1;
	derp.lpfnWndProc = ConfigWndProc;
	derp.lpszClassName = "SpyroEditConfig";
	derp.hInstance = inst;

	if (!RegisterClassEx(&derp))
		goto End;

	HWND hwndConfig = CreateWindowEx(0, "SpyroEditConfig", "SpyroEdit " STR(VERSION) " Config", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
									CW_USEDEFAULT, CW_USEDEFAULT, 300, 100, NULL, NULL, inst, NULL);

	if (!hwndConfig)
		goto End;

	// Create controls
	HWND ctrlPluginList = CreateWindowEx(0, "ComboBox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 16, 200, 200, hwndConfig, NULL, inst, NULL);

	// Find plugins in folder and add them to the plugin list
	WIN32_FIND_DATA find;
	HANDLE hFind;

	hFind = FindFirstFile("*", &find);

	if (hFind) {
		while (FindNextFile(hFind, &find)) {
			int i;
			i = 0;
		}
	}

	FindClose(hFind);

	// Begin message loop
	configuring = true;
	while (configuring) {
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE) > 0) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		Sleep(10);
	}

	End:
	UnregisterClass("SpyroEditConfig", GetModuleHandle(NULL));*/
	if (!route_GPU_module)
		LoadRouteGPU();

	if (route_GPUconfigure)
		return route_GPUconfigure();
	else
		return 0;
}

DLLFUNCTION long __stdcall GPUtest(void) {return 0;}
DLLFUNCTION void __stdcall GPUabout(void) {MessageBox(NULL, INFORMATION, "About", MB_OK); return;}
DLLFUNCTION char* __stdcall PSEgetLibName() {return plugin_name;}
DLLFUNCTION unsigned long __stdcall PSEgetLibType() {return 2;}
DLLFUNCTION unsigned long __stdcall PSEgetLibVersion() {return (RELEASE << 16) | (VERSION << 8) | SUBVERSION;}

void LoadRouteGPU() {
	if (route_GPU_module) {
		return; // GPU already loaded
	}

	route_GPU_module = LoadLibrary(".\\plugins\\SPYRO3GPU.dll");

	if (route_GPU_module == NULL) {
		route_GPU_module = LoadLibrary(".\\SPYRO3GPU.dll");
		if (route_GPU_module == NULL) {
			MessageBox(NULL, "[Spyro 3 Live Editor] Failed to load GPU.\n\n"
				"GPU plugin should be in 'plugins\\SPYRO3GPU.dll'", "", MB_OK);

			return;
		}
	}

	LOADROUTE(GPUinit,          (long          (__stdcall *)()));
	LOADROUTE(GPUshutdown,      (long          (__stdcall*)()));
	LOADROUTE(GPUopen,          (long          (__stdcall*)(HWND)));
	LOADROUTE(GPUclose,         (long          (__stdcall*)()));
	LOADROUTE(GPUreadData,      (unsigned long (__stdcall*)()));
	LOADROUTE(GPUreadDataMem,   (void          (__stdcall*)(unsigned long*, int)));
	LOADROUTE(GPUreadStatus,    (unsigned long (__stdcall*)()));
	LOADROUTE(GPUwriteData,     (void          (__stdcall*)(unsigned long)));
	LOADROUTE(GPUwriteDataMem,  (void          (__stdcall*)(unsigned long*, int)));
	LOADROUTE(GPUwriteStatus,   (void          (__stdcall*)(unsigned long)));
	LOADROUTE(GPUdmaChain,      (long          (__stdcall*)(unsigned long*, unsigned long)));
	LOADROUTE(GPUupdateLace,    (void          (__stdcall*)()));
	LOADROUTE(GPUdisplayText,   (void          (__stdcall*)(char*)));
	LOADROUTE(GPUmakeSnapshot,  (void          (__stdcall*)()));
	LOADROUTE(GPUfreeze,        (long          (__stdcall*)(unsigned long, GPUFreeze_t*)));
	LOADROUTE(GPUgetScreenPic,  (long          (__stdcall*)(unsigned char*)));
	LOADROUTE(GPUshowScreenPic, (long          (__stdcall*)(unsigned char*)));
	LOADROUTE(GPUclearDynarec,  (void          (__stdcall*)(void (__stdcall*)())));
	LOADROUTE(GPUsetMode,       (void          (__stdcall*)(unsigned long)));
	LOADROUTE(GPUgetMode,       (long          (__stdcall*)()));
	LOADROUTE(GPUconfigure,     (long          (__stdcall*)()));

	route_GPUinit();
}

}
