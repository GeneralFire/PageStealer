#pragma once
#include <Windows.h>

namespace asrdrv {

constexpr LPCTSTR kDeviceName{ TEXT("\\\\.\\AsrDrv101") };

constexpr DWORD kIoctlReadPciDword	{ 0x222840 };
constexpr DWORD kIoctlReadMsr		{ 0x222848 };
constexpr DWORD kIoctlWritePciDword	{ 0x222844 };

struct ReadPciDwordReq {
	CHAR bus;
	CHAR device;
	CHAR func;
	CHAR padding;
	WORD offset;
	DWORD ret_val;
};

struct WritePciDwordReq {
	CHAR bus;
	CHAR device;
	CHAR func;
	CHAR padding;
	WORD offset;
	DWORD val;
};

struct MsrReadBuffer {
	DWORD low;
	DWORD reserved;
	DWORD msr_addr;
	DWORD high;
};

}