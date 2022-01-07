#pragma once

#include "CoreDBG.h"
#include "driver_control.hpp"
#include "VirtualsHeader.h"


#define ARG3FCN_REFWIRTE 0x177F

class VadExplorer
{

private:

    static UINT8 OriginalBytes[];
    static UINT8 OriginalBytesRes[];
    static UINT8 StoreRes[];

    typedef struct _MMVAD_FLAGS
    {
        ULONG VadType : 3;                                                        //0x0
        ULONG Protection : 5;                                                     //0x0
        ULONG PreferredNode : 6;                                                  //0x0
        ULONG PrivateMemory : 1;                                                  //0x0
        ULONG PrivateFixup : 1;                                                   //0x0
        ULONG Graphics : 1;                                                       //0x0
        ULONG Enclave : 1;                                                        //0x0
        ULONG PageSize64K : 1;                                                    //0x0
        ULONG ShadowStack : 1;                                                    //0x0
        ULONG Spare : 6;                                                          //0x0
        ULONG HotPatchAllowed : 1;                                                //0x0
        ULONG NoChange : 1;                                                       //0x0
        ULONG ManySubsections : 1;                                                //0x0
        ULONG DeleteInProgress : 1;                                               //0x0
        ULONG LockContended : 1;                                                  //0x0
        ULONG Lock : 1;                                                           //0x0
    } MMVAD_FLAGS, *PMMVAD_FLAGS;

    typedef struct _MMVAD_FLAGS1
    {
        ULONG CommitCharge : 31;                                                  //0x0
        ULONG MemCommit : 1;                                                      //0x0
    } _MMVAD_FLAGS1, *PMMVAD_FLAGS1;



    typedef struct _EX_PUSH_LOCK
    {
        union
        {
            struct
            {
                ULONGLONG Locked : 1;                                             //0x0
                ULONGLONG Waiting : 1;                                            //0x0
                ULONGLONG Waking : 1;                                             //0x0
                ULONGLONG MultipleShared : 1;                                     //0x0
                ULONGLONG Shared : 60;                                            //0x0
            };
            ULONGLONG Value;                                                    //0x0
            VOID* Ptr;                                                          //0x0
        };

    } EX_PUSH_LOCK, *PEX_PUSH_LOCK;

    typedef struct _MMVAD_SHORT
    {
        union
        {
            struct
            {
                struct _MMVAD_SHORT* NextVad;                                   //0x0
                VOID* ExtraCreateInfo;                                          //0x8
            };
            struct _RTL_BALANCED_NODE VadNode;                                  //0x0
        };
        ULONG StartingVpn;                                                      //0x18
        ULONG EndingVpn;                                                        //0x1c
        UCHAR StartingVpnHigh;                                                  //0x20
        UCHAR EndingVpnHigh;                                                    //0x21
        UCHAR CommitChargeHigh;                                                 //0x22
        UCHAR SpareNT64VadUChar;                                                //0x23
        LONG ReferenceCount;                                                    //0x24
        struct _EX_PUSH_LOCK PushLock;                                          //0x28
        union
        {
            ULONG LongFlags;                                                    //0x30
            struct _MMVAD_FLAGS VadFlags;                                       //0x30
            volatile ULONG VolatileVadLong;                                     //0x30
        } u;                                                                    //0x30
        union
        {
            ULONG LongFlags1;                                                   //0x34
            struct _MMVAD_FLAGS1 VadFlags1;                                     //0x34
        } u1;                                                                   //0x34
        struct _MI_VAD_EVENT_BLOCK* EventList;                                  //0x38
    } MMVAD_SHORT, *PMMVAD_SHORT;

    typedef struct _VAD_INFO
    {
        LONG level;
        ULONG pAddress;
        /*PCONTROL_AREA pControlArea;
        PFILE_OBJECT pFileObject;
        PUNICODE_STRING Name;*/
    } VAD_INFO, * PVAD_INFO;


    static VOID DisplayVadInfo(PMMVAD_SHORT pVadInfo);
    static bool SetMMVadByPtr(PMMVAD_SHORT VadPtrInKernelSpace, PMMVAD_SHORT pVadValue);
    static UINT64 _GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA);

    static BOOL HookDispatchRoutineAndInsertMiAllocateVad();
    static BOOL HookDispatchRoutineAndInsertMiInsertVad();
    static BOOL HookDispatchRoutineAndInsertMiInsertVadCharges();
    static BOOL HookDispatchRoutineRestoreOriginalBytes();

    static UINT64 CallMiAllocateVad(UINT64 StartingVirtualAddress,
        UINT64 EndingVirtualAddress,
        CHAR Deletable);

    static UINT64 CallMiInsertVad(UINT64 VadPtr,
        UINT64 EPROCESS,
        CHAR Deletable);

    static UINT64 CallMiInsertVadCharges(UINT64 VadPtr,
        UINT64 EPROCESS);

public:

    typedef struct _PUBLIC_VADINFO
    {
        UINT64 PtrInKernelSpace;
        UINT64 StartingVpn;
        UINT64 EndingVpn;

    } PUBLIC_VADINFO, PPUBLIC_VADINFO;

    static UINT64 GetTargetVADByRootVadAndVA(UINT64 RootVad, UINT64 VA);
	static VOID ListVAD(UINT64 PParentVAD, LONG level);
    static UINT64 GetVadRootByEPROCESS(UINT64 EPROCESS);
    static std::vector<PUBLIC_VADINFO> GetVadInfoVectorByRootVad(UINT64 RootVad);
    
    
    static BOOL AllocMemoryUsingVadHook(UINT64 StartingVirtualAddress, UINT64 EndingVirtualAddress, UINT64 EPROCESS);


};
