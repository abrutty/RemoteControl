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

// Git test
// 唯一的应用程序对象

CWinApp theApp;
//using namespace std;

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
            CServerSocket* pServer = CServerSocket::getInstance();
            int count = 0;
            if (pServer->InitSocket() == false) {
                MessageBox(nullptr, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK|MB_ICONERROR);
                exit(0);
            }
            while (CServerSocket::getInstance()!=nullptr) {
                if (pServer->AcceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(nullptr, _T("多次无法接入用户"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(nullptr, _T("无法接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
                }
                TRACE("accept client true\r\n");
                int ret = pServer->DealCommand();
                TRACE("Sever DealCommand ret = %d\r\n", ret);
                if (ret > 0) {
                    ret = cmd.ExcuteCommand(ret);
                    if (ret != 0) {
                        TRACE("执行命令失败，%d, ret=%d\r\n", pServer->GetPacket().sCmd, ret);
                    }
                    pServer->CloseClient(); // 采用短连接方式
                    TRACE("clinet closed\r\n");
                }
                
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
