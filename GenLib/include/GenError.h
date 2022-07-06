#pragma once
#include "GenType.h"

// GenError: static class for Gen error handling
enum GenErrorCode {
	GENERR_UNKNOWN = -1, // this really shouldn't be used ever

	GENERR_INVALIDARG = 0,
	GENERR_LOCKEDRESOURCE,
	GENERR_UNLOCKEDRESOURCE,
	GENERR_RESOURCEERROR,
	GENERR_GENIDCONFLICT,

	GENERR_MISCWARNING,
	GENERR_THEREISNOERROR, // received when you call GetError and no error has occured
};

struct GenErr {
	GenErr(const char* errorDesc, genu8 errorCode) : errDesc(errorDesc), errCode(errorCode) {};
	GenErr(const char* errorDesc, genu8 errorCode, genbool8 errorWarning) : errDesc(errorDesc), errCode(errorCode), errIsWarning(errorWarning) {};

	const char* errDesc; // human-readable description of error
	genu8 errCode; // error code
	genbool8 errIsWarning; // whether this error is just a warning (suspicious but not fatal)
};

struct GenWarn : public GenErr {
	GenWarn(const char* errorDesc, genu8 errorCode) : GenErr(errorDesc, errorCode, true) {};
};

typedef void (*generrorhandler)(const GenErr& error);

class GenError {
	public:
		static void CallError(const GenErr& error); // Declare an error

		static void SetOnError(generrorhandler handler); // Sets the error handler function, called when an error takes place
		static const GenErr& GetLastError(); // Returns last error occured
};