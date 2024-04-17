// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"

#include <direct.h>
#include <io.h>
#include <list>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Git test
// 唯一的应用程序对象

CWinApp theApp;

//using namespace std;

typedef struct file_info {
    file_info(){
        isInvalid = false;
        isDirectory = false;
        hasNext = true;
        memset(szFileName, 0, sizeof(szFileName));
    }
    bool isInvalid;         // 是否有效
    bool isDirectory;       // 是否为目录
    bool hasNext;           // 是否还有后续
    char szFileName[256];   // 文件名
}FILEINFO, *PFILEINFO;
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

int MakeDirectoryInfo() {
    std::string filePath;
    //std::list<FILEINFO> listFileInfo;
    if (CServerSocket::getInstance()->GetFilePath(filePath)==false) {
        OutputDebugString(_T("不是获取文件列表的命令"));
        return -1;
    }
    if (_chdir(filePath.c_str()) != 0) {  // 切换到输入路径判断该路径是否存在
        FILEINFO finfo;
        finfo.isInvalid = true;
        finfo.isDirectory = true;
        finfo.hasNext = false;
        memcpy(finfo.szFileName, filePath.c_str(), filePath.length());
        CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
       // listFileInfo.push_back(finfo);
        OutputDebugString(_T("没有权限访问目录"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {// 获取当前文件节
		OutputDebugString(_T("没有找到任何文件"));
		return -3;
    }
    do {
        FILEINFO finfo;
        finfo.isDirectory = (fdata.attrib & _A_SUBDIR) != 0;
        memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
        //listFileInfo.push_back(finfo);

    } while (!_findnext(hfind,&fdata)); // 获取下一个文件节点
    // 通过以上两个函数配合，从盘符开始，通过do...while() 循环可得到遍历文件
    // 发送信息到控制端
    FILEINFO finfo;
    finfo.hasNext = false;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int RunFile() {
    std::string filePath;
    CServerSocket::getInstance()->GetFilePath(filePath);
    ShellExecuteA(nullptr, nullptr, filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	CPacket pack(3, nullptr, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int DownloadFile() {
	std::string filePath;
	CServerSocket::getInstance()->GetFilePath(filePath);
    long long data = 0;
    FILE* pFile = nullptr;
    errno_t err = fopen_s(&pFile, filePath.c_str(), "rb"); // 用fopen会报warning，或者使用fopen，禁用warning
    if (err != 0) {
		CPacket pack(4, (BYTE*)data, 8);
		CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != nullptr) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)&data, 8);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
        } while (rlen >= 1024); // <1024说明读到文件尾        
        fclose(pFile);
    }
	CPacket pack(4, nullptr, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}

int MouseEvent() {
    MOUSEEV mouse;
    if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
        DWORD nFlags = 0;
        switch (mouse.nButton){
        case 0: //左键
            nFlags = 1;
            break;
        case 1: // 右键
            nFlags = 2;
            break;
        case 2: // 中键
            nFlags = 4;
            break;
        case 3: // 没有按键
            nFlags = 8;
            break;
        default:
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction){
        case 0: // 单击
            nFlags |= 0x10;
            break;
        case 1: // 双击
            nFlags |= 0x20;
            break;
        case 2: // 按下
            nFlags |= 0x40;
            break;
        case 3: // 放开
            nFlags |= 0x80;
            break;
        default:
            break;
        }
        switch (nFlags){
		case 0x21:  // 左键双击
            mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
        case 0x11:  // 左键单击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
		case 0x41:  // 左键按下
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x81:  // 左键放开
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		
		case 0x22:  // 右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x12:  // 右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x42:  // 右键按下
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x82:  // 右键放开
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			break;

		case 0x24:  // 中键双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x14:  // 中键单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x44:  // 中键按下
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
			break;
		case 0x84:  // 中键放开
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			break;
        case 0x08:  // 单纯鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        default:
            break;
        }
    }
    else {
        OutputDebugString(_T("获取鼠标参数操作失败"));
        return -1;
    }
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
            case 1: 
                MakeDriverInfo(); // 查看磁盘分区
                break;
            case 2: 
                MakeDirectoryInfo(); // 查看指定目录下的文件
                break;
            case 3:
                RunFile(); // 打开文件
                break;
            case 4:
                DownloadFile(); // 下载文件
                break;
            case 5:
                MouseEvent();
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
