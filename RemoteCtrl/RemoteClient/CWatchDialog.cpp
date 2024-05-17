// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "ClientController.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPackAck) // 自定义消息注册
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if (!isScreen) ClientToScreen(&point); // 转换为相对屏幕左上角的坐标（屏幕内的绝对坐标）
	m_picture.ScreenToClient(&point); // 转换为客户区域坐标（相对picture控件左上角的坐标）

	// 本地坐标到远程坐标
	m_picture.GetWindowRect(clientRect);
	// width0, height0是客户端显示窗口的宽和高
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = m_nObjWidth, height = m_nObjHeight; // 远程端（被控端）屏幕大小
	int x = point.x * width / width0; // width/width0 = x/point.x
	int y = point.y * height / height0;
	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_isFull = false; // 最开始没有缓存数据
	//SetTimer(0, 45, nullptr);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	//if (nIDEvent == 0) {
	//	CClientController* pParent = CClientController::getInstance();
	//	if (m_isFull == true) {
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);
	//		m_nObjWidth = m_image.GetWidth();
	//		m_nObjHeight = m_image.GetHeight();
	//		m_image.StretchBlt(
	//			m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(nullptr);
	//		m_image.Destroy();
	//		m_isFull = false; // 设为false，线程函数里才能更新
	//	}
	//}
	CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam==-1) || (lParam==-2)) { // 错误处理

	}
	else if (lParam == 1) { // 对方关闭了套接字

	}
	else {
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != nullptr) {
			switch (pPacket->sCmd) {
			case 6:
			{
				if (m_isFull) {
					CTool::Bytes2Image(m_image, pPacket->strData);
					CRect rect;
					m_picture.GetWindowRect(rect);
					m_nObjWidth = m_image.GetWidth();
					m_nObjHeight = m_image.GetHeight();
					m_image.StretchBlt(
						m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
					m_picture.InvalidateRect(nullptr);
					m_image.Destroy();
					m_isFull = false; // 设为false，线程函数里才能更新
				}
				break;
			}
			case 5:
			case 7:
			case 8:
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point) // point 是客户端坐标
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0; // 左键
		event.nAction = 2; // 双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event,sizeof(event));
	}

	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0; // 左键
		event.nAction = 2; // 按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0; // 左键
		event.nAction = 3; // 弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1; // 右键
		event.nAction = 1; // 双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1; // 右键
		event.nAction = 2; // 按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1; // 右键
		event.nAction = 3; // 弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 8; // 没有按键
		event.nAction = 0; // 移动
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point); // 拿到的是屏幕坐标
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point, true);
		// 封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0; // 左键
		event.nAction = 0; // 单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();  屏蔽原来的OK，避免按回车退出远程监控页面
}


void CWatchDialog::OnBnClickedBtnLock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);
}
