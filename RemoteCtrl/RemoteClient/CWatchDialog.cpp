// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

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
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if (isScreen)
		ScreenToClient(&point); // 将指定点的全局屏幕坐标转换为屏幕坐标
	// 本地坐标到远程坐标
	m_picture.GetWindowRect(clientRect);
	// width0, height0是客户端显示窗口的宽和高
	int width0 = clientRect.Width();
	int height0 = clientRect.Height();
	int width = 2560, height = 1440; // 远程端（被控端）屏幕大小
	int x = point.x * width / width0; // width/width0 = x/point.x
	int y = point.y * height / height0;
	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, nullptr);
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull() == true) {
			CRect rect;
			m_picture.GetWindowRect(rect);
			pParent->GetImage().StretchBlt(
				m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(nullptr);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus(); // 设为false，线程函数里才能更新
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point) // point是客户端坐标
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0; // 左键
	event.nAction = 2; // 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0; // 左键
	event.nAction = 2; // 按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0; // 左键
	event.nAction = 3; // 弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1; // 右键
	event.nAction = 1; // 双击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1; // 右键
	event.nAction = 2; // 按下
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 1; // 右键
	event.nAction = 3; // 弹起
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 8; // 没有按键
	event.nAction = 0; // 移动
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	CPoint point;
	GetCursorPos(&point); // 拿到的是屏幕坐标
	// 坐标转换
	CPoint remote = UserPoint2RemoteScreenPoint(point, true);
	// 封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0; // 左键
	event.nAction = 0; // 单击
	CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 获得父窗口
	pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM) & event);
}
