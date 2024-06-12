#pragma once

#include "pch.h"
#include "framework.h"

#include <string>
#include <vector>
#include <list>
#include <map>

#define BUFFER_SIZE 4096000
#define WM_SEND_PACK (WM_USER+1) // ���Ͱ�����
#define WM_SEND_PACK_ACK (WM_USER+2) // ���Ͱ�����Ӧ��

#pragma pack(push)  // ���浱ǰ�ֽڶ����״̬
#pragma pack(1)	// ǿ��ȡ���ֽڶ��룬��Ϊ�������
class CPacket {
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSUm(0) {}
	CPacket(WORD sCmd, const BYTE* pData, size_t nSize); // �������
	CPacket(const CPacket& pack); // ���ƹ���
	CPacket& operator=(const CPacket& pack);
	// ������캯��
	CPacket(const BYTE* pData, size_t& nSize);
	~CPacket() {}
	int Size() { return nLength + 6; } // ���ݰ��Ĵ�С
	const char* Data(std::string& strOut) const; // �����ݰ�ת���ַ�����ʽ

public:
	WORD sHead;				// ��ͷ���̶�λ��0xFEFF
	DWORD nLength;			// �����ȣ��ӿ������ʼ������У�����
	WORD sCmd;				// ��������
	std::string strData;	// ������
	WORD sSUm;				// ��У��
};
#pragma pack(pop)	// ��ԭ�ֽڶ���

// �����Ϣ�ṹ��
typedef struct MouseEvent {
	MouseEvent() {
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	// ������ƶ���˫��
	WORD nButton;	// ������Ҽ����м�
	POINT ptXY;		// ����
}MOUSEEV, * PMOUSEEV;

// �ļ���Ϣ�ṹ��
typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(szFileName, 0, sizeof(szFileName));
	}
	bool isInvalid;         // �Ƿ���Ч
	int isDirectory;        // �Ƿ�ΪĿ¼
	bool hasNext;           // �Ƿ��к���
	char szFileName[256];   // �ļ���
}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1 // CSM=client socket mode �Զ��ر�ģʽ
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
	static CClientSocket* getInstance() { // �����������˽�еģ�ͨ����̬������ʽ����
		if (mInstance == nullptr) {
			mInstance = new CClientSocket;
		}
		return mInstance;
	}
	bool InitSocket();
	int DealCommand();
	// ֻ�ǰ����ݰ��ŵ���Ϣ�����У���û����������
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam=0);
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
	void UpdateAddress(int nIp, int nPort) {
		if ((m_nIP!= nIp) || (m_nPort != nPort)){
			m_nIP = nIp;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_eventInvoke; // �����¼�
	UINT m_nThreadID;
	typedef void(CClientSocket::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc; // ��Ϣ����Ϣ��Ӧ�Ļص�����
	HANDLE m_hThread;
	bool m_bAutoClosed;
	int m_nIP;
	int m_nPort;
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	// ����ģʽ����֤����ϵͳ������ֻ����һ��ʵ�������Խ������������Ϊ˽�У���ֹ�ⲿ���������
	// �����������������Ϊ˽�У������ⲿ����
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
	bool InitSockEnv(); // �׽��ֳ�ʼ��
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
	// WM_SEND_PACK ��Ϣ��Ӧ���������Ͱ����ݵĺ���
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam); // wParam����������ֵ   lParam���������ĳ���
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
