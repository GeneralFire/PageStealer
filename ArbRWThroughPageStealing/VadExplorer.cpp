#include "VadExplorer.h"

UINT8 VadExplorer::OriginalBytes[] = { 0xFC, 0x06, 0x00, 0x00 };
UINT8 VadExplorer::OriginalBytesRes[] = { 0x33, 0xDB, 0x8B, 0xC3 };
UINT8 VadExplorer::StoreRes[] = { 0x48, 0x89, 0x47, 0x10 };

VOID VadExplorer::ListVAD(UINT64 PParentVAD_, LONG level)
{
    PMMVAD_SHORT pVadLeft = NULL;
    PMMVAD_SHORT pVadRight = NULL;
    MMVAD_SHORT ParentVADValue = { 0 };

    if (PParentVAD_ == NULL)
        return;

    PMMVAD_SHORT PParentVAD = reinterpret_cast<PMMVAD_SHORT>(PParentVAD_);

    SetMMVadByPtr((PMMVAD_SHORT)PParentVAD, &ParentVADValue);
    DisplayVadInfo(&ParentVADValue);

    pVadLeft = (PMMVAD_SHORT)ParentVADValue.VadNode.Left;
    pVadRight = (PMMVAD_SHORT)ParentVADValue.VadNode.Right;

    if (pVadLeft != 0x0)
    {
        ListVAD((UINT64)pVadLeft, level + 1);
    }
    if (pVadRight != 0x0)
    {
        ListVAD((UINT64)pVadRight, level + 1);
    }

    return;
}

UINT64 VadExplorer::GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA)
{
    UINT64 res = _GetTargetVADByRootVadAndVA(RootVad, VA);
    if (res == 0)
    {
        debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT FIND VAD FOR 0x%llx\n", __func__, VA);
    }
    return res;
}

UINT64 VadExplorer::_GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA)
{
    UINT64 TargetStartingVpn = VA >> 12;

    MMVAD_SHORT ParentVADValue = { 0 };
    SetMMVadByPtr((PMMVAD_SHORT)RootVad, &ParentVADValue);

    if ((ParentVADValue.StartingVpn | ((UINT64)ParentVADValue.StartingVpnHigh << 32)) == TargetStartingVpn)
    {
        return RootVad;
    }
    PMMVAD_SHORT pVadLeft = NULL;
    PMMVAD_SHORT pVadRight = NULL;

    pVadLeft = (PMMVAD_SHORT)ParentVADValue.VadNode.Left;
    pVadRight = (PMMVAD_SHORT)ParentVADValue.VadNode.Right;

    UINT64 LeftRet = 0;
    UINT64 RightRet = 0;
    if (pVadLeft != 0x0)
    {
        LeftRet = _GetTargetVADByRootVadAndVA((UINT64)pVadLeft, VA);
    }
    if (pVadRight != 0x0)
    {
        RightRet = _GetTargetVADByRootVadAndVA((UINT64)pVadRight, VA);
    }

    if (RightRet | LeftRet)
    {
        return max(RightRet, LeftRet);
    }
    return 0;
}

UINT64 VadExplorer::GetVadRootByEPROCESS(UINT64 EPROCESS)
{
	CoreDBG& coreDBG = CoreDBG::GetInstance();
	UINT64 offset = coreDBG.getFieldOffset((wchar_t*) L"_EPROCESS", (wchar_t*) L"VadRoot");

    // MMVAD_SHORT VadRoot = { 0 };
	DriverControl& dc = DriverControl::GetInstance();
	UINT64 VadRoot = 0;
	if (dc.ReadKernelVA(EPROCESS + offset, sizeof(VadRoot), (UINT8*)&VadRoot))
	{
		return VadRoot;
	}
	debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET VAD ROOT\n");
    return { 0 };
}

/// <summary>
/// read Vad info by vad ptr in kernel space
/// </summary>
/// <param name="VadPtrInKernelSpace">PtrToVad in kernel space</param>
/// <param name="pVadValue">PtrToVadValue in userspace</param>
/// <returns>true if success, otherwise false</returns>
bool VadExplorer::SetMMVadByPtr(PMMVAD_SHORT VadPtrInKernelSpace, PMMVAD_SHORT pVadValue)
{
    DriverControl& dc = DriverControl::GetInstance();

    if (dc.ReadKernelVA((UINT64)VadPtrInKernelSpace, sizeof(MMVAD_SHORT), (UINT8*)pVadValue))
    {
        return true;
    }
    debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET VAD ROOT\n");
    return false;
}

