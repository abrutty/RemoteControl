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

bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed) {
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		TRACE("start thread\r\n");
	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	m_lstSend.push_back(pack);
	m_lock.unlock();
	TRACE(" cmd:%d event: %08X thread_id=%d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE);
	TRACE(" cmd:%d event: %08X thread_id=%d\r\n", pack.sCmd, pack.hEvent, GetCurrentThreadId());
	auto it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	}
	return false;
}

void CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}

void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) {
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("发送失败\r\n");
				continue;
			}
			auto it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				auto it0 = m_mapAutoClosed.find(head.hEvent);
				do {
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv length=%d  index=%d\r\n", length, index);
					if ((length > 0) || (index > 0)) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) {
								SetEvent(head.hEvent);
								break;
							}
						}
						continue;
					}
					else {
						if (length <= 0 && index <= 0) {
							CloseSocket();
							SetEvent(head.hEvent); // 等到服务器命令关闭后，再通知事件完成
							if (it0 != m_mapAutoClosed.end()) {
								m_mapAutoClosed.erase(it0);
								TRACE("SetEvent sCmd=%d it0->second=%d\r\n", head.sCmd, it0->second);
							}
							else {
								TRACE("异常，没有对应的pair\r\n");
							}
							break;
						}
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (InitSocket() == false) InitSocket();
		}
		Sleep(1);
	}
	CloseSocket();
}

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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
	if (InitSocket() == true) {
		int ret = send(m_sock, (char*)wParam, (int)lParam, 0);
		if (ret > 0) {

		}
		else {
			CloseSocket();
		}
	}
	else {

	}
	
}
