
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_DIR, &CRemoteClientDlg::OnTvnSelchangedTree1)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck) // 注册自定义消息（告诉系统 该消息对应的响应函数）
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x0A072D3F;
	//m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);

}


void CRemoteClientDlg::OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1, true, nullptr, 0);
	if (ret == 0) {
		AfxMessageBox(_T("命令处理失败"));
		return;
	}
}

//void CRemoteClientDlg::ThreadEntryForWatchData(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
//	thiz->ThreadWatchData();
//	_endthread();
//}
//
//void CRemoteClientDlg::ThreadWatchData()
//{
//	Sleep(50);
//	CClientController* pController = CClientController::getInstance();
//	while(!m_isClosed){
//		if (m_isFull == false) { // 更新数据到缓存
//			int ret = pController->SendCommandPacket(6);
//			if (ret == 6) {
//				if(pController->GetImage(m_image)==0){
//					m_isFull = true;
//				}
//				else {
//					TRACE("获取图片失败\r\n");
//				}
//			}
//			else {
//				Sleep(1); // 发送失败休眠1ms，避免死机
//			}
//		}
//		else {
//			Sleep(1);
//		}
//	}
//}

//void CRemoteClientDlg::ThreadEntryForDownFile(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
//	thiz->ThreadDownFile();
//	_endthread();
//}
//
//void CRemoteClientDlg::ThreadDownFile()
//{
//	int nListSelected = m_List.GetSelectionMark();
//	CString strFile = m_List.GetItemText(nListSelected, 0);
//	CFileDialog dlg(
//		false,
//		nullptr,
//		strFile,
//		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
//		nullptr,
//		this
//	);
//
//	if (dlg.DoModal() == IDOK) {
//		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
//		if (pFile == nullptr) {
//			AfxMessageBox(_T("没有权限保存该文件或文件无法创建"));
//			m_dlgStatus.ShowWindow(SW_HIDE);
//			EndWaitCursor();
//			return;
//		}
//		HTREEITEM hSelected = m_Tree.GetSelectedItem();
//		strFile = GetPath(hSelected) + strFile;
//		TRACE("strFile=%s\r\n", strFile);
//		CClientSocket* pClient = CClientSocket::getInstance();
//		do{
//			int ret = CClientController::getInstance()->SendCommandPacket(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
//
//			//int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCTSTR)strFile); // 向主线程发消息，进入消息循环
//			// 注意与服务端锁机那个地方，SendMessage不起效的差异，客户端是MFC环境，服务端是Windows环境，这两个地方的函数所属类不同
//			if (ret < 0) {
//				AfxMessageBox("执行下载命令失败");
//				TRACE("ret=%d\r\n", ret);
//				break;
//			}
//
//			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
//			if (nLength == 0) {
//				AfxMessageBox("文件为空或无法下载文件");
//				break;
//			}
//			long long nCount = 0;
//			while (nCount < nLength) {
//				ret = pClient->DealCommand();
//				if (ret < 0) {
//					AfxMessageBox("传输失败");
//					TRACE("传输失败，ret=%d\r\n", ret);
//					break;
//				}
//				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//				nCount += pClient->GetPacket().strData.size();
//			}
//		} while (false);
//		
//		fclose(pFile);
//		pClient->CloseSocket();
//	}
//	m_dlgStatus.ShowWindow(SW_HIDE);
//	EndWaitCursor();
//	MessageBox(_T("下载完成"), _T("完成"));
//}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + "\\" + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != nullptr);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildItem(HTREEITEM hTree) {
	HTREEITEM sub = nullptr;
	do {
		sub = m_Tree.GetChildItem(hTree);
		if (sub != nullptr) m_Tree.DeleteItem(sub);
	} while (sub != nullptr);
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == nullptr) return;
	if (m_Tree.GetChildItem(hTreeSelected) == nullptr) return; // 为空说明点的是一个文件
	DeleteTreeChildItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	std::list<CPacket> lstPackets;
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, 
		(BYTE*)(LPCTSTR)strPath, strPath.GetLength(),(WPARAM)hTreeSelected);
	
	if (lstPackets.size() > 0) {
		TRACE("lstPackets size=%d\r\n", lstPackets.size());
		for (auto it = lstPackets.begin(); it != lstPackets.end(); it++) {
			
		}
	}
}

