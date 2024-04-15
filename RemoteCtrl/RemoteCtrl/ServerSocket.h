#pragma once
#include "pch.h"
#include "framework.h"

#define BUFFER_SIZE 4096
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	CPacket(const CPacket& pack) {
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSUm = pack.sSUm;
	}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSUm = pack.sSUm;
		}
		return *this;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;	// i始终指向已读到数据最新的位置
		for (; i < nSize; i++) {
			if ( *(WORD*)(pData + i) == 0xFEFF ) {
				sHead = *(WORD*)(pData + i); // 找到包头
				i += 2;	// 防止只有包头FEFF，占两个字节，但没有数据
				break;
			}
		}
		if (i+4+2+2 > nSize) {// 包数据可能不全，或包头未全部接收到，解析失败  +nLength +sCmd +sSum
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
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSUm) {
			nSize = i; //  head(2字节）nLength(4字节)data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
public:
	WORD sHead;				// 包头，固定位，0xFEFF
	DWORD nLength;			// 包长度，从控制命令开始，到和校验结束
	WORD sCmd;				// 控制命令
	std::string strData;	// 包数据
	WORD sSUm;				// 和校验
};

class CServerSocket
{
public:
	static CServerSocket* getInstance() { // 构造和析构是私有的，通过静态函数方式访问
		if (mInstance == NULL) {
			mInstance = new CServerSocket;
		}
		return mInstance;
	}
	BOOL InitSocket() {
		if (m_sock == -1) return FALSE;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(9527);

		if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return FALSE;
		if (listen(m_sock, 1) == -1) return FALSE;
		return TRUE;
	}
	BOOL AcceptClient() {
		sockaddr_in client_addr;
		int client_sz = sizeof(client_addr);
		m_client = accept(m_sock, (sockaddr*)&client_addr, &client_sz);
		if (m_client == -1) return FALSE;
		return TRUE;
	}
	int DealCommand() {
		if (m_client == -1) return -1;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer+index, BUFFER_SIZE -index, 0);
			if (len <= 0) return -1;
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	BOOL Send(const char* pData, int nSize) {
		if (m_client == -1) return FALSE;
		return send(m_client, pData, nSize, 0) > 0;
	}
private:
	SOCKET m_sock, m_client;
	CPacket m_packet;
	// 单例模式，保证整个系统周期内只产生一个实例，所以将构造和析构设为私有，禁止外部构造和析构
	// 将构造和析构函数设为私有，避免外部控制
	CServerSocket& operator=(const CServerSocket&& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字"), _T("初始化错误！"), MB_OK | MB_ICONERROR); // _T用来保证编码兼容性
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	};
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv() {
		// 套接字初始化
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		else {
			m_sock = socket(PF_INET, SOCK_STREAM, 0);
			return TRUE;
		}
	}
	static void releaseInstance() {
		if (mInstance != NULL) {
			delete mInstance;
			mInstance = NULL;
		}
	}
	static CServerSocket* mInstance;
	// 私有类，帮助调用析构函数
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			releaseInstance();
		}
	};
	static CHelper mHelper;
};

//extern CServerSocket server;  // 使用extern声明，会在main函数之前执行
