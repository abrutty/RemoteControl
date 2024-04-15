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
		size_t i = 0;	// iʼ��ָ���Ѷ����������µ�λ��
		for (; i < nSize; i++) {
			if ( *(WORD*)(pData + i) == 0xFEFF ) {
				sHead = *(WORD*)(pData + i); // �ҵ���ͷ
				i += 2;	// ��ֹֻ�а�ͷFEFF��ռ�����ֽڣ���û������
				break;
			}
		}
		if (i+4+2+2 > nSize) {// �����ݿ��ܲ�ȫ�����ͷδȫ�����յ�������ʧ��  +nLength +sCmd +sSum
			nSize = 0;
			return;
		}

		nLength = *(DWORD*)(pData + i);
		i += 4;
		if (nLength + i > nSize) { // ��û����ȫ���յ�������ֻ�յ�һ�룬����ʧ��
			nSize = 0;
			return;
		}

		sCmd = *(WORD*)(pData + i);
		i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2); // ��ȥsCmd��sSum�ĳ���
			memcpy((void*)strData.c_str(), pData + i, nLength - 4); //c_str()��string ����ת����c�е��ַ�����ʽ
			i += nLength - 4;
		}

		sSUm = *(WORD*)(pData + i);
		i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++) { // ���к�У��
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSUm) {
			nSize = i; //  head(2�ֽڣ�nLength(4�ֽ�)data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
public:
	WORD sHead;				// ��ͷ���̶�λ��0xFEFF
	DWORD nLength;			// �����ȣ��ӿ������ʼ������У�����
	WORD sCmd;				// ��������
	std::string strData;	// ������
	WORD sSUm;				// ��У��
};

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