VOID VadExplorer::DisplayVadInfo(PMMVAD_SHORT pVad)
{
    debug::printf_d(debug::LogLevel::LOG, 
        "\t[+]Starting VPN 0x%llx\n   \tEnding VPN   0x%llx\n\tProtection", 
        pVad->StartingVpn,
        pVad->EndingVpn,
        pVad->u.VadFlags.Protection
    );
    
    // debug::printf_d(debug::LogLevel::LOG, "   Ending VPN   0x%llx", pVad->EndingVpn);
    // debug::printf_d(debug::LogLevel::LOG, "      Control Area : 0x%x", pVadInfo->pControlArea);
    // debug::printf_d(debug::LogLevel::LOG, "      File Object : 0x%x", pVadInfo->pFileObject);
    // debug::printf_d(debug::LogLevel::LOG, "      Name : %wZ", pVadInfo->Name);
    return;
}


std::vector<VadExplorer::PUBLIC_VADINFO> VadExplorer::GetVadInfoVectorByRootVad(UINT64 RootVad)
{
    MMVAD_SHORT ParentVADValue = { 0 };
    std::vector<VadExplorer::PUBLIC_VADINFO> out = {};
    SetMMVadByPtr((PMMVAD_SHORT)RootVad, &ParentVADValue);

    PUBLIC_VADINFO instance = { 0 };
    instance.EndingVpn = ParentVADValue.EndingVpn | ((UINT64)ParentVADValue.EndingVpnHigh << 32);
    instance.PtrInKernelSpace = RootVad;
    instance.StartingVpn = ParentVADValue.StartingVpn | ((UINT64)ParentVADValue.StartingVpnHigh << 32);
    out.push_back(instance);

    PMMVAD_SHORT pVadLeft = NULL;
    PMMVAD_SHORT pVadRight = NULL;

    pVadLeft = (PMMVAD_SHORT)ParentVADValue.VadNode.Left;
    pVadRight = (PMMVAD_SHORT)ParentVADValue.VadNode.Right;

    std::vector<VadExplorer::PUBLIC_VADINFO> LeftRet = {};
    std::vector<VadExplorer::PUBLIC_VADINFO> RightRet = {};
    if (pVadLeft != 0x0)
    {
        LeftRet = GetVadInfoVectorByRootVad((UINT64)pVadLeft);
        out.insert(out.end(), LeftRet.begin(), LeftRet.end());
    }
    if (pVadRight != 0x0)
    {
        RightRet = GetVadInfoVectorByRootVad((UINT64)pVadRight);
        out.insert(out.end(), RightRet.begin(), RightRet.end());
    }

    return out;
}

BOOL VadExplorer::HookDispatchRoutineAndInsertMiAllocateVad()
{
    DriverControl& dc = DriverControl::GetInstance();
    CoreDBG& coreDbg = CoreDBG::GetInstance();


    UINT64 IntelDrvBase = coreDbg.GetModuleBase((char*)"IntelDrvX64.sys");

    UINT64 MiAllocateVadFcnPtr = coreDbg.GetKernelBase() +
        coreDbg.getKernelSymbolAddress((char*)"MiAllocateVad");

    UINT64 PtrToFuntionToRewrite = IntelDrvBase + ARG3FCN_REFWIRTE;

    

    UINT32 dAddress = MiAllocateVadFcnPtr - PtrToFuntionToRewrite - 5;

    UINT64 PhysicalMemoryOfFunctionToRewrite = dc.Virt2Phys(PtrToFuntionToRewrite);
    
    BOOL bRes = dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 1,
        sizeof(dAddress),
        (UINT8*) &dAddress);

    bRes &= dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 5,
        sizeof(StoreRes),
        (UINT8*)&StoreRes);

    return bRes;
}

BOOL VadExplorer::HookDispatchRoutineAndInsertMiInsertVad()
{
    DriverControl& dc = DriverControl::GetInstance();
    CoreDBG& coreDbg = CoreDBG::GetInstance();


    UINT64 IntelDrvBase = coreDbg.GetModuleBase((char*)"IntelDrvX64.sys");

    UINT64 MiAllocateVadFcnPtr = coreDbg.GetKernelBase() +
        coreDbg.getKernelSymbolAddress((char*)"MiInsertVad");

    UINT64 PtrToFuntionToRewrite = IntelDrvBase + ARG3FCN_REFWIRTE;



    UINT32 dAddress = MiAllocateVadFcnPtr - PtrToFuntionToRewrite - 5;

    UINT64 PhysicalMemoryOfFunctionToRewrite = dc.Virt2Phys(PtrToFuntionToRewrite);

    

    BOOL bRes = dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 1,
        sizeof(dAddress),
        (UINT8*)&dAddress);

    bRes &= dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 5,
        sizeof(StoreRes),
        (UINT8*)&StoreRes);

    return bRes;

}

