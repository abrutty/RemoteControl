#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

std::map<UINT, CClientController::MSGFUNC>CClientController::m_mapFunc;
CClientController* CClientController::m_instance = nullptr;
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new CClientController;
        struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] = {
            //{WM_SEND_PACK, &CClientController::OnSendPack},
            //{WM_SEND_DATA, &CClientController::OnSendData},
            {WM_SHOW_STATUS, &CClientController::OnShowStatus},
            {WM_SHOW_WATCH, &CClientController::OnShowWatcher},
            {(UINT)-1, nullptr}
        };
        for (int i = 0; MsgFuncs[i].func != nullptr; i++) {
            m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
        }
    }
    return m_instance;
}

int CClientController::InitController()
{
    m_hThread = (HANDLE)_beginthreadex(nullptr, 0, &CClientController::threadEntry, this, 0, &m_hThreadID); // 创建线程
    m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
    return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
    pMainWnd = &m_remoteDlg;
    return m_remoteDlg.DoModal();
}

bool CClientController::SendCommandPacket(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
    bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
    return ret;
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(
		false,
		nullptr,
        strPath,
            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
            nullptr,
            & m_remoteDlg
            );
            if (dlg.DoModal() == IDOK) {
                m_strRemote = strPath;
                m_strLocal = dlg.GetPathName();
                FILE* pFile = fopen(m_strLocal, "wb+");
                if (pFile == nullptr) {
                    AfxMessageBox(_T("本地没有权限保存文件"));
                    return -1;
                }
                SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
                m_remoteDlg.BeginWaitCursor();
                m_statusDlg.m_info.SetWindowText(_T("命令正在执行"));
                m_statusDlg.ShowWindow(SW_SHOW);
                m_statusDlg.CenterWindow(&m_remoteDlg);
                m_statusDlg.SetActiveWindow();
            }
            return 0;
}

void CClientController::DownloadEnd()
{
    m_statusDlg.ShowWindow(SW_HIDE);
    m_remoteDlg.EndWaitCursor();
    m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
}

void CClientController::StartWatchScreen()
{
    m_isClosed = false;
    m_hThreadWatch = (HANDLE)_beginthread(&CClientController::ThreadWatchScreenEntry, 0, this);
    m_watchDlg.DoModal();
    m_isClosed = true;
    WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::ThreadWatchScreen()
{
    Sleep(50);
    ULONGLONG nTick = GetTickCount64();
    while (!m_isClosed) {
        if (m_watchDlg.isFull() == false) {
            if (GetTickCount64() - nTick < 200){ // 控制发送图片的频率为200ms一次
                Sleep(200 - DWORD(GetTickCount64() - nTick));
            }
            nTick = GetTickCount64();
            int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, nullptr, 0);
            if (ret == 1) {
                TRACE("成功发送图片\r\n");
            }
            else {
                TRACE("获取图片失败\r\n");
            }
        }
        Sleep(1);
    }
}

void CClientController::ThreadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->ThreadWatchScreen();
	_endthread();
}

void CClientController::threadFunc()
{
    MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_SEND_MESSAGE) {
            MSGINFO* pmsg = (MSGINFO*)msg.wParam;
            HANDLE hEvent = (HANDLE*)msg.lParam;
			auto it = m_mapFunc.find(pmsg->msg.message);
			if (it != m_mapFunc.end()) {
				pmsg->result = (this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);
            }
            else {
                pmsg->result = -1;
            }
            SetEvent(hEvent);
        }
        else {
            auto it = m_mapFunc.find(msg.message);
            if (it != m_mapFunc.end()) {
                (this->*it->second)(msg.message, msg.wParam, msg.lParam);
            }
        }
    }
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
    CClientController* thiz = (CClientController*)arg;
    thiz->threadFunc();
    _endthreadex(0);
    return 0;
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    return m_watchDlg.DoModal();
}
