#include "pch.h"
#include "ClientSocket.h"

CClientSocket* CClientSocket::mInstance = NULL; // 静态成员变量类内声明，类外初始化

CClientSocket::CHelper CClientSocket::mHelper; // 会调用构造，是私有的，其他地方调用不了，保证唯一

// 因为其他有多个cpp文件包含了ClientSocket.h，所以实现放在cpp文件，头文件只放定义，否则会产生冲突
std::string GetErrInfo(int WSAErrCode) {
	std::string ret;
	LPVOID lpMsgBuf = nullptr;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		WSAErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, nullptr
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false) {
		delete pData;
	}
	return ret;
}

CClientSocket::CClientSocket(const CClientSocket& ss) {
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClosed = ss.m_bAutoClosed;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	for (auto it = ss.m_mapFunc.begin(); it != ss.m_mapFunc.end(); it++) {
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}
CClientSocket::CClientSocket()
	:m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClosed(true), m_hThread(INVALID_HANDLE_VALUE) 
{
	if (InitSockEnv() == false) {
		MessageBox(NULL, _T("无法初始化套接字"), _T("初始化错误！"), MB_OK | MB_ICONERROR); // _T用来保证编码兼容性
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) {
		TRACE("网络消息处理线程启动失败\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);

	struct {
		UINT message;
		MSGFUNC func;
	} funcs[] = {
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false) {
			TRACE("插入失败，消息值：%d 函数值：%08X, 序号：%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}
unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)){ // 只有用GetMessage获得消息后，线程才能起效
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get message %08X\r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock = %d\r\n", m_sock);
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam; // 防止内存泄露
	HWND hWnd = (HWND)lParam;
	size_t nTemp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTemp);
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0) {
			size_t index = 0; // index 记录数据尾部位置，相当于长度
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if ((length>0) || (index>0)) {
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE) {
							CloseSocket();
							return;
						}
						index -= nLen;
						memmove(pBuffer, pBuffer + nLen, index);
					}	
				}
				else { // 对方关闭套接字或网络异常
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(current.sCmd, NULL, 0), 1);
				}
			}	
		}
		else {
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); 
		}
	}
	else {
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
	
}
