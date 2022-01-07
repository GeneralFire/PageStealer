#pragma once

#include "DbgHelpWrapper.h"
#include "DiaHelpWrapper.h"
#include "PE.h"
#include <map>
#include "sys.hpp"

#define DEBUG_CHECK 

namespace GKB
{
	typedef struct _RTL_PROCESS_MODULE_INFORMATION
	{
		HANDLE Section;
		PVOID MappedBase;
		PVOID ImageBase;
		ULONG ImageSize;
		ULONG Flags;
		USHORT LoadOrderIndex;
		USHORT InitOrderIndex;
		USHORT LoadCount;
		USHORT OffsetToFileName;
		UCHAR FullPathName[256];
	} RTL_PROCESS_MODULE_INFORMATION, * PRTL_PROCESS_MODULE_INFORMATION;

	typedef struct _RTL_PROCESS_MODULES
	{
		ULONG NumberOfModules;
		RTL_PROCESS_MODULE_INFORMATION Modules[1];
	} RTL_PROCESS_MODULES, * PRTL_PROCESS_MODULES;
}

class CoreDBG {
private:
	DiaHelpWrapper DiaPdb;
	DbgHelpWrapper DbgPdb;
	PE pe;
	char* getPdb();
	UINT64 KernelBase = 0;
	UINT64 PfnDatabaseAddr = 0;
	UINT64 PfnDataBase = 0;
	std::map<wchar_t*, std::map<wchar_t*, UINT64>> OffsetsDict;

	CoreDBG();
	~CoreDBG();

public:
	static inline CoreDBG& GetInstance() {
		static CoreDBG instance;
		return instance;
	}
	
	ULONG64 GetKernelBase();
	ULONG64 getFieldOffset(wchar_t* typeName_, wchar_t* fieldName_);
	ULONG64 getKernelSymbolAddress(char* symbolName);

};