#pragma once
#include <windows.h>
#include <cstdint>
#include "globals.h"

#ifndef CLZLL64
#define CLZLL64( x ) ( int )__lzcnt64( x )
#endif

#define LOG2( x ) \
    ( ( unsigned ) \
        ( 8 * sizeof ( unsigned long long ) - CLZLL64( ( x ) ) - 1 ) )

#ifndef PAGE_SIZE
#define PAGE_SIZE ( 1024 * 4 )
#endif

#ifndef PAGE_SHIFT
#define PAGE_SHIFT LOG2( PAGE_SIZE )
#endif

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

//
// arch: x64
// virtual address definition
//
typedef union _VIRTUAL_ADDRESS
{
    PVOID value;
    struct
    {
        uint64_t offset : 12;
        uint64_t pt_index : 9;
        uint64_t pd_index : 9;
        uint64_t pdp_index : 9;
        uint64_t pml4_index : 9;
        uint64_t reserved : 16;
    };
} VIRTUAL_ADDRESS, * PVIRTUAL_ADDRESS;


//
// arch: x64
// page map level 4 entry definition
//
typedef union _PML4E
{
    uint64_t value;
    struct
    {
        uint64_t present : 1;
        uint64_t writable : 1;
        uint64_t user_access : 1;
        uint64_t write_through : 1;
        uint64_t cache_disabled : 1;
        uint64_t accessed : 1;
        uint64_t ignored_3 : 1;
        uint64_t size : 1;
        uint64_t ignored_2 : 4;
        uint64_t pfn : 36;
        uint64_t reserved_1 : 4;
        uint64_t ignored_1 : 11;
        uint64_t execution_disabled : 1;
    };
} PML4E, * PPML4E;

typedef struct _PML4T
{
    PML4E TableEntry[512];
} PML4T, * PPML4T;
//
// arch: x64
// page directory pointer entry definition
//
typedef union PDPE
{
    uint64_t value;
    struct
    {
        uint64_t present : 1;
        uint64_t writable : 1;
        uint64_t user_access : 1;
        uint64_t write_through : 1;
        uint64_t cache_disabled : 1;
        uint64_t accessed : 1;
        uint64_t ignored_3 : 1;
        uint64_t size : 1;
        uint64_t ignored_2 : 4;
        uint64_t pfn : 36;
        uint64_t reserved_1 : 4;
        uint64_t ignored_1 : 11;
        uint64_t execution_disabled : 1;
    };
} PDPE, * PPDPE;

//
// arch: x64
// page directory entry definition
//
typedef union _PDE
{
    uint64_t value;
    struct
    {
        uint64_t present : 1;
        uint64_t writable : 1;
        uint64_t user_access : 1;
        uint64_t write_through : 1;
        uint64_t cache_disabled : 1;
        uint64_t accessed : 1;
        uint64_t ignored1 : 1;
        uint64_t size : 1;
        uint64_t ignored_2 : 4;
        uint64_t pfn : 36;
        uint64_t reserved_1 : 4;
        uint64_t ignored_1 : 11;
        uint64_t execution_disabled : 1;
    };
} PDE, * PPDE;

//
// arch: x64
// page table entry definition
//
typedef union _PTE
{
    uint64_t value;
    struct
    {
        uint64_t present : 1;
        uint64_t writable : 1;
        uint64_t user_access : 1;
        uint64_t write_through : 1;
        uint64_t cache_disabled : 1;
        uint64_t accessed : 1;
        uint64_t dirty : 1;
        uint64_t access_type : 1;
        uint64_t global : 1;
        uint64_t ignored_2 : 3;
        uint64_t pfn : 36;
        uint64_t reserved_1 : 4;
        uint64_t ignored_3 : 7;
        uint64_t protection_key : 4;
        uint64_t execution_disabled : 1;
    };
} PTE, * PPTE;

template<typename T>
struct PAGE_ENTRY
{
    PVOID pointer;
    T value;
};

typedef struct _VirtualAddressTableEntries
{
    PAGE_ENTRY<PML4E> pml4_entry;
    PAGE_ENTRY<PDPE> pdp_entry;
    PAGE_ENTRY<PDE> pd_entry;
    PAGE_ENTRY<PTE> pt_entry;

} VirtualAddressTableEntries, *PVirtualAddressTableEntries;

//0x1 bytes (sizeof)
struct _MMPFNENTRY1
{
    UCHAR PageLocation : 3;                                                   //0x0
    UCHAR WriteInProgress : 1;                                                //0x0
    UCHAR Modified : 1;                                                       //0x0
    UCHAR ReadInProgress : 1;                                                 //0x0
    UCHAR CacheAttribute : 2;                                                 //0x0
};

//0x1 bytes (sizeof)
struct _MMPFNENTRY3
{
    UCHAR Priority : 3;                                                       //0x0
    UCHAR OnProtectedStandby : 1;                                             //0x0
    UCHAR InPageError : 1;                                                    //0x0
    UCHAR SystemChargedPage : 1;                                              //0x0
    UCHAR RemovalRequested : 1;                                               //0x0
    UCHAR ParityError : 1;                                                    //0x0
};

