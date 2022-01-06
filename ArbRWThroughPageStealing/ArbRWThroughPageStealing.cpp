// ArbRWThroughPageStealing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PageStealer.h"

int main()
{
    CoreDBG& coreDbg = CoreDBG::GetInstance();


    //while (1)
    //{
    //    UINT64 a = coreDbg.getKernelSymbolAddress((char*)"MiState");
    //    a = coreDbg.getKernelSymbolAddress((char*)"MiVisibleState");
    //    a = coreDbg.getKernelSymbolAddress((char*)"MiState");
    //}
    DWORD DestPID = PageStealer::GetPIDByName(L"ArbRWThroughPageStealing.exe");

    DWORD SourcePID = PageStealer::GetPIDByName(L"mspaint.exe");

    // PPML4T pPml4T = static_cast<PPML4T>(PageStealer::GetProceessPageTableL4(PID1));

    PVOID pa = PageStealer::VTOP((UINT64)&DestPID, (UINT64) PageStealer::GetKPROCESSByPID(DestPID), NULL);

    
    // PVOID va =  PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace((UINT64) PageStealer::GetKPROCESSByPID(PID), 0x13000, 3);
    PageStealer::MapVirtualPageToAnotherProcess(SourcePID,
        DestPID,
        0x7FF7AA6B4030,
        TRUE);

    return 0;
}
