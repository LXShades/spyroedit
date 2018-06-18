#include <Windows.h>
#include <string>
#include "SpyroData.h"

void SaveMobys() {
	if (!mobys) {
		return;
	}

	char filename[MAX_PATH];
	GetLevelFilename(filename, SEF_MOBYLAYOUT, true);
	HANDLE file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return;

	std::string fileData;

	fileData.reserve(12800);

	for (int i = 0; i < 500; ++i) {
		if (mobys[i].state == -1) {
			break;
		}

		char tempBuffer[250];
		sprintf(tempBuffer, "X: %i, Y: %i, Z: %i, RotX: %i, RotY: %i, RotZ: %i\x0D\x0A", mobys[i].x, mobys[i].y, mobys[i].z, mobys[i].angle.x, mobys[i].angle.y, mobys[i].angle.z);

		fileData.append(tempBuffer);
	}
	
	DWORD written;
	WriteFile(file, fileData.c_str(), fileData.length(), &written, nullptr);

	CloseHandle(file);
}

void LoadMobys() {
	if (!mobys) {
		return;
	}

	char filename[MAX_PATH];
	GetLevelFilename(filename, SEF_MOBYLAYOUT, true);
	HANDLE file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (file == INVALID_HANDLE_VALUE)
		return;

	char* fileData = new char[GetFileSize(file, nullptr) + 1];
	DWORD read;
	ReadFile(file, fileData, GetFileSize(file, nullptr), &read, nullptr);
	fileData[GetFileSize(file, nullptr)] = '\0';

	std::string fileString(fileData);
	int index = 0;

	for (int i = 0; i < 500; ++i) {
		if (mobys[i].state == -1) {
			break;
		}
		
		index = fileString.find_first_not_of(" \r\n", index);

		while (index >= 0 && fileString[index] != '\r' && fileString[index] != '\n' && fileString[index]) {
			int tagEnd = fileString.find_first_of(":", index);
			int valueEnd = fileString.find_first_of(",\r\n", tagEnd);
			const char* const dur = &fileString.c_str()[index];
			auto interpretParams = [&](auto& nameArray, auto& ptrArray) {
				// What the heck am I writing
				for (int i = 0; i < nameArray.size(); ++i) {
					if (!fileString.compare(index, tagEnd - index, nameArray.begin()[i])) {
						*(ptrArray.begin()[i]) = std::stoi(fileString.substr(fileString.find_first_not_of(" ", tagEnd + 1), valueEnd - index));
						break;
					}
				}
			};
			
			// omg isn't this too easy now? C++11 is magic
			interpretParams(std::initializer_list<const char*>{"X", "Y", "Z"}, std::initializer_list<int*>{&mobys[i].x, &mobys[i].y, &mobys[i].z});
			interpretParams(std::initializer_list<const char*>{"RotX", "RotY", "RotZ"}, std::initializer_list<int8*>{&mobys[i].angle.x, &mobys[i].angle.y, &mobys[i].angle.z});
			
			int lastIndex = index;
			index = fileString.find_first_not_of(", ", valueEnd);

			const char* debug = &fileString.c_str()[lastIndex];
			const char* debug2 = &fileString.c_str()[index];
		
			if (index < 0) {
				goto EndFunction;
			}
		}

		if (index < 0 || (fileString[index] != '\n' && fileString[index] != '\r')) {
			break;
		}
	}

	EndFunction:;
	delete[] fileData;

	CloseHandle(file);
}