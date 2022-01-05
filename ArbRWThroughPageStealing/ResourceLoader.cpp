#include "ResourceLoader.h"
#include "resource_array.hpp"
#include "globals.h"


ResourceLoader::ResourceLoader()
{
	currentMode = GetCurrentMode();
}

BOOL ResourceLoader::unmapDrv() {

	StopAndRemoveByPath(storedAsrDrvName);
	StopAndRemoveByPath(storedIqvwName);
	StopAndRemoveByPath(storedAtzioName);
	return TRUE;
};

std::wstring ResourceLoader::extractResource(ResId resId) {

	DWORD fileSize = 0;
	UINT64 pData = 0;

	switch (resId) {
		case ResId::AsrDrvX32:
			fileSize = sizeof(resource_array::asrdrv_32);
			pData = (UINT64)resource_array::asrdrv_32;
			break;

		case ResId::AsrDrvX64:
			fileSize = sizeof(resource_array::asrdrv_64);
			pData = (UINT64)resource_array::asrdrv_64;
			break;

		case ResId::IqvwX32:
			fileSize = sizeof(resource_array::iqvw_32);
			pData = (UINT64)resource_array::iqvw_32;
			break;

		case ResId::IqvwX64:
			fileSize = sizeof(resource_array::iqvw_64);
			pData = (UINT64)resource_array::iqvw_64;
			break;

		case ResId::AtzioX64:
			fileSize = sizeof(resource_array::atzio_64);
			pData = (UINT64)resource_array::atzio_64;
			break;

		case ResId::AtzioX32:
			fileSize = sizeof(resource_array::atzio_32);
			pData = (UINT64)resource_array::atzio_32;
			break;

		case ResId::MsDiaX64:
			fileSize = sizeof(resource_array::msdia_64);
			pData = (UINT64)resource_array::msdia_64;
			break;

		case ResId::MsDiaX32:
			fileSize = sizeof(resource_array::msdia_32);
			pData = (UINT64)resource_array::msdia_32;
			break;

		default:
			debug::printf_d(debug::LogLevel::FATAL, "UNABLE TO EXTRACT RESOURCE");

		
	}
	
	std::wstring wstrPathToDriver = getResourcePath(resId);

	HANDLE targetFileHandle = CreateFileW(wstrPathToDriver.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	DWORD written_bytes = 0;
	WriteFile(targetFileHandle,
		(LPCVOID)pData,
		fileSize,
		&written_bytes,
		NULL);

	CloseHandle(targetFileHandle);

	return wstrPathToDriver;
}

ResourceLoader::CurrentMode ResourceLoader::GetCurrentMode() {

	BOOL isUnderWow = FALSE;
	if (!IsWow64Process(GetCurrentProcess(), &isUnderWow)) {
		isUnderWow = TRUE;
	}

	if (isUnderWow) {
		return CurrentMode::x86UnderX64;
	}
	if (sizeof(SIZE_T) == 8) {
		return CurrentMode::x64;
		
	}
	if (!isUnderWow
		&& sizeof(SIZE_T) == 4) {
		return CurrentMode::x86;
	};

	return CurrentMode::unkn;
}

BOOL ResourceLoader::CreateAndRunServiceByResIndex(ResId resId) {

	std::wstring pathToExtractedDriver = extractResource(resId);
	if (resId == ResId::AsrDrvX32
		|| resId == ResId::AsrDrvX64) {
		
		storedAsrDrvName = pathToExtractedDriver;
		return RegisterAndStartServiceByPath(storedAsrDrvName);
	}

	if (resId == ResId::IqvwX32
		|| resId == ResId::IqvwX64) {

		storedIqvwName = pathToExtractedDriver;
		return RegisterAndStartServiceByPath(storedIqvwName);
	}

	if (resId == ResId::AtzioX32
		|| resId == ResId::AtzioX64) {

		storedAtzioName = pathToExtractedDriver;
		return RegisterAndStartServiceByPath(storedAtzioName);
	}

	return FALSE;

}

BOOL ResourceLoader::RegisterAndStartServiceByPath(std::wstring pathToDriver) {

	const static DWORD ServiceTypeKernel = 1;
	const std::wstring driver_name = std::filesystem::path(pathToDriver).filename().wstring();
	const std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
	const std::wstring nPath = L"\\??\\" + pathToDriver;

	HKEY dservice;
	LSTATUS status = RegCreateKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &dservice); //Returns Ok if already exists
	if (status != ERROR_SUCCESS) {
		return FALSE;
	}

	status = RegSetKeyValueW(dservice, NULL, L"ImagePath", REG_EXPAND_SZ, nPath.c_str(), (DWORD)nPath.size() * sizeof(WCHAR));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		return FALSE;
	}

	status = RegSetKeyValueW(dservice, NULL, L"Type", REG_DWORD, &ServiceTypeKernel, sizeof(DWORD));
	if (status != ERROR_SUCCESS) {
		RegCloseKey(dservice);
		return FALSE;
	}

	RegCloseKey(dservice);

	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll == NULL) {
		return FALSE;
	}

	PVOID RtlAdjustPrivilege = (PVOID)GetProcAddress(ntdll, "RtlAdjustPrivilege");
	PVOID NtLoadDriver = (PVOID)GetProcAddress(ntdll, "NtLoadDriver");


	ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
	BOOLEAN SeLoadDriverWasEnabled;
	NTSTATUS Status = 0xFFFFFFFF;
	if (currentMode == CurrentMode::unkn) {
#ifdef PRINT
		printf("??");
#endif
		return FALSE;
	};

	switch (currentMode)
	{
	case CurrentMode::x64:
		Status = (_RtlAdjustPrivilegeX64(RtlAdjustPrivilege))(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &SeLoadDriverWasEnabled);
		break;

	case CurrentMode::x86UnderX64:
	case CurrentMode::x86:
		Status = (_RtlAdjustPrivilegeX32(RtlAdjustPrivilege))(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &SeLoadDriverWasEnabled);
		break;

	default:

#if defined(DEBUG) | defined(PRINT)
		printf("unknMode ResLoader err\n");
#endif
		throw std::logic_error("unknMode err");
		break;
	}

	if (!NT_SUCCESS(Status)) {
#ifdef PRINT
		printf("Fatal error: failed to acquire SE_LOAD_DRIVER_PRIVILEGE. Make sure you are running as administrator.\n");
#endif
		return FALSE;
	}

	std::wstring wdriver_name(driver_name.begin(), driver_name.end());
	wdriver_name = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + wdriver_name;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_name.c_str());

	switch (currentMode)
	{
	case CurrentMode::x64:
		Status = (_NtLoadDriverX64(NtLoadDriver))(&serviceStr);
		break;

	case CurrentMode::x86UnderX64:
	case CurrentMode::x86:
		Status = (_NtLoadDriverX32(NtLoadDriver))(&serviceStr); 
		break;

	}

