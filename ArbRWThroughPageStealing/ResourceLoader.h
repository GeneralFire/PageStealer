#pragma once
#include <iostream>
#include <Windows.h>
#include <NTSecAPI.h>
#include <filesystem>
#include <libloaderapi.h>
#include "debug.hpp"

class ResourceLoader {
public:
	ResourceLoader();

	enum class ResId {
		AsrDrvX64 = 0,
		IqvwX64,
		AsrDrvX32,
		IqvwX32,
		AtzioX64,
		AtzioX32,
		MsDiaX64,
		MsDiaX32,
	};

	BOOL unmapDrv();
	BOOL mapDrv();

	BOOL extractAndInstallMsdiaDlls();
	BOOL removeMsdiaDlls();

	enum class CurrentMode {
		unkn = 0,
		x64 = 1,
		x86UnderX64,
		x86,
	};

	CurrentMode currentMode = CurrentMode::unkn;
	
	std::wstring extractResource(ResId resId);

private:
	// OpenSCManager + OpenService + StartService
	BOOL RegisterAndStartServiceByPath(std::wstring pathToDriver);

	
	BOOL CreateAndRunServiceByResIndex(ResId);
	std::wstring getResourcePath(ResId resId);

	CurrentMode GetCurrentMode();

	BOOL StopAndRemoveByPath(std::wstring driver_name);

	const wchar_t* resourceNames[8] = {
		L"AsrDrvX64.sys",
		L"IntelDrvX64.sys",
		L"AsrDrvX32.sys",
		L"IntelDrvX32.sys",
		L"AtzioDrvX64.sys",
		L"AtzioDrvX32.sys",
		L"MsDiaX64.dll",
		L"MsDiaX32.dll",
	};

	typedef NTSTATUS(__stdcall *_NtLoadDriverX32)(PUNICODE_STRING DriverServiceName);
	typedef NTSTATUS(__stdcall *_NtUnloadDriverX32)(PUNICODE_STRING DriverServiceName);
	typedef NTSTATUS(__stdcall *_RtlAdjustPrivilegeX32)(_In_ ULONG Privilege, _In_ BOOLEAN Enable, _In_ BOOLEAN Client, _Out_ PBOOLEAN WasEnabled);

	typedef NTSTATUS(* _NtLoadDriverX64)(PUNICODE_STRING DriverServiceName);
	typedef NTSTATUS(* _NtUnloadDriverX64)(PUNICODE_STRING DriverServiceName);
	typedef NTSTATUS(* _RtlAdjustPrivilegeX64)(_In_ ULONG Privilege, _In_ BOOLEAN Enable, _In_ BOOLEAN Client, _Out_ PBOOLEAN WasEnabled);

	std::wstring storedAsrDrvName;
	std::wstring storedIqvwName;
	std::wstring storedAtzioName;
};