#pragma once
#include <map>
#include "atlimage.h"
#include "ServerSocket.h"
#include "LockDialog.h"
#include "resource.h"
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <list>
#include "Tool.h"
class CCommand
{
public:
	CCommand();
	~CCommand(){}
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)(); // ��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction; // ������ŵ����ܵ�ӳ��
	CLockDialog dlg;
	unsigned thread_id;
protected:
	// �鿴���ش��̷�����������Ϣ��������ݰ�
	int MakeDriverInfo() {
		std::string result;  // A ��->1 B��->2 C��->3
		for (int i = 1; i <= 26; i++) {  // �������̷�
			if (_chdrive(i) == 0) {// =0 ��ʾ�ɹ�
				if (result.size() > 0) result += ",";
				result += 'A' + i - 1;
			}
		}
		CPacket pack(1, (BYTE*)result.c_str(), result.size());
		CTool::Dump((BYTE*)pack.Data(), pack.Size());
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MakeDirectoryInfo() {
		std::string filePath;
		//std::list<FILEINFO> listFileInfo;
		if (CServerSocket::getInstance()->GetFilePath(filePath) == false) {
			OutputDebugString(_T("���ǻ�ȡ�ļ��б������"));
			return -1;
		}
		if (_chdir(filePath.c_str()) != 0) {  // �л�������·���жϸ�·���Ƿ����
			FILEINFO finfo;
			finfo.hasNext = false;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("û��Ȩ�޷���Ŀ¼"));
			return -2;
		}
		_finddata_t fdata;
		intptr_t  hfind;
		if ((hfind = _findfirst("*", &fdata)) == -1L) {// ��ȡ��ǰ�ļ�
			OutputDebugString(_T("û���ҵ��κ��ļ�"));
			FILEINFO finfo;
			finfo.hasNext = false;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		do {
			FILEINFO finfo;
			finfo.isDirectory = (fdata.attrib & _A_SUBDIR) != 0;
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE("szFileName=%s\r\n", finfo.szFileName);
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
		} while (_findnext(hfind, &fdata) == 0); // ��ȡ��һ���ļ��ڵ�
		_findclose(hfind);
		// ͨ����������������ϣ����̷���ʼ��ͨ��do...while() ѭ���ɵõ������ļ�
		// ������Ϣ�����ƶ�
		FILEINFO finfo;
		finfo.hasNext = false;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int RunFile() {
		std::string filePath;
		CServerSocket::getInstance()->GetFilePath(filePath);
		ShellExecuteA(nullptr, nullptr, filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		CPacket pack(3, nullptr, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int DownloadFile() {
		std::string filePath;
		CServerSocket::getInstance()->GetFilePath(filePath);
		long long data = 0;
		FILE* pFile = nullptr;
		errno_t err = fopen_s(&pFile, filePath.c_str(), "rb"); // ��fopen�ᱨwarning������ʹ��fopen������warning
		if (err != 0) {
			CPacket pack(4, (BYTE*)data, 8);
			CServerSocket::getInstance()->Send(pack);
			return -1;
		}
		if (pFile != nullptr) {
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			CPacket head(4, (BYTE*)&data, 8);
			CServerSocket::getInstance()->Send(head);
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do {
				rlen = fread(buffer, 1, 1024, pFile);
				CPacket pack(4, (BYTE*)buffer, rlen);
				CServerSocket::getInstance()->Send(pack);
			} while (rlen >= 1024); // <1024˵�������ļ�β        
			fclose(pFile);
		}
		CPacket pack(4, nullptr, 0);
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}

	int MouseEvent() {
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
			DWORD nFlags = 0;
			switch (mouse.nButton) {
			case 0: //���
				nFlags = 1;
				break;
			case 1: // �Ҽ�
				nFlags = 2;
				break;
			case 2: // �м�
				nFlags = 4;
				break;
			case 3: // û�а���
				nFlags = 8;
				break;
			default:
				break;
			}
			if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction) {
			case 0: // ����
				nFlags |= 0x10;
				break;
			case 1: // ˫��
				nFlags |= 0x20;
				break;
			case 2: // ����
				nFlags |= 0x40;
				break;
			case 3: // �ſ�
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			switch (nFlags) {
			case 0x21:  // ���˫��
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x11:  // �������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41:  // �������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81:  // ����ſ�
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x22:  // �Ҽ�˫��
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x12:  // �Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42:  // �Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82:  // �Ҽ��ſ�
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;

			case 0x24:  // �м�˫��
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x14:  // �м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44:  // �м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84:  // �м��ſ�
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08:  // ��������ƶ�
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			default:
				break;
			}
		}
		else {
			OutputDebugString(_T("��ȡ����������ʧ��"));
			return -1;
		}
	}

	int SendScreen() {
		CImage screen;  // GDI����ȫ���豸�ӿ�
		HDC hScreen = ::GetDC(nullptr);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitPerPixel);   // ������Ļ
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);   // ��ͼ���Ƶ���Ļ��
		ReleaseDC(nullptr, hScreen);

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);   // ���ڴ��з���һ��ȫ�ֵĶ�
		if (hMem == nullptr) return -1;
		IStream* pStream = nullptr;    // �����ڴ�������ͼƬ�浽�ڴ���
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // ��ȫ�ֶ����������ڴ���
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG);  // ����ͼ���ڴ�����
			//screen.Save(_T("test1.png"), Gdiplus::ImageFormatPNG);   // ����ͼ���ļ�
			LARGE_INTEGER begin = { 0 };
			pStream->Seek(begin, STREAM_SEEK_SET, nullptr); // ����󣬽���ָ��ָ�����ͷ
			PBYTE pData = (PBYTE)GlobalLock(hMem);  // ��������ȡ����
			SIZE_T nSize = GlobalSize(hMem);
			CPacket pack(6, pData, nSize);
			CServerSocket::getInstance()->Send(pack);
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}
	

	// ���������̣߳��������߳��о�һֱ������Ϣѭ�����ղ�����������Ϣ
	static unsigned __stdcall ThreadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->ThreadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void ThreadLockDlgMain() {
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
		dlg.Create(IDD_DIALOG_INFO, nullptr);
		dlg.ShowWindow(SW_SHOW);    // ��ģ̬�Ի���

		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = (LONG)GetSystemMetrics(SM_CXFULLSCREEN) * 1.10;
		TRACE("right=%d,bottom=%d\r\n", rect.right, rect.bottom);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText != nullptr) {
			CRect rtText;
			pText->GetWindowRect(rtText); // ��ȡ�����ı��Ŀ�͸�
			int nWidth = rtText.Width();
			int x = (rect.right - nWidth) / 2; // �ı����е�x����
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2; // �ı����е�y����
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // �����ö�
		ShowCursor(false);  // ����ʾ���
		//::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), nullptr), SW_HIDE); // ����������
		dlg.GetWindowRect(rect);    // ��ȡ�Ի���ķ�Χ
		rect.right = 1;
		rect.bottom = 1;
		ClipCursor(rect);   // ����������ڶԻ���Χ��
		// ������Ϣѭ��������Ч
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) { // ���°���
				if (msg.wParam == 0x1B) break; // ����esc���˳�
			}
		}
		ClipCursor(nullptr); // ȡ����귶Χ����
		ShowCursor(true); // �ָ����
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), nullptr), SW_SHOW); // �ָ�������
		dlg.DestroyWindow();
	}
	int LockMachine() {
		if (dlg.m_hWnd == nullptr || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
			_beginthreadex(nullptr, 0, &CCommand::ThreadLockDlg, this, 0, &thread_id);
			TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
		}
		CPacket pack(7, nullptr, 0);
		CServerSocket::getInstance()->Send(pack);
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
		return 0;
	}

	int UnlockMachine() {
		PostThreadMessage(thread_id, WM_KEYDOWN, 0x1B, 0); // ��Ϣ���Ƹ����̣߳������Ǿ��
		//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x1B, 0x01E0001);
		CPacket pack(8, nullptr, 0);
		CServerSocket::getInstance()->Send(pack);
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
		return 0;
	}

	int TestConnect() {
		CPacket pack(1981, nullptr, 0);
		int ret = CServerSocket::getInstance()->Send(pack);
		TRACE("send ret = %d\r\n", ret);
		return 0;
	}

	int DeleteLocalFile() {
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		DeleteFile(strPath.c_str());
		CPacket pack(9, nullptr, 0);
		int ret = CServerSocket::getInstance()->Send(pack);
		TRACE("send ret = %d\r\n", ret);
		return 0;
	}
};

