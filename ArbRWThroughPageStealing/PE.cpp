#include "PE.h"

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

char pdbFileNameBuffer[0x100];
char guidAsString[0x100];

// 
// Check whether the specified IMAGE_OPTIONAL_HEADER belongs to 
// a PE32 or PE32+ file format 
// 
// Return value: "true" if succeeded (bPE32Plus contains "true" if the file 
//  format is PE32+, and "false" if the file format is PE32), 
//  "false" if failed 
// 
bool IsPE32Plus(PIMAGE_OPTIONAL_HEADER pOptionalHeader, bool& bPE32Plus)
{
	// Note: The function does not check the header for validity. 
	// It assumes that the caller has performed all the necessary checks. 

	// IMAGE_OPTIONAL_HEADER.Magic field contains the value that allows 
	// to distinguish between PE32 and PE32+ formats 

	if (pOptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		// PE32 
		bPE32Plus = false;
	}
	else if (pOptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		// PE32+
		bPE32Plus = true;
	}
	else
	{
		// Unknown value -> Report an error 
		bPE32Plus = false;
		return false;
	}

	return true;

}

// 
// The function walks through the section headers, finds out the section 
// the given RVA belongs to, and uses the section header to determine 
// the file offset that corresponds to the given RVA 
// 
// Return value: "true" if succeeded, "false" if failed 
// 
bool GetFileOffsetFromRVA(PIMAGE_NT_HEADERS pNtHeaders, DWORD Rva, DWORD& FileOffset)
{
	// Check parameters 

	if (pNtHeaders == 0)
	{
		_ASSERT(0);
		return false;
	}


	// Look up the section the RVA belongs to 

	bool bFound = false;

	PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeaders);

	for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++, pSectionHeader++)
	{
		DWORD SectionSize = pSectionHeader->Misc.VirtualSize;

		if (SectionSize == 0) // compensate for Watcom linker strangeness, according to Matt Pietrek 
			pSectionHeader->SizeOfRawData;

		if ((Rva >= pSectionHeader->VirtualAddress) &&
			(Rva < pSectionHeader->VirtualAddress + SectionSize))
		{
			// Yes, the RVA belongs to this section 
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		// Section not found 
		return false;
	}


	// Look up the file offset using the section header 

	INT Diff = (INT)(pSectionHeader->VirtualAddress - pSectionHeader->PointerToRawData);

	FileOffset = Rva - Diff;


	// Complete 

	return true;

}

// 
// Returns (in [out] parameters) the RVA and size of the debug directory, 
// using the information in IMAGE_OPTIONAL_HEADER.DebugDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
// 
// Return value: "true" if succeeded, "false" if failed
// 
bool GetDebugDirectoryRVA(PIMAGE_OPTIONAL_HEADER pOptionalHeader, DWORD& DebugDirRva, DWORD& DebugDirSize)
{
	// Check parameters 

	if (pOptionalHeader == 0)
	{
		_ASSERT(0);
		return false;
	}


	// Determine the format of the PE executable 

	bool bPE32Plus = false;

	if (!IsPE32Plus(pOptionalHeader, bPE32Plus))
	{
		// Probably invalid IMAGE_OPTIONAL_HEADER.Magic
		return false;
	}


	// Obtain the debug directory RVA and size 

	if (bPE32Plus)
	{
		PIMAGE_OPTIONAL_HEADER64 pOptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)pOptionalHeader;

		DebugDirRva = pOptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

		DebugDirSize = pOptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	}
	else
	{
		PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)pOptionalHeader;

		DebugDirRva = pOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

		DebugDirSize = pOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
	}

	if ((DebugDirRva == 0) && (DebugDirSize == 0))
	{
		// No debug directory in the executable -> no debug information 
		return true;
	}
	else if ((DebugDirRva == 0) || (DebugDirSize == 0))
	{
		// Inconsistent data in the data directory 
		return false;
	}


	// Complete 

	return true;

}

inline bool fileAlredyExist(const std::string& name) {

	std::ifstream f((name + ".pdb").c_str());
	return f.good();
}

bool PE::openFile(char* filename) {
	ntoskrnl.open(filename, std::ios::binary | std::fstream::in);
	if (ntoskrnl.good())
		return true;
	return false;
}

void PE::closeFile() {
	ntoskrnl.close();
}

char* PE::getPdbFile(){
	
	pdbIdentifier pdbIdentifier = parseExeHeader();
	GUID pdbGuid = pdbIdentifier.pdbGuid;
	LPOLESTR guidString;
	if (FAILED(StringFromCLSID(pdbGuid, &guidString)))
		;// fatal((char*)"PE:getPdbFile StringFromCLSID fail");
	
	// BD8C7CDE 0D90 7F0F 55 54 F1 60 B9 9C A6 021
	sprintf_s(guidAsString, "%08lX%04hX%04hX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX", 
		pdbGuid.Data1, pdbGuid.Data2, pdbGuid.Data3,
		pdbGuid.Data4[0], pdbGuid.Data4[1], pdbGuid.Data4[2], pdbGuid.Data4[3],
		pdbGuid.Data4[4], pdbGuid.Data4[5], pdbGuid.Data4[6], pdbGuid.Data4[7]);
	
	if (!fileAlredyExist(guidAsString)) {

		TCHAR szOldTitle[MAX_PATH];
		GetConsoleTitle(szOldTitle, MAX_PATH);
		
		//SetConsoleTitle(_T("downloading pdb"));

		char cmdCommand[0x100];
		//printf("call curl -L http://msdl.microsoft.com/download/symbols/%s/%s%x/%s -o %s.pdb\n", pdbIdentifier.pdbFileName, guidAsString, pdbIdentifier.age, pdbIdentifier.pdbFileName, guidAsString);


		sprintf_s(cmdCommand, 
			"curl -Ls http://msdl.microsoft.com/download/symbols/%s/%s%x/%s -o %s.pdb",
			pdbIdentifier.pdbFileName, 
			guidAsString, 
			pdbIdentifier.age, 
			pdbIdentifier.pdbFileName, 
			guidAsString);

		system(cmdCommand);
		//SetConsoleTitle(szOldTitle);
	}
	
	TCHAR NPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, NPath);
	sprintf_s(pdbFileNameBuffer, "%S\\%s.pdb", NPath, guidAsString);

	return pdbFileNameBuffer;
}

pdbIdentifier PE::parseExeHeader() {

	long long begin, end, fileSize;

	begin = ntoskrnl.tellg();
	ntoskrnl.seekg(0, std::ios::end);
	end = ntoskrnl.tellg();
	fileSize = end - begin;

	char* file = (char*)malloc((size_t)fileSize);
	ntoskrnl.seekg(0, std::ios::beg);
	ntoskrnl.read(file, fileSize);

	char* base_pointer = (char*) file;

	// This is where the MZ...blah header lives (the DOS header)
	IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)base_pointer;

	if (dos_header == NULL)
		;// fatal((char*)"PE::parseExeHeader pNtHeaders dereference null ptr");

	PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)MakePtr(PIMAGE_NT_HEADERS, dos_header, dos_header->e_lfanew);

	// We want the PE header.
	IMAGE_FILE_HEADER* file_header = (IMAGE_FILE_HEADER*)(base_pointer + dos_header->e_lfanew + 4);

	// Straight after that is the optional header (which technically is optional, but in practice always there.)
	IMAGE_OPTIONAL_HEADER* opt_header = (IMAGE_OPTIONAL_HEADER*)(((char*)file_header) + sizeof(IMAGE_FILE_HEADER));

	DWORD debugDirRva = 0;
	DWORD debugDirSize = 0;
	DWORD debugDirOffset = 0;
	
	GetDebugDirectoryRVA(&pNtHeaders->OptionalHeader, debugDirRva, debugDirSize);
	GetFileOffsetFromRVA(pNtHeaders, debugDirRva, debugDirOffset);

	// Convert that data to the right type.
	PIMAGE_DEBUG_DIRECTORY pDebugDir = MakePtr(PIMAGE_DEBUG_DIRECTORY, base_pointer, debugDirOffset);
	
	int NumEntries = debugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);

	struct PdbInfo {
		DWORD     Signature;
		GUID      Guid;
		DWORD     Age;
		char      PdbFileName[256];
	};
	PdbInfo* pdb_info = NULL;
	pdbIdentifier pdbIdent = { 0 };
	// Check to see that the data has the right type
	for (int i = 0; i < NumEntries; pDebugDir++, i++) {
		if (IMAGE_DEBUG_TYPE_CODEVIEW == pDebugDir->Type)
		{
			pdb_info = (PdbInfo*)(base_pointer + pDebugDir->PointerToRawData);
			if (0 == memcmp(&pdb_info->Signature, "RSDS", 4)) {
				pdbIdent.age = pdb_info->Age;
				pdbIdent.pdbFileName = pdb_info->PdbFileName;
				pdbIdent.pdbGuid = pdb_info->Guid;
				free(file);
				return pdbIdent;
				//printf("PDB path: %s\n", pdb_info->PdbFileName);
			}
		}
	}
	free(file);
	return { 0 };
}