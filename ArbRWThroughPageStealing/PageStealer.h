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

	static PVOID GetKPROCESSByPID(DWORD PID);
	static PVOID GetProceessPageTableL4(DWORD PID);
	static DWORD GetPIDByName(std::wstring ProcessName);
	static PVOID VTOP(UINT64 va, UINT64 KPROCESS, PVirtualAddressTableEntries ret);
	static PVOID MapSinglePhysicalPageToProcessVirtualAddressSpace(UINT64 KPROCESS, UINT64 PA, DWORD PageCount);
	static BOOL MapVirtualPageToAnotherProcess(DWORD SourcePID, DWORD DestPID, UINT64 VA, BOOL MakePageWritable);
private:
	
	static std::set<ULONG_PTR> GetEprocessCandidatesByPID(DWORD PID);
	static PVOID GetDirectoryTableFromKPROCESS(PVOID KPROCESS);
};