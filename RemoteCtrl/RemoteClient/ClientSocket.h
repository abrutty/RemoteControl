#pragma once

#include "pch.h"
#include "framework.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#define BUFFER_SIZE 4096000
#define WM_SEND_PACK (WM_USER+1) // 发送包数据

#pragma pack(push)  // 保存当前字节对齐的状态
#pragma pack(1)	// 强制取消字节对齐，改为连续存放
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	// 打包构造
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize, HANDLE hEvent) {
		this->sHead = 0xFEFF;
		this->nLength = nSize + 4;	// 数据长度+sCmd长度(2)+sSum长度(2)
		this->sCmd = sCmd;
		if (nSize > 0) {
			this->strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			this->strData.clear();
		}

		this->sSUm = 0;
		for (size_t j = 0; j < strData.size(); j++) { // 进行和校验
			this->sSUm += BYTE(strData[j]) & 0xFF;
		}
		this->hEvent = hEvent;
	}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSUm = pack.sSUm;
		hEvent = pack.hEvent;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSUm = pack.sSUm;
			hEvent = hEvent;
		}
		return *this;
	}
	// 解包构造函数
	CPacket(const BYTE* pData, size_t& nSize) :hEvent(INVALID_HANDLE_VALUE){
		size_t i = 0;	// i 始终指向已读到数据最新的位置
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i); // 找到包头
				i += 2;	// 防止只有包头FEFF，占两个字节，但没有数据
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {// 包数据可能不全，或包头未全部接收到，解析失败  +nLength +sCmd +sSum
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) { // 包没有完全接收到，比如只收到一半，解析失败
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); // 减去sCmd和sSum的长度
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()把string 对象转换成c中的字符串样式
			i += nLength - 4;
		}

		sSUm = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) { // 进行和校验
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSUm) {
			nSize = i; //  head(2字节）nLength(4字节)data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	int Size() { return nLength + 6; } // 数据包的大小
	const char* Data(std::string& strOut) const{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;
		pData += 2;
		*(DWORD*)pData = nLength;
		pData += 4;
		*(WORD*)pData = sCmd;
		pData += 2;
		memcpy(pData, strData.c_str(), strData.size());
		pData += strData.size();
		*(WORD*)pData = sSUm;
		return strOut.c_str();
	}

public:
	WORD sHead;				// 包头，固定位，0xFEFF
	DWORD nLength;			// 包长度，从控制命令开始，到和校验结束
	WORD sCmd;				// 控制命令
	std::string strData;	// 包数据
	WORD sSUm;				// 和校验
	HANDLE hEvent;
	//std::string strOut;		// 整个包的数据
};
#pragma pack(pop)	// 还原字节对齐


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

typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // 是否有效
	int isDirectory;       // 是否为目录
	bool hasNext;           // 是否还有后续
	char szFileName[256];   // 文件名
}FILEINFO, * PFILEINFO;

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
	bool InitSocket() {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(m_nIP);
		serv_addr.sin_port = htons(m_nPort);

		if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定IP不存在");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			AfxMessageBox("连接失败");
			TRACE("连接失败,%d,%s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		TRACE("InitSocket成功\r\n");
		return true;
	}

	int DealCommand() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0) && ((int)index <= 0)) return -1; // len是size_t，recv返回-1时会变成大于0的数，所以要强转为int判断
			TRACE("index = %d, len=%d\r\n", index, len);
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("command = %d\r\n", m_packet.sCmd);
			if (len > 0) {
				TRACE("index = %d, len=%d\r\n", index, len);
				memmove(buffer, buffer + len, index - len);
				index -= len;

				return m_packet.sCmd;
			}
		}
		delete[] buffer;
		return -1;
	}

	bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
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
	typedef void(CClientSocket::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClosed;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&> m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	// 单例模式，保证整个系统周期内只产生一个实例，所以将构造和析构设为私有，禁止外部构造和析构
	// 将构造和析构函数设为私有，避免外部控制
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_hThread = INVALID_HANDLE_VALUE;
		m_bAutoClosed = ss.m_bAutoClosed;
		m_sock = ss.m_sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
		struct {
			UINT message;
			MSGFUNC func;
		} funcs[] = {
			{WM_SEND_PACK, &CClientSocket::SendPack},
			{0,NULL}
		};
		for (int i = 0; funcs[i].message != 0; i++) {
			if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
				TRACE("插入失败，消息值：%d 函数值：%08X, 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
			}
		}
	}
	CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(true), m_hThread(INVALID_HANDLE_VALUE){
		if (InitSockEnv() == false) {
			MessageBox(NULL, _T("无法初始化套接字"), _T("初始化错误！"), MB_OK | MB_ICONERROR); // _T用来保证编码兼容性
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	};
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	};
	static void threadEntry(void* arg);
	void threadFunc();
	void threadFunc2();
	bool InitSockEnv() {
		// 套接字初始化
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return false;
		}
		else {
			m_sock = socket(PF_INET, SOCK_STREAM, 0);
			return true;
		}
	}
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
