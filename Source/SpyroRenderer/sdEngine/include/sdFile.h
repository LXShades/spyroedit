#pragma once
#include "sdTypes.h"

typedef void* HANDLE;
class FileDetails;

enum FileAccess {
	FA_READ = 1,
	FA_WRITE = 2,
	FA_READWRITE = 3
};

// File class
// Most of these functions are named after the stdio functions
class File {
	public:
		File();
#ifdef DEBUG
		~File();
#endif
		// Create: Creates a file of name 'filename'. Existing files will be overwritten. Read/Write access is expected by default
		bool Create(const wchar* filename, int access = (FA_WRITE | FA_READ));
		// Open: Opens a file of name 'filename' with the specified access rights. If the file doesn't exist, the function will fail regardless of access rights.
		bool Open(const wchar* filename, int access);
		// Close: Closes the file (if a file is open)
		void Close();

		// Seek and Tell: Sets the file stream pointer and gets it, respectively
		// As a general rule, a file's stream pointer never exceeds the end or beginning of the file.
		void Seek(uint64 position, bool isRelative = false);
		uint64 Tell() const;

		// GetSize: Returns file size in bytes
		uint64 GetSize() const;

		/* Read and Write: Reads and writes data from and to files respectively. Returns the amount of bytes read or written.
		// A read request that overruns the end of the file will be capped accordingly.
		   A write request may have an unexpected error (due to Windows) which may result in a partial write. The return value will be the amount of bytes unavoidably
		   written to the file. */
		int Read(void* buffer, uint32 numBytes);
		int Write(const void* buffer, uint32 numBytes);

		/* ReadLine: Reads a line of text (character-limited by maxChars) and advances the file pointer to the next line. Returns amount of characters read, DISCLUDING
					 nullptr terminator. The string is always nullptr-terminated. */
		int ReadLine(char* stringOut, uint32 maxChars);

		// Eof: Checks if the seeker pointer is at or past the end of the file. This is just equivalent to (Tell() == Size()).
		bool Eof() const;

	private:
		void UpdateSize();

		HANDLE hFile;
		uint8 accessFlags;

		uint64 fileSize;

		uint64 streamPointer;
};

class FileSearcher {
	public:
		FileSearcher();
		~FileSearcher();

		// Search functions
		// SearchInFolder: Searches folder '[folderName]' for searchString. A nullptr searchString will find all files or folders in a folder
		bool SearchInFolder(const wchar* folderName, const wchar* searchString = nullptr);

		// Result functions
		int GetNumResults();

		const wchar* GetResultName(int index);
		uint8 GetResultAttributes(int index);

	private:
		void Cleanup();

		Array<FileDetails> results;
};

class FileDetails {
public:
	FileDetails() : name(), fileAttributes(0) {};
	FileDetails(const String& name_, uint8 attributeFlags) : name(name_), fileAttributes(attributeFlags) {};

	void SetName(const wchar* name) {
		this->name = name;
	}

	const String& GetName() const {
		return name;
	}

	void SetAttributes(uint8 attributes) {
		fileAttributes = attributes;
	}

	uint8 GetAttributes() const {
		return fileAttributes;
	}
private:
	String name; // name of the file

	uint8 fileAttributes; // file's attributes, of FileAttributeFlags
};

enum FileAttributeFlags : uint8 {
	FA_DIRECTORY = 1
};

enum FileError {
	SDFE_UNKNOWNERROR = 1,
	SDFE_FOPENFAILED,
	SDFE_BADACCESS,
	SDFE_BADID,
	SDFE_NOFOLDER
};