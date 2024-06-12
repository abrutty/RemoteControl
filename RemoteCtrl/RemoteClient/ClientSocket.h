#pragma once

#include "pch.h"
#include "framework.h"

#include <string>
#include <vector>
#include <list>
#include <map>

#define BUFFER_SIZE 4096000
#define WM_SEND_PACK (WM_USER+1) // 发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2) // 发送包数据应答

#pragma pack(push)  // 保存当前字节对齐的状态
#pragma pack(1)	// 强制取消字节对齐，改为连续存放
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize); // 打包构造
	CPacket(const CPacket& pack); // 复制构造
	CPacket& operator=(const CPacket& pack);
	// 解包构造函数
	CPacket(const BYTE* pData, size_t& nSize);
	~CPacket() {}
	int Size() { return nLength + 6; } // 数据包的大小
	const char* Data(std::string& strOut) const; // 将数据包转成字符串形式

public:
	WORD sHead;				// 包头，固定位，0xFEFF
	DWORD nLength;			// 包长度，从控制命令开始，到和校验结束
	WORD sCmd;				// 控制命令
	std::string strData;	// 包数据
	WORD sSUm;				// 和校验
};
#pragma pack(pop)	// 还原字节对齐

// 鼠标消息结构体
typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	// 点击、移动、双击
	WORD nButton;	// 左键、右键、中键
	POINT ptXY;		// 坐标
}MOUSEEV, * PMOUSEEV;

// 文件信息结构体
typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // 是否有效
	int isDirectory;        // 是否为目录
	bool hasNext;           // 是否还有后续
	char szFileName[256];   // 文件名
}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1 // CSM=client socket mode 自动关闭模式
};


typedef struct PacketData {
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data) {
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data) {
		if (this != &data) {
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}
}PACKET_DATA;

std::string GetErrInfo(int WSAErrCode);
class CClientSocket
{
public:
	static CClientSocket* getInstance() { // 构造和析构是私有的，通过静态函数方式访问
		if (mInstance == nullptr) {
			mInstance = new CClientSocket;
		}
		return mInstance;
	}
	bool InitSocket();
	int DealCommand();
	// 只是把数据包放到消息队列中，并没有真正发送
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam=0);
	bool GetFilePath(std::string& filePath) { // 获取文件列表
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {
			filePath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {
		return m_packet;
	}
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(int nIp, int nPort) {
		if ((m_nIP!= nIp) || (m_nPort != nPort)){
			m_nIP = nIp;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_eventInvoke; // 启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc; // 消息和消息对应的回调函数
	HANDLE m_hThread;
	bool m_bAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	// 单例模式，保证整个系统周期内只产生一个实例，所以将构造和析构设为私有，禁止外部构造和析构
	// 将构造和析构函数设为私有，避免外部控制
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss);
	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	static unsigned __stdcall threadEntry(void* arg);
	void threadFunc();
	bool InitSockEnv(); // 套接字初始化
	static void releaseInstance() {
		if (mInstance != NULL) {
			delete mInstance;
			mInstance = NULL;
		}
	}
	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	// WM_SEND_PACK 消息对应的真正发送包数据的函数
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam); // wParam：缓冲区的值   lParam：缓冲区的长度
	static CClientSocket* mInstance;
	// 私有类，帮助调用析构函数
	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			releaseInstance();
		}
	};
	static CHelper mHelper;
};
