#include "DbgHelpWrapper.h"

SYMBOL_INFO gSymbolInfo = { 0 };

DbgHelpWrapper::DbgHelpWrapper() {
	hProcess = (HANDLE) GetCurrentProcess();
}

DbgHelpWrapper::~DbgHelpWrapper() {
	DeinitializeDbgHelp();
}

BOOL DbgHelpWrapper::InitializeDbgHelp(LPSTR SymbolsPath) {
	if (SymbolsPath == NULL) 
		SymbolsPath = (LPSTR)DefaultSymbolsPath;

	SymSetOptions(SYMOPT_UNDNAME);	// possible WRONG
	if (IsInitialized) 
		DeinitializeDbgHelp();

	HeapSetInformation(0, (HEAP_INFORMATION_CLASS)1, 0, 0);
	IsInitialized = SymInitialize(hProcess, 0, TRUE);

	return IsInitialized;
}

BOOL DbgHelpWrapper::DeinitializeDbgHelp() {
	if (IsInitialized) {
		if (SymCleanup(hProcess)) 
			IsInitialized = FALSE;
	}
	return IsInitialized;
}

BOOL DbgHelpWrapper::LoadSymbols(LPSTR ModulePath) {
	_splitpath_s(ModulePath, 0, 0, 0, 0, ModuleName, 0x100, Extension, 0x10);

	ModuleBase = SymLoadModuleEx(hProcess, 
		INVALID_HANDLE_VALUE,	// hFile
		ModulePath,				// ImageName
		ModuleName,				// ModuleName
		0x1000000,				// BaseOfDll
		0x1000000,				// DllSize
		NULL,					// Data
		0);		// Flags

	return ModuleBase != 0;
}

BOOL DbgHelpWrapper::GetRootSymbol(LPSTR SymbolName, PULONG SymbolIndex) {
	SYMBOL_INFO SymbolInfo;
	SymbolInfo.SizeOfStruct = sizeof(SymbolInfo);
	BOOL Status = SymGetTypeFromName(hProcess, ModuleBase, SymbolName, &SymbolInfo);
	if (Status) 
		*SymbolIndex = SymbolInfo.Index;
	return Status;
}

BOOL DbgHelpWrapper::GetChildrenCount(ULONG SymbolIndex, OUT PULONG ChildrenCount) {
	return SymGetTypeInfo(hProcess, ModuleBase, SymbolIndex, TI_GET_CHILDRENCOUNT, ChildrenCount);
}

ULONG DbgHelpWrapper::GetSymbolIndex(LPWSTR SymbolName, ULONG* IndicesBuffer, ULONG IndicesCount) {
	for (ULONG i = 0; i < IndicesCount; i++) {
		LPWSTR CurrentSymbolName = NULL;
		if (GetSymbolName(IndicesBuffer[i], &CurrentSymbolName)) {
			if (wcscmp(CurrentSymbolName, SymbolName) == 0) {
				FreeSymbolName(SymbolName);
				return IndicesBuffer[i];
			}

			FreeSymbolName(SymbolName);
		}
	}

	return 0;
}


BOOL DbgHelpWrapper::GetSymbolName(ULONG SymbolIndex, OUT LPWSTR* SymbolName) { 
	return SymGetTypeInfo(hProcess, ModuleBase, SymbolIndex, TI_GET_SYMNAME, SymbolName);
}

VOID DbgHelpWrapper::FreeSymbolName(LPWSTR SymbolName) {
	VirtualFree(SymbolName, 0, MEM_RELEASE);
}

BOOL DbgHelpWrapper::GetSymbolOffset(ULONG SymbolIndex, OUT PULONG Offset) {
	return SymGetTypeInfo(hProcess, ModuleBase, SymbolIndex, TI_GET_OFFSET, Offset);
}

BOOL DbgHelpWrapper::GetSymbolInfoFromName(LPSTR symbolName, SYMBOL_INFO* symbolInfo) {

	symbolInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbolInfo->MaxNameLen = MAX_SYM_NAME;
	return SymFromName(hProcess, symbolName, symbolInfo);
}

void DbgHelpWrapper::PrintSymbolInfo(SYMBOL_INFO* symbol) {

	
	printf("   name : %s\n", symbol->Name);
	printf("   addr : %llx\n", symbol->Address);
	printf("   size : %x\n", symbol->Size);
	printf("  flags : %x\n", symbol->Flags);
	printf("   type : %x\n", symbol->TypeIndex);

	printf("modbase : %x\n", (unsigned int) symbol->ModBase);
	printf("  value : %llx\n", symbol->Value);
	printf("    reg : %x\n", symbol->Register);
	printf("  scope : %s (%x)\n", GetStrTag(symbol->Scope), symbol->Scope);
	printf("    tag : %s (%x)\n", GetStrTag(symbol->Tag), symbol->Tag);
	printf("  index : %x\n", symbol->Index);
	
}

const char* DbgHelpWrapper::GetStrTag(DWORD tag) {
	switch (tag) {
	case SymTagNull:                    return "SymTagNull";
	case SymTagExe:                     return "SymTagExe";
	case SymTagCompiland:               return "SymTagCompiland";
	case SymTagCompilandDetails:        return "SymTagCompilandDetails";
	case SymTagCompilandEnv:            return "SymTagCompilandEnv";
	case SymTagFunction:                return "SymTagFunction";
	case SymTagBlock:                   return "SymTagBlock";
	case SymTagData:                    return "SymTagData";
	case SymTagAnnotation:              return "SymTagAnnotation";
	case SymTagLabel:                   return "SymTagLabel";
	case SymTagPublicSymbol:            return "SymTagPublicSymbol";
	case SymTagUDT:                     return "SymTagUDT";
	case SymTagEnum:                    return "SymTagEnum";
	case SymTagFunctionType:            return "SymTagFunctionType";
	case SymTagPointerType:             return "SymTagPointerType";
	case SymTagArrayType:               return "SymTagArrayType";
	case SymTagBaseType:                return "SymTagBaseType";
	case SymTagTypedef:                 return "SymTagTypedef,";
	case SymTagBaseClass:               return "SymTagBaseClass";
	case SymTagFriend:                  return "SymTagFriend";
	case SymTagFunctionArgType:         return "SymTagFunctionArgType,";
	case SymTagFuncDebugStart:          return "SymTagFuncDebugStart,";
	case SymTagFuncDebugEnd:            return "SymTagFuncDebugEnd";
	case SymTagUsingNamespace:          return "SymTagUsingNamespace,";
	case SymTagVTableShape:             return "SymTagVTableShape";
	case SymTagVTable:                  return "SymTagVTable";
	case SymTagCustom:                  return "SymTagCustom";
	case SymTagThunk:                   return "SymTagThunk";
	case SymTagCustomType:              return "SymTagCustomType";
	case SymTagManagedType:             return "SymTagManagedType";
	case SymTagDimension:               return "SymTagDimension";
	default:                            return "---";
	}
}

ULONG64 DbgHelpWrapper::GetKernelSymbolAddress(LPSTR symbolName) {
	SYMBOL_INFO* symbolInfo = &gSymbolInfo;
	if (GetSymbolInfoFromName(symbolName, symbolInfo)) {
		return (symbolInfo->Address - symbolInfo->ModBase);
	}
	else 
		return -1;
}

