#include "pch.h"
#include "Command.h"

CCommand::CCommand() :thread_id(0) {
	struct {
		int nCmd;
		CMDFUNC func;
	} data[] = {
		{1, &CCommand::MakeDriverInfo}, // 查看磁盘分区
		{2, &CCommand::MakeDirectoryInfo}, // 查看指定目录下的文件
		{3, &CCommand::RunFile}, // 打开文件
		{4, &CCommand::DownloadFile}, // 下载文件
		{5, &CCommand::MouseEvent},
		{6, &CCommand::SendScreen}, // 发送屏幕
		{7, &CCommand::LockMachine}, // 锁机
		{8, &CCommand::UnlockMachine}, // 解锁
		{9, &CCommand::DeleteLocalFile}, // 删除文件
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

// 获取驱动信息
int CCommand::MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string result;  // A 盘->1 B盘->2 C盘->3
	for (int i = 1; i <= 26; i++) {  // 遍历磁盘符
		if (_chdrive(i) == 0) {// =0 表示成功
			if (result.size() > 0) result += ",";
			result += 'A' + i - 1;
		}
	}
	lstPacket.emplace_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
	return 0;
}

// 获取目录信息
int CCommand::MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string filePath = inPacket.strData;
	if (_chdir(filePath.c_str()) != 0) {  // 切换到输入路径判断该路径是否存在
		FILEINFO finfo;
		finfo.hasNext = false;
		lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		OutputDebugString(_T("没有权限访问目录"));
		return -2;
	}
	_finddata_t fdata;
	intptr_t  hfind;
	if ((hfind = _findfirst("*", &fdata)) == -1L) {// 获取当前文件
		OutputDebugString(_T("没有找到任何文件"));
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
	} while (_findnext(hfind, &fdata) == 0); // 获取下一个文件节点
	_findclose(hfind);
	// 通过以上两个函数配合，从盘符开始，通过do...while() 循环可得到遍历文件
	// 发送信息到控制端
	FILEINFO finfo;
	finfo.hasNext = false;
	lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
	return 0;
}

// 鼠标操作
int CCommand::MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	MOUSEEV mouse;
	memcpy(&mouse, inPacket.strData.c_str(), sizeof(mouse));

	DWORD nFlags = 0;
	switch (mouse.nButton) {
	case 0: //左键
		nFlags = 1;
		break;
	case 1: // 右键
		nFlags = 2;
		break;
	case 2: // 中键
		nFlags = 4;
		break;
	case 3: // 没有按键
		nFlags = 8;
		break;
	default:
		break;
	}
	if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
	switch (mouse.nAction) {
	case 0: // 单击
		nFlags |= 0x10;
		break;
	case 1: // 双击
		nFlags |= 0x20;
		break;
	case 2: // 按下
		nFlags |= 0x40;
		break;
	case 3: // 放开
		nFlags |= 0x80;
		break;
	default:
		break;
	}
	switch (nFlags) {
	case 0x21:  // 左键双击
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x11:  // 左键单击
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x41:  // 左键按下
		mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x81:  // 左键放开
		mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
		break;

	case 0x22:  // 右键双击
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x12:  // 右键单击
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x42:  // 右键按下
		mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x82:  // 右键放开
		mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
		break;

	case 0x24:  // 中键双击
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x14:  // 中键单击
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x44:  // 中键按下
		mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x84:  // 中键放开
		mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		break;
	case 0x08:  // 单纯鼠标移动
		mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
		break;
	default:
		break;
	}
	lstPacket.emplace_back(CPacket(5, nullptr, 0));
	return 0;
}

// 发送屏幕截图
int CCommand::SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	CImage screen;  // GDI对象，全局设备接口
	HDC hScreen = ::GetDC(nullptr);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);   // 创建屏幕
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);   // 把图像复制到屏幕中
	ReleaseDC(nullptr, hScreen);

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);   // 在内存中分配一个全局的堆
	if (hMem == nullptr) return -1;
	IStream* pStream = nullptr;    // 建立内存流，把图片存到内存中
	HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);  // 在全局对象上设置内存流
	if (ret == S_OK) {
		screen.Save(pStream, Gdiplus::ImageFormatPNG);  // 保存图像到内存流中
		//screen.Save(_T("test1.png"), Gdiplus::ImageFormatPNG);   // 保存图像到文件
		LARGE_INTEGER begin = { 0 };
		pStream->Seek(begin, STREAM_SEEK_SET, nullptr); // 保存后，将流指针恢复到开头
		PBYTE pData = (PBYTE)GlobalLock(hMem);  // 上锁，读取数据
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
	dlg.ShowWindow(SW_SHOW);    // 非模态对话框

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
		pText->GetWindowRect(rtText); // 获取锁机文本的宽和高
		int nWidth = rtText.Width();
		int x = (rect.right - nWidth) / 2; // 文本居中的x坐标
		int nHeight = rtText.Height();
		int y = (rect.bottom - nHeight) / 2; // 文本居中的y坐标
		pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
	}
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); // 窗口置顶
	ShowCursor(false);  // 不显示鼠标
	//::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), nullptr), SW_HIDE); // 隐藏任务栏
	dlg.GetWindowRect(rect);    // 获取对话框的范围
	rect.right = 1;
	rect.bottom = 1;
	ClipCursor(rect);   // 将鼠标限制在对话框范围内
	// 加上消息循环才能生效
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) { // 按下按键
			if (msg.wParam == 0x1B) break; // 按下esc键退出
		}
	}
	ClipCursor(nullptr); // 取消鼠标范围限制
	ShowCursor(true); // 恢复鼠标
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), nullptr), SW_SHOW); // 恢复任务栏
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

// 打开文件
int CCommand::RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
{
	std::string filePath = inPacket.strData;
	ShellExecuteA(nullptr, nullptr, filePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	lstPacket.emplace_back(CPacket(3, nullptr, 0));
	return 0;
}

// 下载文件
int CCommand::DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket) {
	std::string filePath = inPacket.strData;
	long long data = 0;
	FILE* pFile = nullptr;
	errno_t err = fopen_s(&pFile, filePath.c_str(), "rb"); // 用fopen会报warning，或者使用fopen，禁用warning
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
		} while (rlen >= 1024); // <1024说明读到文件尾        
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