BOOL VadExplorer::HookDispatchRoutineAndInsertMiInsertVadCharges()
{
    DriverControl& dc = DriverControl::GetInstance();
    CoreDBG& coreDbg = CoreDBG::GetInstance();


    UINT64 IntelDrvBase = coreDbg.GetModuleBase((char*)"IntelDrvX64.sys");

    UINT64 MiAllocateVadFcnPtr = coreDbg.GetKernelBase() +
        coreDbg.getKernelSymbolAddress((char*)"MiInsertVadCharges");

    if (MiAllocateVadFcnPtr == coreDbg.GetKernelBase())
    {
        debug::printf_d(debug::LogLevel::FATAL, "CANNOT HOOK\n");
    }
    UINT64 PtrToFuntionToRewrite = IntelDrvBase + ARG3FCN_REFWIRTE;



    UINT32 dAddress = MiAllocateVadFcnPtr - PtrToFuntionToRewrite - 5;

    UINT64 PhysicalMemoryOfFunctionToRewrite = dc.Virt2Phys(PtrToFuntionToRewrite);



    BOOL bRes = dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 1,
        sizeof(dAddress),
        (UINT8*)&dAddress);

    bRes &= dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 5,
        sizeof(StoreRes),
        (UINT8*)&StoreRes);

    return bRes;

}
BOOL VadExplorer::HookDispatchRoutineRestoreOriginalBytes()
{
    DriverControl& dc = DriverControl::GetInstance();
    CoreDBG& coreDbg = CoreDBG::GetInstance();

    UINT64 IntelDrvBase = coreDbg.GetModuleBase((char*)"IntelDrvX64.sys");

    UINT64 PtrToFuntionToRewrite = IntelDrvBase + ARG3FCN_REFWIRTE;

    UINT64 PhysicalMemoryOfFunctionToRewrite = dc.Virt2Phys(PtrToFuntionToRewrite);

    BOOL bRes = dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 1,
        sizeof(UINT32),
        (UINT8*)OriginalBytes);

    bRes &= dc.WriteOverMapViewOfSection(
        PhysicalMemoryOfFunctionToRewrite + 5,
        sizeof(OriginalBytesRes),
        (UINT8*)&OriginalBytesRes);

    return bRes;
}


/// <summary>
/// Allocates VAD
/// </summary>
/// <param name="StartingVirtualAddress"></param>
/// <param name="EndingVirtualAddress"></param>
/// <param name="Deletable"></param>
/// <returns>if not zero, then success</returns>
UINT64 VadExplorer::CallMiAllocateVad(UINT64 StartingVirtualAddress,
    UINT64 EndingVirtualAddress,
    CHAR Deletable)
{
    if (HookDispatchRoutineAndInsertMiAllocateVad())
    {
        DriverControl& dc = DriverControl::GetInstance();

        UINT64 res = dc.CallFcn3Arg(StartingVirtualAddress,
            EndingVirtualAddress,
            Deletable);
        HookDispatchRoutineRestoreOriginalBytes();
        return res;
    }

    return 0;
}

/// <summary>
/// Insetr VAD into tree
/// </summary>
/// <param name="VadPtr">VAD PTR</param>
/// <param name="EPROCESS">EPROCESS PTR</param>
/// <param name="Deletable">Always true?</param>
/// <returns>NTSTATUS?</returns>
UINT64 VadExplorer::CallMiInsertVad(UINT64 VadPtr,
    UINT64 EPROCESS,
    CHAR Deletable)
{
    if (HookDispatchRoutineAndInsertMiInsertVad())
    {
        DriverControl& dc = DriverControl::GetInstance();

        UINT64 res = dc.CallFcn3Arg(VadPtr,
            EPROCESS,
            Deletable);
        HookDispatchRoutineRestoreOriginalBytes();
        return res;
    }

    return 0;
}

/// <summary>
/// calls insertvadcharges
/// </summary>
/// <param name="VadPtr">ptr to VAD</param>
/// <param name="EPROCESS">ERPTOCESS</param>
/// <returns>if zero then success</returns>
UINT64 VadExplorer::CallMiInsertVadCharges(UINT64 VadPtr,
    UINT64 EPROCESS)
{
    if (HookDispatchRoutineAndInsertMiInsertVadCharges())
    {
        DriverControl& dc = DriverControl::GetInstance();

        UINT64 res = dc.CallFcn3Arg(VadPtr,
            EPROCESS,
            0);
        HookDispatchRoutineRestoreOriginalBytes();
        return res;
    }

    return 0;
}

BOOL VadExplorer::AllocMemoryUsingVadHook(UINT64 StartingVirtualAddress, UINT64 EndingVirtualAddress, UINT64 EPROCESS)
{

    UINT64 VADPTR = VadExplorer::CallMiAllocateVad(StartingVirtualAddress,
        EndingVirtualAddress,
        1);

    if (VADPTR)
    {
        UINT64 MIVCRes = VadExplorer::CallMiInsertVadCharges(VADPTR,
            EPROCESS
        );

        if (MIVCRes == 0)
        {
            UINT64 MIV = VadExplorer::CallMiInsertVad(VADPTR,
                EPROCESS,
                1
            );

            return TRUE;
        }
    }

    return FALSE;
}