#if defined(PRINT) | defined(DEBUG)
	printf("[+] NtLoadDriver Status 0x%lx\n", Status);
#endif

	if (Status == 0xc000010e)	// alredy loaded
		return TRUE;

	return NT_SUCCESS(Status);
}

BOOL ResourceLoader::StopAndRemoveByPath(std::wstring driver_path) {

	currentMode = GetCurrentMode();

	std::wstring driver_name = std::filesystem::path(driver_path).filename().wstring();

	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (ntdll == NULL)
	{
		// should not leave files
		_wremove(driver_path.c_str());
		return FALSE;
	}
	std::wstring wdriver_name(driver_name.begin(), driver_name.end());
	wdriver_name = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" + wdriver_name;
	UNICODE_STRING serviceStr;
	RtlInitUnicodeString(&serviceStr, wdriver_name.c_str());

	HKEY driver_service;
	std::wstring servicesPath = L"SYSTEM\\CurrentControlSet\\Services\\" + driver_name;
	LSTATUS status = RegOpenKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &driver_service);
	if (status != ERROR_SUCCESS)
	{
		if (status == ERROR_FILE_NOT_FOUND) {

			// should not leave files
			_wremove(driver_path.c_str());
			return TRUE;
		}
		// should not leave files
		_wremove(driver_path.c_str());
		return FALSE;
	}
	RegCloseKey(driver_service);

	PCHAR NtUnloadDriver = (PCHAR)GetProcAddress(ntdll, "NtUnloadDriver");
	NTSTATUS st = -1;

	switch (currentMode)
	{
	case CurrentMode::x64:
		st = (_NtUnloadDriverX64(NtUnloadDriver))(&serviceStr);
		break;


	case CurrentMode::x86UnderX64:
	case CurrentMode::x86:
		st = (_NtUnloadDriverX32(NtUnloadDriver))(&serviceStr);
		break;

	}

