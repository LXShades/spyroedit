#pragma once
// SDERROR: Reports an error. Used in engine functions. Depending on debug modes, errors may be collected.
#define SDERROR(condition, errorCode, ...) Sys::Error(condition, __FUNCDNAME__, errorCode, #errorCode, __VA_ARGS__)
// SDWARNING: Reports a warning. Used in engine functions to report a non-fatal error, or an invalid function argument that was changed/ignored to prevent erroros. Debug only.
#define SDWARNING(...) Sys::Warning(__VA_ARGS__)

#ifdef _DEBUG
#define __sdederp(a) #a
#define __sdeherp(a) __sdederp(a)
#define SDASSERT(condition) do {if (!(condition)) Sys::Assert(#condition "\n\nCondition failed!\n\nin " __FILE__ "\nline " __sdeherp(__LINE__) "\n\nBreak here?");} while(0)
#else
#define SDASSERT(condition) do {if (!(condition)) break;} while (0) // condition may have a function, so a dummy 'if' statement is executed here just in case
#endif

 enum SdError // general errors (reserved)
 {
	 SDE_MISC = 0,   // Note: use this error macro when using SDERROR with unknown/placeholder errors
	 SDE_RESERVED = 1, // Starting error code for all sub-errors
 };
 
enum SDRendererError {
	SDRE_D3DINITFAILED = SDE_RESERVED,
	SDRE_DXGIINITFAILED,
	SDRE_DEFSHADERSFAILED,
	SDRE_INPUTLAYOUTFAILED,
	SDRE_SCRATCHBUFFERFAILED,
	SDRE_CBUFFERLOCKFAILED,
	SDRE_VBUFFERLOCKFAILED,
	SDRE_WICERROR,
	SDRE_WICOFFLINE,
	SDRE_BMPERROR,
	SDRE_FILEFAILED, // failed to create or open file

	SDRE_WINFONTFAILED,
};

enum SdRendererResourceError {
	SDRRE_D3DERROR = SDE_RESERVED,
	SDRRE_SWAPCHAINFAIL,
	SDRRE_SURFACEINITFAIL,
	SDRRE_DEPTHINITFAIL,
	SDRRE_TEXTSURFACEFAIL,
	SDRRE_FLAGCONFLICT, 
	SDRRE_NOACCESS,
};

enum SdUiError {
	SDUIE_NOVIRTUALFUNCTIONS = 0
};

enum SdSdmError {
	SDME_FILEERROR = SDE_RESERVED,
	SDME_BADFORMAT,
	SDME_TOOMANYTYPES,
};