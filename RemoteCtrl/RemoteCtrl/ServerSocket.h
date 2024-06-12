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
	static CServerSocket* getInstance() { // �����������˽�еģ�ͨ����̬������ʽ����
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
	//bool GetFilePath(std::string& filePath) { // ��ȡ�ļ��б�
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
	// ����ģʽ����֤����ϵͳ������ֻ����һ��ʵ�������Խ������������Ϊ˽�У���ֹ�ⲿ���������
	// �����������������Ϊ˽�У������ⲿ����
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽���"), _T("��ʼ������"), MB_OK | MB_ICONERROR); // _T������֤���������
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	};
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	};
	BOOL InitSockEnv(); // �׽��ֳ�ʼ��
	static void releaseInstance() {
		if (mInstance != NULL) {
			delete mInstance;
			mInstance = NULL;
		}
	}
	static CServerSocket* mInstance;
	// ˽���࣬����������������
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

//extern CServerSocket server;  // ʹ��extern����������main����֮ǰִ��
