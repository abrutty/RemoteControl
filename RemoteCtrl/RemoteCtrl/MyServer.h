#pragma once
#include "MyThread.h"
#include <map>
#include "CQueue.h"
#include <MSWSock.h>

enum MyOperator {
    MyNone,
    MyAccept,
    MyRecv,
    MySend,
    MyError
};
class MyServer;
class MyClient;
typedef std::shared_ptr<MyClient> PCLIENT;

class MyOverlapped {
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator; // 操作
    std::vector<char> m_buffer; // 缓冲区
    ThreadWorker m_worker; // 处理函数
	MyServer* m_server; // 服务器对象
};
template<MyOperator> class AcceptOverlapped;
typedef AcceptOverlapped<MyAccept> ACCEPTOVERLAPPED;

class MyClient {
public:
	MyClient();
	~MyClient() {
		closesocket(m_sock);
	}
	void SetOverlapped(PCLIENT& ptr);
	operator SOCKET() {
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD() {
		return &m_received;
	}
	sockaddr_in* getLocalAddr() { return &m_local_addr; }
	sockaddr_in* getRemoteAddr() { return &m_remote_addr; }
private:
	SOCKET m_sock;
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
	std::vector<char> m_buffer;
	sockaddr_in m_local_addr;
	sockaddr_in m_remote_addr;
	bool m_isbusy;
};

template<MyOperator>
class AcceptOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();
	PCLIENT m_client;
};


template<MyOperator>
class RecvtOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	RecvtOverlapped() :m_operator(MyRecv), m_worker(this, &RecvtOverlapped::RecvWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}
	int RecvWorker() {

	}
};
typedef RecvtOverlapped<MyRecv> RECVOVERLAPPED;

template<MyOperator>
class SendOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	SendOverlapped() :m_operator(MySend), m_worker(this, &SendOverlapped::SendWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024*256);
	}
	int SendWorker() {

	}
};
typedef SendOverlapped<MySend> SENDOVERLAPPED;

template<MyOperator>
class ErrorOverlapped : public MyOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() :m_operator(MyError), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {

	}
};
typedef ErrorOverlapped<MyError> ERROROVERLAPPED;





class MyServer : public ThreadFuncBase
{
public:
	MyServer(const std::string& ip="0.0.0.0", short port=9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~MyServer(){}
	bool StartService() {
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
	
	bool NewAccept() {
		PCLIENT pClient(new MyClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
		if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE) { // 两个+16是规定
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
		return true;
	}
private:
	void CreateSocket() {
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	}
	
	int threadIocp() {
		DWORD transferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE)) {
			if (transferred > 0 && (CompletionKey != 0)) {
				MyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, MyOverlapped, m_overlapped);
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
private:
	MyThreadPool m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	sockaddr_in m_addr;
	std::map<SOCKET, std::shared_ptr<MyClient>> m_client;
	CQueue<MyClient> m_lstClient;
};

