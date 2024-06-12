#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::mInstance = NULL; // 静态成员变量类内声明，类外初始化

CServerSocket::CHelper CServerSocket::mHelper; // 会调用构造，是私有的，其他地方调用不了，保证唯一
//CServerSocket* pserver = CServerSocket::getInstance();

int CServerSocket::Run(SOCKET_CALLBACK callback, void* arg, short port)
{
	bool ret = InitSocket(port);
	if (ret == false) return -1;
	std::list<CPacket> lstPackets;
	m_callback = callback;
	m_arg = arg;
	int count = 0;
	while (true) {
		if (AcceptClient() == false) {
			if (count >= 3) return -2;
			count++;
		}
		int ret = DealCommand();
		if (ret > 0) {
			m_callback(m_arg, ret, lstPackets, m_packet);
			while (lstPackets.size() > 0) {
				Send(lstPackets.front());
				lstPackets.pop_front();
			}
		}
		CloseClient();
	}
}

bool CServerSocket::InitSocket(short port)
{
	if (m_sock == -1) return false;
	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	if (bind(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) return false;
	if (listen(m_sock, 1) == -1) return false;
	return true;
}

bool CServerSocket::AcceptClient()
{
	TRACE("enter AcceptClient\r\n");
	sockaddr_in client_addr;
	int client_sz = sizeof(client_addr);
	m_client = accept(m_sock, (sockaddr*)&client_addr, &client_sz);
	TRACE("m_client=%d\r\n", m_client);
	if (m_client == -1) return false;
	return true;
}

int CServerSocket::DealCommand()
{
	if (m_client == -1) return -1;
	char* buffer = new char[BUFFER_SIZE];
	if (buffer == nullptr) {
		TRACE("buffer内存不足\r\n");
		return -2;
	}
	memset(buffer, 0, BUFFER_SIZE);
	size_t index = 0;
	while (true) {
		size_t len = recv(m_client, buffer + index, (int)BUFFER_SIZE - index, 0);
		if (len <= 0) {
			delete[] buffer;
			return -1;
		}
		TRACE("server recv: %d	index= %d\r\n", len, index);
		index += len;
		len = index;
		m_packet = CPacket((BYTE*)buffer, len);
		if (len > 0) {
			memmove(buffer, buffer + len, BUFFER_SIZE - len);
			index -= len;
			TRACE("index= %d, len=%d\r\n", index, len);
			delete[] buffer;
			return m_packet.sCmd;
		}
	}
	delete[] buffer;
	return -1;
}

BOOL CServerSocket::InitSockEnv()
{
	// 套接字初始化
	WSADATA data;
	if (WSAStartup(MAKEWORD(2, 0), &data) != 0) {
		return FALSE;
	}
	else {
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
		return TRUE;
	}
}


