#pragma once

#include <windows.h>
#include <dia2.h>
#include <dbghelp.h>
#include <stdio.h>
#pragma comment(lib, "dbghelp.lib")

class DbgHelpWrapper {
private:
	BOOL    IsInitialized = FALSE;
	HANDLE  hProcess = 0;
	HANDLE	ghProc = 0;
	DWORD64 ModuleBase = 0;

	LPCSTR DefaultSymbolsPath = "srv*C:\\Symbols*http://msdl.microsoft.com/download/symbols";
	CHAR ModuleName[0x100] = "";
	CHAR Extension[0x10] = "";
public:
	DbgHelpWrapper();
	~DbgHelpWrapper();

	// Инициализация/деинициализация DbgHelp, вызывать перед использованием:
	BOOL InitializeDbgHelp(LPSTR SymbolsPath = NULL);
	BOOL DeinitializeDbgHelp();

	// Загрузить/скачать символы для указанного модуля (*.exe/*.dll/*.sys):
	BOOL LoadSymbols(LPSTR ModulePath);

	// Получение символов:
	BOOL GetRootSymbol(LPSTR SymbolName, PULONG SymbolIndex);
	BOOL GetChildrenCount(ULONG SymbolIndex, OUT PULONG ChildrenCount);
	//BOOL GetChildrenSymbols(ULONG ParentSymbolIndex, ULONG* IndicesBuffer, ULONG MaxIndices, OUT ULONG &ChildrenCount);
	
	// Получение индекса нужного символа:
	ULONG GetSymbolIndex(LPWSTR SymbolName, ULONG* IndicesBuffer, ULONG IndicesCount);
	//GetKernelSymbolAddressULONG GetSymbolIndex(ULONG ParentSymbolIndex, LPWSTR SymbolName);

	// Получение информации о символе:
	BOOL GetSymbolName(ULONG SymbolIndex, OUT LPWSTR* SymbolName);
	VOID FreeSymbolName(LPWSTR SymbolName);
	BOOL GetSymbolOffset(ULONG SymbolIndex, OUT PULONG Offset);
	BOOL GetSymbolInfoFromName(LPSTR symbolName, SYMBOL_INFO* symbolInfo);

	// Вывод информаии о символе:
	void PrintSymbolInfo(SYMBOL_INFO* symbol);
	const char* GetStrTag(DWORD tag);
	ULONG64 GetKernelSymbolAddress(LPSTR symbolName);


};