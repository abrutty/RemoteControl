#pragma once
class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // 02X输出域宽为2，右对齐，不足的用字符0替代
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}

	static bool IsAdmin() {
		HANDLE hToken = NULL; // 句柄是一个void*类型，这样可以操作系统屏蔽内部细节，保证安全，有更新时也能保证兼容
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) { // 拿句柄
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) { // 拿token信息
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;
		}
		printf("length of token information is %d\r\n", len);
		return false;
	}

	static void ShowError() {
		LPTSTR lpMessageBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_CUSTOM_DEFAULT),
			lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("错误"), 0);
		LocalFree(lpMessageBuf);
	}

	static bool RunAsAdmin() {
		// 获取管理员权限，使用该权限创建进程
		// 本地安全策略 开启Administrator账户	禁止空密码只能登录本地控制台 才能起效
		/*HANDLE hToken = NULL;
		BOOL ret = LogonUser((LPCSTR)"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		DWORD err = GetLastError();
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("登录错误"), _T("程序错误"), 0);
			exit(0);
		}
		OutputDebugString((LPCTSTR)"login administrator success\r\n");
		MessageBox(NULL, _T("login成功"), _T("成功"), 0);*/

		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };

		/*ret = CreateProcessWithTokenW(hToken, LOGON_WITH_PROFILE, NULL, (LPWSTR)sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, (LPSTARTUPINFOW)&si, &pi);
		CloseHandle(hToken);*/
		BOOL ret = CreateProcessWithLogonW((LPCWSTR)_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, (LPSTARTUPINFOW)&si, &pi);
		DWORD err = GetLastError();
		 
		if (!ret) {
			ShowError();
			MessageBox(NULL, sPath, _T("创建进程失败"), 0);
			return false;
		}
		MessageBox(NULL, _T("创建进程成功"), _T("成功"), 0);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// 通过写入startup文件夹的方式实现开机自启动
	static bool WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
	}

	static bool Init()
	{
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"错误: GetModuleHandle 失败\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			wprintf(L"错误: MFC 初始化失败\n");
			return false;
		}
		return true;
	}
};