#ifdef PRINT
	printf("[+] NtUnloadDriver Status 0x%lx\n", st);
#endif
	if (st != 0x0) {
#ifdef PRINT
		printf("[-] Driver Unload Failed!!\n");
#endif
	}


	status = RegDeleteKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
	if (status != ERROR_SUCCESS) {

		// should not leave files
		_wremove(driver_path.c_str());
		return FALSE;
	}

	// should not leave files
	_wremove(driver_path.c_str());

	return TRUE;
}

std::wstring ResourceLoader::getResourcePath(ResId resId) {

	WCHAR pathToDriver[MAX_PATH] = { 0 };
	GetCurrentDirectoryW(MAX_PATH, pathToDriver);
	wcsncat_s(pathToDriver,
		L"\\",
		wcslen(L"\\")
	);
	wcsncat_s(
		pathToDriver,
		resourceNames[(unsigned int) resId],
		wcslen(resourceNames[(unsigned int)resId])
	);

	return std::wstring(pathToDriver);
}

BOOL ResourceLoader::mapDrv() {
	currentMode = GetCurrentMode();

	if (currentMode == CurrentMode::unkn) {
		throw std::logic_error("\n");
	}

	BOOL bRes = FALSE;

	if (currentMode == CurrentMode::x86UnderX64
		|| currentMode == CurrentMode::x64) 
	{
		bRes = (
				 CreateAndRunServiceByResIndex(ResId::AsrDrvX64)
			& CreateAndRunServiceByResIndex(ResId::IqvwX64)
			& CreateAndRunServiceByResIndex(ResId::AtzioX64)
			);
	}
	else
	{
		bRes = (
			CreateAndRunServiceByResIndex(ResId::AsrDrvX32) 
			& CreateAndRunServiceByResIndex(ResId::IqvwX32)
			& CreateAndRunServiceByResIndex(ResId::AtzioX32)
			);
	}

	return bRes;
}


BOOL ResourceLoader::extractAndInstallMsdiaDlls()
{
	std::wstring pathToExtractedDllX32, pathToExtractedDllX64;

	if (currentMode == CurrentMode::x86UnderX64
		|| currentMode == CurrentMode::x64)
	{
		pathToExtractedDllX64 = extractResource(ResId::MsDiaX64);
		pathToExtractedDllX32 = extractResource(ResId::MsDiaX32);
	}
	else if (currentMode == CurrentMode::x86)
	{
		pathToExtractedDllX32 = extractResource(ResId::MsDiaX32);
	}


	//printf("CoCreateInstance err %lx. [1] \ncall regsvr32 msdia140X.dll", hr);
	char cmdBuffer[0x100];

	char* buf = nullptr;
	size_t sz = 0;
	if (FAILED(_dupenv_s(&buf, &sz, "WINDIR")))
		debug::printf_d(debug::LogLevel::ERR, (char*)"_DiaHelpWrapper:initialize _dupenv_s err");

#ifdef _WIN64
	sprintf_s(cmdBuffer, "%s\\System32\\regsvr32.exe %S /s", buf, pathToExtractedDllX64.c_str());
	//printf("cmd: %s\n", cmdBuffer);
	system(cmdBuffer);

	sprintf_s(cmdBuffer, "%s\\System32\\regsvr32.exe %S /s", buf, pathToExtractedDllX32.c_str());
	//printf("cmd: %s\n", cmdBuffer);
	system(cmdBuffer);
#else
	sprintf_s(cmdBuffer, "%s\\SysWoW64\\regsvr32.exe %S /s", buf, pathToExtractedDllX32.c_str());
	//printf("cmd: %s\n", cmdBuffer);
	system(cmdBuffer);

#endif // _WIN32


	return TRUE;
}

BOOL ResourceLoader::removeMsdiaDlls()
{
	_wremove(getResourcePath(ResId::MsDiaX32).c_str());
	_wremove(getResourcePath(ResId::MsDiaX64).c_str());

	return TRUE;
}

