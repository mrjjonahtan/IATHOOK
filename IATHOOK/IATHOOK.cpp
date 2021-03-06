// IATHOOK.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


DWORD g_dwIATHookFlag = 0;	//HOOK 状态 1 HOOK 0 未HOOK
DWORD g_dwOldAddr;			//原始函数地址
DWORD g_dwNewAddr;			//HOOK函数地址

DWORD pOldFuncAddr = (DWORD)GetProcAddress(LoadLibrary(L"USER32.dll"), "MessageBoxW");

int WINAPI MyMessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
BOOL SetIATHook(DWORD dwOldAddr, DWORD dwNewAddr);
BOOL UnIATHook();


int main()
{
	//安装Hook
	SetIATHook(pOldFuncAddr,(DWORD)MyMessageBox);
	
	MessageBox(NULL, L"测试IAT HOOK", L"IAT HOOK", MB_OK);

	//卸载Hook
	UnIATHook();

	getchar();
	return 0;
}

int WINAPI MyMessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	//定义函数指针
	typedef int (WINAPI *PFNMESSAGEBOX)(HWND, LPCSTR, LPCSTR, UINT);

	//获取参数
	wprintf(L"%x %s %s %x\n", (DWORD)hWnd, lpText, lpCaption, uType);

	//执行真正的函数
	int ret = ((PFNMESSAGEBOX)pOldFuncAddr)(hWnd, lpText, lpCaption, uType);

	//获取返回值
	printf("return:%x\n", ret);

	return ret;
}

BOOL SetIATHook(DWORD dwOldAddr, DWORD dwNewAddr)
{
	BOOL dFlag = FALSE;
	DWORD dwImageBase = 0;
	PDWORD pFuncAddr = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = NULL;

	//get imageBase
	dwImageBase = (DWORD)::GetModuleHandle(NULL);

	pNtHeader = (PIMAGE_NT_HEADERS)(dwImageBase + ((PIMAGE_DOS_HEADER)dwImageBase)->e_lfanew);
	pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(dwImageBase + pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress);

	//遍历IAT表 找到函数地址
	while (pImportDescriptor->FirstThunk != 0 && dFlag == FALSE)
	{
		pFuncAddr = (PDWORD)(dwImageBase + pImportDescriptor->FirstThunk);
		while (*pFuncAddr)
		{
			if (dwOldAddr == *pFuncAddr)
			{
				//修改内存页的保护属性  
				DWORD dwOldProtect;
				MEMORY_BASIC_INFORMATION mbi;
				::VirtualQuery(pFuncAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
				::VirtualProtect(pFuncAddr, sizeof(DWORD), PAGE_READWRITE, &dwOldProtect);

				*pFuncAddr = dwNewAddr;

				::VirtualProtect(pFuncAddr, sizeof(DWORD), dwOldProtect, NULL);

				dFlag = TRUE;
				break;
			}
			pFuncAddr++;
		}
		pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pImportDescriptor + sizeof(_IMAGE_IMPORT_DESCRIPTOR));
	}
	
	//修改状态
	g_dwOldAddr = dwOldAddr;
	g_dwNewAddr = g_dwNewAddr;
	g_dwIATHookFlag = 1;

	return dFlag;
}

BOOL UnIATHook()
{
	BOOL dFlag = FALSE;
	DWORD dwImageBase = 0;
	PDWORD pFuncAddr = NULL;
	PIMAGE_NT_HEADERS pNtHeader = NULL;
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor = NULL;

	//判断是否Hook
	if (!g_dwIATHookFlag) {
		OutputDebugString(L"UnIATHook失败：尚未进行IAT HOOK!");
		return dFlag;
	}

	//get imageBase
	dwImageBase = (DWORD)GetModuleHandle(NULL);
	pNtHeader = (PIMAGE_NT_HEADERS)(dwImageBase + ((PIMAGE_DOS_HEADER)dwImageBase)->e_lfanew);
	pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)(dwImageBase + pNtHeader->OptionalHeader.DataDirectory[1].VirtualAddress);

	//遍历IAT表 找到函数地址
	while (pImportDescriptor->FirstThunk != 0 && dFlag == false)
	{
		pFuncAddr = (PDWORD)(dwImageBase + pImportDescriptor->FirstThunk);
		while (*pFuncAddr)
		{
			if (g_dwNewAddr == *pFuncAddr)
			{
				//修改内存页的保护属性 
				DWORD dwOldProtect;
				MEMORY_BASIC_INFORMATION mbi;
				::VirtualQuery(pFuncAddr, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
				::VirtualProtect(pFuncAddr, sizeof(DWORD), PAGE_READWRITE, &dwOldProtect);

				//如果找到要HOOK的函数
				*pFuncAddr = g_dwOldAddr;
				::VirtualProtect(pFuncAddr, sizeof(DWORD), dwOldProtect, NULL);

				dFlag = TRUE;
				break;
			}
			pFuncAddr++;
		}
		pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)pImportDescriptor + sizeof(PIMAGE_IMPORT_DESCRIPTOR));
	}

	//修改状态
	g_dwOldAddr = 0;
	g_dwNewAddr = 0;
	g_dwIATHookFlag = 0;

	return dFlag;
}
