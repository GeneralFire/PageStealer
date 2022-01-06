#pragma once

#include "DbgHelpWrapper.h"
#include "DiaHelpWrapper.h"
#include "PE.h"
#include <map>

#define DEBUG_CHECK 

class CoreDBG {
private:
	DiaHelpWrapper DiaPdb;
	DbgHelpWrapper DbgPdb;
	PE pe;
	char* getPdb();

	std::map<wchar_t*, std::map<wchar_t*, UINT64>> OffsetsDict;

	CoreDBG();
	~CoreDBG();

public:
	static inline CoreDBG& GetInstance() {
		static CoreDBG instance;
		return instance;
	}
	
	ULONG64 getFieldOffset(wchar_t* typeName_, wchar_t* fieldName_);
	ULONG64 getKernelSymbolAddress(char* symbolName);

};