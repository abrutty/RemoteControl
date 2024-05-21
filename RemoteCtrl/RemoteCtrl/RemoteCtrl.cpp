// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 唯一的应用程序对象

CWinApp theApp;
//using namespace std;
//开机启动时，程序的权限是跟随启动用户的，如果两者权限不一致，程序会启动失败
//开机启动对环境变量有影响，如果依赖dll，可能启动失败

// 解决方法：
// 复制这些dll到system32和sysWOW64下面【system32下面大部分是64位程序，而sysWOW64下大部分是32位程序
// 这时可以使用静态库解决

// 通过修改注册表的方式实现开机自启动
void WriteRegisterTable(const CString& strPath) { 
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	char sPath[MAX_PATH] = "";
	char sSys[MAX_PATH] = "";
	std::string strExe = "\\RemoteCtrl.exe ";
	GetCurrentDirectoryA(MAX_PATH, sPath);
	GetSystemDirectoryA(sSys, sizeof(sSys));
	std::string strCmd = "mklink " + std::string(sSys) + strExe + std::string(sPath) + strExe;
	system(strCmd.c_str());
	HKEY hKey = NULL;
	int ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置开机自启动失败\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}

	ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		MessageBox(NULL, _T("设置开机自启动失败\n"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
		exit(0);
	}
	RegCloseKey(hKey);
}

// 通过写入startup文件夹的方式实现开机自启动
void WriteStartupDir(const CString& strPath) {
    CString strCmd = GetCommandLine();
    strCmd.Replace(_T("\""), _T(""));
    BOOL ret = CopyFile(strCmd, strPath, FALSE);
    if (ret == FALSE) {
        MessageBox(NULL, _T("复制文件失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
}
void ChooseAutoInvoke() {
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")); // 此处的SysWOW64目录要注意，可能是system32目录
    CString strPath = CString(_T("C:\\Users\\zyx\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemeteCtrl.exe")); 
    if (PathFileExists(strPath)) return;
    
    CString strInfo = _T("该程序用于合法途径\n");
    strInfo += _T("继续运行该程序，将处于被监控状态\n");
    strInfo += _T("按取消退出程序\n");
    strInfo += _T("按是，该程序将被复制到机器上并开机自启动\n");
    strInfo += _T("按否，程序只运行一次就退出\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath); 
        WriteStartupDir(strPath);
    }
    else {
        if (ret == IDCANCEL) {
            exit(0);
        }
    }
    return;
}
int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            CCommand cmd;
            ChooseAutoInvoke();
            int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret) {
            case -1:
				MessageBox(nullptr, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
            case -2:
				MessageBox(nullptr, _T("多次无法接入用户"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
				exit(0);
                break;
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
