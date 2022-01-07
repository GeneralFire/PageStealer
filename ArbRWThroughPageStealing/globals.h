#pragma once

#include <Windows.h>

#if !defined(NT_SUCCESS)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

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
} RTL_BALANCED_NODE, * PRTL_BALANCED_NODE;

extern "C" void __stdcall RtlInitUnicodeString(
	PUNICODE_STRING DestinationString,
	PCWSTR          SourceString
);

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
	SystemPowerInformation_,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemMmSystemRangeStart_ = 50,
	SystemIsolatedUserModeInformation = 165
} SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS;

extern "C" NTSTATUS WINAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, OPTIONAL PULONG ReturnLength);

struct SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX // Size=28
{
	PVOID Object; // Size=4 Offset=0
	ULONG_PTR  UniqueProcessId; // Size=4 Offset=4
	ULONG_PTR  HandleValue; // Size=4 Offset=8
	ULONG GrantedAccess; // Size=4 Offset=12
	USHORT CreatorBackTraceIndex; // Size=2 Offset=16
	USHORT ObjectTypeIndex; // Size=2 Offset=18
	ULONG HandleAttributes; // Size=4 Offset=20
	ULONG Reserved; // Size=4 Offset=24
};

struct SYSTEM_HANDLE_INFORMATION_EX // Size=36
{
	ULONG_PTR NumberOfHandles; // Size=4 Offset=0
	ULONG_PTR  Reserved; // Size=4 Offset=4
	SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1]; // Size=36 Offset=8
};
