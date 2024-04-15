#pragma once
#include "pch.h"
#include "framework.h"

class CServerSocket
{
public:
	static CServerSocket* getInstance() { // �����������˽�еģ�ͨ����̬������ʽ����
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
		if (m_client == -1) return 0;
		char buffer[1024] = "";
		while (true){
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) return -1;
		}
	}
	BOOL Send(const char* pData, int nSize) {
		if (m_client == -1) return FALSE;
		return send(m_client, pData, nSize, 0) > 0;
	}
private:
	SOCKET m_sock, m_client;
	// ����ģʽ����֤����ϵͳ������ֻ����һ��ʵ�������Խ������������Ϊ˽�У���ֹ�ⲿ���������
	// �����������������Ϊ˽�У������ⲿ����
	CServerSocket& operator=(const CServerSocket&& ss) {}
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
	BOOL InitSockEnv() {
		// �׽��ֳ�ʼ��
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
