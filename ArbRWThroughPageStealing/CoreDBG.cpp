#include "CoreDBG.h"

CoreDBG::CoreDBG() {

#ifndef DEBUG_CHECK

#else
	// DbgHelpWrapper init
	boolean Status = DbgPdb.InitializeDbgHelp(NULL);
	if (Status == 0)
#ifdef PRINT
		fatal((char*)"CoreDbg.cpp: DbgPdb.InitializeDbgHelp err")
#endif
		;

	char* pdbPtr = getPdb();

	Status = DbgPdb.LoadSymbols(pdbPtr);
	if (Status == 0)
#ifdef PRINT
		 fatal((char*)"CoreDbg.cpp: DbgPdb.LoadSymbols err");
#endif
		;
	// DiaHelpWrapper init
	DiaPdb.initialize(pdbPtr);
	if (!DiaPdb.IsInitialized)
#ifdef PRINT
		fatal((char*)"DIA2.cpp: DiaPdb not initialized");
#endif
		;
#endif
}

CoreDBG::~CoreDBG() {
	
}

char* CoreDBG::getPdb() {
#ifndef DEBUG_CHECK
	//printf("CHECK DEBUG_CHECK\n getPdb err\n");
	return NULL;
#else

	char krnlAbsPath[256];

	// https://social.msdn.microsoft.com/Forums/en-US/cfdf4474-266b-4ef5-8992-7fbdc3147521/accessing-files-from-system32-directory-using-32-bit-application-on-64-bit-machine?forum=netfx64bit
	// 
	char* buf = nullptr;
	size_t sz = 0;
	if (FAILED(_dupenv_s(&buf, &sz, "WINDIR")))
#ifdef PRINT
		fatal((char*)"_DiaHelpWrapper:initialize _dupenv_s err");
#endif
		;
	sprintf_s(krnlAbsPath, "%s\\System32\\ntoskrnl.exe", buf);
	//sprintf(krnlAbsPath, "%s\\ntoskrnl.exe", "C:");
	//printf("%s\n", krnlAbsPath);
	PVOID OldValue = NULL;

	if (!pe.openFile(krnlAbsPath)) {
		Wow64DisableWow64FsRedirection(&OldValue);
		if (!pe.openFile(krnlAbsPath)) {
			Wow64RevertWow64FsRedirection(&OldValue);
#ifdef PRINT
			fatal((char*)"getPdb: open ntoskrnl.exe err");
#endif
			;
		}
		else
			Wow64RevertWow64FsRedirection(&OldValue);
	}

	pe.getPdbFile();	// теперь pdbFileNameBuffer указывает на pdb файлик

	std::fstream ntoskrnl;
	ntoskrnl.open(pdbFileNameBuffer, std::fstream::in);

	if (ntoskrnl.good()) {
		ntoskrnl.close();
		return pdbFileNameBuffer;
	}

#ifdef PRINT
	fatal((char *)"dia2.cpp getPdb err");
#endif

	return NULL;
#endif
}

ULONG64 CoreDBG::GetKernelBase()
{
	if (KernelBase != 0)
	{
		return KernelBase;
	}
	else
	{
		GKB::PRTL_PROCESS_MODULES ModuleInfo;
		ModuleInfo = (GKB::PRTL_PROCESS_MODULES)VirtualAlloc(NULL,
			1024 * 1024,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE); // Allocate memory for the module list

		sys::NtQuerySystemInformation(
			(sys::SYSTEM_INFORMATION_CLASS)11,
			ModuleInfo,
			1024 * 1024,
			NULL
		);

		if (ModuleInfo == NULL)
		{
			debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET KERNEL BASE\n", __func__);
		}

		KernelBase = (uint64_t)ModuleInfo->Modules[0].ImageBase;
		VirtualFree(ModuleInfo, 0, MEM_RELEASE);
		return KernelBase;
	}
}
ULONG64 CoreDBG::getFieldOffset(wchar_t* typeName_, wchar_t* fieldName_) {

	UINT64 offset = 0;
	try
	{
		offset = OffsetsDict.at(typeName_).at(fieldName_);
		debug::printf_d(debug::LogLevel::LOG, "%s Using existing keys for (%S, %S)\n", __func__, typeName_, fieldName_);
		return offset;
	}
	catch (const std::out_of_range)
	{
		debug::printf_d(debug::LogLevel::LOG, "%s First getFieldOffset for (%S, %S). Let's try to find it\n", __func__, typeName_, fieldName_);
	}

	
	if (!DiaPdb.getSymbolOffsetInKernelType(typeName_, fieldName_, &offset))
	{
		debug::printf_d(debug::LogLevel::FATAL, "%s cannot get offsets for (%S, %S)\n", __func__, typeName_, fieldName_);
		offset = -1;
	}

	OffsetsDict[typeName_][fieldName_] = offset;
	return offset;
}

ULONG64 CoreDBG::getKernelSymbolAddress(char* symbolName) {
	return DbgPdb.GetKernelSymbolAddress(symbolName);
}