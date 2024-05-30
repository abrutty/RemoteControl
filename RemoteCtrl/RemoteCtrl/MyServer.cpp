#include "pch.h"
#include "MyServer.h"
#include "Tool.h"

#pragma warning (disable:4407)
template<MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_operator = MyAccept;
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}


template<MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize() > 0) {
		LPSOCKADDR pLocalAddr, pRemoteAddr;
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocalAddr, &lLength,
			(sockaddr**)&pRemoteAddr, &rLength);
		memcpy(m_client->getLocalAddr(), pLocalAddr, sizeof(sockaddr_in));
		memcpy(m_client->getRemoteAddr(), pRemoteAddr, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client, (ULONG_PTR)m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING)) {
			TRACE("WSARecv failed %d\r\n", ret);
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

MyClient::MyClient() :m_isbusy(false), m_flags(0),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_vecSend(this,(SENDCALLBACK)&MyClient::SendData)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_local_addr, 0, sizeof(m_local_addr));
	memset(&m_remote_addr, 0, sizeof(m_remote_addr));
}

MyClient::~MyClient()
{
	m_buffer.clear();
	closesocket(m_sock);
}

void MyClient::SetOverlapped(MyClient* ptr)
{
	m_overlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;
}

MyClient::operator LPOVERLAPPED()
{
	return &m_overlapped->m_overlapped;
}

LPWSABUF MyClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPWSABUF MyClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

LPOVERLAPPED MyClient::RecvOverlapped()
{
	return &m_recv->m_overlapped;
}

LPOVERLAPPED MyClient::SendOverlapped()
{
	return &m_send->m_overlapped;
}

int MyClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0) return -1;
	m_used += (size_t)ret;
	CTool::Dump((BYTE*)m_buffer.data(), ret);
	return 0;
}
int MyClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data)) {
		return 0;
	}
	return -1;
}
int MyClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING)) {
			CTool::ShowError();
			return ret;
		}
	}
	return 0;
}
MyServer::~MyServer()
{
	for (auto it = m_client.begin(); it != m_client.end(); it++) {
		delete it->second;
		it->second = NULL;
	}
	closesocket(m_sock);
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}
bool MyServer::StartService()
{
	CreateSocket();
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(ThreadWorker(this, (FUNCTYPE)&MyServer::threadIocp));
	if (!NewAccept()) return false;
	return true;
}

bool MyServer::NewAccept()
{
	MyClient* pClient = new MyClient();
	pClient->SetOverlapped(pClient);
	m_client.insert(std::pair<SOCKET, MyClient*>(*pClient, pClient));
	if (AcceptEx(m_sock, *pClient, *pClient, 0, 
		sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,  // 两个+16是规定
		*pClient, *pClient) == FALSE) 
	{ 
		if (WSAGetLastError() != ERROR_SUCCESS && (WSAGetLastError() != WSA_IO_PENDING)) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void MyServer::BindNewSocket(SOCKET s, ULONG_PTR nKey)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, nKey, 0);
}

int MyServer::threadIocp()
{
	DWORD transferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
		if (CompletionKey != 0) {
			MyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MyOverlapped, m_overlapped);
			pOverlapped->m_server = this;
			switch (pOverlapped->m_operator)
			{
			case MyAccept:
			{
				ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case MyRecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case MySend:
			{
				SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			case MyError:
			{
				ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispatchWorker(pOver->m_worker);
			}
			break;
			default:
				break;
			}
		}
		else {
			return -1;
		}
	}
	return 0;
}

template<MyOperator op>
SendOverlapped<op>::SendOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<MyOperator op>
RecvOverlapped<op>::RecvOverlapped() {
	m_operator = MyRecv;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}