#pragma once

#include "driver_control.hpp"
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include "globals.h"
#include "CoreDBG.h"
#include "VirtualsHeader.h"
#include <set>
#include "debug.hpp"

class PageStealer
{
public:
	typedef struct _ProcessMinimalInfo
	{
		DWORD PID;
		UCHAR ImageFileName[15];
	} PROCESS_MINIMAL_INFO, * PPROCESS_MINIMAL_INFO;

	static PROCESS_MINIMAL_INFO GetPMIByProcessName(std::string ProcessName);
	
	static UINT64 GetKPROCESSByPMI(PPROCESS_MINIMAL_INFO PMI);
	static PVOID GetProceessPageTableL4(PPROCESS_MINIMAL_INFO PMI);
	
	static PVOID VTOP(UINT64 va, UINT64 KPROCESS, PVirtualAddressTableEntries ret);
	static PVOID MapSinglePhysicalPageToProcessVirtualAddressSpace(UINT64 KPROCESS, UINT64 PA, DWORD PageCount);
	static BOOL MapVirtualPageToAnotherProcess(PPROCESS_MINIMAL_INFO SourcePMI, PPROCESS_MINIMAL_INFO DestPMI, UINT64 VA, BOOL MakePageWritable);
	static BOOL StealEntireVirtualAddressSpace(PPROCESS_MINIMAL_INFO SourcePMI, PPROCESS_MINIMAL_INFO DestPMI, UINT64 VA);

private:
	
	
	static DWORD GetPIDByName(std::wstring ProcessName);
	static UINT64 _GetKPROCESSByPMI(PPROCESS_MINIMAL_INFO PMI);
	static UINT64 GetDirectoryTableFromKPROCESS(UINT64 KPROCESS);
};