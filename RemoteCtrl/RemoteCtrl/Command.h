#pragma once
#include <map>
#include "atlimage.h"
#include "ServerSocket.h"
#include "LockDialog.h"
#include "resource.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "Tool.h"
#include "Packet.h"
class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
			if (ret != 0) {
				TRACE("执行命令失败，%d, ret=%d\r\n", status, ret);
			}
		}
		else {
			MessageBox(nullptr, _T("无法接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>& lstPacket, CPacket& inPacket); // 成员函数指针
	std::map<int, CMDFUNC> m_mapFunction; // 从命令号到功能的映射
	CLockDialog dlg;
	unsigned thread_id;
protected:
	// 查看本地磁盘分区，并把信息打包成数据包
	int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 获取目录信息并打包
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 打开文件
	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 下载文件
	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 鼠标操作
	int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 发送屏幕截图
	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket);


	// 锁机的子线程，放在主线程中就一直处于消息循环，收不到解锁的消息
	static unsigned __stdcall ThreadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->ThreadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	// 真正实现锁机的函数
	void ThreadLockDlgMain();
	// 锁机命令打包
	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket);
	// 解锁命令打包
	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		PostThreadMessage(thread_id, WM_KEYDOWN, 0x1B, 0); // 消息机制根据线程，而不是句柄
		lstPacket.emplace_back(CPacket(8, nullptr, 0));
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
		return 0;
	}
	// 连接测试
	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket) {
		lstPacket.emplace_back(CPacket(1981, nullptr, 0));
		return 0;
	}
	// 删除文件
	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket);
};

