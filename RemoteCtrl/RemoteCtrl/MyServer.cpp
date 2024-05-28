#include "pch.h"
#include "MyServer.h"
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
	if (*(LPDWORD)*m_client.get() > 0) {
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->getLocalAddr(), &lLength,
			(sockaddr**)m_client->getRemoteAddr(), &rLength);

		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}

MyClient::MyClient() :m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED())
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_local_addr, 0, sizeof(m_local_addr));
	memset(&m_remote_addr, 0, sizeof(m_remote_addr));
}

void MyClient::SetOverlapped(PCLIENT& ptr)
{
	m_overlapped->m_client = ptr;
}

MyClient::operator LPOVERLAPPED()
{
	return &m_overlapped->m_overlapped;
}