void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	while (pInfo->hasNext) {
		if (!pInfo->isDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}

		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack=%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	//CClientController::getInstance()->CloseSocket();
}

void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int listSelected = m_List.HitTest(ptList);
	if (listSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPopup = menu.GetSubMenu(0);
	if (pPopup != nullptr) {
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}


void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret = CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox(_T("文件下载失败"));
		TRACE("文件下载失败ret=%d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	TRACE("strFile=%s\r\n", strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("执行文件删除命令失败");
		return;
	}
	AfxMessageBox("删除文件成功");
	LoadFileCurrent();
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	TRACE("strFile=%s\r\n", strFile);
	int ret = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("执行文件下载命令失败");
		return;
	}
}

//LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
//{
//	int ret = 0;
//	int cmd = wParam >> 1;
//	switch (cmd) {
//	case 4:
//	{
//		CString strFile = (LPCTSTR)lParam;
//		ret = CClientController::getInstance()->SendCommandPacket(wParam >> 1, wParam & 1, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
//	}
//		break;
//	case 5: // 鼠标操作
//	{
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
//	}
//		break;
//	case 6:
//	case 7:
//	case 8:
//	{
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1);
//	}
//		break;
//	default:
//		ret = -1;
//	}
//
//	return ret;
//}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2)) { // 错误处理

	}
	else if (lParam == 1) { // 对方关闭了套接字

	} 
	else {
		if (wParam != NULL) {
			CPacket head = *(CPacket*)wParam;
			delete (CPacket*)wParam;  // 马上销毁，避免内存泄露
			switch (head.sCmd) {
			case 1: // 获取驱动信息
			{
				std::string drivers = head.strData;
				std::string dr;
				m_Tree.DeleteAllItems();
				for (size_t i = 0; i < drivers.length(); i++) {
					if (drivers[i] == ',') {
						dr += ":";
						HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
						m_Tree.InsertItem("", hTemp, TVI_LAST);
						dr.clear();
						continue;
					}
					dr += drivers[i];
				}
				if (dr.size() > 0) {
					dr += ":";
					HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
					m_Tree.InsertItem("", hTemp, TVI_LAST);
				}
			}				
				break;
			case 2: // 获取文件信息
			{
				PFILEINFO pInfo = (PFILEINFO)head.strData.c_str();
				if (pInfo->hasNext == false) break;
				if (pInfo->isDirectory) {
					if (((CString)(pInfo->szFileName) == ".") || ((CString)(pInfo->szFileName) == "..")) {
						break;
					}
					HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, (HTREEITEM)lParam, TVI_LAST);
					m_Tree.InsertItem("", hTemp, TVI_LAST);
				}
				else {
					m_List.InsertItem(0, pInfo->szFileName);
				}
			}
				break;
			case 3:
				TRACE("run file done\r\n");
				break;
			case 4:
			{
				static LONGLONG length = 0, index=0;
				if (length == 0) {
					length = *(long long*)head.strData.c_str();
					if (length == 0) {
						AfxMessageBox("文件长度为0或无法读取文件");
						CClientController::getInstance()->DownloadEnd();
					}
				}
				else if ((length > 0) && (index >= length)) {
					fclose((FILE*)lParam);
					length = 0;
					index = 0;
					CClientController::getInstance()->DownloadEnd();
				}
				else {
					FILE* pFile = (FILE*)lParam;
					fwrite(head.strData.c_str(), 1, head.strData.size(), pFile);
					index += head.strData.size();
				}
			}
				break;
			case 9:
				TRACE("delete file done\r\n");
				break;
			case 1981:
				TRACE("test connect success\r\n");
				break;
			default:
				TRACE("unkonwn data received\r\n");
				break;
			}	
		}
	}
	return 0;
}
