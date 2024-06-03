// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Command.h"
#include "CQueue.h"
#include <MSWSock.h>
#include "MyServer.h"
#include <conio.h>
#include "MySocket.h"
#include "MyNetWork.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define INVOKE_PATH _T("C:\\Users\\zyx\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemeteCtrl.exe")
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


bool ChooseAutoInvoke(const CString& strPath) {
    //CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")); // 此处的SysWOW64目录要注意，可能是system32目录
    if (PathFileExists(strPath)) return true;
    
    CString strInfo = _T("该程序用于合法途径\n");
    strInfo += _T("继续运行该程序，将处于被监控状态\n");
    strInfo += _T("按取消退出程序\n");
    strInfo += _T("按是，该程序将被复制到机器上并开机自启动\n");
    strInfo += _T("按否，程序只运行一次就退出\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
        //WriteRegisterTable(strPath); 
        
		if (CTool::WriteStartupDir(strPath) == false) {
			MessageBox(NULL, _T("复制文件失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
		}
    }
    else {
        if (ret == IDCANCEL) {
            return false;
        }
    }
    return true;
}

void iocp() {
	MyServer server;
	server.StartService();
	getchar();
}


void initsock()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
}
void clearsock() {
	WSACleanup();
}
int RecvFromCB(void* arg, const MyBuffer& buffer, MySockAddrIn& addr) {
	ESever* server = (ESever*)arg;
	return server->Sendto(addr, buffer);
}
int SendToCB(void* arg,const MySockAddrIn& addr, int ret) {
	ESever* server = (ESever*)arg;
	printf("sendto done\r\n");
	return 0;
}
void udp_server() {
	std::list<MySockAddrIn> lstclients;
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	EServerParameter param("127.0.0.1", 20000, MYTYPE::MyTypeUDP, NULL, NULL, NULL, RecvFromCB, SendToCB);
	ESever server(param);
	server.Invoke(&server);
	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
	getchar();
	return;
	//SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	
}
void udp_client(bool ishost = true) {
	Sleep(2000);
	sockaddr_in server, client;
	int len = sizeof(client);
	server.sin_family = AF_INET;
	server.sin_port = htons(20000);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("%s(%d):%s error\r\n", __FILE__, __LINE__, __FUNCTION__);
		return;
	}
	if (ishost) { // 主客户端
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		MyBuffer msg = "hello world\n";
		int ret = sendto(sock, msg.c_str(), (int)msg.size(), 0, (sockaddr*)&server, sizeof(server));
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), (int)msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d):%s 主客户端收到 %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				printf("%s(%d):%s 主客户端 ip=%08X port=%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
			ret = recvfrom(sock, (char*)msg.c_str(), (int)msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d):%s 主客户端第二次收到 %d， %s\r\n", __FILE__, __LINE__, __FUNCTION__, ret, msg.c_str());
			if (ret > 0) {
				//printf("%s(%d):%s 主客户端 ip=%08X port=%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s msg=%s\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str());
			}
		}
	}
	else { // 从客户端
		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		std::string msg = "hello world\n";
		int ret = sendto(sock, msg.c_str(), (int)msg.size(), 0, (sockaddr*)&server, sizeof(server));
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock, (char*)msg.c_str(), (int)msg.size(), 0, (sockaddr*)&client, &len);
			printf("%s(%d):%s 从客户端收到 %d\r\n", __FILE__, __LINE__, __FUNCTION__, ret);
			if (ret > 0) {
				sockaddr_in addr;
				memcpy(&addr, msg.c_str(), sizeof(sockaddr_in));
				sockaddr_in* paddr = (sockaddr_in*)&addr;
				printf("%s(%d):%s 从客户端 ip=%08X port=%d\r\n", __FILE__, __LINE__, __FUNCTION__, client.sin_addr.s_addr, ntohs(client.sin_port));
				printf("%s(%d):%s 从客户端拿到的：ip=%08X port=%d\r\n", __FILE__, __LINE__, __FUNCTION__, paddr->sin_addr.s_addr, ntohs(paddr->sin_port));
				msg = "hello,i am client\n";
				ret = sendto(sock, (char*)msg.c_str(), sizeof(msg), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
				printf("%s(%d):%s 从客户端发送：%s len=%d\r\n", __FILE__, __LINE__, __FUNCTION__, msg.c_str(),ret);
			}
		}
	}
	closesocket(sock);
}
int main(int argc, char* argv[])
{
	//if (!CTool::Init()) return 1;
	/*initsock();
	if (argc == 1) {
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		std::string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		DWORD err = GetLastError();
		if (bRet) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID=%d\r\n", pi.dwProcessId);
			TRACE("线程ID=%d\r\n", pi.dwThreadId);
			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID=%d\r\n", pi.dwProcessId);
				TRACE("线程ID=%d\r\n", pi.dwThreadId);
				udp_server();
			}	
		}
	}
	else if (argc == 2) { 
		udp_client();
	}
	else {
		udp_client(false);
	}
	clearsock();
	return 0;*/
	//iocp();
	//exit(0);  exit(0)类似于_endthread，直接终止了，不会触发析构，导致内存泄漏
	
  /*  if (CTool::IsAdmin()) {
        if (!CTool::Init()) return 1;
        OutputDebugString("administrator\r\n");
        MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);*/
       // if (ChooseAutoInvoke(INVOKE_PATH) == true) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret) {
			case -1:
				MessageBox(nullptr, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(nullptr, _T("多次无法接入用户"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
				break;
			}
        //}    
    //}
    //else {
    //    OutputDebugString("normal user\r\n");
    //    MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
    //    /*if (CTool::RunAsAdmin() == false) {
    //        CTool::ShowError();
    //    }*/
    //    return 0;
    //}
    return 0;
}
