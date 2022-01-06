// ArbRWThroughPageStealing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PageStealer.h"

int main()
{
    CoreDBG& coreDbg = CoreDBG::GetInstance();

    UINT64 sampleUINT64 = 32;
    //while (1)
    //{
    //    UINT64 a = coreDbg.getKernelSymbolAddress((char*)"MiState");
    //    a = coreDbg.getKernelSymbolAddress((char*)"MiVisibleState");
    //    a = coreDbg.getKernelSymbolAddress((char*)"MiState");
    //}
    PageStealer::PROCESS_MINIMAL_INFO DestPMI = PageStealer::GetPMIByProcessName("ArbRWThroughPageStealing.exe");

    PageStealer::PROCESS_MINIMAL_INFO SourcePID = PageStealer::GetPMIByProcessName("mspaint.exe");

    // PPML4T pPml4T = static_cast<PPML4T>(PageStealer::GetProceessPageTableL4(PID1));

    PVOID pa = PageStealer::VTOP((UINT64)&sampleUINT64, (UINT64) PageStealer::GetKPROCESSByPMI(&DestPMI), NULL);

    
    // PVOID va =  PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace((UINT64) PageStealer::GetKPROCESSByPID(PID), 0x13000, 3);
    PageStealer::MapVirtualPageToAnotherProcess(&SourcePID,
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