//0x8 bytes (sizeof)
struct _MMPTE
{
    union
    {
        ULONGLONG Long;                                                     //0x0
        volatile ULONGLONG VolatileLong;                                    //0x0
        //struct _MMPTE_HARDWARE Hard;                                        //0x0
        //struct _MMPTE_PROTOTYPE Proto;                                      //0x0
        //struct _MMPTE_SOFTWARE Soft;                                        //0x0
        //struct _MMPTE_TIMESTAMP TimeStamp;                                  //0x0
        //struct _MMPTE_TRANSITION Trans;                                     //0x0
        //struct _MMPTE_SUBSECTION Subsect;                                   //0x0
        //struct _MMPTE_LIST List;                                            //0x0
    } u;                                                                    //0x0
};

//0x8 bytes (sizeof)
struct _MI_ACTIVE_PFN
{
    union
    {
        struct
        {
            ULONGLONG Tradable : 1;                                           //0x0
            ULONGLONG NonPagedBuddy : 43;                                     //0x0
        } Leaf;                                                             //0x0
        struct
        {
            ULONGLONG Tradable : 1;                                           //0x0
            ULONGLONG WsleAge : 3;                                            //0x0
            ULONGLONG OldestWsleLeafEntries : 10;                             //0x0
            ULONGLONG OldestWsleLeafAge : 3;                                  //0x0
            ULONGLONG NonPagedBuddy : 43;                                     //0x0
        } PageTable;                                                        //0x0
        ULONGLONG EntireActiveField;                                        //0x0
    };
};

//0x8 bytes (sizeof)
struct _MIPFNBLINK
{
    union
    {
        struct
        {
            ULONGLONG Blink : 36;                                             //0x0
            ULONGLONG NodeBlinkHigh : 20;                                     //0x0
            ULONGLONG TbFlushStamp : 4;                                       //0x0
            ULONGLONG Unused : 2;                                             //0x0
            ULONGLONG PageBlinkDeleteBit : 1;                                 //0x0
            ULONGLONG PageBlinkLockBit : 1;                                   //0x0
            ULONGLONG ShareCount : 62;                                        //0x0
            ULONGLONG PageShareCountDeleteBit : 1;                            //0x0
            ULONGLONG PageShareCountLockBit : 1;                              //0x0
        };
        ULONGLONG EntireField;                                              //0x0
        volatile LONGLONG Lock;                                             //0x0
        struct
        {
            ULONGLONG LockNotUsed : 62;                                       //0x0
            ULONGLONG DeleteBit : 1;                                          //0x0
            ULONGLONG LockBit : 1;                                            //0x0
        };
    };
};

//0x30 bytes (sizeof)
struct _MMPFN
{
    union
    {
        struct _LIST_ENTRY ListEntry;                                       //0x0
        struct _RTL_BALANCED_NODE TreeNode;                                 //0x0
        struct
        {
            union
            {
                struct _SINGLE_LIST_ENTRY NextSlistPfn;                     //0x0
                VOID* Next;                                                 //0x0
                ULONGLONG Flink : 36;                                         //0x0
                ULONGLONG NodeFlinkHigh : 28;                                 //0x0
                struct _MI_ACTIVE_PFN Active;                               //0x0
            } u1;                                                           //0x0
            union
            {
                struct _MMPTE* PteAddress;                                  //0x8
                ULONGLONG PteLong;                                          //0x8
            };
            struct _MMPTE OriginalPte;                                      //0x10
        };
    };
    struct _MIPFNBLINK u2;                                                  //0x18
    union
    {
        struct
        {
            USHORT ReferenceCount;                                          //0x20
            struct _MMPFNENTRY1 e1;                                         //0x22
        };
        struct
        {
            struct _MMPFNENTRY3 e3;                                         //0x23
            struct
            {
                USHORT ReferenceCount;                                          //0x20
            } e2;                                                               //0x20
        };
        struct
        {
            ULONG EntireField;                                              //0x20
        } e4;                                                               //0x20
    } u3;                                                                   //0x20
    USHORT NodeBlinkLow;                                                    //0x24
    UCHAR Unused : 4;                                                         //0x26
    UCHAR Unused2 : 4;                                                        //0x26
    union
    {
        UCHAR ViewCount;                                                    //0x27
        UCHAR NodeFlinkLow;                                                 //0x27
        struct
        {
            UCHAR ModifiedListBucketIndex : 4;                                //0x27
            UCHAR AnchorLargePageSize : 2;                                    //0x27
        };
    };
    union
    {
        ULONGLONG PteFrame : 36;                                              //0x28
        ULONGLONG ResidentPage : 1;                                           //0x28
        ULONGLONG Unused1 : 1;                                                //0x28
        ULONGLONG Unused2 : 1;                                                //0x28
        ULONGLONG Partition : 10;                                             //0x28
        ULONGLONG FileOnly : 1;                                               //0x28
        ULONGLONG PfnExists : 1;                                              //0x28
        ULONGLONG Spare : 9;                                                  //0x28
        ULONGLONG PageIdentity : 3;                                           //0x28
        ULONGLONG PrototypePte : 1;                                           //0x28
        ULONGLONG EntireField;                                              //0x28
    } u4;                                                                   //0x28
};
