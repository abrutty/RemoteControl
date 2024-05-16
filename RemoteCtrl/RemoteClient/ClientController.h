#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "CWatchDialog.h"
#include <map>
#include "resource.h"
#include "Tool.h"


//#define WM_SEND_DATA (WM_USER+2) // ֻ��������
#define WM_SHOW_STATUS (WM_USER+3) // չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4) // Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000) // �Զ�����Ϣ����
class CClientController
{
public:
	static CClientController* getInstance(); // ��ȡȫ��Ψһ����
	int InitController(); // ��ʼ������
	int Invoke(CWnd*& pMainWnd); // ����
	LRESULT SendMessage(MSG msg); // ������Ϣ
	void UpdateAddress(int nIp, int nPort) {  // ���������������ַ
		CClientSocket::getInstance()->UpdateAddress(nIp, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	/*bool SendPacket(const CPacket& pack) {
		CClientSocket* pClient = CClientSocket::getInstance();
		if (pClient->InitSocket() == false) return false;
		pClient->Send(pack);
	}*/
	// 1-->�鿴���̷���
	// 2-->�鿴ָ��Ŀ¼�µ��ļ�
	// 3-->���ļ�
	// 4-->�����ļ�
	// 5-->������
	// 6-->������Ļ����
	// 7-->����
	// 8-->����
	// 9-->ɾ���ļ�
	// 1981-->��������
	int SendCommandPacket(int nCmd, bool autoClose = true, BYTE* pData = nullptr, size_t nLength = 0, std::list<CPacket> *plstPacks=nullptr);
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void ThreadDownloadFile();
	static void ThreadDownloadEntry(void* arg); // ��̬��������ʹ��thisָ�룬�˺�������רע���߳̿��
	void ThreadWatchScreen(); // ����ʹ��thisָ�룬רע�ڴ����߼�
	static void ThreadWatchScreenEntry(void* arg);
	CClientController():m_statusDlg(&m_remoteDlg), m_watchDlg(&m_remoteDlg) {
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_hThreadID = -1;
		m_isClosed = true;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc(); // �������̺߳���
	static unsigned __stdcall threadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = nullptr;
		}
	}
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MSGINFO {
		MSG msg;
		LRESULT result;
		MSGINFO(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MSGINFO(const MSGINFO& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MSGINFO& operator=(const MSGINFO& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	} MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload; // ���������ļ����߳�
	unsigned m_hThreadID;
	HANDLE m_hThreadWatch;
	bool m_isClosed; // �����Ƿ�ر�Զ�̿��ƶԻ���   ��һ�ε㿪ʼ��ذ�ť��Ȼ��رռ�ضԻ��򣬵ڶ��ε��ذ�ť�ͻ������
	CString m_strRemote; // �����ļ���Զ��·��
	CString m_strLocal;  // �����ļ������صı���·��
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			//CClientController::getInstance();
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

