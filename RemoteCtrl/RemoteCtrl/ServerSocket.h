#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

#define BUFFER_SIZE 4096000


typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket);

class CServerSocket
{
public:
	static CServerSocket* getInstance() { // 构造和析构是私有的，通过静态函数方式访问
		if (mInstance == nullptr) {
			mInstance = new CServerSocket;
		}
		return mInstance;
	}
	
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527);
protected:
	bool InitSocket(short port);
	bool AcceptClient();
	int DealCommand();
	bool Send(const char* pData, int nSize) {
		if (m_client == -1) return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		if (m_client == -1) return false;
		//Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}
	//bool GetFilePath(std::string& filePath) { // 获取文件列表
	//	if ( ((m_packet.sCmd >= 2)&&(m_packet.sCmd <= 4)) || (m_packet.sCmd==9)){
	//		filePath = m_packet.strData;
	//		return true;
	//	}
	//	return false;
	//}
	/*bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));
			return true;
		}
		return false;
	}*/
	/*CPacket& GetPacket() {
		return m_packet;
	}*/
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_sock, m_client;
	CPacket m_packet;
	// 单例模式，保证整个系统周期内只产生一个实例，所以将构造和析构设为私有，禁止外部构造和析构
	// 将构造和析构函数设为私有，避免外部控制
	CServerSocket& operator=(const CServerSocket& ss) {}
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
	BOOL InitSockEnv(); // 套接字初始化
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
