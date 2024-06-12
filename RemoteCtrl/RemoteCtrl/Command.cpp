#include "pch.h"
#include "Command.h"

CCommand::CCommand() :thread_id(0) {
	struct {
		int nCmd;
		CMDFUNC func;
	} data[] = {
		{1, &CCommand::MakeDriverInfo}, // �鿴���̷���
		{2, &CCommand::MakeDirectoryInfo}, // �鿴ָ��Ŀ¼�µ��ļ�
		{3, &CCommand::RunFile}, // ���ļ�
		{4, &CCommand::DownloadFile}, // �����ļ�
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen}, // ������Ļ
		{7, &CCommand::LockMachine}, // ����
		{8, &CCommand::UnlockMachine}, // ����
		{9, &CCommand::DeleteLocalFile}, // ɾ���ļ�
		{1981, &CCommand::TestConnect},
		{-1,nullptr},
	};
	for (int i = 0; data[i].nCmd != -1; i++) {
		m_mapFunction.insert(std::pair<int, CMDFUNC>(data[i].nCmd, data[i].func));
	}
}

int CCommand::ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::map<int, CMDFUNC>::iterator it = m_mapFunction.find(nCmd);
	if (it == m_mapFunction.end()) {
		return -1;
	}
	return (this->*it->second)(lstPacket, inPacket);
}

// ��ȡ������Ϣ
int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string result;  // A ��->1 B��->2 C��->3
	for (int i = 1; i <= 26; i++) {  // �������̷�
		if (_chdrive(i) == 0) {// =0 ��ʾ�ɹ�
			if (result.size() > 0) result += ",";
			result += 'A' + i - 1;
		}
	}
	lstPacket.emplace_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
	return 0;
}

// ��ȡĿ¼��Ϣ
int CCommand::MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string filePath = inPacket.strData;
	if (_chdir(filePath.c_str()) != 0) {  // �л�������·���жϸ�·���Ƿ����
		FILEINFO finfo;
		finfo.hasNext = false;
		lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		OutputDebugString(_T("û��Ȩ�޷���Ŀ¼"));
		return -2;
	}
	_finddata_t fdata;
	intptr_t  hfind;
	if ((hfind = _findfirst("*", &fdata)) == -1L) {// ��ȡ��ǰ�ļ�
		OutputDebugString(_T("û���ҵ��κ��ļ�"));
		FILEINFO finfo;
		finfo.hasNext = false;
		lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return -3;
	}
	do {
		FILEINFO finfo;
		finfo.isDirectory = (fdata.attrib & _A_SUBDIR) != 0;
		memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
		TRACE("szFileName=%s\r\n", finfo.szFileName);
		lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
	} while (_findnext(hfind, &fdata) == 0); // ��ȡ��һ���ļ��ڵ�
	_findclose(hfind);
	// ͨ����������������ϣ����̷���ʼ��ͨ��do...while() ѭ���ɵõ������ļ�
	// ������Ϣ�����ƶ�
	FILEINFO finfo;
	finfo.hasNext = false;
	lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
	return 0;
}

// ������
int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	MOUSEEV mouse;
	memcpy(&mouse, inPacket.strData.c_str(), sizeof(mouse));

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
	lstPacket.emplace_back(CPacket(5, nullptr, 0));
	return 0;
}

// ������Ļ��ͼ
int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
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
		lstPacket.emplace_back(CPacket(6, pData, nSize));
		GlobalUnlock(hMem);
	}
	pStream->Release();
	GlobalFree(hMem);
	screen.ReleaseDC();
	return 0;
}

void CCommand::ThreadLockDlgMain()
{
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

int CCommand::LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	if (dlg.m_hWnd == nullptr || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
		_beginthreadex(nullptr, 0, &CCommand::ThreadLockDlg, this, 0, &thread_id);
		TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
	}
	lstPacket.emplace_back(CPacket(7, nullptr, 0));
	TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, thread_id);
	return 0;
}

// ���ļ�
int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string filePath = inPacket.strData;
	ShellExecuteA(nullptr, nullptr, filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	lstPacket.emplace_back(CPacket(3, nullptr, 0));
	return 0;
}

// �����ļ�
int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string filePath = inPacket.strData;
	long long data = 0;
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, filePath.c_str(), "rb"); // ��fopen�ᱨwarning������ʹ��fopen������warning
	if (err != 0) {
		lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
		return -1;
	}
	if (pFile != nullptr) {
		fseek(pFile, 0, SEEK_END);
		data = _ftelli64(pFile);
		lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
		fseek(pFile, 0, SEEK_SET);
		char buffer[1024] = "";
		size_t rlen = 0;
		do {
			rlen = fread(buffer, 1, 1024, pFile);
			lstPacket.emplace_back(CPacket(4, (BYTE*)buffer, rlen));
		} while (rlen >= 1024); // <1024˵�������ļ�β        
		fclose(pFile);
	}
	else {
		lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
	}
	return 0;
}

int CCommand::DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string strPath = inPacket.strData;
	DeleteFile(strPath.c_str());
	lstPacket.emplace_back(CPacket(9, nullptr, 0));
	return 0;
}