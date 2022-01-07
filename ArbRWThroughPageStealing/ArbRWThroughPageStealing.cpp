// ArbRWThroughPageStealing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PageStealer.h"
#include "VadExplorer.h"

int main()
{
    PageStealer::PROCESS_MINIMAL_INFO DestPMI = PageStealer::GetPMIByProcessName("ArbRWThroughPageStealing.exe");

    PVOID sampleAlloc = VirtualAllocEx(OpenProcess(PROCESS_VM_OPERATION, TRUE, GetCurrentProcessId()),
        (PVOID) 0x0000017174540000, PAGE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READONLY);

    UINT64 sampleUINT64 = 32;

    PageStealer::PROCESS_MINIMAL_INFO SourcePMI = PageStealer::GetPMIByProcessName("mspaint.exe");

    UINT64 DestKPROCESS = PageStealer::GetKPROCESSByPMI(&DestPMI);
    UINT64 SourceKPROCESS = PageStealer::GetKPROCESSByPMI(&SourcePMI);

    PVOID pa = PageStealer::VTOP((UINT64)&sampleUINT64, 
        DestKPROCESS, 
        NULL);

    UINT64 SourceRootVADPtr = VadExplorer::GetVadRootByEPROCESS(SourceKPROCESS);
    UINT64 DestRootVADPtr = VadExplorer::GetVadRootByEPROCESS(DestKPROCESS);

    UINT64 VAD = VadExplorer::GetTargetVADByRootVadAndVA(DestRootVADPtr, (UINT64) sampleAlloc);

    PageStealer::StealEntireVirtualAddressSpace(
        &SourcePMI,
        &DestPMI,
        true);

    pa = PageStealer::VTOP(0x7FF7C06A4000, DestKPROCESS, NULL);
 
    return 0;
}
