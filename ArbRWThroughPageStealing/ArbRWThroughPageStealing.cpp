// ArbRWThroughPageStealing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PageStealer.h"
#include "VadExplorer.h"

int main()
{

    printf("!");
    PVOID sampleAlloc = VirtualAllocEx(OpenProcess(PROCESS_VM_OPERATION, TRUE, GetCurrentProcessId()),
        0, PAGE_SIZE, MEM_COMMIT, PAGE_READONLY);

    UINT64 sampleUINT64 = 32;

    PageStealer::PROCESS_MINIMAL_INFO DestPMI = PageStealer::GetPMIByProcessName("ArbRWThroughPageStealing.exe");
    PageStealer::PROCESS_MINIMAL_INFO SourcePMI = PageStealer::GetPMIByProcessName("mspaint.exe");

    // PPML4T pPml4T = static_cast<PPML4T>(PageStealer::GetProceessPageTableL4(PID1));

    PVOID pa = PageStealer::VTOP((UINT64)&sampleUINT64, (UINT64) PageStealer::GetKPROCESSByPMI(&DestPMI), NULL);

    UINT64 RootVADPtr = VADEXPLORER::GetVadRootByEPROCESS(PageStealer::GetKPROCESSByPMI(&SourcePMI));
    VADEXPLORER::ListVAD(RootVADPtr, 0);
    UINT64 VAD = VADEXPLORER::GetTargetVADByRootVadAndVA(RootVADPtr, 0x7FF662654000);
    // PVOID va =  PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace((UINT64) PageStealer::GetKPROCESSByPID(PID), 0x13000, 3);
    PageStealer::MapVirtualPageToAnotherProcess(&SourcePMI,
        &DestPMI,
        0x7FF7C06A4010,
        TRUE);

    //PageStealer::MapVirtualPageToAnotherProcess(SourcePID,
    //    DestPID,
    //    0x7FF7C06A5000,
    //    TRUE);

    //PageStealer::MapVirtualPageToAnotherProcess(SourcePID,
    //    DestPID,
    //    0x7FF7C06A6000,
    //    TRUE);

    pa = PageStealer::VTOP(0x7FF7C06A4000, (UINT64)PageStealer::GetKPROCESSByPMI(&DestPMI), NULL);
    return 0;
}
