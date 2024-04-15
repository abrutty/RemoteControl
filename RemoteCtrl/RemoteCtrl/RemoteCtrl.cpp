// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#include <direct.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Git test
// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void Dump(BYTE* pData, size_t nSize) {
    std::string strOut;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strOut += "\n";
        snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // 02X输出域宽为2，右对齐，不足的用字符0替代
        strOut += buf;
    }
    strOut += "\n";
    OutputDebugStringA(strOut.c_str());
}
// 查看本地磁盘分区，并把信息打包成数据包
int MakeDriverInfo() {
    std::string result; // A盘->1 B盘->2 C盘->3
    for (int i = 1; i <= 26; i++) {  // 遍历磁盘符
        if (_chdrive(i) == 0) {// =0 表示成功
            if (result.size() > 0) result += ",";
            result += 'A' + i - 1;
        }
    }
    CPacket pack(1, (BYTE*)result.c_str(), result.size());
    Dump((BYTE*)pack.Data(), pack.Size());
    // CServerSocket::getInstance()->Send(pack);
    return 0;
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
            int nCmd = 1;
            switch (nCmd)
            {
            case 1: MakeDriverInfo();
                break;
            default:
                break;
            }
            MakeDriverInfo();
            // TODO: 在此处为应用程序的行为编写代码。
			//CServerSocket* pServer = CServerSocket::getInstance();
            /*int count = 0;
			if (pServer->InitSocket() == FALSE) {
				MessageBox(NULL, _T(""), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
				exit(0);
			}
			while ((pServer = CServerSocket::getInstance()) != NULL) {
				if (pServer->AcceptClient() == FALSE) {
					if (count >= 3) {
						MessageBox(NULL, _T("超过三次无法接入用户，退出"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
						exit(0);
					}
					MessageBox(NULL, _T(""), _T("接入用户失败"), MB_OK | MB_ICONERROR);
					count++;
				}
				int ret = pServer->DealCommand();
			}  */
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
