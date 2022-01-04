#pragma once
#include <Windows.h>

namespace iqvw {

constexpr LPCTSTR kDeviceName{ TEXT("\\\\.\\Nal") };

constexpr DWORD kIoctlMain{ 0x80862007 };
constexpr DWORD kIoctlSecondary3{ 0x80862013 };

enum class Subcommand {
	IoRead8			= 0x1,
	IoRead16		= 0x2,
	IoRead32		= 0x3,
	IoWrite8		= 0x7,
	IoWrite16		= 0x8,
	IoWrite32		= 0x9,
	MemRead8		= 0xD,
	MemRead16		= 0xE,
	MemRead32		= 0xF,
	MemWrite8		= 0x13,
	MemWrite16		= 0x14,
	MemWrite32		= 0x15,
	MapIoSpace		= 0x19,
	UnmapIoSpace	= 0x1A,
	Virt2Phys		= 0x25,
	MemCopy			= 0x33
};

struct GetPhysAddrBufferInfo {
	UINT64 case_number;
	UINT64 reserved;
	UINT64 return_physical_address;
	UINT64 address_to_translate;
};

struct MapIoSpaceBufferInfoX64 {
	UINT64 case_number;
	UINT64 reserved;
	UINT64 return_value;
	UINT64 return_virtual_address;
	UINT64 physical_address_to_map;
	UINT32 size;
};

struct MapIoSpaceBufferInfoX32 {
	DWORD case_number;
	BYTE gap4[12];
	const void* pvoid10;
	DWORD return_value;
	UINT64 physical_address_to_map;
	DWORD size;
	DWORD mapToUserspaceFlag;
	DWORD Unkn;
};

struct CopyMemoryBufferInfoX64 {
	UINT64 case_number;
	UINT64 reserved;
	UINT64 source;
	UINT64 destination;
	UINT64 length;
};

struct CopyMemoryBufferInfoX32 {
	DWORD case_number;
	BYTE gap4[12];
	DWORD source;
	DWORD destination;
	DWORD length;
	DWORD unkn;
	DWORD dword20;
	DWORD dword24;
	DWORD dword28;
};

struct UnmapIoSpaceBufferInfoX64 {
	UINT64 case_number;
	UINT64 reserved1;
	UINT64 reserved2;
	UINT64 virt_address;
	UINT64 reserved3;
	UINT32 number_of_bytes;
};

struct UnmapIoSpaceBufferInfoX32 {
	DWORD case_number;
	BYTE gap4[12];
	DWORD pvoid10;
	DWORD virt_address;
	UINT32 reserved1;
	UINT32 reserved2;
	DWORD number_of_bytes;
};

struct ReadWriteInfoX32 {
	UINT32 case_number;
	BYTE gap4[12];
	UINT32 return_value;
	UINT32 address;
	UINT32 value;
};

struct ReadWriteInfoX64 {
	UINT64 case_number;
	UINT64 reserved;
	UINT64 return_value;
	UINT64 address;
	UINT64 value;
};

}