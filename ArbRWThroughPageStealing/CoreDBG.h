#pragma once

#include "DbgHelpWrapper.h"
#include "DiaHelpWrapper.h"
#include "PE.h"
#define DEBUG_CHECK 

class CoreDBG {
private:
	DiaHelpWrapper DiaPdb;
	DbgHelpWrapper DbgPdb;
	PE pe;
	char* getPdb();
	CoreDBG();
	~CoreDBG();

public:
	static inline CoreDBG& GetInstance() {
		static CoreDBG instance;
		return instance;
	}

	ULONG64 getFieldOffset(char* typeName, char* fieldName);
	ULONG64 getKernelSymbolAddress(char* symbolName);

};