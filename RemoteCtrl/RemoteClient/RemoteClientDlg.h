#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2) // 发送包数据应答
#endif // !WM_SEND_PACK_ACK

//#define WM_SEND_PACKET (WM_USER+1) // 自定义发送数据包的消息，为了解决线程校验通不过的问题，因为在子线程去处理下载文件
// 1.自定义消息--->2.自定义消息响应函数--->3.注册消息--->4.实现消息函数

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	
private:
	bool m_isClosed; // 监视是否关闭远程控制对话框   第一次点开始监控按钮，然后关闭监控对话框，第二次点监控按钮就会出问题
	//因为点一次按钮就开一次线程，点两次就开了两个线程，两个线程都将屏幕存到同一个image
private:
	//static void ThreadEntryForWatchData(void* arg); 
	//void ThreadWatchData();	
	//static void ThreadEntryForDownFile(void *arg); // 下载大文件的线程入口函数
	//void ThreadDownFile();
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildItem(HTREEITEM hTree);
	void LoadFileInfo();
	void LoadFileCurrent(); // 删除文件后刷新显示
	
// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnTvnSelchangedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam); // 自定义消息响应函数
};
