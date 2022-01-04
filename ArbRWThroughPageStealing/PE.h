#pragma once

#include <fstream>
#include <algorithm>
#include <iterator>
#include <combaseapi.h>

extern char pdbFileNameBuffer[0x100];
extern char guidAsString[0x100];

struct pdbIdentifier {
	GUID pdbGuid;
	DWORD age;
	char* pdbFileName;
};

class PE {
	char sGuid[33] = { 0 };
	std::ifstream ntoskrnl;
public:
	char* getPdbFile();
	bool openFile(char* filename);
	void closeFile();

private:
	pdbIdentifier parseExeHeader();
};