#include "PageStealer.h"

/// <summary>
/// RETURNS DirectoryTable content
/// </summary>
/// <param name="PID">target process PID</param>
/// <returns>Directory Table content; allocates buffer internally</returns>
/// 
VOID* PageStealer::GetProceessPageTableL4(DWORD PID)
{
	printf("%s FIX ME\n", __func__);
	return (VOID*)NULL;

	UINT64 EPROCESSAddr = (UINT64)GetKPROCESSByPID(PID);
	
	CoreDBG& coreDbg = CoreDBG::GetInstance();
	UINT64 DirBaseOffset = coreDbg.getFieldOffset((wchar_t*)L"_KPROCESS", (wchar_t*)L"DirectoryTableBase");

	DriverControl& dc = DriverControl::GetInstance();
	UINT64 DirBaseAddress = {0};

	if (dc.ReadKernelVA(EPROCESSAddr + DirBaseOffset, 8, (UINT8*)&DirBaseAddress))
	{
		printf("PROCESS PAGETABLE PTR = 0x%llx\n ", DirBaseAddress);
		VOID* PageTable = malloc(4 * 1024);

		if (dc.ReadOverMapViewOfSection(DirBaseAddress, 4 * 1024, (UINT8*) PageTable))
		{
			return PageTable;
		}
		else
		{
			free(PageTable);
		}
	}

    return (VOID*)0;
}

