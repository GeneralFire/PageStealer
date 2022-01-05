#include "DiaHelpWrapper.h"

DiaHelpWrapper::DiaHelpWrapper() {
	IsInitialized = FALSE;
	gpDiaSource = NULL;
	gpDiaSession = NULL;
	gDiaRoot = NULL;

}

DiaHelpWrapper::~DiaHelpWrapper() {
	IsInitialized = FALSE;
	CoUninitialize();
	
}

void DiaHelpWrapper::initialize(char* pdfFile) {
	wchar_t * vOut = new wchar_t[strlen(pdfFile) + 1];
	mbstowcs_s(NULL, vOut, strlen(pdfFile) + 1, pdfFile, strlen(pdfFile));


	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
		;// fatalStatus((char*)"CoInitialize\n", hr);

	hr = CoCreateInstance(__uuidof(DiaSource),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(IDiaDataSource),
		(void**) &gpDiaSource);

	if (FAILED(hr)) {

		ResourceLoader rs;
		rs.extractAndInstallMsdiaDlls(); // register MsDiaDlls

		hr = CoCreateInstance(__uuidof(DiaSource),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(IDiaDataSource),
			(void**)&gpDiaSource);

		if (FAILED(hr))
			debug::printf_d(debug::LogLevel::FATAL, (char*)"CoCreateInstance failed[2]\n", hr);

		rs.removeMsdiaDlls();
	}

	hr = gpDiaSource->loadDataFromPdb(vOut);
	if (FAILED(hr))
		;// fatalStatus((char*)"gpDiaSource->loadDataFromPdb:\n", hr);

	hr = gpDiaSource->openSession(&gpDiaSession);
	if (FAILED(hr))
		;// fatalStatus((char*)"gpDiaSource->openSession:\n", hr);

	hr = gpDiaSession->get_globalScope(&gDiaRoot);
	if (FAILED(hr))
		;// fatalStatus((char*)"gpDiaSession->get_globalScope\n", hr);

	IsInitialized = TRUE;
	return;
}


void DiaHelpWrapper::printAllEnum(IDiaEnumSymbols* enu) {
	CComPtr<IDiaSymbol> sym;
	ULONG celt = 0;
	enu->Reset();
	BSTR name;

	while (SUCCEEDED(enu->Next(1, &sym, &celt)) &&
		celt == 1) {

		if (SUCCEEDED(sym->get_name(&name)))
			printf(" - %S\n", name);
		sym = 0;
	};

	return;
}

bool DiaHelpWrapper::getSymbolByName(IDiaEnumSymbols* enu, wchar_t* symbolName, IDiaSymbol** symbol) {
	CComPtr<IDiaSymbol> sym;
	ULONG celt = 0;
	enu->Reset();
	//HRESULT hr = enu->Next(1, &sym, &celt);
	BSTR name = {};

	while (SUCCEEDED(enu->Next(1, &sym, &celt)) &&
		celt == 1) {

		if (SUCCEEDED(sym->get_name(&name)))
		{

			if (!wcscmp(name, symbolName)) {
				*symbol = sym;
				SysFreeString(name);
				name = NULL;
				return true;
			}
			SysFreeString(name);

		}
		sym = 0;
	};

	return false;
}

void DiaHelpWrapper::getSymInfo(IDiaSymbol* symbol) {
	BSTR name;
	if (SUCCEEDED(symbol->get_name(&name)))
		printf("%S\n", name);
	else
		printf("get_name err\r\n");
	return;
}

bool DiaHelpWrapper::getSymbolChildren(IDiaSymbol* symbol, IDiaEnumSymbols ** enu) {
	if (SUCCEEDED(symbol->findChildren(SymTagNull,
		NULL,
		nsNone,
		enu)))
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool DiaHelpWrapper::getSymbolOffset(IDiaSymbol* symbol, LONG* symbolOffset) {

	if (SUCCEEDED(symbol->get_offset(symbolOffset)))
		return true;
	else return false;
}

bool DiaHelpWrapper::getSymbolType(IDiaSymbol* symbol, IDiaSymbol** symbolType) {
	if (SUCCEEDED(symbol->get_type(symbolType)))
		return true;
	else
		return false;
}

bool DiaHelpWrapper::getSymbolOffsetInKernelType(wchar_t* typeName, wchar_t* fieldName, LONG* symbolOffset) {
	
	CComPtr<IDiaEnumSymbols> enu;
	if (!gDiaRoot->findChildren(SymTagNull,
		NULL,
		nsNone,
		&enu)) 
	{
		enu->Reset();
		CComPtr<IDiaSymbol> sym;
		bool status = getSymbolByName(enu, typeName, &sym);
		if (status) {
			CComPtr<IDiaEnumSymbols> EnuInner;
			status = getSymbolChildren(sym, &EnuInner);
			if (status) {
				// printAllEnum(EnuInner);
				EnuInner->Reset();

				CComPtr<IDiaSymbol> SymInner;
				status = getSymbolByName(EnuInner, fieldName, &SymInner);

				if (status) {
					if (SUCCEEDED(SymInner->get_offset(symbolOffset)))
						return true;
					else
						printf("getSymbolOffsetInKernelType cant get_offset");
				}
				else
					printf("getSymbolOffsetInKernelType cant getSymbolByName");
			}
			else
				printf("getSymbolOffsetInKernelType cant get typeName children");
		}
		else 
			printf("getSymbolOffsetInKernelType cant enum typeName");
	}
	else
		printf("getSymbolOffsetInKernelType cant get root");
	return false;
}