#pragma once
#pragma once
#include <Windows.h>

namespace atszio {

	constexpr LPCTSTR kDeviceName{ TEXT("\\\\.\\ATSZIO") };

	constexpr DWORD kIoctlMapMem{ 0x8807200C };
	constexpr DWORD kIoctlUnmapMem{ 0x88072010 };

	typedef struct {
		SIZE_T CountOfBytes;
		HANDLE Handle;
		SIZE_T MapLength;
		PVOID PA;
		PVOID* VA;
	} MapMemIoCtlStruct, * pMapMemIoCtlStruct;

	typedef struct {
		ULONGLONG CountOfBytes;
		ULONGLONG Handle;
		ULONGLONG MapLength;
		ULONGLONG PA;
		ULONGLONG* VA;
	} MapMemIoCtlStructX64, * pMapMemIoCtlStructX64;

	typedef struct {
		UINT64 gap0;
		HANDLE Handle;
		UINT8 gapC[20];
		UINT32 VA;
	} UnmapMemIoCtlStruct, * pUnmapMemIoCtlStruct;

	typedef struct {
		UINT64 gap0;
		HANDLE handle;	// should be zero?
		UINT64 gap10[2];
		UINT64 VA;
	} UnmapMemIoCtlStructX64, * pUnmapMemIoCtlStructX64;

}