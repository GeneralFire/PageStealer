#pragma once

#include <dia2.h>
#include <assert.h>
#include <stdio.h>
#include "ResourceLoader.h"
#include "debug.hpp"

class DiaHelpWrapper {
private:
	IDiaDataSource* gpDiaSource;
	IDiaSession* gpDiaSession;
	IDiaSymbol* gDiaRoot;

public:
	BOOL    IsInitialized = FALSE;
	DiaHelpWrapper();
	~DiaHelpWrapper();

	void initialize(char* pdfFile);

	// root // sym children
	bool getAllRootChildren(IDiaEnumSymbols ** enu);
	bool getSymbolChildren(IDiaSymbol* symbol, IDiaEnumSymbols ** enu);


	bool getSymbolByName(IDiaEnumSymbols * enu, wchar_t* symbolName, IDiaSymbol** symbol);

	// symbol info
	void getSymInfo(IDiaSymbol* symbol);
	void printAllEnum(IDiaEnumSymbols * enu);

	bool getSymbolOffset(IDiaSymbol* symbol, LONG* symbolOffset);
	bool getSymbolType(IDiaSymbol* symbol, IDiaSymbol** symbolType);

	bool getSymbolOffsetInKernelType(wchar_t* typeName, wchar_t* fieldName, LONG* symbolOffset);


};
