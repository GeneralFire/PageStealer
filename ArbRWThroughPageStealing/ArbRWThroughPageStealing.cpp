// ArbRWThroughPageStealing.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "PageStealer.h"

int main()
{
    DWORD PID1 = PageStealer::GetPIDByName(L"ArbRWThroughPageStealing.exe");

    DWORD NotepadPID = PageStealer::GetPIDByName(L"notepad++.exe");

    // PPML4T pPml4T = static_cast<PPML4T>(PageStealer::GetProceessPageTableL4(PID1));

    PVOID pa = PageStealer::VTOP((UINT64)&PID1, (UINT64) PageStealer::GetKPROCESSByPID(PID1), NULL);

    // PVOID va =  PageStealer::MapSinglePhysicalPageToProcessVirtualAddressSpace((UINT64) PageStealer::GetKPROCESSByPID(PID), 0x13000, 3);
    PageStealer::MapVirtualPageToAnotherProcess(NotepadPID,
        PID1,
        0x7FF615731000,
        TRUE);
    return 0;
}
