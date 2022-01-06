#pragma once

#include "CoreDBG.h"
#include "driver_control.hpp"

#define MM_ZERO_ACCESS         0  // this value is not used. 
#define MM_READONLY            1 
#define MM_EXECUTE             2 
#define MM_EXECUTE_READ        3 
#define MM_READWRITE           4  // bit 2 is set if this is writable. 
#define MM_WRITECOPY           5 
#define MM_EXECUTE_READWRITE   6 
#define MM_EXECUTE_WRITECOPY   7 
#define MM_NOCACHE             8 
#define MM_DECOMMIT         0x10 
#define MM_NOACCESS         MM_DECOMMIT|MM_NOCACHE

class VadExplorer
{

private:

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

    typedef struct _RTL_BALANCED_NODE
    {
        union
        {
            struct _RTL_BALANCED_NODE* Children[2];                             //0x0
            struct
            {
                struct _RTL_BALANCED_NODE* Left;                                //0x0
                struct _RTL_BALANCED_NODE* Right;                               //0x8
            };
        };
        union
        {
            struct
            {
                UCHAR Red : 1;                                                    //0x10
                UCHAR Balance : 2;                                                //0x10
            };
            ULONGLONG ParentValue;                                              //0x10
        };
    } RTL_BALANCED_NODE, *PRTL_BALANCED_NODE;

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
};
