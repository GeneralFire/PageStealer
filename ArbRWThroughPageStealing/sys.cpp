#include <Windows.h>

#include "sys.hpp"

#pragma comment(lib, "ntdll.lib")

namespace sys {

Mode GetCurrentMode() {
	BOOL is_under_wow = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &is_under_wow)) {
		is_under_wow = TRUE;
	}

	if (is_under_wow) {
		return Mode::X86UnderX64;
	}

	if (sizeof(size_t) == 8) {
		return Mode::X64;
	}

	if (!is_under_wow && sizeof(size_t) == 4) {
		return Mode::X86;
	};

	return Mode::Unkn;
}

bool SetPrivilege(HANDLE token, LPCTSTR privilege, bool enable) {
	LUID luid;
	if (!LookupPrivilegeValue(nullptr, privilege, &luid)) {
		return false;
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if (enable) {
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	}
	else {
		tp.Privileges[0].Attributes = 0;
	}

	if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
		return false;
	}

	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		return false;
	}

	return true;
}

NTSTATUS NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS system_information_class, PVOID system_information, ULONG system_information_length, PULONG return_length) {
	static NtQuerySystemInformation_t pNtQuerySystemInformation = nullptr;
	
	if (pNtQuerySystemInformation != nullptr) {
		return pNtQuerySystemInformation(system_information_class, system_information, system_information_length, return_length);
	}

	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll != nullptr) {
		pNtQuerySystemInformation = reinterpret_cast<NtQuerySystemInformation_t>(GetProcAddress(ntdll, "NtQuerySystemInformation"));

		if (pNtQuerySystemInformation != nullptr) {
			return pNtQuerySystemInformation(system_information_class, system_information, system_information_length, return_length);
		}
	}

	return STATUS_INVALID_PARAMETER;
}

}