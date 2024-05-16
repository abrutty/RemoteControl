#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "CWatchDialog.h"
#include <map>
#include "resource.h"
#include "Tool.h"


//#define WM_SEND_DATA (WM_USER+2) // 只发送数据
#define WM_SHOW_STATUS (WM_USER+3) // 展示状态
#define WM_SHOW_WATCH (WM_USER+4) // 远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000) // 自定义消息处理
class CClientController
{
public:
	static CClientController* getInstance(); // 获取全局唯一对象
	int InitController(); // 初始化操作
	int Invoke(CWnd*& pMainWnd); // 启动
	LRESULT SendMessage(MSG msg); // 发送消息
	void UpdateAddress(int nIp, int nPort) {  // 更新网络服务器地址
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
	// 1-->查看磁盘分区
	// 2-->查看指定目录下的文件
	// 3-->打开文件
	// 4-->下载文件
	// 5-->鼠标操作
	// 6-->发送屏幕内容
	// 7-->锁机
	// 8-->解锁
	// 9-->删除文件
	// 1981-->测试连接
	int SendCommandPacket(int nCmd, bool autoClose = true, BYTE* pData = nullptr, size_t nLength = 0, std::list<CPacket> *plstPacks=nullptr);
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	void ThreadDownloadFile();
	static void ThreadDownloadEntry(void* arg); // 静态函数不能使用this指针，此函数可以专注于线程框架
	void ThreadWatchScreen(); // 可以使用this指针，专注于处理逻辑
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
	void threadFunc(); // 真正的线程函数
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
	HANDLE m_hThreadDownload; // 控制下载文件的线程
	unsigned m_hThreadID;
	HANDLE m_hThreadWatch;
	bool m_isClosed; // 监视是否关闭远程控制对话框   第一次点开始监控按钮，然后关闭监控对话框，第二次点监控按钮就会出问题
	CString m_strRemote; // 下载文件的远程路径
	CString m_strLocal;  // 下载文件到本地的保存路径
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

