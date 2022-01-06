#include "VadExplorer.h"

VOID VADEXPLORER::ListVAD(UINT64 PParentVAD_, LONG level)
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

UINT64 VADEXPLORER::GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA)
{
    UINT64 res = _GetTargetVADByRootVadAndVA(RootVad, VA);
    if (res == 0)
    {
        debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT FIND VAD FOR 0x%llx\n", __func__, VA);
    }
    return res;
}

UINT64 VADEXPLORER::_GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA)
{
    UINT64 TargetStartingVpn = VA >> 12;

    MMVAD_SHORT ParentVADValue = { 0 };
    SetMMVadByPtr((PMMVAD_SHORT)RootVad, &ParentVADValue);

    if (ParentVADValue.StartingVpn == TargetStartingVpn)
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

UINT64 VADEXPLORER::GetVadRootByEPROCESS(UINT64 EPROCESS)
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
bool VADEXPLORER::SetMMVadByPtr(PMMVAD_SHORT VadPtrInKernelSpace, PMMVAD_SHORT pVadValue)
{
    DriverControl& dc = DriverControl::GetInstance();

    if (dc.ReadKernelVA((UINT64)VadPtrInKernelSpace, sizeof(MMVAD_SHORT), (UINT8*)pVadValue))
    {
        return true;
    }
    debug::printf_d(debug::LogLevel::FATAL, "%s CANNOT GET VAD ROOT\n");
    return false;
}

VOID VADEXPLORER::DisplayVadInfo(PMMVAD_SHORT pVad)
{
    debug::printf_d(debug::LogLevel::LOG, 
        "\t[+]Starting VPN 0x%llx\n   \tEnding VPN   0x%llx\n\tProtection", 
        pVad->StartingVpn,
        pVad->EndingVpn);
    
    // debug::printf_d(debug::LogLevel::LOG, "   Ending VPN   0x%llx", pVad->EndingVpn);
    // debug::printf_d(debug::LogLevel::LOG, "      Control Area : 0x%x", pVadInfo->pControlArea);
    // debug::printf_d(debug::LogLevel::LOG, "      File Object : 0x%x", pVadInfo->pFileObject);
    // debug::printf_d(debug::LogLevel::LOG, "      Name : %wZ", pVadInfo->Name);
    return;
}