#pragma once
class CTool
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strOut;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strOut += "\n";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF); // 02X������Ϊ2���Ҷ��룬��������ַ�0���
			strOut += buf;
		}
		strOut += "\n";
		OutputDebugStringA(strOut.c_str());
	}

	static bool IsAdmin() {
		HANDLE hToken = NULL; // �����һ��void*���ͣ��������Բ���ϵͳ�����ڲ�ϸ�ڣ���֤��ȫ���и���ʱҲ�ܱ�֤����
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) { // �þ��
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) { // ��token��Ϣ
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
		MessageBox(NULL, lpMessageBuf, _T("����"), 0);
		LocalFree(lpMessageBuf);
	}

	static bool RunAsAdmin() {
		// ��ȡ����ԱȨ�ޣ�ʹ�ø�Ȩ�޴�������
		// ���ذ�ȫ���� ����Administrator�˻�	��ֹ������ֻ�ܵ�¼���ؿ���̨ ������Ч
		/*HANDLE hToken = NULL;
		BOOL ret = LogonUser((LPCSTR)"Administrator", NULL, NULL, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &hToken);
		DWORD err = GetLastError();
		if (!ret) {
			ShowError();
			MessageBox(NULL, _T("��¼����"), _T("�������"), 0);
			exit(0);
		}
		OutputDebugString((LPCTSTR)"login administrator success\r\n");
		MessageBox(NULL, _T("login�ɹ�"), _T("�ɹ�"), 0);*/

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
			MessageBox(NULL, sPath, _T("��������ʧ��"), 0);
			return false;
		}
		MessageBox(NULL, _T("�������̳ɹ�"), _T("�ɹ�"), 0);
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// ͨ��д��startup�ļ��еķ�ʽʵ�ֿ���������
	static bool WriteStartupDir(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		return CopyFile(sPath, strPath, FALSE);
	}

	static bool Init()
	{
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
};

