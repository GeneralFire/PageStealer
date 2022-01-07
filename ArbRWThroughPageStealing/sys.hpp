#pragma once
#include <Windows.h>

namespace sys {

	template <typename T>
	T* AllocateLocked(unsigned int size) {
		auto p = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE | PAGE_NOCACHE);
		if (p != nullptr) {
			VirtualLock(p, size);
		}

		return static_cast<T*>(p);
	}

	template <typename T>
	void FreeLocked(volatile T* p, unsigned int size) {
		VirtualUnlock(const_cast<T*>(p), size);
		VirtualFree(const_cast<T*>(p), 0, MEM_RELEASE);
	}

	enum class Mode {
		Unkn,
		X64,
		X86UnderX64,
		X86
	};

	struct RTL_PROCESS_MODULE_INFORMATION_x64 {
		HANDLE Section;
		UINT64 MappedBase;
		UINT64 ImageBase;
		ULONG ImageSize;
		ULONG Flags;
		USHORT LoadOrderIndex;
		USHORT InitOrderIndex;
		USHORT LoadCount;
		USHORT OffsetToFileName;
		UINT64 FullPathName;
	};

	struct RTL_PROCESS_MODULE_INFORMATION_native {
		HANDLE Section;
		PVOID MappedBase;
		PVOID ImageBase;
		ULONG ImageSize;
		ULONG Flags;
		USHORT LoadOrderIndex;
		USHORT InitOrderIndex;
		USHORT LoadCount;
		USHORT OffsetToFileName;
		UCHAR FullPathName[256];
	};

	struct RTL_PROCESS_MODULE_INFORMATION_x32 {
		HANDLE Section;
		UINT32 MappedBase;
		UINT32 ImageBase;
		ULONG ImageSize;
		ULONG Flags;
		USHORT LoadOrderIndex;
		USHORT InitOrderIndex;
		USHORT LoadCount;
		USHORT OffsetToFileName;
		UINT32 FullPathName;
	};

	struct RTL_PROCESS_MODULES_x64 {
		ULONG NumberOfModules;
		RTL_PROCESS_MODULE_INFORMATION_x64 Modules[ANYSIZE_ARRAY];
	};

	struct RTL_PROCESS_MODULES_native {
		ULONG NumberOfModules;
		RTL_PROCESS_MODULE_INFORMATION_native Modules[ANYSIZE_ARRAY];
	};

	struct RTL_PROCESS_MODULES_x32 {
		ULONG NumberOfModules;
		RTL_PROCESS_MODULE_INFORMATION_x32 Modules[ANYSIZE_ARRAY];
	};

	enum class SYSTEM_INFORMATION_CLASS {
		SystemModuleInformation = 0xB,
		SystemModuleInformationEx = 0x4D
	};

	using NtQuerySystemInformation_t = NTSTATUS(*)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

	Mode GetCurrentMode();
	bool SetPrivilege(HANDLE token, LPCTSTR privilege, bool enable);
	NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS system_information_class, PVOID system_information, ULONG system_information_length, PULONG return_length);

}