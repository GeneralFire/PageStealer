#include "PageStealer.h"

std::map<DWORD, UINT64> PageStealer::EprocessDictionary;
std::map<UINT64, UINT64> PageStealer::DirTableDictionary;
UINT64 PageStealer::PfnDataBase = 0;

/// <summary>
/// RETURNS DirectoryTable content
/// </summary>
/// <param name="PID">target process PID</param>
/// <returns>Directory Table content; allocates buffer internally</returns>
/// 
VOID* PageStealer::GetProceessPageTableL4(PPROCESS_MINIMAL_INFO PMI)
{
	printf("%s FIX ME\n", __func__);
	return (VOID*)NULL;

	UINT64 EPROCESSAddr = (UINT64)GetKPROCESSByPMI(PMI);
	
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

UINT64 PageStealer::_GetKPROCESSByPMI(PPROCESS_MINIMAL_INFO PMI)
{
	DWORD PID = PMI->PID;
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

	UINT64 PIDBuffer = 0;
	UCHAR ImageFileNameBuffer[15] = { 0 };
	UINT64 ret = 0;
	DriverControl& dc = DriverControl::GetInstance();

	for (UINT i = 0; i < lpHandleInformation->NumberOfHandles; i++)
	{
		if (lpHandleInformation->Handles[i].UniqueProcessId == (DWORD)4)
		{
			if (dc.ReadKernelVA(((UINT64)(lpHandleInformation->Handles[i].Object) + PIDOffset), 8, (UINT8*)&PIDBuffer) && PIDBuffer == PID)
			{
				ret = (UINT64) lpHandleInformation->Handles[i].Object;
				break;
			}
		}
		
		if (lpHandleInformation->Handles[i].UniqueProcessId == PMI->PID)
		{
			if (dc.ReadKernelVA(((UINT64)(lpHandleInformation->Handles[i].Object) + ImageFileNameOffset), 15, (UINT8*)ImageFileNameBuffer))
			{
				if (!memcmp(ImageFileNameBuffer, PMI->ImageFileName, 14))
				{
					ret = (UINT64)lpHandleInformation->Handles[i].Object;
					break;
				}
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

	UINT64 DirTable = (UINT64) GetDirectoryTableFromKPROCESS(KPROCESS);
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

UINT64 PageStealer::GetKPROCESSByPMI(PPROCESS_MINIMAL_INFO PMI)
{
	UINT64 EPROCESS = 0;
	try
	{
		EPROCESS = EprocessDictionary.at(PMI->PID);
		debug::printf_d(debug::LogLevel::LOG, "%s Using existing keys for (%s, %lx)\n", __func__, PMI->ImageFileName, PMI->PID);
		return EPROCESS;
	}
	catch (const std::out_of_range)
	{
		debug::printf_d(debug::LogLevel::LOG, "%s First for (%s, %lx). Let's try to find it\n", __func__, PMI->ImageFileName, PMI->PID);
	}
	EPROCESS = _GetKPROCESSByPMI(PMI);
	if (EPROCESS == 0)
	{
		debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT FIND EPROCESS", __func__);
	}
	EprocessDictionary[PMI->PID] = EPROCESS;
	debug::printf_d(debug::LogLevel::LOG, "%s KPROCESS for %s 0x%llx\n", __func__, PMI->ImageFileName, EPROCESS);
	return EPROCESS;
}

UINT64 PageStealer::GetDirectoryTableFromKPROCESS(UINT64 KPROCESS)
{
	UINT64 DirBaseAddress = 0;
	try
	{
		DirBaseAddress = DirTableDictionary.at(KPROCESS);
		debug::printf_d(debug::LogLevel::LOG, "%s Using existing keys for (%llx)\n", __func__, KPROCESS);
		return DirBaseAddress;
	}
	catch (const std::out_of_range)
	{
		debug::printf_d(debug::LogLevel::LOG, "%s First for (%llx). Let's try to find it\n", __func__, KPROCESS);
	}

	CoreDBG& coreDbg = CoreDBG::GetInstance();
	UINT64 DirBaseOffset = coreDbg.getFieldOffset((wchar_t*)L"_KPROCESS", (wchar_t*)L"DirectoryTableBase");

	DriverControl& dc = DriverControl::GetInstance();
	if (dc.ReadKernelVA(KPROCESS + DirBaseOffset, 8, (UINT8*)&DirBaseAddress))
	{
		DirTableDictionary[KPROCESS] = DirBaseAddress;
		return DirBaseAddress;
	}
	else
	{
		return 0;
	}
}

PVOID PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace(UINT64 KPROCESS, UINT64 PA, DWORD PageCount)
{
	debug::printf_d(debug::LogLevel::FATAL, "%s FIX ME", __func__);
	
	return NULL;
}


/*
Idea:
Allocate Vitrual memory on Dest process at the same VA; (+check if is it already exist (true->fail))
Edit PTE point to same physical memory as in source (+check if is it doesnt exist (true->fail))
*/
/// <summary>
/// Allocates page in destination process on the same virtual address
/// then edit dir table entries to point to physical page of source process
/// </summary>
/// <param name="SourcePID">Source process Pid</param>
/// <param name="DestPID">Destination process Pid</param>
/// <param name="va_">Virtual address in source process</param>
/// <param name="MakePageWritable">Mark pages in destination process as writable</param>
/// <returns>return TRUE if success, otherwise FALSE</returns>
BOOL PageStealer::MapVirtualPageToAnotherProcess(PPROCESS_MINIMAL_INFO SourcePMI, PPROCESS_MINIMAL_INFO DestPMI, UINT64 va_, BOOL MakePageWritable)
{
	// debug::printf_d(debug::LogLevel::FATAL, "%s FIX ME", __func__);
	
	if (SourcePMI->PID == -1 || DestPMI->PID == -1 || SourcePMI->PID == 0 || DestPMI->PID == 0)
	{
		debug::printf_d(debug::LogLevel::ERR, "%s WRONG ARG (PID)\n", __func__);
		return FALSE;
	}

	if (va_ & 0xFFF)
	{
		UINT64 mask = -1 & ~0xFFF;
		debug::printf_d(debug::LogLevel::WARN, "%s UNALIGNED VA(0x%llx). MAPPING WHOLE PAGE(0x%llx)\n", __func__, va_, va_ & mask);	
		va_ &= mask;
	}

	VIRTUAL_ADDRESS va = { (PVOID)va_ };

	UINT64 SourceDirTable = (UINT64) GetDirectoryTableFromKPROCESS(GetKPROCESSByPMI(SourcePMI));
	UINT64 DestDirTable = (UINT64) GetDirectoryTableFromKPROCESS(GetKPROCESSByPMI(DestPMI));

	// get table entries from dest 
	VirtualAddressTableEntries OriginalDestTableEntries = { 0 };
	VTOP((UINT64)va.value, (UINT64)GetKPROCESSByPMI(DestPMI), &OriginalDestTableEntries);
	if ((OriginalDestTableEntries.pt_entry.value.present
		& OriginalDestTableEntries.pd_entry.value.present
		& OriginalDestTableEntries.pdp_entry.value.present
		& OriginalDestTableEntries.pml4_entry.value.present))
	{
		debug::printf_d(debug::LogLevel::WARN, "%s TARGET VIRTUAL ADDRESS ALREADY EXIST IN DEST (0x%llx)\n", __func__, va.value);
		return FALSE;
	}

	debug::printf_d(debug::LogLevel::VERBOSE, "%s TARGET VIRTUAL ADDRESS DOESNT IN DEST (0x%llx). LETS MAP PHYS HERE\n", __func__, va.value);

	// get table entries from source process
	VirtualAddressTableEntries SourceTableEntries = { 0 };

	if (!VTOP((UINT64)va.value, (UINT64)GetKPROCESSByPMI(SourcePMI), &SourceTableEntries)
		&& 
		(!(SourceTableEntries.pt_entry.value.present
		&& SourceTableEntries.pd_entry.value.present
		&& SourceTableEntries.pdp_entry.value.present
		&& SourceTableEntries.pml4_entry.value.present)))
	{
		debug::printf_d(debug::LogLevel::ERR, "%s TARGET VIRTUAL ADDRESS DOESN'T EXIST IN SORUCE (0x%llx)\n", __func__, va.value);
		return FALSE;
	}

	HANDLE DestProcessHandle = OpenProcess(PROCESS_VM_OPERATION, TRUE, DestPMI->PID);
	if (DestProcessHandle == INVALID_HANDLE_VALUE)
	{
		debug::printf_d(debug::LogLevel::ERR, "%s CANNOT OPEN PROCESS\n", __func__);
		return FALSE;
	}

	PVOID VirtualAllocRes = NULL;
	

	VirtualAllocRes = VirtualAllocEx(OpenProcess(PROCESS_VM_OPERATION, TRUE, GetCurrentProcessId()),
		(PVOID)va.value, 
		PAGE_SIZE, MEM_COMMIT |MEM_RESERVE, 
		MakePageWritable?PAGE_READWRITE:PAGE_READONLY
	);

	if (VirtualAllocRes == NULL)
	{
		DWORD LE = GetLastError();
		debug::printf_d(debug::LogLevel::WARN, "%s last err: %llx\n", __func__, LE);
		// debug::printf_d(debug::LogLevel::WARN, "LET'S JUST EDIT PAGE TABLE\n");
		return FALSE;
	}

	if (VirtualAllocRes == (PVOID)va.value)
	{
		debug::printf_d(debug::LogLevel::VERBOSE, "%s SUCCESSFULLY ALLOCATED THE SAME VA (0x%llx)\n", __func__, VirtualAllocRes);
		UINT64 a = *(UINT64*)(VirtualAllocRes);
	}

	if (VirtualAllocRes != (PVOID)va.value && VirtualAllocRes != NULL)
	{
		debug::printf_d(debug::LogLevel::WARN, "%s ALLOCATED PAGE DOESN'T MATCH SPECIFIED VIRTUAL ADDRESS\n", __func__);
		BOOL bRes = VirtualFreeEx(DestProcessHandle, VirtualAllocRes, 0, MEM_RELEASE);
		if (!bRes)
		{
			debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT FREE??\n", __func__);
		}
		CloseHandle(DestProcessHandle);
		return FALSE;
	}

	DriverControl& dc = DriverControl::GetInstance();

	// get table entries after allocation from dest 
	OriginalDestTableEntries = { 0 };
	if (!VTOP((UINT64)va.value, (UINT64)GetKPROCESSByPMI(DestPMI), &OriginalDestTableEntries)
		&& 
		(!(OriginalDestTableEntries.pt_entry.value.present
		&& OriginalDestTableEntries.pd_entry.value.present
		&& OriginalDestTableEntries.pdp_entry.value.present
		&& OriginalDestTableEntries.pml4_entry.value.present)))
	{
		debug::printf_d(debug::LogLevel::VERBOSE, "%s (AFTER ALLOCATION) TARGET VIRTUAL ADDRESS DOESNT IN DEST (0x%llx)\n", __func__, va.value);
		return FALSE;
	}


	CoreDBG& CoreDbg = CoreDBG::GetInstance();
	UINT64 PfnDatabaseAddressPtr = CoreDbg.GetKernelBase() + CoreDbg.getKernelSymbolAddress((char*)"MmPfnDatabase");

	if (!PfnDataBase)
	{
		if (!dc.ReadKernelVA(PfnDatabaseAddressPtr, sizeof(UINT64), (UINT8*)&PfnDataBase))
		{
			debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET PFN DATABASE\n", __func__);
		}
	}

	// 1. inc share count

	_MMPFN PfnDataBaseEntry = { 0 };
	debug::printf_d(debug::LogLevel::LOG, "%s Loking for pfn entry at 0x%llx\n", 
		__func__,
		PfnDataBase + sizeof(_MMPFN) * SourceTableEntries.pt_entry.value.pfn
	);
	if (!dc.ReadKernelVA(PfnDataBase + sizeof(_MMPFN) * SourceTableEntries.pt_entry.value.pfn,
		sizeof(_MMPFN),
		(UINT8*)&PfnDataBaseEntry))
	{
		debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET PFN DATABASE ENTRY\n", __func__);
	}

	if (!dc.ReadKernelVA(PfnDataBase + sizeof(_MMPFN) * OriginalDestTableEntries.pt_entry.value.pfn,
		sizeof(_MMPFN),
		(UINT8*)&PfnDataBaseEntry))
	{
		debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET PFN DATABASE ENTRY\n", __func__);
	}

	// 2. write pt to dest process
	/*dc.WriteOverMapViewOfSection((UINT64)OriginalDestTableEntries.pt_entry.pointer,
			sizeof(PTE), (UINT8*)&SourceTableEntries.pt_entry.value.value);
			*/

	CloseHandle(DestProcessHandle);

	return TRUE;
}

BOOL PageStealer::StealEntireVirtualAddressSpace(PPROCESS_MINIMAL_INFO SourcePMI, PPROCESS_MINIMAL_INFO DestPMI, BOOL DropHighUserMemory)
{
	// debug::printf_d(debug::LogLevel::FATAL, "%s FIX ME", __func__);

	UINT64 DestKPROCESS = PageStealer::GetKPROCESSByPMI(DestPMI);
	UINT64 SourceKPROCESS = PageStealer::GetKPROCESSByPMI(SourcePMI);

	UINT64 SourceRootVADPtr = VadExplorer::GetVadRootByEPROCESS(SourceKPROCESS);
	UINT64 DestRootVADPtr = VadExplorer::GetVadRootByEPROCESS(DestKPROCESS);

	std::vector<VadExplorer::PUBLIC_VADINFO> DestVadInfoVec = VadExplorer::GetVadInfoVectorByRootVad(SourceRootVADPtr);

	for (VadExplorer::PUBLIC_VADINFO& a : DestVadInfoVec)
	{
		if (DropHighUserMemory)
		{
			if (((a.StartingVpn >> 32) == 0))
			{
				PageStealer::MapVirtualPageToAnotherProcess(SourcePMI,
					DestPMI,
					a.StartingVpn << 12,
					TRUE);
			}
			else
			{
				debug::printf_d(debug::LogLevel::LOG, "%s SKIPPING THIS (0x%llx) PAGE COZ IT'S HIGH\n", __func__, a.StartingVpn << 12);
			}
		}
		else
		{
			PageStealer::MapVirtualPageToAnotherProcess(SourcePMI,
				DestPMI,
				a.StartingVpn << 12,
				TRUE);
		}
	}

	return FALSE;
}

PageStealer::PROCESS_MINIMAL_INFO PageStealer::GetPMIByProcessName(std::string ProcessName)
{
	PROCESS_MINIMAL_INFO retPMI = { 0 };

	memcpy_s(retPMI.ImageFileName, min(14, strlen(ProcessName.c_str())), 
		ProcessName.c_str(), min(14, strlen(ProcessName.c_str())));
	std::wstring wProcessName(ProcessName.begin(), ProcessName.end());
	retPMI.PID = GetPIDByName(wProcessName);
	if (retPMI.PID != -1)
	{
		return retPMI;
	}

	debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET PMI\n", __func__);
	return { 0 };
}