DWORD PageStealer::GetPIDByName(std::wstring ProcessName)
{
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32FirstW(snapshot, &entry) == TRUE)
    {
        while (Process32NextW(snapshot, &entry) == TRUE)
        {
            if (_wcsicmp(entry.szExeFile, ProcessName.c_str()) == 0)
            {
                return entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return -1;
}

UINT64 PageStealer::GetEprocessCandidatesByPID(DWORD PID)
{
	DWORD cPID = PID;
	ULONG uLength = 0;
	// std::set<ULONG_PTR> out = {};

	const ULONG SystemExtendedHandleInformation = 0x40;
	// This particular SYSTEM_INFORMATION_CLASS doesn't accurately return the correct number of bytes required
	// some extra space is needed to avoid NTSTATUS C0000004 (STATUS_INFO_LENGTH_MISMATCH)
	//

	unsigned char lpProbeBuffer[1024] = { 0 };

	NTSTATUS status = NtQuerySystemInformation(
		static_cast<SYSTEM_INFORMATION_CLASS>(SystemExtendedHandleInformation),
		&lpProbeBuffer,
		sizeof(lpProbeBuffer),
		&uLength
	);

	if (!uLength) {

		return 0;
	}
	uLength += 50 * (sizeof(SYSTEM_HANDLE_INFORMATION_EX) + sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
	PVOID lpBuffer = VirtualAlloc(nullptr, uLength, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!lpBuffer) {

		return 0;
	}

	RtlSecureZeroMemory(lpBuffer, uLength);

	ULONG uCorrectSize = 0;
	status = NtQuerySystemInformation(
		static_cast<SYSTEM_INFORMATION_CLASS>(SystemExtendedHandleInformation),
		lpBuffer,
		uLength,
		&uCorrectSize
	);

	if (!NT_SUCCESS(status)) {

		return 0;
	}

	SYSTEM_HANDLE_INFORMATION_EX* lpHandleInformation = reinterpret_cast<SYSTEM_HANDLE_INFORMATION_EX*>(lpBuffer);

	CoreDBG& coreDbg = CoreDBG::GetInstance();
	UINT64 PIDOffset = coreDbg.getFieldOffset((wchar_t*)L"_EPROCESS", (wchar_t*)L"UniqueProcessId");
	UINT64 ImageFileNameOffset = coreDbg.getFieldOffset((wchar_t*)L"_EPROCESS", (wchar_t*)L"ImageFileName");

	if (PIDOffset == -1 || ImageFileNameOffset == -1)
	{
		return 0;
	}

	UINT64 ret = 0;
	for (UINT i = 0; i < lpHandleInformation->NumberOfHandles; i++)
	{
		if (lpHandleInformation->Handles[i].UniqueProcessId == (DWORD)4)
		{
			UINT64 buffer = 0;
			DriverControl& dc = DriverControl::GetInstance();
			if (dc.ReadKernelVA(((UINT64)(lpHandleInformation->Handles[i].Object) + PIDOffset), 8, (UINT8*)&buffer) && buffer == PID)
			{
				ret = (UINT64) lpHandleInformation->Handles[i].Object;
			}
		}
	}
	VirtualFree(lpBuffer, 0, MEM_RELEASE);
	return ret;

}

/// <summary>
/// SIMPLE VTOP. Doesn't check is table entry present. 
/// </summary>
/// <param name="va_">Virtual address to be transated</param>
/// <param name="KPROCESS">KPROCESS/EPROCESS ptr of target process</param>
/// <param name="VATableEntriesRetRequest">PTR to PVirtualAddressTableEntries struct to get table entries, NULL if just VTOP physical address</param>
/// <returns>0 if failed, otherwise nonzero value</returns>
VOID* PageStealer::VTOP(UINT64 va_, UINT64 KPROCESS, PVirtualAddressTableEntries VATableEntriesRetRequest)
{
	DriverControl& dc = DriverControl::GetInstance();
	VIRTUAL_ADDRESS va = reinterpret_cast<VIRTUAL_ADDRESS&> (va_);

	PAGE_ENTRY<PML4E> pml4_entry;
	PAGE_ENTRY<PDPE> pdp_entry;
	PAGE_ENTRY<PDE> pd_entry;
	PAGE_ENTRY<PTE> pt_entry;

	debug::printf_d(debug::LogLevel::LOG, "%s translating 0x%llx\n", __func__, va.value);

	UINT64 DirTable = (UINT64) GetDirectoryTableFromKPROCESS((PVOID)KPROCESS);
	debug::printf_d(debug::LogLevel::LOG, "DirectoryTable 0x%llx\n", DirTable);

	pml4_entry.pointer = (PPML4E)DirTable + va.pml4_index;
	debug::printf_d(debug::LogLevel::LOG,  "PML4E 0x%llx\n", pml4_entry.pointer);

	if (dc.ReadOverMapViewOfSection((UINT64)pml4_entry.pointer,
		sizeof(PML4E), (UINT8*)&pml4_entry.value.value))
	{
		pdp_entry.pointer = (PPDPE)(pml4_entry.value.pfn << PAGE_SHIFT) + va.pdp_index;
		debug::printf_d(debug::LogLevel::LOG, "PDPE 0x%llx\n", pdp_entry.pointer);

		if (dc.ReadOverMapViewOfSection((UINT64)pdp_entry.pointer,
			sizeof(PDPE), (UINT8*)&pdp_entry.value))
		{
			pd_entry.pointer = (PPDE)(pdp_entry.value.pfn << PAGE_SHIFT) + va.pd_index;
			debug::printf_d(debug::LogLevel::LOG, "PDE 0x%llx\n", pd_entry.pointer);

			if (dc.ReadOverMapViewOfSection((UINT64)pd_entry.pointer,
				sizeof(PDE), (UINT8*)&pd_entry.value))
			{
				pt_entry.pointer = (PPTE) (pd_entry.value.pfn << PAGE_SHIFT) + va.pt_index;
				// printf("PTE 0x%llx\n", pt_entry.pointer);
				debug::printf_d(debug::LogLevel::LOG, "PTE 0x%llx\n", pt_entry.pointer);
				if (dc.ReadOverMapViewOfSection((UINT64)pt_entry.pointer,
					sizeof(PTE), (UINT8*)&pt_entry.value))
				{
					debug::printf_d(debug::LogLevel::LOG, "VTOP RES (%llx): %llx\n", va.value, (pt_entry.value.pfn << PAGE_SHIFT) + va.offset);
					
					if (VATableEntriesRetRequest != NULL)
					{
						VATableEntriesRetRequest->pt_entry = pt_entry;
						VATableEntriesRetRequest->pd_entry = pd_entry;
						VATableEntriesRetRequest->pdp_entry = pdp_entry;
						VATableEntriesRetRequest->pml4_entry = pml4_entry;
						debug::printf_d(debug::LogLevel::LOG, "RET TABLE ENTRIES\n");
					}
					return (VOID*)(pt_entry.value.pfn << PAGE_SHIFT + va.offset);
				}
			}
		}
	}

	debug::printf_d(debug::LogLevel::ERR, "CANNOT CONVER VA TO PA\n");
	return (VOID*) -1;
}

PVOID PageStealer::GetKPROCESSByPID(DWORD PID)
{
	UINT64 EPROCESS = GetEprocessCandidatesByPID(PID);
	if (EPROCESS == 0)
	{
		throw std::invalid_argument("CANNOT FIND EPROCESS");
		// debug::printf_d(debug::LogLevel::LOG, "%s Eprocess candidates >= 1. Just pick first(aka *begin())\n", __func__);
		// PVOID ret = (PVOID) EPROCESS;
		// return ret;
	}

	return (PVOID) EPROCESS;
}

PVOID PageStealer::GetDirectoryTableFromKPROCESS(PVOID KPROCESS_)
{
	UINT64 KPROCESS = (UINT64)KPROCESS_;
	CoreDBG& coreDbg = CoreDBG::GetInstance();
	UINT64 DirBaseOffset = coreDbg.getFieldOffset((wchar_t*)L"_KPROCESS", (wchar_t*)L"DirectoryTableBase");

	DriverControl& dc = DriverControl::GetInstance();
	UINT64 DirBaseAddress = { 0 };

	if (dc.ReadKernelVA(KPROCESS + DirBaseOffset, 8, (UINT8*)&DirBaseAddress))
	{
		return (PVOID)DirBaseAddress;
	}
	else
	{
		return NULL;
	}
}

PVOID PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace(UINT64 KPROCESS, UINT64 PA, DWORD PageCount)
{
	PVOID DirectoryTable = GetDirectoryTableFromKPROCESS((PVOID)KPROCESS);

	if ((PA & 0xFFF) != 0)
	{
		debug::printf_d(debug::LogLevel::ERR, "%S: PA should be aligned\n");
		// return NULL;
	}

	// 1. get free virtual address
	

	VIRTUAL_ADDRESS va = { (PVOID) malloc(PAGE_SIZE)};
	
	PAGE_ENTRY<PML4E> pml4_entry = { 0 };

	DriverControl& dc = DriverControl::GetInstance();
	BOOL FailFlag = true;

	VirtualAddressTableEntries VATableEntries = { 0 };
	VTOP((UINT64)va.value, KPROCESS, &VATableEntries);


	//for (unsigned int PML4Iterator = 0; PML4Iterator < 512; PML4Iterator++)
	//{
	//	if (dc.ReadOverMapViewOfSection((UINT64)DirectoryTable + sizeof(PML4E) * PML4Iterator, sizeof(PML4E), (UINT8*)&pml4_entry.value.value))
	//	{
	//		if (!pml4_entry.value.present)
	//		{
				// fix this entry
				pml4_entry.value.present = 0;
				pml4_entry.value.writable = 1;
				pml4_entry.value.user_access = 1;
				pml4_entry.value.accessed = 1;
				pml4_entry.value.ignored_3 = 1;
				pml4_entry.value.ignored_2 = 8;
				pml4_entry.value.ignored_1 = 0xa0;

				FailFlag = false;
	/*			break;
			}
		}
	}*/

	if (FailFlag)
	{
		debug::printf_d(debug::LogLevel::ERR, "CANNOT FIND FREE PML4");
		return NULL;
	}

	PAGE_ENTRY<PDPE> pdp_entry = { 0 };
	pdp_entry.value.present = 0; // fix me
	pdp_entry.value.writable = 1;
	pdp_entry.value.user_access = 1;
	pdp_entry.value.accessed = 1;
	pdp_entry.value.ignored_3 = 1;
	pdp_entry.value.ignored_2 = 8;
	pdp_entry.value.ignored_1 = 0xa0;

	PAGE_ENTRY<PDE> pd_entry = { 0 };
	pd_entry.value.present = 0;	// fix me
	pd_entry.value.writable = 1;
	pd_entry.value.user_access = 1;
	pd_entry.value.accessed = 1;
	pd_entry.value.ignored_2 = 8;
	pd_entry.value.ignored_1 = 0xa0;
	pd_entry.value.ignored1 = 0x1;

	PAGE_ENTRY<PTE> pt_entry = { 0 };
	pt_entry.value.present = 0;	// fix me
	pt_entry.value.writable = 1;
	pt_entry.value.user_access = 1;
	pt_entry.value.accessed = 1;
	pt_entry.value.dirty = 1;
	pt_entry.value.ignored_2 = 4;
	pt_entry.value.ignored_3 = 0x10;


	pt_entry.value.pfn = 0;

	pml4_entry.pointer = (PPML4E)DirectoryTable + va.pml4_index;
	debug::printf_d(debug::LogLevel::ERR, "PML4E 0x%llx\n", pml4_entry.pointer);

	//if (dc.WriteOverMapViewOfSection((UINT64)pml4_entry.pointer,
	//	sizeof(PML4E), (UINT8*)&pml4_entry.value.value))
	//{
	//	pdp_entry.pointer = (PPDPE)(pml4_entry.value.pfn << PAGE_SHIFT) + va.pdp_index;
	//	debug::printf_d("PDPE 0x%llx\n", pdp_entry.pointer);

	//	if (dc.WriteOverMapViewOfSection((UINT64)pdp_entry.pointer,
	//		sizeof(PDPE), (UINT8*)&pdp_entry.value))
	//	{
	//		pd_entry.pointer = (PPDE)(pdp_entry.value.pfn << PAGE_SHIFT) + va.pd_index;
	//		debug::printf_d("PDE 0x%llx\n", pd_entry.pointer);

	//		if (dc.WriteOverMapViewOfSection((UINT64)pd_entry.pointer,
	//			sizeof(PDE), (UINT8*)&pd_entry.value))
	//		{
	//			pt_entry.pointer = (PPTE)(pd_entry.value.pfn << PAGE_SHIFT) + va.pt_index;
	//			// printf("PTE 0x%llx\n", pt_entry.pointer);
	//			debug::printf_d("PTE 0x%llx\n", pt_entry.pointer);
	//			if (dc.WriteOverMapViewOfSection((UINT64)pt_entry.pointer,
	//				sizeof(PTE), (UINT8*)&pt_entry.value))
	//			{
	//				debug::printf_d("%llx\n", (pt_entry.value.pfn << PAGE_SHIFT) + va.offset);
	//				return (VOID*)(pt_entry.value.pfn << PAGE_SHIFT + va.offset);
	//			}
	//		}
	//	}
	//}
	debug::printf_d(debug::LogLevel::ERR, "%s FIX ME", __func__);
	return NULL;
}

/// <summary>
/// Allocates page in destination process on the same virtual address
/// then edit dir table entries to point to physical page of source process
/// </summary>
/// <param name="SourcePID">Source process Pid</param>
/// <param name="DestPID">Destination process Pid</param>
/// <param name="va_">Virtual address in source process</param>
/// <param name="MakePageWritable">Mark pages in destination process as writable</param>
/// <returns>return TRUE if success, otherwise FALSE</returns>
BOOL PageStealer::MapVirtualPageToAnotherProcess(DWORD SourcePID, DWORD DestPID, UINT64 va_, BOOL MarkPageWritable)
{
	//
	
	if (SourcePID == -1 || DestPID == -1 || SourcePID == 0 || DestPID == 0)
	{
		debug::printf_d(debug::LogLevel::ERR, "%s WRONG ARG (PID)\n", __func__);
		return FALSE;
	}

	if (va_ && 0xFFF)
	{
		debug::printf_d(debug::LogLevel::WARN, "%s UNALIGNED VA. MAPPING WHOLE PAGE\n", __func__);
		va_ = (va_ >> 12) << 12;
	}

	VIRTUAL_ADDRESS va = { (PVOID)va_ };

	UINT64 SourceDirTable = (UINT64) GetDirectoryTableFromKPROCESS(GetKPROCESSByPID(SourcePID));
	UINT64 DestDirTable = (UINT64) GetDirectoryTableFromKPROCESS(GetKPROCESSByPID(DestPID));

	// get table entries from source process
	VirtualAddressTableEntries SourceTableEntries = { 0 };
	VTOP((UINT64) va.value, (UINT64) GetKPROCESSByPID(SourcePID), &SourceTableEntries);
	if (!(SourceTableEntries.pt_entry.value.present
		& SourceTableEntries.pd_entry.value.present
		& SourceTableEntries.pdp_entry.value.present
		& SourceTableEntries.pml4_entry.value.present))
	{
		debug::printf_d(debug::LogLevel::ERR, "%s TARGET VIRTUAL ADDRESS DOESN'T EXIST IN SORUCE\n", __func__);
		return FALSE;
	}

	// get table entries from dest 
	VirtualAddressTableEntries OriginalDestTableEntries = { 0 };
	VTOP((UINT64)va.value, (UINT64)GetKPROCESSByPID(DestPID), &OriginalDestTableEntries);
	if ((OriginalDestTableEntries.pt_entry.value.present
		& OriginalDestTableEntries.pd_entry.value.present
		& OriginalDestTableEntries.pdp_entry.value.present
		& OriginalDestTableEntries.pml4_entry.value.present))
	{
		debug::printf_d(debug::LogLevel::WARN, "%s TARGET VIRTUAL ADDRESS ALREADY EXIST IN DEST\n", __func__);
		return FALSE;
	}

	HANDLE DestProcessHandle = OpenProcess(PROCESS_VM_OPERATION, TRUE, DestPID);
	if (DestProcessHandle == INVALID_HANDLE_VALUE)
	{
		debug::printf_d(debug::LogLevel::ERR, "%s CANNOT OPEN PROCESS\n", __func__);
		return FALSE;
	}

	PVOID VirtualAllocRes = NULL;
	
	if (MarkPageWritable)
	{
		VirtualAllocRes = VirtualAllocEx(DestProcessHandle, (PVOID)va.value, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE);
	}
	else
	{
		VirtualAllocRes = VirtualAllocEx(DestProcessHandle, (PVOID)va.value, PAGE_SIZE, MEM_COMMIT, PAGE_READONLY);
	}
	if (VirtualAllocRes == NULL)
	{
		DWORD LE = GetLastError();
		debug::printf_d(debug::LogLevel::WARN, "%s last err: %llx\n", __func__, LE);
		debug::printf_d(debug::LogLevel::WARN, "LET'S JUST EDIT PAGE TABLE\n");
		// return FALSE;
	}

	if (VirtualAllocRes != (PVOID)va.value && VirtualAllocRes != NULL)
	{
		debug::printf_d(debug::LogLevel::WARN, "%s ALLOCATED PAGE DOESN'T MATCH SPECIFIED VIRTUAL ADDRESS\n", __func__);
		BOOL bRes = VirtualFreeEx(DestProcessHandle, VirtualAllocRes, 0, MEM_RELEASE);
		if (!bRes)
		{
			debug::printf_d(debug::LogLevel::WARN, "%s CANNOT FREE??\n", __func__);
		}
		CloseHandle(DestProcessHandle);
		return FALSE;
	}

	DriverControl& dc = DriverControl::GetInstance();

	if (!OriginalDestTableEntries.pml4_entry.value.present)
	{
		dc.WriteOverMapViewOfSection((UINT64)OriginalDestTableEntries.pml4_entry.pointer,
			sizeof(PML4E), (UINT8*)&SourceTableEntries.pml4_entry.value.value);

	}
	if (!OriginalDestTableEntries.pdp_entry.value.present)
	{
		dc.WriteOverMapViewOfSection((UINT64)OriginalDestTableEntries.pdp_entry.pointer,
			sizeof(PDPE), (UINT8*)&SourceTableEntries.pdp_entry.value.value);
	}
	if (!OriginalDestTableEntries.pd_entry.value.present)
	{
		dc.WriteOverMapViewOfSection((UINT64)OriginalDestTableEntries.pd_entry.pointer,
			sizeof(PDE), (UINT8*)&SourceTableEntries.pd_entry.value.value);
	}
	if (!OriginalDestTableEntries.pt_entry.value.present)
	{
		dc.WriteOverMapViewOfSection((UINT64)OriginalDestTableEntries.pt_entry.pointer,
			sizeof(PTE), (UINT8*)&SourceTableEntries.pt_entry.value.value);
	}

	CloseHandle(DestProcessHandle);
	DWORD OldProtect = 0;

	//MEMORY_BASIC_INFORMATION info = { 0 };
	//SIZE_T len = VirtualQuery(va.value, &info, sizeof(info));

	//BOOL bRes = VirtualProtect(va.value,
	//	1024,
	//	PAGE_EXECUTE_READWRITE,
	//	&OldProtect);
	//DWORD LE = GetLastError();

	UINT32 a = *(UINT32*)(va.value);

	return FALSE;
}

BOOL PageStealer::StealEntireVirtualAddressSpace(DWORD SourcePID, DWORD DestPID, UINT64 va_)
{

	return FALSE;
}