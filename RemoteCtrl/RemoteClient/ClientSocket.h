#pragma once

#include "pch.h"
#include "framework.h"

#include <string>
#include <vector>
#define BUFFER_SIZE 4096000
#pragma pack(push)  // ���浱ǰ�ֽڶ����״̬
#pragma pack(1)	// ǿ��ȡ���ֽڶ��룬��Ϊ�������
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	// �������
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize) {
		this->sHead = 0xFEFF;
		this->nLength = nSize + 4;	// ���ݳ���+sCmd����(2)+sSum����(2)
		this->sCmd = sCmd;
		if (nSize > 0) {
			this->strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else {
			this->strData.clear();
		}

		this->sSUm = 0;
		for (size_t j = 0; j < strData.size(); j++) { // ���к�У��
			this->sSUm += BYTE(strData[j]) & 0xFF;
		}
	}
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
	// ������캯��
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;	// iʼ��ָ���Ѷ����������µ�λ��
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i); // �ҵ���ͷ
				i += 2;	// ��ֹֻ�а�ͷFEFF��ռ�����ֽڣ���û������
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {// �����ݿ��ܲ�ȫ�����ͷδȫ�����յ�������ʧ��  +nLength +sCmd +sSum
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
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSUm) {
			nSize = i; //  head(2�ֽڣ�nLength(4�ֽ�)data...
			return;
		}
		nSize = 0;
	}
	~CPacket() {}
	int Size() { return nLength + 6; } // ���ݰ��Ĵ�С
	const char* Data() {
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
	WORD sHead;				// ��ͷ���̶�λ��0xFEFF
	DWORD nLength;			// �����ȣ��ӿ������ʼ������У�����
	WORD sCmd;				// ��������
	std::string strData;	// ������
	WORD sSUm;				// ��У��
	std::string strOut;		// ������������
};
#pragma pack(pop)	// ��ԭ�ֽڶ���


typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	// �ƶ��������˫��
	WORD nButton;	// ������Ҽ�������
	POINT ptXY;		// ����
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // �Ƿ���Ч
	int isDirectory;       // �Ƿ�ΪĿ¼
	bool hasNext;           // �Ƿ��к���
	char szFileName[256];   // �ļ���
}FILEINFO, * PFILEINFO;

std::string GetErrInfo(int WSAErrCode);
class CClientSocket
{
public:
	static CClientSocket* getInstance() { // �����������˽�еģ�ͨ����̬������ʽ����
		if (mInstance == nullptr) {
			mInstance = new CClientSocket;
		}
		return mInstance;
	}
	bool InitSocket(int nIp, int nPort) {
		if (m_sock != INVALID_SOCKET) CloseSocket();
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (m_sock == -1) return false;
		sockaddr_in serv_addr;
		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(nIp);
		serv_addr.sin_port = htons(nPort);

		if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("ָ��IP������");
			return false;
		}
		int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
		if (ret == -1) {
			AfxMessageBox("����ʧ��");
			TRACE("����ʧ��,%d,%s\r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}
	
	int DealCommand() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if ( (len<=0) && (index<=0) ) return -1;
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			TRACE("command = %d\r\n", m_packet.sCmd);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	bool Send(const char* pData, int nSize) {
		if (m_sock == -1) return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack) {
		TRACE("m_sock = %d\r\n", m_sock);
		if (m_sock == -1) return false;
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0;
	}
	bool GetFilePath(std::string& filePath) { // ��ȡ�ļ��б�
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
private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	// ����ģʽ����֤����ϵͳ������ֻ����һ��ʵ�������Խ������������Ϊ˽�У���ֹ�ⲿ���������
	// �����������������Ϊ˽�У������ⲿ����
	CClientSocket& operator=(const CClientSocket&& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket() {
		if (InitSockEnv() == false) {
			MessageBox(NULL, _T("�޷���ʼ���׽���"), _T("��ʼ������"), MB_OK | MB_ICONERROR); // _T������֤���������
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	};
	~CClientSocket() {
		closesocket(m_sock);
		WSACleanup();
	};
	bool InitSockEnv() {
		// �׽��ֳ�ʼ��
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
	static CClientSocket* mInstance;
	// ˽���࣬����������������
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
